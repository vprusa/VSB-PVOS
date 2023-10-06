#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <time.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/**
Vytvořte si proces s N potomky, kdy potomci se náhodně ukončí různým způsobem. Např. korektně i nekorektně: neplatný pointer, dělení 0. Proveďte exec, např. programu ls.
*/
void first(int N) {
  printf("first\n");
  pid_t child_pid, wpid;
  int status = 0;

  pid_t pid = 0;
  int pid_id = 0;
  for (int i = 1; i <= N; i++) {
    if(pid == 0) {
      pid = fork();
      pid_id = i;
    }
  }

  if(pid == 0) {
    pid_id = 0;
    printf("Main pid %i: %i\n", pid_id, pid);
  } else {
    int ri = (rand() + pid_id) % (N-1);
    printf("pid %i: %i, rand: %i\n", pid_id, pid, ri);
    // nahodne ukonceni

    switch(pid_id) {
      case 1:
        printf("neplatny pointer\n");
        int * ptr = (231987264123123123);
        printf("%s\n", ptr);
      break;
      case 2:
        printf("deleni nulou\n");
        float fail = 1.0f / 0.0f;
      break;
      case 3:
      default:
        printf("exec ls\n");
        int status;
        char *args[2];

        args[0] = "/bin/ls";        // first arg is the full path to the executable
        args[1] = NULL;             // list of args must be NULL terminated

        if ( fork() == 0 )
            execv( args[0], args ); // child: call execv with the path and the args
        else
            wait( &status );        // parent: wait for the child (not really necessary)
      break;
    }
  }
  
  printf("waiting: pid: %i, rand: %i\n", pid_id, pid);
  while ((wpid = wait(&status)) > 0); // this way, the father waits for all the child processes 
  printf("first - done\n");
}

/**
Rodič si bude stále udržovat N potomků. N bude argument programu.

Myšlenka:
*/
void second(int N) {
  printf("second\n");
  int T = 1;
  int Tu = 100000;
  /**
  while () {
    if ( fork() == 0 ) { sleep( rand() % T ); return 0; }
    usleep( ? );
    while... waitpid( ... NOHANG ).... // posbirá jen ukončené potomky
  }
  */

  /*
   * Rodič bude opět rozpoznávat způsob ukončení potomka a povede si statistiku, kolik procesů vytvořil, kolik jich skončilo a cca každou vteřinu vypíše stav.
  */

  pid_t child_pid, wpid;
  int status = 0;

  pid_t pid = 0;
  int pid_id = 0;
/*  for (int i = 1; i <= N; i++) {
    if(pid == 0) {
      pid = fork();
      pid_id = i;
    }

    if ( fork() == 0 ) { 
      sleep( rand() % T ); return 0; 
    }
    usleep( Tu );
    // while waitpid( ... NOHANG ).... // posbirá jen ukončené potomky
    while ((wpid = wait(&status)) > 0);
  }*/

  printf("waiting: pid: %i, rand: %i\n", pid_id, pid);
  // while ((wpid = wait(&status)) > 0); // this way, the father waits for all the child processes 
  printf("second - done\n");
}

int second2(int NUM_PROCESSES) {
    int MAX_TIME = 1;
    int pid, status;
    int running_time[NUM_PROCESSES];
    int finish_state[NUM_PROCESSES];
    int num_finished = 0;
    int num_exceptions = 0;

    // Spawn child processes
    for (int i = 0; i < NUM_PROCESSES; i++) {
        pid = fork();
        if (pid == 0) {
            // Child process
//            srand(time(NULL));
            running_time[i] = rand() % MAX_TIME + 1;
            printf("Process %d started\n", i);
            printf("Running time: %d ms\n", running_time[i]);
            sleep(running_time[i]);
            if (rand()  % 2 == 0) {
                printf("Process %d finished successfully\n", i);
                finish_state[i] = 0;
            } else {
                printf("Process %d ended with exception\n", i);
                finish_state[i] = 1;
                num_exceptions++;
            }
            exit(0);
        } else if (pid < 0) {
            // Error spawning process
            printf("Error spawning process %d\n", i);
            exit(1);
        }
    }

    // Wait for child processes to finish
    for (int i = 0; i < NUM_PROCESSES; i++) {
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            num_finished++;
        }
    }

    // Print statistics
    printf("Number of finished processes: %d\n", num_finished);
    printf("Number of exceptions: %d\n", num_exceptions);
    for (int i = 0; i < NUM_PROCESSES; i++) {
        printf("Process %d: Running time = %d ms, Finish state = %d\n", i, running_time[i], finish_state[i]);
    }

//    return 0;
}

int main() {
   // printf() displays the string inside quotation
   int parent_pid = getpid();
   printf("init %i\n", parent_pid);
   srand(time(NULL));
//   first(5);
   printf("second %i vs new %i\n", parent_pid, getpid());
   if(getpid() == parent_pid) {
       second2(5);
   }
   return 0;
}
