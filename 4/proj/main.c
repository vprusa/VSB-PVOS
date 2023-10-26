#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


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
    int fd;
    char buffer[2048];
    int result;
    // TODO stty -F /dev/tty -icanon
    // pro testovaci ucely testovaci soubor, pozdeji nutne resit cekani
    fd = open("test.txt", O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    result = readline(fd, buffer, sizeof(buffer) - 1, 100);

    if (result == -1) {
        printf("Chyba pri cteni\n");
        // TODO rozlisit
        //        Jen readline vrací timeout jako errno s hodnotou ETIME nebo ETIMEDOUT.
        // TODO print do perror("xxx");
    } else if (result == 0) {
        printf("Prazdny vstup\n");
    } else {
        printf("Vystup: %s\n", buffer);
    }

    close(fd);
    return 0;
}

/**
 * Precte z fd do ptr (buffer) len znaku, nebo do timeoutu
 *
 * @param fd
 * @param ptr
 * @param len
 * @param tout_ms
 * @return
 */
int readline( int fd, void *ptr, int buffer_size, int tout_ms) {
    int read_bytes = 0;
    char *buffer = ptr;

    int ix = 0, bytes_malloced = 0;

    if (!buffer)
    {
        bytes_malloced = 64;
        buffer = (char *)malloc(bytes_malloced);
        buffer_size = bytes_malloced;
    }

    for (;;++ix)
    {
        int ch;

        if (ix == buffer_size - 1)
        {
            if (!bytes_malloced)
                break;
            bytes_malloced += bytes_malloced;
            buffer = (char *)realloc(buffer, bytes_malloced);
            buffer_size = bytes_malloced;
        }

        ch = getchar();
        if (ch == EOF)
        {
            if (bytes_malloced)
                free(buffer);
            return 0;
        }
        if (ch == '\n')
            break;
        buffer[ix] = ch;
    }

    /* 0 terminate buffer. */
    buffer[ix] = '\0';

    return read_bytes;
}