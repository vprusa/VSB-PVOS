#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/select.h>
/**
 * Zadani:
 *    Napište si dvě aplikace, kdy
 *    a) jedna bude měnit obsah sdílené paměti,
 *    kde bude 10 čísel a jejich součet.
 *    b) Druhá aplikace provede kontrolu. Notifikace pomocí pojmenované roury.
 *    Data int *pole, struct data { char [11][20]; }
 *    Dokončete readline.
 */

void app_1( );
void app_2( );


void app_1( ) {
}

void app_2( ) {
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s [1|2]\n", argv[0]);
        return 1;
    } else {
        switch (argv[1][0]) {
            case '1':
                app_1();
                break;
            case '2':
                app_2();
                break;
            default:
                printf("Usage: %s [1|2]\n", argv[0]);
                return 1;
        }
    }
    return 0;
}
