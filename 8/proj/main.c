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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <string.h>
#include <semaphore.h>

/**
 * Zadani:
 *
 * Pouze s pomocí fronty zpráv System V i Posix implementujte problém “plnění přepravky”.
 *   Pokuste se u System V využít zprávy více typů.
 *   1.  Jeden (program) proces bude plnit přepravku výrobky, přepravka a její stav je součástí zprávy.
 *         Do přepravky může v dané chvíli přistupovat jen jeden proces.
 *         Přepravka má kapacitu a aktuální stav.
 *   2.  Druhý (program) proces bude přepravky vyměňovat. Po výměně rozběhne “plniče”.
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

// Structure for the message containing crate information
struct CrateMessage {
    long mtype; // Message type
    struct Crate crate; // Crate information
};

int app_1(int app_1_cnt);
int app_2();

int app_1_X(int app_1_cnt);
int app_2_X();

/*
 *  1.  Jeden (program) proces bude plnit přepravku výrobky, přepravka a její stav je součástí zprávy.
 *      Do přepravky může v dané chvíli přistupovat jen jeden proces.
 *      Přepravka má kapacitu a aktuální stav.
 */
int app_1(int app_1_cnt) {
    debug(ORANGE_COLOR "  app_1: " RESET_COLOR "Started...");
    // Attempt to find the existing shared memory segment
    int shm_id = shmget(SHM_KEY, sizeof(struct Crate), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget");
        return 1;
    }

    struct Crate *crate = (struct Crate *)shmat(shm_id, NULL, 0);
    if (crate == (struct Crate *)(-1)) {
        perror("shmat");
        return 1;
    }

    // Initialize the crate
    crate->current_load = 0;
    crate->capacity = MAX_CAPACITY;

    // Create or get System V message queue
    int msgq_id = msgget(SHM_KEY, IPC_CREAT | 0666);
    if (msgq_id == -1) {
        perror("msgget");
        return 1;
    }

    // Reset the message queue to ensure it starts "filling the crate" if it doesn't exist
    if (msgctl(msgq_id, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        return 1;
    }

    // Create a new message queue
    msgq_id = msgget(SHM_KEY, IPC_CREAT | 0666);
    if (msgq_id == -1) {
        perror("msgget");
        return 1;
    }

    // Message structure for sending crate information
    struct CrateMessage crateMsg;
    crateMsg.mtype = 1; // Message type 1

    while (app_1_cnt > 0) {
        debug(GREEN_COLOR "   app_1: " RESET_COLOR "Waiting for permission to proceed...");

        int counter = APP_1_MAX_COUNTER; // Initialize the counter
        while (counter > 0) {
            debug(GREEN_COLOR "   app_1: " RESET_COLOR "While...");

//            // Wait for permission to proceed using System V message queue
////            if (msgrcv(msgq_id, &crateMsg, sizeof(struct Crate), 1, 0) == -1) {
//            if (msgrcv(msgq_id, &crateMsg, sizeof(struct Crate), 1, 0) == -1) {
//                perror("msgrcv");
//                debug(GREEN_COLOR "   app_1: " RESET_COLOR "msgrcv - error...");
//                return 1;
//            }
            debug(GREEN_COLOR "   app_1: " RESET_COLOR "Proceeding...");

            if (crate->current_load < crate->capacity) {
                crate->current_load++;
                debug( GREEN_COLOR "app_1: " RESET_COLOR "Filling the crate, current load: %d/%d",
                      crate->current_load, crate->capacity);
            } else {
                debug("  " GREEN_COLOR "app_1: " RESET_COLOR "Exchanging the crate");
            }

            // Signal completion of the task using System V message queue
            crateMsg.mtype = 2; // Message type 2 for completion
            if (msgsnd(msgq_id, &crateMsg, sizeof(struct Crate), 0) == -1) {
                perror("msgsnd");
                return 1;
            }

            counter--; // Decrement the counter
            usleep(100000);
        }
        sleep(5);
        app_1_cnt--;
    }

    // Detach shared memory
    shmdt(crate);

    // Remove the message queue
    if (msgctl(msgq_id, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        return 1;
    }

    return 0;
}

/*
 *  2.  Druhý (program) proces bude přepravky vyměňovat. Po výměně rozběhne “plniče”.
 */
int app_2() {
    debug(GREEN_COLOR "  app_2: " RESET_COLOR "Started...");
   
    return 0;
}

/*
 *  1.  Jeden (program) proces bude plnit přepravku výrobky, přepravka a její stav je součástí zprávy.
 *      Do přepravky může v dané chvíli přistupovat jen jeden proces.
 *      Přepravka má kapacitu a aktuální stav.
 */
int app_1_X(int app_1_cnt) {
    debug(BRIGHT_CYAN_COLOR "  app_1_X: " RESET_COLOR "Started...");

    return 0;
}

/*
 *  2.  Druhý (program) proces bude přepravky vyměňovat. Po výměně rozběhne “plniče”.
 */
int app_2_X() {
    // sanity wait
    sleep(1);
    debug(MAGNETA_COLOR "  app_2_X: " RESET_COLOR "Started...");

    return 0;
}

void help(char * s) {
    printf("Usage: %s [1|2] [V|X] [1_cnt]\n", s);
    printf("   char 1 - 1 - filler, 2 - emptier [X]\n");
    printf("   char 2 - X - posix, V - System V\n");
    printf("   char 3 - 1_cnt - app_1 Crates count\n");
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
