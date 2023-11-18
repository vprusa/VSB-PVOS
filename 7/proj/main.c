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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>  // Include POSIX semaphore library
#include <sys/sem.h>


/**
 * Zadani:
 * Pomocí semaforu System V i Posix implementujte problém “plnění přepravky”. 
 * Pokuste se u System V využít “wait-for-zero” u semop.
 *    1. Jeden (program) proces bude plnit přepravku - sdílenou paměť - výrobky. 
 *       Do přepravky může v dané chvíli přistupovat jen jeden proces. 
 *       Přepravka má kapacitu a aktuální stav.
 *    2. Druhý (program) proces bude přepravky vyměňovat. Po výměně rozběhne “plniče”.
 */


// ANSI escape codes for color formatting
#define GREEN_COLOR "\033[0;32m"
#define ORANGE_COLOR "\033[0;33m"
#define BLUE_COLOR "\033[0;34m"
#define MAGNETA_COLOR "\033[0;35m"
#define CYAN_COLOR "\033[0;36m"
#define BRIGHT_CYAN_COLOR "\033[0;96m"
#define RESET_COLOR "\033[0m"

#define DEBUG

#ifdef DEBUG
#define debug(fmt, ...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt "\n", \
        __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define debug(fmt, ...) // Nothing
#endif

// Maximum capacity of the crate
#define MAX_CAPACITY 3
#define APP_1_MAX_COUNTER 10
#define APP_2_MAX_COUNTER 10

// Key for the shared memory and semaphore set
#define SHM_KEY 1234
#define SEM_KEY 5678

#define X_SEM_NAME "/x_sem"  // Unique name for the POSIX semaphore

// Structure for the crate
struct Crate {
    int capacity;
    int current_load;
    int sem_id;
};

// Structure for the semaphore operations
struct sembuf wait_op = {0, -1, 0};
struct sembuf signal_op = {0, 1, 0};

int app_1(int app_1_cnt);
int app_2();

int app_1_X(int app_1_cnt);
int app_2_X();

/*
 *    1. Jeden (program) proces bude plnit přepravku - sdílenou paměť - výrobky.
 *       Do přepravky může v dané chvíli přistupovat jen jeden proces.
 *       Přepravka má kapacitu a aktuální stav.
 */
int app_1(int app_1_cnt) {
    // Attempt to find the existing shared memory segment
    int shm_id = shmget(SHM_KEY, sizeof(struct Crate), 0);

    if (shm_id == -1) {
        // If the shared memory segment doesn't exist, create it
        shm_id = shmget(SHM_KEY, sizeof(struct Crate), IPC_CREAT | 0666);
        if (shm_id == -1) {
            perror("shmget");
            return 1;
        }
    }

    struct Crate *crate = (struct Crate *)shmat(shm_id, NULL, 0);
    if (crate == (struct Crate *)(-1)) {
        perror("shmat");
        return 1;
    }

    crate->current_load = 0;
    crate->capacity = MAX_CAPACITY;
    crate->sem_id = 0;

    while (app_1_cnt > 0) {
        debug(MAGNETA_COLOR "   app_1: " RESET_COLOR "Waiting for permission to proceed");

        int counter = APP_1_MAX_COUNTER; // Initialize the counter to 1000
        while (counter > 0) {
            // Debug message with green color prefix
            debug(MAGNETA_COLOR "   app_1: " RESET_COLOR "Waiting for permission to proceed");
            semop(crate->sem_id, &wait_op, 1); // Wait for permission to proceed
            debug(MAGNETA_COLOR "   app_1: " RESET_COLOR "Proceeding...");

            if (crate->current_load < crate->capacity) {
                crate->current_load++;
                debug(MAGNETA_COLOR "app_1: " RESET_COLOR "Filling the crate, current load: %d/%d",
                      crate->current_load, crate->capacity);
            } else {
                debug("  " MAGNETA_COLOR "app_1: " RESET_COLOR "Exchanging the crate");
    //            crate->current_load = 0;
            }

            debug(MAGNETA_COLOR "   app_1: " RESET_COLOR "Signaling completion of the task start");
            semop(crate->sem_id, &signal_op, 1); // Signal completion of the task

            // Debug message with green color prefix
            debug(MAGNETA_COLOR "   app_1: " RESET_COLOR "Signaling completion of the task end");

            counter--; // Decrement the counter
            //        sleep(1);
            usleep(100000);
        }
        sleep(5);
        app_1_cnt--;
    }

    shmdt(crate);
    return 0;
}

