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
    int timelimit = 5000;
    // pro testovaci ucely testovaci soubor, pozdeji nutne resit cekani
    // fd = open("test.txt", O_RDONLY);
    // dalsi moznost je pouzit tty dle: `stty -F /dev/tty -icanon`
    fd = open("/dev/tty", O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    int result = readline(fd, buffer, sizeof(buffer) - 1, timelimit);

    // vyhodnoceni vystupu
    if (result == -1) {
        if (errno == ETIMEDOUT) {
            printf("\nreadline - chyba pri cteni - dosazen timelimit: %d\n", timelimit);
            perror("readline - timeout\n");
//            perror("readline - chyba pri cteni - dosazen timelimit: %d\n", timelimit);
        } else {
            perror("readline - chyba pri cteni\n");
        }
    } else if (result == 0) {
        perror("prazdny vstup\n");
    } else {
        buffer[result] = '\0';  // ukončení řetězce
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
int readline( int fd, void *ptr, int len, int tout_ms) {
    // inicializace promennych
    int retval;
    int bytes_read = 0; // pocet prectenych bytu
    int n; // citac nasledy prectenych bytu

    // pomocne promenne pro rizeni casovani
    struct timeval timeout;
    struct timeval now;
    struct timeval endtime;

    // promenna pro mnozinu deskriptoru ze kterych se cte
    fd_set rfds;

    // nastaveni flajecek dekstiprotu
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        // nepovedlo se nastavit vlajecky deksriptoru a tak neuspesne ukoncuji cteni
        return -1;
    }
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


        // aktualizuji cas
        gettimeofday(&now, NULL);
        timersub(&endtime, &now, &timeout);

        // pokud vyprsel casovy limit nebo doslo k chybe, tak ukoncuji cteni
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
            } else if (
                    n == 0 ||
                    ((char *)ptr + bytes_read)[0] == '\n' ||
                    ((char *)ptr + bytes_read)[0] == '\r'
            ) {
                // uspesne cteni, ale konec souboru
                char * last_char = ((char *)(ptr + bytes_read + n));
                last_char[0]= '\0';
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