#include <stdio.h>
#include <unistd.h>

#include <time.h>
#include <stdlib.h>

#include <sys/wait.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <stdio.h>
#include <stdatomic.h>
#include <pthread.h>
#include <memory.h>
#include<stdio.h>
#include<unistd.h>
#include<signal.h>
#include <unistd.h>

/**
 *
 * Zadani:
 *   Naimplementujte “vrtuli” ze znaků “-/|\” pomocí printf.
 *   1) Jednou s fflush a
 *   2) pak se změnou nastavení bufferu.
 *   Vrtule: str='-/|\'; n=1; while true; do n=$[(n+1)%4]; echo -en "${str:$n:1} \r"; sleep 0.1; done
 *   3) Zkuste vrtuli se signálem SIGALRM.
 *   Nastavení rychlejšího časování viz:
 *   github https://github.com/osy-cs/osy-gthreads/blob/main/gthr.c.
 *   Vytvořte si program s několika (min. 5) potomky a pokuste se je řídit pomocí SIGSTOP a SIGCONT.
 *   Je potřeba řídit “něco” viditelného.
 *
 *   https://linuxhint.com/sigalarm_alarm_c_language/
 *   https://networklessons.com/uncategorized/pause-linux-process-with-sigstop-sigcont
 */


_Atomic int proc_exit_calls = 0;
_Atomic int proc_exit_loop_calls = 0;
_Atomic int proc_exit_loop_return_calls = 0;

void vrtule_1();
void vrtule_2();
void vrtule_3();
void vrtule_4();

void proc_exit() {
    int wstat;
    pid_t pid;

    atomic_fetch_add(&proc_exit_calls, 1);
    while (1) {
        atomic_fetch_add(&proc_exit_loop_calls, 1);
        pid = wait3(&wstat, WNOHANG, NULL);
        if (pid == 0 || pid == -1) {
            atomic_fetch_add(&proc_exit_loop_return_calls, 1);
            return;
        }
        printf("-- Return code: %d, %i, %i, %i, %i\n",
               wstat, getpid(), proc_exit_calls, proc_exit_loop_calls, proc_exit_loop_return_calls);
    }
}

int* second_assignment(int NUM_PROCESSES, int shouldSleep) {
    signal (SIGCHLD, proc_exit);
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
        // TODO parametrize
        if(shouldSleep == 1) {
            printf("... start sleeping, pid: %i\n", getpid());
            while ( 1 ) {
                sleep( 1 ); return NULL;
            }
        } else {
            printf("... returning: pid: %i\n", getpid());
            return NULL;
        }
    }

    if(getpid() == parent_pid) {
        printf("\n!!! Main process, %i, %i, %i, %i\n", getpid(), getppid(), pid, parent_pid);

        time_t start_time = time(NULL);
        int proc_exit_calls_times_same = 0;
        int proc_exit_calls_prev = proc_exit_calls;
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
                printf("?? Statistics proc_exit_calls: %i, proc_exit_loop_calls: %i, proc_exit_loop_return_calls: %i, run_time: %i[s]\n", proc_exit_calls, proc_exit_loop_calls, proc_exit_loop_return_calls, cur_running_time);

                for (int i = 0; i < NUM_PROCESSES; i++) {
                    if(finish_state[i] == -1) {
                        running_time[i] = (time(NULL) - start_time_all);
                    }
                }

                if(proc_exit_calls == proc_exit_calls_prev) {
                    if(++proc_exit_calls_times_same > 3) {
                        printf("?? breaking the loop because number of exited processes did not change in %i loops.\n", proc_exit_calls_times_same);
                        break;
                    }
                }else {
                    proc_exit_calls_prev = proc_exit_calls;
                }
            }
            int status3 = -1;
            pid_t result = waitpid(-1, &status3, WNOHANG);
            if(result != -1) {
                printf("\n?? No children processes running, stop the loop: statistics %i: pid: %i, %i, %i\n",
                       num_finished, pid, getpid(), getppid());
                break;
            }
        } while (num_finished < NUM_PROCESSES);

        time_t cur_running_time = (time(NULL) - start_time_all);
        printf("\n?? Statistics of %i: pid: %i, %i, %i, run_time: %i[s]\n", num_finished, pid, getpid(), getppid(), cur_running_time);
        printf("\n?? Statistics proc_exit_calls: %i, proc_exit_loop_calls: %i, proc_exit_loop_return_calls: %i, run_time: %i[s]\n", proc_exit_calls, proc_exit_loop_calls, proc_exit_loop_return_calls, cur_running_time);
    }
}

int main() {
/*    int parent_pid = getpid();
    srand(time(NULL));
    if(parent_pid == getpid()) {
        printf("!!! Should sleep\n");
        second_assignment(1000, 1);
    }
    if(parent_pid == getpid()) {
        printf("!!! Should not sleep\n");
        second_assignment(1000, 0);
    }*/

// TODO argumentize
//    vrtule_1();
//    vrtule_2();
//    vrtule_3();
    vrtule_4();
    return 0;
}



/**
 * Zadani:
 *   Naimplementujte “vrtuli” ze znaků “-/|\” pomocí printf. Jednou s fflush
 */
void vrtule_1() {
    char str[] = "-/|\\";
    int n = 1;
    while (1) {
        n = (n + 1) % 4;
        printf("%c \r", str[n]);
        fflush(stdout);
        sleep(1);
    }
}

/**
 *  a pak se změnou nastavení bufferu.
 *
 *  https://www.tutorialspoint.com/c_standard_library/c_function_setvbuf.htm
 */
void vrtule_2() {
    char str[] = "-/|\\";
    int n = 0;
    int buff_size = 1;
    char buff[buff_size];
    memset( buff, '\0', sizeof( buff ));
    fprintf(stdout, "Going to set full buffering on\n");
    setvbuf(stdout, buff, _IOFBF, buff_size);
    while (1) {
        fprintf(stdout, "%c\r", str[n]);
        sleep(1);
        fprintf(stdout, "%c\r", str[n + 1]);
        sleep(1);
        fprintf(stdout, "%c\r", str[n + 2]);
        fflush(stdout);
        sleep(1);
        fprintf(stdout, "%c\r", str[n + 3]);
        sleep(1);
        n = n % 4;
    }
}


volatile int n = 0;
void sig_handler(int signum){
    char str[] = "-/|\\";
    fprintf(stdout, "\r%c", str[n]);
    fflush(stdout);
    n = (n+1) % 4;
}

/**
 *   Zkuste vrtuli se signálem SIGALRM.
 */
void vrtule_3() {
    printf("Vrtule 3 - SIGALRM\n");
    signal(SIGALRM,sig_handler); // Register signal handler

    while(1) {
// pokud misto ualarm budu chtit pouzit `alarm`, tak minimalni rychlost toceni vrtule je 1s
//        alarm(1);
//        usleep(100000);  // pod 1 sekundu alarm negunfuje ani s `usleep(X)`
        ualarm(100000, 0);
        sleep(1);
    }
}

/**
 * Vytvořte si program s několika (min. 5) potomky a pokuste se je řídit pomocí SIGSTOP a SIGCONT.
 *   Je potřeba řídit “něco” viditelného.
 */
void vrtule_4() {

    int childs_cnt = 5;
    int childs[childs_cnt];
    pid_t pid = -1;
    for(int i = 0; i < childs_cnt; i++) {
        if(pid != 0) {
            pid = fork();
            childs[i] = pid;
        }
    }
    if(pid == 0) {
        printf("!child %d\n", pid);
    } else {
//        printf("?parent %d\n", pid);
    }

}