/*
 *    2. Druhý (program) proces bude přepravky vyměňovat. Po výměně rozběhne “plniče”.
 */
int app_2() {
    // sanity wait
    sleep(1);

    // Attaching to the existing shared memory
    int shm_id = shmget(SHM_KEY, sizeof(struct Crate), 0666);
    if (shm_id == -1) {
        perror("shmget");
        return 1;
    }

    struct Crate *crate = (struct Crate *)shmat(shm_id, NULL, 0);
    if (crate == (struct Crate *)(-1)) {
        perror("shmat");
        return 1;
    }

    int counter = APP_2_MAX_COUNTER; // Initialize the counter to 1000
    while (counter > 0) {
        debug(ORANGE_COLOR "  app_2: " RESET_COLOR "Waiting for permission to proceed");
        semop(crate->sem_id, &wait_op, 1); // Wait for permission to proceed
        debug(ORANGE_COLOR "  app_2: " RESET_COLOR "Proceeding...");

        if (crate->current_load > 0) {
            crate->current_load--;
            debug(ORANGE_COLOR "app_2: " RESET_COLOR "Emptying crate, current load: %d/%d" RESET_COLOR,
                   crate->current_load, crate->capacity);
        } else {
            debug(ORANGE_COLOR "  app_2: " RESET_COLOR "Empty crate" RESET_COLOR);
//            crate->current_load = 0;
            debug(ORANGE_COLOR "   app_2: " RESET_COLOR "Signaling completion of the task start");
            semop(crate->sem_id, &signal_op, 1); // Signal completion of the task
        }

        debug(ORANGE_COLOR "  app_2: " RESET_COLOR "Signaling completion of the task end");
        counter--; // Decrement the counter
        sleep(1);
    }

    shmdt(crate);

    return 0;
}

/*
 *    1. Jeden (program) proces bude plnit přepravku - sdílenou paměť - výrobky.
 *       Do přepravky může v dané chvíli přistupovat jen jeden proces.
 *       Přepravka má kapacitu a aktuální stav.
 */
