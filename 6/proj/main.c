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
#define FIFO_NAME "myfifo"
#define SHM_NAME "myshm"

#define DEBUG

#ifdef DEBUG
#define debug(fmt, ...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt "\n", \
        __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define debug(fmt, ...) // Nothing
#endif
int app_1();
int app_2();

int app_1() {
    debug("app_1 - start");
//    int segment_id = shmget(IPC_PRIVATE, SHM_SIZE, S_IRUSR | S_IWUSR);
//    int segment_id = shm_open(SHM_NAME, SHM_SIZE, S_IRUSR | S_IWUSR);
    int segment_id = shm_open(SHM_NAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    int* shared_memory = (int*) shmat(segment_id, NULL, 0);

/*
    for (int i = 0; i < 10; i++) {
        shared_memory[i] = i;
        shared_memory[10] += i;
    }
*/

    sleep(10);
//    shmdt(shared_memory);
    shm_unlink(SHM_NAME);
    debug("app_2 - end");
    return 0;
}

int app_2() {
    debug("app_2 - start");
    sleep(1);
    int segment_id = shm_open(SHM_NAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    int* shared_memory = (int*) shmat(segment_id, NULL, 0);

/*    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += shared_memory[i];
    }*/

//    shmdt(shared_memory);
//    shmctl(segment_id, IPC_RMID, NULL);
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
