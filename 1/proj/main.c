#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <time.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/**
Vytvořte si proces s N potomky, kdy potomci se náhodně ukončí různým způsobem. Např. korektně i nekorektně: neplatný pointer, dělení 0. Proveďte exec, např. programu ls.
*/
void first(int N) {
  printf("!!! Starting first first\n");
  pid_t child_pid, wpid;
  int status = 0;
  pid_t parent_pid = getpid();
  pid_t pid = 0;
  int pid_id = 0;
  for (int i = 0; i < N; i++) {
    if(getpid() == parent_pid) {
        pid_id = i;
        pid = fork();
    }
  }

  //if(pid > 0) {
  if(getpid() == parent_pid) {
    printf("\n!!! Main pid %i\n", parent_pid);
  } else {
    int ri = (rand() + pid_id) % (N-1);
    printf("- Child pid_id %i: %i, rand: %i\n", pid_id, getpid(), ri);

    // nahodne ukonceni
    switch(pid_id) {
      case 1:
        printf("- ? neplatny pointer, pid: %i\n", getpid());
//        int * ptr = (231987264123123123);
//        printf("%s\n", ptr);
        exit(11);
      break;
      case 2:
        printf("- ? deleni nulou, pid: %i\n", getpid());
        float fail = 1.0f / 0.0f;
        exit(12);
      break;
      case 3:
//      default:
        printf("- ? exec ls, pid: %i\n", getpid());
        char *args[2];
        int status2 = 0;
        args[0] = "/bin/ls";        // first arg is the full path to the executable
        args[1] = NULL;             // list of args must be NULL terminated

        if ( fork() == 0 )
            execv( args[0], args ); // child: call execv with the path and the args
        else
            wait( &status2 );        // parent: wait for the child (not really necessary)
//        wait( &status2 );        // parent: wait for the child (not really necessary)
      break;
      default:
        printf("- ? default exit(0), pid: %i\n", getpid());
        exit(0);
    }
  }

  if(getpid() == parent_pid) {
    printf("!!! Parent waiting: pid: %i\n", getpid());
    while ((wpid = wait(&status)) > 0); // this way, the father waits for all the child processes
    printf("!!! Parent done: pid: %i\n", getpid());
  }

}

/**
  Rodič si bude stále udržovat N potomků. N bude argument programu.
  Myšlenka:
  while () {
    if ( fork() == 0 ) { sleep( rand() % T ); return 0; }
    usleep( ? );
    while... waitpid( ... NOHANG ).... // posbirá jen ukončené potomky
  }
  * Rodič bude opět rozpoznávat způsob ukončení potomka a povede si statistiku, kolik procesů vytvořil, kolik jich skončilo a cca každou vteřinu vypíše stav.
  */
int second2(int NUM_PROCESSES) {

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
        int rand_in_range = (((rand() + getpid()) % NUM_PROCESSES));
        int rand_sleep_time = (rand_in_range * 4 ) + 5;
        printf("!!! sleep: pid: %i, pid_id: %i, pid: %i, rir: %i, sleep time: %i\n",
               getpid(), pid_id, getpid(), rand_in_range, rand_sleep_time);
        sleep((rand_sleep_time));
        srand(time(NULL));
        int new_rand = rand_in_range % 3;
        printf("!!! sleep: pid: %i, pid_id: %i, pid: %i, rir: %i, sleep time: %i - %i - done\n",
               pid, pid_id, getpid(), rand_in_range, rand_sleep_time, new_rand);

        if (new_rand < 1) {
            printf("!!! Exit ungracefully - 42\n");
            exit(42);
        } else if (new_rand < 2) {
            printf("!!! Exit ungracefully - 43\n");
            exit(43);
        }

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
   first(5);
   if(getpid() == parent_pid) {
//       printf("\n\nSecond assignment %i vs new %i\n", parent_pid, getpid());
//       second2(6);
   }
   return 0;
}