int app_1_X(int app_1_cnt) {
    // Attempt to find the existing shared memory segment
    int shm_id = shmget(SHM_KEY, sizeof(struct Crate), 0);

    if (shm_id == -1) {
        // If the shared memory segment doesn't exist, create it
        shm_id = shmget(SHM_KEY, sizeof(struct Crate), IPC_CREAT | 0666);
        if (shm_id == -1) {
            perror("shmget");
            return 1;
        }
    }

    struct Crate *crate = (struct Crate *)shmat(shm_id, NULL, 0);
    if (crate == (struct Crate *)(-1)) {
        perror("shmat");
        return 1;
    }

    crate->current_load = 0;
    crate->capacity = MAX_CAPACITY;

    // Create a POSIX semaphore
    sem_t *sem = sem_open(X_SEM_NAME, O_CREAT, 0666, 1);  // Initial value 1 for mutual exclusion
    if (sem == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    while (app_1_cnt > 0) {
        debug(BLUE_COLOR "   app_1_X: " RESET_COLOR "Waiting for permission to proceed");

        int counter = APP_1_MAX_COUNTER; // Initialize the counter
        while (counter > 0) {
            // Debug message with green color prefix
            debug(BLUE_COLOR "   app_1_X: " RESET_COLOR "Waiting for permission to proceed");

            // Wait for permission to proceed using POSIX semaphore
            if (sem_wait(sem) == -1) {
                perror("sem_wait");
                return 1;
            }
            debug(BLUE_COLOR "   app_1_X: " RESET_COLOR "Proceeding...");

            if (crate->current_load < crate->capacity) {
                crate->current_load++;
                debug(BLUE_COLOR "app_1_X: " RESET_COLOR "Filling the crate, current load: %d/%d",
                      crate->current_load, crate->capacity);
            } else {
                debug("  " BLUE_COLOR "app_1_X: " RESET_COLOR "Exchanging the crate");
            }

            // Signal completion of the task using POSIX semaphore
            if (sem_post(sem) == -1) {
                perror("sem_post");
                return 1;
            }

            counter--; // Decrement the counter
            usleep(100000);
        }
        sleep(5);
        app_1_cnt--;
    }

    shmdt(crate);

    // Close and unlink the semaphore
    if (sem_close(sem) == -1) {
        perror("sem_close");
        return 1;
    }
    if (sem_unlink(X_SEM_NAME) == -1) {
        perror("sem_unlink");
        return 1;
    }

    return 0;
}

/*
 *    2. Druhý (program) proces bude přepravky vyměňovat. Po výměně rozběhne “plniče”.
 */
int app_2_X() {
    // sanity wait
    sleep(1);

    // Attaching to the existing shared memory
    int shm_id = shmget(SHM_KEY, sizeof(struct Crate), 0666);
    if (shm_id == -1) {
        perror("shmget");
        return 1;
    }

    struct Crate *crate = (struct Crate *)shmat(shm_id, NULL, 0);
    if (crate == (struct Crate *)(-1)) {
        perror("shmat");
        return 1;
    }

    // Create a POSIX semaphore
    sem_t *sem = sem_open(X_SEM_NAME, 0);  // Open the existing semaphore
    if (sem == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    int counter = APP_2_MAX_COUNTER; // Initialize the counter
    while (counter > 0) {
        debug(BRIGHT_CYAN_COLOR "  app_2_X: " RESET_COLOR "Waiting for permission to proceed");

        // Wait for permission to proceed using POSIX semaphore
        if (sem_wait(sem) == -1) {
            perror("sem_wait");
            return 1;
        }
        debug(BRIGHT_CYAN_COLOR "  app_2_X: " RESET_COLOR "Proceeding...");

        if (crate->current_load > 0) {
            crate->current_load--;
            debug(BRIGHT_CYAN_COLOR "app_2_X: " RESET_COLOR "Emptying crate, current load: %d/%d" RESET_COLOR,
                  crate->current_load, crate->capacity);
        } else {
            debug(BRIGHT_CYAN_COLOR "  app_2_X: " RESET_COLOR "Empty crate" RESET_COLOR);
        }

        // Signal completion of the task using POSIX semaphore
        if (sem_post(sem) == -1) {
            perror("sem_post");
            return 1;
        }

        debug(BRIGHT_CYAN_COLOR "  app_2_X: " RESET_COLOR "Signaling completion of the task");
        counter--; // Decrement the counter
        sleep(1);
    }

    shmdt(crate);

    // Close the semaphore (unlinking is not required for existing semaphore)
    if (sem_close(sem) == -1) {
        perror("sem_close");
        return 1;
    }

    return 0;
}

void help(char * s) {
    printf("Usage: %s [1|2] [V|X] [1_cnt]\n", s);
    printf("   char 1 - 1 - master, 2 - slave [X]\n");
    printf("   char 2 - X - posix, V - System V\n");
    printf("   char 3 - 1_cnt - app_1 crates count\n");
}

int main(int argc, char *argv[]) {
    int app_1_cnt = 2;
    if (argc > 2) {
        app_1_cnt = atoi(argv[3]);
    }
    if (argc < 2) {
        help(argv[0]);
        return 2;
    } else {
        char c = 'V';
        if (argc > 2) {
            c = argv[2][0];
        }
        switch (c) {
            // Using posix
            case 'X':
                switch (argv[1][0]) {
                    case '1':
                        return app_1_X(app_1_cnt);
                    case '2':
                        return app_2_X();
                    default:
                        help(argv[0]);
                }
                break;
            default:
                // Using System V
                switch (argv[1][0]) {
                    case '1':
                        return app_1(app_1_cnt);
                    case '2':
                        return app_2();
                    default:
                        help(argv[0]);
                }
        }
    }
    return 2;
}
