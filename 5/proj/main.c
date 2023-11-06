#include <stdio.h>

#include <stdio.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/select.h>

/**
 * Zadani:
 *    Upravte příklad pro SIGIO a rouru na příklad s N potomky a N rourami.
 *    - wtf ... ktery priklad? ...
 *    Sighandler bude využívat strukturu siginfo_t.
 *    - wtf #2
 *    Upravte příklad pro aio tak, aby místo SIGEV_NONE využil SIGEV_SIGNAL a SIGEV_THREAD.
 *    Opět N potomků a N rour.
 *    - wtf #3
 *
 *    Naimplementujte si funkci readline_tout s vyrovnávacím bufferem. Maximálně se snažte,
 *    aby se funkce readline_tout chovala podobně, jako read.
 *        readline je programátorská etuda - implementace řady problémů
 *        může být vyrovnávací buffer lokální proměnná ve funkci/metodě?
 *        - záleží na implementaci, v případě užití singálů, které používají metody pro zpracování signálu bude použití
 *        -- jedné lokální proměnné zamezeno přístupem ke sdíleným datům
 *        může být pouze jeden buffer? Jak spárovat buffer a file descriptor?
 *        -- budeme-li používat globální promennou napříč voláním `main` i funkce `sighandleru`, které sdílí svá datat,
 *        -- tak není nutno pouřívat file
 *        test: echo -en " jojo nene " | ./readline_tout
 *        test: while true; do echo -en " jojo nene "; sleep 1; done | ./readline_tout
 *        co když je řádek delší než buffer?'
 *        - pak buffer načte své maximum a zbytek zahodí, v návratové kódu bude indikace BUFFER overflow,
 *        --- program může mít vstupní parametr vlaječky určující striktost vůči přetečení bufferu a způsob vyřízení
 *        --- této situace
 *        může se přepsat na konci řádku?
 *        - pro náš případ ne a nejsem si vědom, že bych se s touto situací prakticky setkal.
 *        -- teoreticky je možné kdeco. např. přepsat buffer na konci řádku bude-li nastaveno parametrem vlaječky atp.
 *        -- v tomto případě je možné, že bude nutné hlídat mazání textu za běhu, zbytek operace by byl stejný
 *        co na konci souboru?
 *        - podobně u konce souboru, ale je to neobvyklá situace
 *        -- s touto situací se jde prakticky setkat bude-li se číst *.log, do kterého byl zpasán výstup např. `clear`
 *        -- operace
 *       když nastane timeout, co s neúplnými daty v bufferu?
 *       -- nastavitelné chování parameterm vlaječky: a) smařu, b) vypíšu; a pro obě možnosti parametrizace vlaječkami
 */

#define SHM_SIZE 44 // 10 * sizeof(int) + sizeof(int)
#define SHM_NAME "myshm"
#define SHM_NAME_PID "myshmpid"
#define ARR_SIZE 10

#define BUFFER_SIZE 2048

#define DEBUG

#ifdef DEBUG
#define debug(fmt, ...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt "\n", \
        __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define debug(fmt, ...) // Nothing
#endif

static volatile char buffer[BUFFER_SIZE];

int readline( int fd, void *ptr, int len, int tout_ms);

int main(int argc, char *argv[]) {
//    int fd;
    int timelimit = 5000;
//    int mknod(const char * pathname , mode_t mode, dev_t dev);
//    int mkfifo(const char * pathname , mode_t mode);
//    int result =  mkfifo("mypipe", O_RDWR);

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Continuously read from stdin and print the data to stdout
    while ((bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE)) > 0) {
        write(STDOUT_FILENO, buffer, bytes_read);
    }

    if (bytes_read == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }

/*
    int fd;
//    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Open the FIFO for reading
//    fd = open("/tmp/myfifo", O_RDONLY);
//    if (fd == -1) {
//        perror("open");
//        exit(EXIT_FAILURE);
//    }

    // Continuously read from FIFO and print the data to standard output
    while (1) {
//        bytes_read = read(fd, buffer, BUFFER_SIZE);
        if (bytes_read > 0) {
//            write(STDOUT_FILENO, buffer, bytes_read);
            printf("res (%d): %s\n", bytes_read, buffer);
        } else if (bytes_read == 0) {
            // No more data, the writing side of the FIFO was closed
            break;
        } else {
            // An error occurred
            perror("read");
//            close(fd);
//            exit(EXIT_FAILURE);
        }
    }

    close(fd);
*/
    // pro testovaci ucely testovaci soubor, pozdeji nutne resit cekani
    // fd = open("test.txt", O_RDONLY);
    // dalsi moznost je pouzit tty dle: `stty -F /dev/tty -icanon`
    /*fd = open("/dev/tty", O_RDONLY);
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

    close(fd);*/
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