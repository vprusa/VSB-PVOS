#include <stdio.h>

#include <stdio.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

/**
 * Zadani:
 *    Napište si dvě aplikace, kdy
 *    a) jedna bude měnit obsah sdílené paměti,
 *    kde bude 10 čísel a jejich součet.
 *    b) Druhá aplikace provede kontrolu.
 *    b.1) Notifikace pomocí pojmenované roury.
 *    b.2) Data int *pole, struct data { char [11][20]; }  TODO wtf
 *    b.3) Dokončete readline. TODO wtf
 */


#define SHM_SIZE 44 // 10 * sizeof(int) + sizeof(int)
#define SHM_NAME "myshm"
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
//    int segment_id = shmget(IPC_PRIVATE, SHM_SIZE, S_IRUSR | S_IWUSR);
//    int segment_id = shm_open(SHM_NAME, SHM_SIZE, S_IRUSR | S_IWUSR);
    int segment_id = shm_open(SHM_NAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    int size = sizeof(int) * ARR_SIZE + 1;
    ftruncate(segment_id, size);
    int* shared_memory = (int*) shmat(segment_id, NULL, 0);
    int* ptr;
    /* memory map the shared memory object */
    ptr = mmap(0, size, PROT_WRITE, MAP_SHARED, segment_id, 0);

    /* write to the shared memory object */
    for (int i = 0; i < ARR_SIZE; i++) {
        ptr[i] = i;
        ptr[ARR_SIZE] += i;
    }

    sleep(2);
    shm_unlink(SHM_NAME);
    debug("app_2 - end");
    return 0;
}

int app_2() {
    debug("app_2 - start");
    sleep(1);
    int segment_id = shm_open(SHM_NAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    int* shared_memory = (int*) shmat(segment_id, NULL, 0);
    int size = sizeof(int) * ARR_SIZE + 1;

    /* shared memory file descriptor */
    int shm_fd;

    /* pointer to shared memory object */
    int* ptr;
    shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    debug("app_2 - mmap");

    /* memory map the shared memory object */
    ptr = mmap(0, size, PROT_READ, MAP_SHARED, shm_fd, 0);

    debug("app_2 - sum");
    /* read from the shared memory object */
    int sum = ptr[ARR_SIZE];
    debug("app_2 - sum2");
    printf("app_2 - sum: %d\n", sum);

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
