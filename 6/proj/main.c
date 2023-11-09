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
 *    Napište si dvě aplikace, kdy
 *    a) jedna bude měnit obsah sdílené paměti,
 *    kde bude 10 čísel a jejich součet.
 *    b) Druhá aplikace provede kontrolu.
 *    b.1) Notifikace pomocí pojmenované roury.
 *    b.2) Data int *pole, struct data { char [11][20]; }
 *    b.3) Dokončete readline.
 */

/**
 * Upresneni zadani - konverzace:
 *
 * Zdravím pane inženýre Olivko,
 * jsem studentem kombinované formy studia a mimo řešení SP u Vás mám zapsán předmět PVOS.
 * Mám nejasnosti v zadání úkolu PVOS-06:
 *
 * Zadání a mé dotazy:
 *     Napište si dvě aplikace, kdy jedna bude měnit obsah sdílené paměti, kde bude 10 čísel a jejich součet. Druhá aplikace provede kontrolu.
 * - dle https://poli.cs.vsb.cz/edu/pvos/pdf/osnova.pdf předpokládám, že je chtěné použít `sem_open` s pojmenovanou sdílenou pamětí a dále je možné implementovat aplikaci jako 1 program s 2 režimy a přepínačem dle vstupního argumnetu programu, který při spuštění aplikace vybere v kterém režimu program poběží - jestli jako Aplikace č.1 nebo Aplikaci č.2.
 * Notifikace pomocí pojmenované roury.
 * - Přepdokládám, že notifikace je z Aplikace č.2 Aplikaci č.1.
 *     Data int *pole,
 * - rozumím správně tomu, že je `int * pole` se jedná o pole ve sdílené paměti, ve kterém má být zmíněné pole čísel `10 čísel a jejich součet`
 * struct data { char [11][20]; }
 * - k čemu má tato struktura být použita mi není jasné. Napadá mě, že nějak souvisí s rourami, ale nejsem si jist, že mi správné použití této struktury dojde až při samotné implementaci roury.
 * Můžete mi prosím rozvést k čemu tato struktura má být použita?
 *     Dokončete readline.
 * - Zde mě napadají 2 možnosti jak si tuto část zadání vyložit:
 * -- a) je chtěné čekat na vstup před ukončením programu (obou aplikací).
 * -- b) je zde návaznost na předchozí úkol PVOS-05, ale zatím nerozumím jaká.
 * Prosím o rozvedení zadání.
 *
 *
 * Odpoved:
 * Zdravím,
 * notifikace je od toho kdo generuje k tomu, kdo kontroluje.
 * Ukazatel *pole má směřovat do sdílené paměti a někteří meditovali nad řetězcem s čísly oddělenými mezerou,
 * což by se muselo znovu a znovu rozkládat, tak jsem jim navrhoval char[11][20], když už chtějí data textově.
 * - moje pozn.: nechci data textove, protoze mi to prijde jako zbytecne slozitost
 * Implementace readline_tout s bufferem byla zadána v úkolu 5 a měli na to studenti týden, proto v úkolu 6:
 * "dokončete readline" (bylo tam 4 body za úkoly 5 a 8 bodů za úkoly 6).
 * poli
 *
 */
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
    int segment_id_pid = shm_open(SHM_NAME_PID, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    int size_pid = sizeof(int);
    ftruncate(segment_id_pid, size_pid);
    int *ptr_pid;
    /* memory map the shared memory object */
    ptr_pid = mmap(0, size_pid, PROT_WRITE, MAP_SHARED, segment_id_pid, 0);

    /* write current pid to the shared memory object */
    ptr_pid[0] = getpid();
    debug("app_2 - pause");
    /* read from the shared memory object */
    // Wait for the signal indefinitely
    while (1) {
        pause();
    }
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
