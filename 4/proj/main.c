#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <memory.h>

/**
 * Zadani:
 * 0. Pro čtení z terminálu po znacích si nastavte “stty -F /dev/tty -icanon”.
 * 1. Implementujte funkci int readline( int fd, void *ptr, int len, int tout_ms );
 *    pomocí NONBLOCK, select a poll.
 *    Chování funkce read a readline musí být stejné.
 *    Jen readline vrací timeout jako errno s hodnotou ETIME nebo ETIMEDOUT.
 *
 * Viz. man gettimeofday a man timeradd & spol.
 */

int readline( int fd, void *ptr, int len, int tout_ms );

int main(int argc, char *argv[]) {
    while(1) {
        readline();
    }
    return 0;
}

int readline( int fd, void *ptr, int len, int tout_ms) {
    return 0;
}