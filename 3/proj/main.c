#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <memory.h>

/**
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

void vrtule_1();
void vrtule_2();
void vrtule_3();
void vrtule_4();

int main(int argc, char *argv[]) {
    printf("argc: %d\n", argc);
    printf("argv[0]: %s\n", argv[0]);
    printf("- vyber vrtule pomoci: ./main [1,2,3,4]\n");
    printf("   pr. `./main 1`\n");
    if(argc < 2) {
        vrtule_1();
        return 1;
    }
    printf("argv[1]: %s\n\n", argv[1]);
    switch(argv[1][0]) {
        case '1':
            vrtule_1();
            break;
        case '2':
            vrtule_2();
            break;
        case '3':
            vrtule_3();
            break;
        case '4':
        default:
            vrtule_4();
            break;
    }
    return 0;
}

/**
 * Zadani:
 *   Naimplementujte “vrtuli” ze znaků “-/|\” pomocí printf. Jednou s fflush
 */
void vrtule_1() {
    printf("vrtule_1 - flush\n");
    char str[] = "-/|\\";
    int n = 1;
    while (1) {
        n = (n + 1) % 4;
        printf("%c \r", str[n]);
        fflush(stdout);
        // sleep(1); // v sekundach je moc rotace pomalu
        usleep(100000);

    }
}

/**
 *  a pak se změnou nastavení bufferu.
 *
 *  https://www.tutorialspoint.com/c_standard_library/c_function_setvbuf.htm
 */
void vrtule_2() {
    printf("vrtule_2 - buffer\n");
    char str[] = "-/|\\";
    int n = 0;
    int buff_size = 1;
    char buff[buff_size];
    memset( buff, '\0', sizeof( buff ));
    fprintf(stdout, "Going to set full buffering on\n");
    setvbuf(stdout, buff, _IOFBF, buff_size);
    while (1) {
        fprintf(stdout, "%c\r", str[n]);
        usleep(100000);
        fprintf(stdout, "%c\r", str[n + 1]);
        usleep(100000);
        fprintf(stdout, "%c\r", str[n + 2]);
        usleep(100000);
        fprintf(stdout, "%c\r", str[n + 3]);
        usleep(100000);
        fflush(stdout);
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
 *
 *   https://networklessons.com/uncategorized/pause-linux-process-with-sigstop-sigcont
 */
void vrtule_4() {

    char str[] = "-/|\\";
    int childs_cnt = 8;

    fprintf(stdout, "Vrtule 4 - fork - rodic synchronizuje praci deti\n");

    int childs[childs_cnt];
    pid_t pid = -1;
    int idx = -1;
    for(int i = 0; i < childs_cnt; i++) {
        if(pid != 0) {
            pid = fork();
            idx = i;
            childs[i] = pid;
        }
    }
    if(pid == 0) {
        usleep(100000);
        while(1) {
            fprintf(stdout, "\r%c", str[idx % 4]);
            fflush(stdout); // nutne vyprazdnit buffer pro kazdeho potomka
            usleep(100000);
        }
    } else {
        while(1) {
            // pri nespravnem nastaveni rychlosti muze nastat problem se vzorkovanim vypisu na strane bufferu
            for(int i = 0; i < childs_cnt; i++) {
                kill(childs[i], SIGCONT);
                usleep(100000);
                kill(childs[i], SIGSTOP);
            }
        }
    }

}
