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
}

/**
Rodič si bude stále udržovat N potomků. N bude argument programu.

Myšlenka:
*/
void second() {
  /**
  while () {
    if ( fork() == 0 ) { sleep( rand() % T ); return 0; }
    usleep( ? );
    while... waitpid( ... NOHANG ).... // posbirá jen ukončené potomky
  }
  */
}

int main() {
   // printf() displays the string inside quotation
   printf("Hello, World!\n");
   srand(time(NULL));
   first(5);
   return 0;
}
