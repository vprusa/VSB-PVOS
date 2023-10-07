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
//    int status;

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
    printf("first\n");
    pid_t child_pid, wpid;
    int status = 0;

    pid_t pid = 0;
    int pid_id = 0;
    for (int i = 1; i <= N; i++) {
        if(pid == 0) { // TODO
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
//    int status;

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
/*
  pid_t child_pid, wpid;
  int status = 0;

  pid_t pid = 0;
  int pid_id = 0;*/
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

  /*printf("waiting: pid: %i, rand: %i\n", pid_id, pid);
  // while ((wpid = wait(&status)) > 0); // this way, the father waits for all the child processes 
  printf("second - done\n");*/
}

int second2(int NUM_PROCESSES) {
    int MAX_TIME = 1;

//    pid_t pid = getpid();
    int status;
    pid_t wpid = 0;
    int running_time[NUM_PROCESSES];
    int finish_state[NUM_PROCESSES];
    pid_t pids[NUM_PROCESSES];
    int num_finished = 0;
    int num_exceptions = 0;
//    struct timeval start_time;
//    struct timeval end_time;
    time_t start_time_all = time(NULL);
/*
    printf("Process %i starting childs\n", pid);
    for (int i = 0; i <= NUM_PROCESSES; i++) {
//        if (pid == getpid()) {
        if (wpid == 0) {
            wpid = fork();
        } else {
        }
        printf("Process %i : %i started\n", i, getpid());
    }*/
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
//        if(pid == 0) {
//        if(pid == 0) {
//            printf("Start %i : %i %i %i started\n", i, parent_pid, getpid(), getppid());
//        }
    }

/*    for (int i = 0; i < NUM_PROCESSES; i++) {
        if(pid == 0) {
            running_time[i] = 0;
            pid = fork();
            pids[i] = pid;
            pid_id = i;
        }
    }*/


/*
    if(getpid() == 0) {
        printf("main process\n", 0, getpid());
        // Wait for child processes to finish
        for (int i = 0; i < NUM_PROCESSES; i++) {
//            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                num_finished++;
            }
        }
    }
*/
//    if (getpid() != parent_pid) {
    if (pid == 0) {
//        float xx = 13123 / 0.0f;
        int rand_sleep_time = (((rand() + getpid()) % NUM_PROCESSES) * 4) + 5;
        printf("!!! sleeping: pid: %i, pid_id: %i, sleep time: %i\n", getpid(), pid_id, rand_sleep_time);
        sleep((rand_sleep_time));
//        running_time[pid_id] = rand_sleep_time;
        printf("!!! sleeping: pid: %i, pid_id: %i, sleep time: %i - done\n", pid, pid_id, rand_sleep_time);
    }


//    printf("waiting: pid: %i, pid_id: %i\n", pid, pid_id);
//    while ((wpid = wait(&status)) > 0); // this way, the father waits for all the child processes
    // Wait for child processes to finish

//    for (int i = 0; i < NUM_PROCESSES; i++) {

    if(getpid() == parent_pid) {
//    if(pid > 0) {
        printf(".-!!! Main process, %i, %i, %i, %i\n", getpid(), getppid(), pid, parent_pid);

//        time_t rawtime;
//        struct tm * timeinfo;
//        time ( &rawtime);
//        timeinfo = localtime ( &rawtime );

//        printf ( "Current local time and date: %s", asctime (timeinfo) );

        time_t start_time = time(NULL);

//        pid_t wpid;
        do {
            int status2 = 1;
//            pid_t last;
//            while((last = waitpid(-1, &status, WNOHANG)) > 0) {
//            while((waitpid(-1, &status, WNOHANG)) > 0) {
//            last = waitpid(-1, &status, 0);
//            while ((wpid = wait(&status)) > 0) { // this way, the father waits for all the child processes
                pid_t child_pid;
                for (int i = 0; i < NUM_PROCESSES; i++) {
                    child_pid = waitpid(pids[i], &status2, WNOHANG);
//                    if(WIFEXITED(status2)) {
                    if(WIFEXITED(status2) && finish_state[i] == -1) {
                        num_finished++;
//                        printf("... exited: nth: %i, %i: status: %d - %i done\n", i, pids[i], status2, num_finished);
                        printf("... exited: nth: %i, %i: status: - %i done\n", i, pids[i], num_finished);
                        finish_state[i] = WEXITSTATUS(status2);
//                    } else {
//                        printf("... not exited: nth: %i, %i: status: - %i done\n", i, pids[i], num_finished);
                    }

//                    if (WIFEXITED(status2)) {
//                        printf("... exited: nth: %i,  %i: status: %i - %i done\n", last, pids[i], status2, num_finished);
//                        printf("... exited: nth: %i, %i: status: %d - %i done\n", i, pids[i], status2, num_finished);
//                    }
//                    finish_state[i] = status2;
//                    if (child_pid > 0) {
//                        last = child_pid;
//                        break;
//                    }
                }
//                waitpid(child_pid, &status2, WNOHANG);

//            }

//
//            time_t rawtime_now;
//            struct tm * timeinfo_now;
//            time ( &rawtime_now);
//            timeinfo_now = localtime ( &rawtime_now );

//            time_t start_time = ;
            time_t end_time = time(NULL) - start_time;
            if(end_time > 1) {
                start_time = time(NULL);

//                printf ( "Current local time and date: %i\n", start_time);
//                printf("Status of %i: pid: %i, %i, %i, %i\n", num_finished, pid, child_pid, getpid(), end_time);
//                printf("\n?? Status of %i: pid: %i, %i, %i, %i\n", num_finished, pid, child_pid, getpid(), status);
                printf("\n?? Status ");
                printf("of %i: pid: %i, %i, %i\n", num_finished, pid, getpid(), getppid());
                for (int i = 0; i < NUM_PROCESSES; i++) {
                    printf("?? - status of %i: %i s: %i, pid: %i\n", i, pids[i], finish_state[i], getpid());
                }
            }


        } while (num_finished < NUM_PROCESSES);

        printf("\n?? Status ");
        printf("of %i: pid: %i, %i, %i\n", num_finished, pid, getpid(), getppid());
        for (int i = 0; i < NUM_PROCESSES; i++) {
            printf("?? - status of %i: %i s: %i, pid: %i\n", i, pids[i], finish_state[i], getpid());
        }

/*        while (num_finished < NUM_PROCESSES) {
    //        waitpid(pid, &status, 0);
//            printf("waiting of %i: pid: %i, pid_id: %i\n", num_finished, pid, pid_id);
            for (int i = 0; i < NUM_PROCESSES; i++) {
                printf("status: pid_id: %i, running_time: %i, status: %i\n", i, running_time[i], finish_state[i]);
            }
            for (int i = 0; i < NUM_PROCESSES; i++) {
//                int stat = -1;
                waitpid(pids[i], &(finish_state[i]), 0);
//                waitpid(pid, &(finish_state[i]), 0);
//                if (!WIFEXITED(finish_state[i])) {
//                    waitpid(pids[i], &(finish_state[i]), 0);
//                }
//                finish_state[i] = stat;
                printf("exited: nth: %i, status: %i - done\n", pids[i], finish_state[pid_id]);
                if (WIFEXITED(finish_state[i])) {
                    num_finished++;
//                    printf("exited: nth: %i, status: %i - done\n", pid, finish_state[pid_id]);
                }
            }
            sleep(2);
        }*/
    }
//    printf("waiting: pid: %i, pid_id: %i\n", pid, pid_id);
//    while ((wpid = wait(&status)) > 0); // this way, the father waits for all the child processes


    if(getpid() != parent_pid) {
//        finish_state[pid_id] = status;
//        int run_time = (time(NULL) - start_time_all);
        printf("... - done - %i\n", getpid());
//        printf("... done: %d, status: %i - time: %i - done\n", pid, getpid(), running_time);
//        printf("first - done, %d\n", num_finished);
    }

/*
    // Spawn child processes
    for (int i = 0; i < NUM_PROCESSES; i++) {
        pid = fork();
        if (pid != 0) {
            // Child process
            gettimeofday(&start_time, NULL);
            running_time[i] = 0;
            printf("Process %d started\n", i);
            printf("Running time: %d seconds\n", running_time[i]);
            while (running_time[i] < MAX_TIME * 1000) {
                // Simulate work
                usleep(1000);
                running_time[i] += 1000;
            }
            if (rand() % 2 == 0) {
                printf("Process %d finished successfully\n", i);
                finish_state[i] = 0;
            } else {
                printf("Process %d ended with exception\n", i);
                finish_state[i] = 1;
                num_exceptions++;
            }
            gettimeofday(&end_time, NULL);
//            running_time[i] = (end_time.tv_sec - start_time.tv_sec) * 1000 + (end_time.tv_usec - start_time.tv_usec) / 1000;
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
    }*/

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
