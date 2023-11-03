#include <stdio.h>

#include <stdio.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
// sender.c
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

/**
 * Zadani:
 *    Upravte příklad pro SIGIO a rouru na příklad s N potomky a N rourami.
 *    Sighandler bude využívat strukturu siginfo_t.
 *    Upravte příklad pro aio tak, aby místo SIGEV_NONE využil SIGEV_SIGNAL a SIGEV_THREAD.
 *    Opět N potomků a N rour.
 *    Naimplementujte si funkci readline_tout s vyrovnávacím bufferem. Maximálně se snažte,
 *    aby se funkce readline_tout chovala podobně, jako read.
 *        readline je programátorská etuda - implementace řady problémů
 *        může být vyrovnávací buffer lokální proměnná ve funkci/metodě?
 *        - záleží na implementaci, v případě užití singálů, které používají metody pro zpracování signálu bude použití
 *        -- jedné lokální proměnné zamezeno přístupem
 *        může být pouze jeden buffer? Jak spárovat buffer a file descriptor?
 *        -- budeme-li používat globální promennou napříč voláním `main` i funkce `sighandleru`, které sdílí svá datat,
 *        -- tak není nutno pouřívat file
 *        test: echo -en " jojo nene " | ./readline_tout
 *        test: while true; do echo -en " jojo nene "; sleep 1; done | ./readline_tout
 *        co když je řádek delší než buffer?'
 *        - pak buffer načte své maximum a zbytek zahodí, v návratové kódu bude indikace BUFFER overflow,
 *        --- program může mít vstupní parametr vlaječky určující striktost vůči přetečení bufferu a způsob vyřízení
 *        ---této situace
 *        může se přepsat na konci řádku?
 *        - pro náš případ ne a nejsem si vědom, že bych se s touto situací prakticky setkal.
 *        -- teoreticky je možné kdeco. např. přepsat buffer na konci řádku bude-li nastaveno parametrem vlaječky atp.
 *        -- v tomto případě je možné, že bude nutné hlídat mazání textu za běhu, zbytek operace by byl stejný
 *        co na konci souboru?
 *        - podobně u konce souboru, ale je to neobvyklá situace
 *        -- s touto situací se jde prakticky setkat bude-li se číst *.log, do kterého byl zpasán výstup např. `clear`
 *        -- operace
 *       když nastane timeout, co s neúplnými daty v bufferu?
 *       -- nastavitelné chování parameterm vlaječky: a) smařu, b) vypíšu; a pro obě možnosti parametrizace vlaječkami
 */

#define SHM_SIZE 44 // 10 * sizeof(int) + sizeof(int)
#define SHM_NAME "myshm"
#define SHM_NAME_PID "myshmpid"
#define ARR_SIZE 10

#define DEBUG

#ifdef DEBUG
#define debug(fmt, ...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt "\n", \
        __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define debug(fmt, ...) // Nothing
#endif
int app_1();
int app_2();

/**
 * https://www.geeksforgeeks.org/posix-shared-memory-api/
 * @return
 */
