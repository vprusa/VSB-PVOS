#include <stdio.h>

#include <sys/shm.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <mqueue.h>

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
#define MAX_CAPACITY 4
#define APP_1_MAX_COUNTER 10
#define APP_2_MAX_COUNTER 10

// Key for the shared memory and semaphore set
#define SHM_KEY 1234

// Structure for the crate
struct Crate {
    int capacity;
    int current_load;
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
        sleep(2);
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

    // Create or get System V message queue
    int msgq_id = msgget(SHM_KEY, IPC_CREAT | 0666);
    if (msgq_id == -1) {
        perror("msgget");
        return 1;
    }

    // Message structure for receiving crate information
    struct CrateMessage crateMsg;

    int counter = APP_2_MAX_COUNTER; // Initialize the counter
    while (counter > 0) {
        debug(ORANGE_COLOR "  app_2: " RESET_COLOR "Waiting for permission to proceed...");

        // Wait for permission to proceed using System V message queue
        if (msgrcv(msgq_id, &crateMsg, sizeof(struct Crate), 2, 0) == -1) {
            debug(ORANGE_COLOR "  app_2: " RESET_COLOR "No message...");
        }
        debug(ORANGE_COLOR "  app_2: " RESET_COLOR "Proceeding...");

        if (crate->current_load > 0) {
            crate->current_load--;
            debug(ORANGE_COLOR "app_2: " RESET_COLOR "Emptying crate, current load: %d/%d",
                  crate->current_load, crate->capacity);
        } else {
            debug(ORANGE_COLOR "  app_2: " RESET_COLOR "Empty crate");
        }
        counter--; // Decrement the counter
        sleep(1);
    }

    // Detach shared memory
    shmdt(crate);

    return 0;
}

mqd_t crate_mq;

/*
 *  1.  Jeden (program) proces bude plnit přepravku výrobky, přepravka a její stav je součástí zprávy.
 *      Do přepravky může v dané chvíli přistupovat jen jeden proces.
 *      Přepravka má kapacitu a aktuální stav.
 */
int app_1_X(int app_1_cnt) {
    debug(BRIGHT_CYAN_COLOR "  app_1_X: " RESET_COLOR "Started...");

    // Create or open POSIX message queue
    struct mq_attr attr;
    // Set maximum number of messages to MAX_CAPACITY same as the crate capacity
    attr.mq_maxmsg = MAX_CAPACITY;
    attr.mq_msgsize = sizeof(struct CrateMessage);

    crate_mq = mq_open("/crate_mq_X", O_CREAT | O_RDWR | O_NONBLOCK, 0666, &attr);
    if (crate_mq == (mqd_t)-1) {
        perror("mq_open");
        return 1;
    }

    // Message structure for sending crate information
    struct Crate crate;
    crate.capacity = MAX_CAPACITY;
    crate.current_load = 0;

    struct CrateMessage crateMsg;
    crateMsg.mtype = 1; // Message type 1
    crateMsg.crate = crate;

    // Reset the crate queue before entering the first while loop
    while (mq_receive(crate_mq, (char*)&crateMsg, sizeof(struct CrateMessage), NULL) != -1) {
        debug(BRIGHT_CYAN_COLOR "  app_1_X: " RESET_COLOR "Clearing previous crate messages...");
    }

    // Close and unlink POSIX message queue
    mq_close(crate_mq);
    mq_unlink("/crate_mq_X");

    while (app_1_cnt > 0) {

        // Create or open POSIX message queue
        crate_mq = mq_open("/crate_mq_X", O_CREAT | O_RDWR | O_NONBLOCK, 0666, &attr);
        if (crate_mq == (mqd_t)-1) {
            perror("mq_open");
            return 1;
        }

        debug(BRIGHT_CYAN_COLOR "   app_1_X: " RESET_COLOR "Waiting for permission to proceed...");

        int counter = APP_1_MAX_COUNTER; // Initialize the counter
        while (counter > 0) {
            debug(BRIGHT_CYAN_COLOR "   app_1_X: " RESET_COLOR "While...");

            crate = crateMsg.crate;
            debug(BRIGHT_CYAN_COLOR "   app_1_X: " RESET_COLOR "Proceeding...");

            if (crate.current_load < crate.capacity) {
                crate.current_load++;
                debug(BRIGHT_CYAN_COLOR "app_1_X: " RESET_COLOR "Filling the crate, current load: %d/%d",
                      crate.current_load, crate.capacity);

                // Signal completion of the task using POSIX message queue
                crateMsg.mtype = 2; // Message type 2 for completion
                crateMsg.crate = crate;
                // Sends message to queue with priority of current load to force FIFO queue
                if (mq_send(crate_mq, (const char*)&crateMsg, sizeof(struct CrateMessage),
                            crate.current_load) == -1) {
                    perror("mq_send");
                    return 1;
                }

            } else {
                debug("  " BRIGHT_CYAN_COLOR "app_1_X: " RESET_COLOR "Exchanging the crate");
            }


            counter--; // Decrement the counter
            usleep(100000);
        }
        sleep(2);
        app_1_cnt--;
        // Close and unlink POSIX message queue
        mq_close(crate_mq);
        mq_unlink("/crate_mq_X");
    }

    // Close and unlink POSIX message queue
    mq_close(crate_mq);
    mq_unlink("/crate_mq_X");
    return 0;
}

/*
 *  2.  Druhý (program) proces bude přepravky vyměňovat. Po výměně rozběhne “plniče”.
 */
int app_2_X() {
    // sanity wait
    sleep(1);
    debug(MAGNETA_COLOR "  app_2_X: " RESET_COLOR "Started...");

    // Create or open POSIX message queue
    struct mq_attr attr;
    // Set maximum number of messages to MAX_CAPACITY same as the crate capacity
    attr.mq_maxmsg = MAX_CAPACITY;
    attr.mq_msgsize = sizeof(struct CrateMessage);

    crate_mq = mq_open("/crate_mq_X", O_CREAT | O_RDWR | O_NONBLOCK, 0666, &attr);
    if (crate_mq == (mqd_t)-1) {
        perror("mq_open");
        return 1;
    }

    struct Crate crate;
    crate.capacity = MAX_CAPACITY;
    crate.current_load = 0;

    struct CrateMessage crateMsg;
    crateMsg.mtype = 1; // Message type 1

    int counter = APP_2_MAX_COUNTER; // Initialize the counter
    while (counter > 0) {
        debug(MAGNETA_COLOR "  app_2_X: " RESET_COLOR "Waiting for permission to proceed...");

        // Wait for permission to proceed using POSIX message queue
        if (mq_receive(crate_mq, (char*)&crateMsg, sizeof(struct CrateMessage), NULL) == -1) {
            debug(MAGNETA_COLOR "  app_2_X: " RESET_COLOR "No message...");
        }
        crate = crateMsg.crate;
        debug(MAGNETA_COLOR "  app_2_X: " RESET_COLOR "Proceeding...");

        if (crate.current_load > 0) {
            crate.current_load--;
            debug(MAGNETA_COLOR "app_2_X: " RESET_COLOR "Emptying crate, current load: %d/%d",
                  crate.current_load, crate.capacity);
        } else {
            debug(MAGNETA_COLOR "  app_2_X: " RESET_COLOR "Empty crate");
        }

        // Signal completion of the task using POSIX message queue
        crateMsg.mtype = 2; // Message type 2 for completion
        crateMsg.crate = crate;

        counter--; // Decrement the counter
        sleep(1);
    }

    // Close and unlink POSIX message queue
    mq_close(crate_mq);
    mq_unlink("/crate_mq_X");

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
