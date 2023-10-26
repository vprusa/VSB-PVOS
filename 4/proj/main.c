#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/select.h>
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

int readline( int fd, void *ptr, int len, int tout_ms);

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
        printf("Vystup:\n%s\n", buffer);
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
int readline( int fd, void *ptr, int len, int tout_ms) {
    int retval;
    int bytes_read = 0;
    int n;

    struct timeval timeout;
    struct timeval now;
    struct timeval endtime;

    fd_set rfds;

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    // nastaveni neblokujicich operaci nad souborem
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return -1; // pokud se nepovede ukoncuji cteni
    }

    // nastaveni casoveho limitu
    gettimeofday(&now, NULL);
    timeout.tv_sec = tout_ms / 1000;
    timeout.tv_usec = (tout_ms % 1000) * 1000;
    timeradd(&now, &timeout, &endtime);

    // smycka ve ktere ctu davky znaku ze souboru a kontroluji, ze nevypresel casovy limit
    while (bytes_read < len) {
        // nastavim deskriptor souboru do mnoziny deskriptoru, ze kterych se cte
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        // aktualizuji casovy limit
        gettimeofday(&now, NULL);
        timersub(&endtime, &now, &timeout);

        if (timeout.tv_sec < 0 || (timeout.tv_sec == 0 && timeout.tv_usec <= 0)) {
            errno = ETIMEDOUT;
            // pokud vyprsel timout neuspesne ukoncuji cteni
            fcntl(fd, F_SETFL, flags);
            return -1;
        }

        // vyberu deskriptor s maximalni dobu cekani na nej
        retval = select(fd + 1, &rfds, NULL, NULL, &timeout);

        // nepovedlo se ziskat deskriptor souboru
        if (retval == -1) {
            fcntl(fd, F_SETFL, flags);
            return -1; // neuspesne ukoncuji cteni
        } else if (retval) {
            // ctu do bufferu s ofsetem jiz prectenych bytu zbytek nenacetenych bytu
            n = read(fd, (char *)ptr + bytes_read, len - bytes_read);
            if (n == -1) {
                // cteni se nepovedlo
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    // pokud neni chyba blokaci nebo moznosti cist znova, tak ukoncuji neuspesne cteni
                    fcntl(fd, F_SETFL, flags);
                    return -1;
                }
            } else if (n == 0) {
                // uspesne cteni, ale konec souboru
                break;
            } else {
                // zvendu hodnotu citace prectenych znaku hodnotu naposledy prectenych znaku
                bytes_read += n;
            }
        } else {
            // cas vyprsel, neuspesne ukoncuji
            errno = ETIMEDOUT;
            fcntl(fd, F_SETFL, flags);
            return -1;
        }
    }

    // vratim rezim souboru do puvodniho stavu
    if (fcntl(fd, F_SETFL, flags) == -1) {
        // v pripade, ze se nepovedlo nastavit rezim souboru do puvodniho stavu, tak neuspesne ukoncuji cteni
        return -1;
    }

    return bytes_read;
}