int app_1() {
    debug("app_1 - start");


    /* get pid of app_2 and send notification to it */

    /* get pid of app_2 */
    int segment_id_pid = shm_open(SHM_NAME_PID, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    int size_pid = sizeof(int);
    ftruncate(segment_id_pid, 1);
    int* ptr_pid;
    /* memory map the shared memory object */
    ptr_pid = mmap(0, size_pid, PROT_WRITE, MAP_SHARED, segment_id_pid, 0);
    ptr_pid[0] = -1; //getpid();

    pid_t apt2_pid = -1;
    while(1) {
        debug("app_1 - waiting for pid to appear ");
        sleep(1);
        int* ptr_pid2; // pointer to shared memory object
        int shm_fd_pid2 = shm_open(SHM_NAME_PID, O_RDONLY, 0666); // shared memory file descriptor

        /* memory map the shared memory object */
        ptr_pid2 = mmap(0, size_pid, PROT_READ, MAP_SHARED, shm_fd_pid2, 0);
        apt2_pid = ptr_pid2[0];
        if(apt2_pid != -1) {
            break;
        }
    }
    shm_unlink(SHM_NAME_PID);

    debug("app_1 - app_2 pid is %d, going to write to shared memory....", apt2_pid);
    // in apt2_pid there is pid of second app
    // now lets write to shared memory list of numbers and its sum

    int segment_id = shm_open(SHM_NAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    int size = sizeof(int) * ARR_SIZE + 1;
    ftruncate(segment_id, size);
    int* ptr;
    /* memory map the shared memory object */
    ptr = mmap(0, size, PROT_WRITE, MAP_SHARED, segment_id, 0);

    /* write to the shared memory object */
    for (int i = 0; i < ARR_SIZE; i++) {
        ptr[i] = i;
        ptr[ARR_SIZE] += i;
    }

    debug("app_1 - sum is %d, sending notify to app_2", ptr[ARR_SIZE]);

    // now lets notify the other process that we wrote to shared memory
    if (kill(apt2_pid, SIGUSR1) == -1) {
        perror("kill");
        exit(EXIT_FAILURE);
    }

    sleep(2);
    shm_unlink(SHM_NAME);
    debug("app_2 - end");
    // TODO readline_tout
    return 0;
}

void signal_handler(int signum) {
    printf("app_2 - Received signal %d\n", signum);

    sleep(1);
    int size = sizeof(int) * ARR_SIZE + 1;

    int* ptr; // pointer to shared memory object
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666); // shared memory file descriptor
    debug("app_2 - mmap");

    /* memory map the shared memory object */
    ptr = mmap(0, size, PROT_READ, MAP_SHARED, shm_fd, 0);

    debug("app_2 - sum");
    /* read from the shared memory object */
    int sum = ptr[ARR_SIZE];
    debug("app_2 - sum2");
    printf("app_2 - sum: %d\n", sum);
    debug("app_2 - sum_check - start");

    int sum_check = 0;
    /* read and sum check */
    for (int i = 0; i < ARR_SIZE; i++) {
        sum_check += ptr[i];
    }
    debug("app_2 - sum_check_done - start");
    printf("app_2 - sum_check: %d", sum_check);
    if(sum == sum_check) {
        printf(" - OK");
    }else{
        printf(" - KO");
    }
    printf("\n");
    // TODO readline_tout
    exit(EXIT_SUCCESS);
}

int app_2() {
    debug("app_2 - start");

    printf("app_2 - Receiver PID: %d\n", getpid());

    // Register the signal handler
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    debug("app_2 - write current pid %d to shared memory for pid", getpid());

//     write pid to shared memory (and wait till it is read)
//    while(1) {
    int segment_id_pid = shm_open(SHM_NAME_PID, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    int size_pid = sizeof(int);
    ftruncate(segment_id_pid, size_pid);
    int *ptr_pid;
    /* memory map the shared memory object */
    ptr_pid = mmap(0, size_pid, PROT_WRITE, MAP_SHARED, segment_id_pid, 0);

    /* write current pid to the shared memory object */
    ptr_pid[0] = getpid();
//        sleep(1);
//        break;
//    }
    debug("app_2 - pause");
    /* read from the shared memory object */
    // Wait for the signal indefinitely
    while (1) {
        pause();
    }

/*
    sleep(1);
    int size = sizeof(int) * ARR_SIZE + 1;


    int shm_fd; // shared memory file descriptor
    int* ptr; // pointer to shared memory object
    shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    debug("app_2 - mmap");

    *//* memory map the shared memory object *//*
    ptr = mmap(0, size, PROT_READ, MAP_SHARED, shm_fd, 0);

    debug("app_2 - sum");
    *//* read from the shared memory object *//*
    int sum = ptr[ARR_SIZE];
    debug("app_2 - sum2");
    printf("app_2 - sum: %d\n", sum);
    debug("app_2 - sum_check - start");

    int sum_check = 0;
    *//* read and sum check *//*
    for (int i = 0; i < ARR_SIZE; i++) {
        sum_check += ptr[i];
    }
    debug("app_2 - sum_check_done - start");
    printf("app_2 - sum_check: %d", sum_check);
    if(sum == sum_check) {
        printf(" - OK");
    }else{
        printf(" - KO");
    }
    printf("\n");*/

    debug("app_2 - unlink");
    shm_unlink(SHM_NAME);
    debug("app_2 - end");
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s [1|2]\n", argv[0]);
        return 2;
    } else {
        switch (argv[1][0]) {
            case '1':
                return app_1();
            case '2':
                return app_2();
            default:
                printf("Usage: %s [1|2]\n", argv[0]);
        }
    }
    return 2;
}
