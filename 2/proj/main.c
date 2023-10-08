#include <stdio.h>
#include <unistd.h>

#include <time.h>
#include <stdlib.h>

#include <sys/wait.h>

/**
    Vyzkoušejte, kolik procesů vám systém dovolí vytvořit procesů. { while fork…. while waitpid….
    Pro testování si raději nastavte ulimit -u 10000.
    Kolik současně běžících vláken můžete vytvořit. Kód vlákna bude { while ( 1 ) sleep( 1 ); return nullptr; }
    Kolik vláken můžete postupně vytvořit. Kód vlákna bude {return nullptr;} a nedělá se ani join ani detach.
    Udržujte si cca 1000 potomků, zachytávejte SIGCHLD, v sighandleru provádějte while ...waitpid... a počítejte počet signálů a počet ukončených potomků.
 */
int* second_assignment(int NUM_PROCESSES) {

    int running_time[NUM_PROCESSES];
    int finish_state[NUM_PROCESSES];
    pid_t pids[NUM_PROCESSES];
    int num_finished = 0;
    int num_exceptions = 0;
    time_t start_time_all = time(NULL);

    pid_t pid = 0;
    int pid_id = 0;
    for (int i = 0; i <= NUM_PROCESSES; i++) {
        finish_state[i] = -1;
        running_time[i] = -1;
    }

    pid_t parent_pid = getpid();

    for (int i = 0; i < NUM_PROCESSES; i++ ) {
        if(parent_pid == getpid()) {
            pid_id = i;
            pid = fork();
            if(pid == 0) {
                printf("Start %i : %i %i %i started\n", i, parent_pid, getpid(), getppid());
            } else {
                pids[i] = pid;
            }
        }
    }

    if (pid == 0) {
        while ( 1 ) sleep( 1 ); return NULL;
    }

    if(getpid() == parent_pid) {
        printf("\n!!! Main process, %i, %i, %i, %i\n", getpid(), getppid(), pid, parent_pid);

        time_t start_time = time(NULL);

        do {
            for (int i = 0; i < NUM_PROCESSES; i++) {
                int status2 = 1;
                waitpid(pids[i], &status2, WNOHANG);
                if(WIFEXITED(status2) && finish_state[i] == -1) {
                    num_finished++;
                    printf("... exited: nth: %i, %i: status: - %i done\n", i, pids[i], num_finished);
                    finish_state[i] = WEXITSTATUS(status2);
                }
            }

            time_t end_time = time(NULL) - start_time;
            if(end_time >= 1) {
                start_time = time(NULL);
                time_t cur_running_time = (time(NULL) - start_time_all);
                printf("\n?? Status of %i: pid: %i, %i, %i, run_time: %i[s]\n", num_finished, pid, getpid(), getppid(), cur_running_time);
                for (int i = 0; i < NUM_PROCESSES; i++) {
                    if(finish_state[i] == -1) {
                        running_time[i] = (time(NULL) - start_time_all);
                    }
                    printf("?? - status of %i: %i s: %i, pid: %i run_time: %i\n", i, pids[i], finish_state[i], getpid(), running_time[i]);
                }
            }

        } while (num_finished < NUM_PROCESSES);

        time_t cur_running_time = (time(NULL) - start_time_all);
        printf("\n?? Statistics of %i: pid: %i, %i, %i, run_time: %i[s]\n", num_finished, pid, getpid(), getppid(), cur_running_time);
        int failed_processes = 0;
        for (int i = 0; i < NUM_PROCESSES; i++) {
            if(finish_state[i] != 0) {
                failed_processes++;
            }
            printf("?? - status of %i: %i s: %i, pid: %i, run_time: %i[s]\n", i, pids[i], finish_state[i], getpid(), running_time[i]);
        }
        printf("\n?? Statistics failed_processes: %i of %i, run_time: %i[s]\n", failed_processes, NUM_PROCESSES, cur_running_time);
    }

    if(getpid() != parent_pid) {
        printf("... - done - %i\n", getpid());
    }
}

int main() {
   int parent_pid = getpid();
   srand(time(NULL));

   second_assignment(3);
   /*
   first(5);
   if(getpid() == parent_pid) {
       printf("\n\nSecond assignment %i vs new %i\n", parent_pid, getpid());
       second2(6);
   }
   */
   return 0;
}
