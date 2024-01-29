/* serv.cpp  -  Minimal ssleay server for Unix
   30.9.1996, Sampo Kellomaki <sampo@iki.fi>

   Modified to accept multiple clients using fork()
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <poll.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>

#include <time.h>
#include <sys/stat.h>


#define HOME "./"
#define CERTF  HOME "my.crt"
#define KEYF  HOME  "my.key"

#define HOME_DIR "/home/vprusa/workspace/school/VSB/1ZS/PVOS/ukoly/zk.2/proj"


#define CHK_NULL(x) if ((x)==NULL) exit (1)
#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(1); }
#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }

// TODO
// "/home/vprusa/workspace/school/VSB/1ZS/PVOS/ukoly/zk/proj/img/recOut.jpg"
/*
#define IMG_PUNC "./img/sem.png"
#define IMG_0 "./img/nula.png"
#define IMG_1 "./img/jedna.png"
#define IMG_2 "./img/dva.png"
#define IMG_3 "./img/tri.png"
#define IMG_4 "./img/ctyri.png"
#define IMG_5 "./img/pet.png"
#define IMG_6 "./img/sest.png"
#define IMG_7 "./img/sedm.png"
#define IMG_8 "./img/osm.png"
#define IMG_9 "./img/devet.png"
*/

#define IMG_PUNC "/home/vprusa/workspace/school/VSB/1ZS/PVOS/ukoly/zk/proj/img/sem.png"

#define IMG_0 "/home/vprusa/workspace/school/VSB/1ZS/PVOS/ukoly/zk/proj/img/nula.png"
#define IMG_1 "/home/vprusa/workspace/school/VSB/1ZS/PVOS/ukoly/zk/proj/img/jedna.png"
#define IMG_2 "/home/vprusa/workspace/school/VSB/1ZS/PVOS/ukoly/zk/proj/img/dva.png"
#define IMG_3 "/home/vprusa/workspace/school/VSB/1ZS/PVOS/ukoly/zk/proj/img/tri.png"
#define IMG_4 "/home/vprusa/workspace/school/VSB/1ZS/PVOS/ukoly/zk/proj/img/ctyri.png"
#define IMG_5 "/home/vprusa/workspace/school/VSB/1ZS/PVOS/ukoly/zk/proj/img/pet.png"
#define IMG_6 "/home/vprusa/workspace/school/VSB/1ZS/PVOS/ukoly/zk/proj/img/sest.png"
#define IMG_7 "/home/vprusa/workspace/school/VSB/1ZS/PVOS/ukoly/zk/proj/img/sedm.png"
#define IMG_8 "/home/vprusa/workspace/school/VSB/1ZS/PVOS/ukoly/zk/proj/img/osm.png"
#define IMG_9 "/home/vprusa/workspace/school/VSB/1ZS/PVOS/ukoly/zk/proj/img/devet.png"

//#define IMG_OUT "/home/vprusa/workspace/school/VSB/1ZS/PVOS/ukoly/zk/proj/img/out.jpg"
//#define IMG_OUT "/home/vprusa/workspace/school/VSB/1ZS/PVOS/ukoly/zk/proj/img/out.jpg"
//#define IMG_OUT "out.jpg"
#define IMG_OUT "./img/out.jpg"


#define LOG_ERROR               0       // errors
#define LOG_INFO                1       // information and notifications
#define LOG_DEBUG               2       // debug messages

// debug flag
int g_debug = LOG_INFO;
//static const char* socket_path = "/tmp/mysocket";

void log_msg( int t_log_level, const char *t_form, ... ) {
    const char *out_fmt[] = {
            "ERR: (%d-%s) %s\n",
            "INF: %s\n",
            "DEB: %s\n" };

    if ( t_log_level && t_log_level > g_debug ) { return; }

    char l_buf[ 1024 ];
    va_list l_arg;
    va_start( l_arg, t_form );
    vsprintf( l_buf, t_form, l_arg );
    va_end( l_arg );

    switch ( t_log_level ) {
        case LOG_INFO:
        case LOG_DEBUG:
            fprintf( stdout, out_fmt[ t_log_level ], l_buf );
            break;

        case LOG_ERROR:
            fprintf( stderr, out_fmt[ t_log_level ], errno, strerror( errno ), l_buf );
            break;
    }
}

typedef struct {
    int sd;
    SSL_CTX* ctx;
} thread_args;

/* Data structure describing a polling request.  */
//struct pollfd_ssl
struct pollfd_ssl
{
    int fd;			/* File descriptor to poll.  */
    short int events;		/* Types of events poller cares about.  */
    short int revents;		/* Types of events that actually occurred.  */
    // TODO ssl
};

//sem_t mySemaphore;
const char *fifoPath = "/tmp/myfifo";

void handle_client(int sd, SSL_CTX* ctx);

void* handle_client_thread(void* arguments);

void help() {
    printf("Usage: serv [t]\n");
    printf("  t - use threads instead fork for parallelism\n");
}


//long int findSize(char file_name[]) {
long findSize(char * file_name) {
    // opening the file in read mode
    FILE* fp = fopen(file_name, "r");
    // checking if the file exist or not
    if (fp == NULL) {
        printf("File Not Found!\n");
        return -1;
    }
    fseek(fp, 0L, SEEK_END);
    // calculating the size of the file
    long res = ftell(fp);
    // closing the file
    fclose(fp);
    return res;
}


int main(int argc, char *argv[]) {

//    exit(0);

    // Create and initialize a named semaphore
    sem_t *mySemaphore;
    const char *semName = "/mysemaphore";
    mySemaphore = sem_open(semName, O_CREAT, 0644, 1);
    if (mySemaphore == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    sem_close(mySemaphore);
    sem_unlink(semName);
    unlink(fifoPath);


    int use_poll = 1;

    // First lets setup listening for incoming connections
    int listen_sd;
    int client_fd;
    struct sockaddr_in socketAddrClient;
    socklen_t client_len;
    struct sockaddr_in sa_serv;
    const SSL_METHOD *meth;
    SSL_CTX* ctx;

    // init SSL
    SSL_load_error_strings();
    SSLeay_add_ssl_algorithms();
    meth = SSLv23_server_method();
    ctx = SSL_CTX_new (meth);
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        exit(2);
    }

    if (SSL_CTX_use_certificate_file(ctx, CERTF, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(3);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, KEYF, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(4);
    }

    if (!SSL_CTX_check_private_key(ctx)) {
        fprintf(stderr,"Private key does not match the certificate public key\n");
        exit(5);
    }


    if (use_poll) {
        printf("Using poll for handling clients...\n");

        int l_sock_listen;
        in_addr l_addr_any = {INADDR_ANY};
        sockaddr_in l_srv_addr;
        struct sockaddr_in6 l_srv_addr6 = {};

        int socket_sock;
        sockaddr_un sun, remote;
        int s2 = 0;


        int listen_sock_fd = -1, client_sock_fd = -1;
        struct sockaddr_in6 server_addr, client_addr;
        socklen_t client_addr_len;
        char str_addr[INET6_ADDRSTRLEN];
        int ret, flag;
        int l_sock_client = -1;

        // Start listening
        listen_sd = socket(AF_INET, SOCK_STREAM, 0);
        CHK_ERR(listen_sd, "socket");

        memset(&sa_serv, '\0', sizeof(sa_serv));
        sa_serv.sin_family = AF_INET;
        sa_serv.sin_addr.s_addr = INADDR_ANY;
        sa_serv.sin_port = htons(1111);  /* Server Port number */

        // Enable the port number reusing
        int l_opt = 1;
        if (setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR,
                       &l_opt, sizeof(l_opt)) < 0) {
            log_msg(LOG_ERROR, "Unable to set socket option!");
        }

        // err will be used as return variable for most of the functions, even `read` that may return size of read data
        int err = bind(listen_sd, (struct sockaddr *) &sa_serv, sizeof(sa_serv));
        CHK_ERR(err, "bind");
        err = listen(listen_sd, 5);
        CHK_ERR(err, "listen");

        // go!
        while (1) {

            // list of fd sources
            pollfd l_read_poll[2];
//            pollfd_ssl l_read_poll[ 2 ];

            l_read_poll[0].fd = STDIN_FILENO;
            l_read_poll[0].events = POLLIN;
//            l_read_poll[ 1 ].fd = l_sock_listen;
            l_read_poll[1].fd = listen_sd;
            l_read_poll[1].events = POLLIN;
            // https://www.man7.org/linux/man-pages/man7/unix.7.html - todo ...
            while (1) { // wait for new client
                // select from fds

                int timeout = -1;
                int l_poll = poll(l_read_poll, 2, timeout);

                if (l_poll < 0) {
                    log_msg(LOG_ERROR, "Function poll failed!");
                    exit(1);
                }

                if (l_read_poll[0].revents & POLLIN) { // data on stdin
                    char buf[128];
                    int len = read(STDIN_FILENO, buf, sizeof(buf));
                    if (len < 0) {
                        log_msg(LOG_DEBUG, "Unable to read from stdin!");
                        exit(1);
                    }

                    log_msg(LOG_DEBUG, "Read %d bytes from stdin");
                }

                //  if ( l_read_poll[ 1 ].events) {}

                if (l_read_poll[1].revents & POLLIN) {
                    // new client?

                    client_len = sizeof(socketAddrClient);
                    client_fd = accept(listen_sd, (struct sockaddr *) &socketAddrClient, &client_len);
                    l_sock_client = client_fd;
                    CHK_ERR(client_fd, "accept");

                    uint l_lsa = sizeof(l_srv_addr);
                    // my IP
                    getsockname(l_sock_client, (sockaddr *) &l_srv_addr, &l_lsa);
                    log_msg(LOG_INFO, "My IP: '%s'  port: %d",
                            inet_ntoa(l_srv_addr.sin_addr), ntohs(l_srv_addr.sin_port));
                    // client IP
                    getpeername(l_sock_client, (sockaddr *) &l_srv_addr, &l_lsa);
                    log_msg(LOG_INFO, "Client IP: '%s'  port: %d",
                            inet_ntoa(l_srv_addr.sin_addr), ntohs(l_srv_addr.sin_port));
//                    handle_client(client_fd, ctx);

                    pid_t pid = fork();
                    if(pid == 0) {
                        log_msg(LOG_INFO, "Start handle - Child process");
                        // child
                        handle_client(client_fd, ctx);
//                        close(listen_sd);
//                        close(client_fd);
                        exit(0);
                    } else {
                        // parent
                        log_msg(LOG_INFO, "Start handle - Parent process");
//                        close(client_fd);
                    }

//                    break;
                }

            } // while wait for client

            // poll done
        }
    }

    close (listen_sd);
    SSL_CTX_free (ctx); // Release context
}

void* handle_client_thread(void* arguments) {
    printf("New thread handler stared...\n");
    thread_args* args = (thread_args*)arguments;
    handle_client(args->sd, args->ctx);
    close(args->sd);
    free(arguments); // Don't forget to free the allocated memory
    printf("New thread handler stared...\n");
    return NULL;
}


void handle_client(int sd, SSL_CTX* ctx) {
    printf("New fork handler stared...\n");
    SSL* ssl;
    ssl = SSL_new (ctx);                           CHK_NULL(ssl);
    SSL_set_fd (ssl, sd);
    int err = SSL_accept (ssl);                    CHK_SSL(err);

    printf ("SSL connection using %s\n", SSL_get_cipher (ssl));

    X509* client_cert = SSL_get_peer_certificate (ssl);
    if (client_cert != NULL) {
        char* str = X509_NAME_oneline (X509_get_subject_name (client_cert), 0, 0);
        CHK_NULL(str);
        printf ("\t subject: %s\n", str);
        OPENSSL_free (str);

        str = X509_NAME_oneline (X509_get_issuer_name (client_cert), 0, 0);
        CHK_NULL(str);
        printf ("\t issuer: %s\n", str);
        OPENSSL_free (str);

        X509_free (client_cert);
    } else {
        printf ("Client does not have certificate.\n");
    }

    int retry_time = -1;
    int dim_width = -1;
    int dim_height = -1;
    char cmd[4096];
    char buf[4096]; // may not be sufficient for long messages

//    int read_ok_dim = 0;
    int read_ok_size = 0;
    int sent_size = 0;
    int outFileSize = -1;
    int t_hour = -1;
    int t_min = -1;
    int readb = -1;
    readb = SSL_read (ssl, buf, sizeof(buf) - 1);    CHK_SSL(err);
    if(readb > 0) {
        buf[readb] = '\0';
        char dim_delim = buf[10];
        log_msg(LOG_INFO, "Got %d chars:'%s'", read, buf);
        if(readb > 9) {
            if(    buf[0] == 'T'
                && buf[1] == 'I'
                && buf[2] == 'M'
                && buf[3] == 'E'
                && buf[4] == ' '
                && buf[7] == ':' // TODO floating delim
            ) {
                // TODO time: H:MM or H:M ?
                buf[7] = '\0';
                t_hour = atoi(&buf[5]);

                buf[10] = '\0';
                t_min = atoi(&buf[8]);
                log_msg(LOG_INFO, "Parsed Time: %d:%d", t_hour, t_min);
            } else {
                // TODO ERRROR
            }
        }
        if(readb > 11) {
            if( dim_delim == ' ' // TODO floating space delim and rest of msg
               && buf[11] == 'S'
               && buf[12] == 'I'
               && buf[13] == 'Z'
               && buf[14] == 'E'
               && buf[15] == ' '
               && buf[17] == 'x' // TODO floating delim ...
               ) {
                // TODO dim WWWxHHH, WWxHHH or WWWxHH, etc...
                // DIM
                buf[17] = '\0';
                dim_width = atoi(&buf[16]);
                dim_height = atoi(&buf[18]);
                log_msg(LOG_INFO, "Parsed Size: %dx%d", dim_width, dim_height);
            }
        } else {
        }
    } else {
        // TODO err
    }

    // if time got, then generate image
    if(t_hour != -1 && t_min != -1) {
        log_msg(LOG_INFO, "Will generate image data..");

        // round minutes to closest 10
        int t_hour_min_round = t_min - (t_min % 10) ;
        int t_hour_lower = t_hour % 12;
        char img_hour[20];
        char img_min[20]; // minute23.png

        sprintf(img_hour, "hour%.2d%.2d.png",
                t_hour_lower, t_hour_min_round);
        sprintf(img_min, "minute%.2d.png", t_min);

//        log_msg(LOG_INFO, "Generated time: %s:%s", img_hour, img_min);
        log_msg(LOG_INFO,
                "Generated time file names: %s %s",
                img_hour, img_min);

        log_msg(LOG_INFO, "Will generate image..");
        sem_t *mySemaphore;

        // Create and initialize a named semaphore
        const char *semName = "/mysemaphore";
        mySemaphore = sem_open(semName, O_CREAT, 0644, 1);
        if (mySemaphore == SEM_FAILED) {
            perror("sem_open");
            exit(EXIT_FAILURE);
        }

        int MAX_ARGS = 10;

        if(dim_width > 0 && dim_height > 0) {
// "convert img/ring.png img/hour0940.png img/minute42.png
// -layers flatten -resize 400x300! -"
            sprintf(cmd,
//                        "convert "
                    "convert "
                    "%s/img/ring.png " //
                    "%s/img/%s " // hour
                    "%s/img/%s " // min
                    " -quiet -layers flatten -resize %dx%d! -", // "%s ",
                    HOME_DIR, HOME_DIR, img_hour, HOME_DIR, img_min,
                    dim_width, dim_height
                    // ,IMG_OUT
            );
        } else {
            MAX_ARGS = 8;
            //   "convert img/ring.png img/hour0940.png img/minute42.png
            //   -layers flatten -",
            sprintf(cmd,
//                        "convert "
                    "convert "
                    "%s/img/ring.png " //
                    "%s/img/%s " // hour
                    "%s/img/%s " // min
                    " -quiet -layers flatten -",
                    HOME_DIR, HOME_DIR, img_hour, HOME_DIR, img_min
                    // ,IMG_OUT
            );
        }
        log_msg(LOG_INFO, "Generating Image cmd: %s\n", cmd);


        log_msg(LOG_INFO, "Creating named pipe: %s", fifoPath);


        // Create a named pipe
        unlink(fifoPath);

        if (mkfifo(fifoPath, 0666) == -1) {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
//        log_msg(LOG_INFO, "Created named pipe: %s", fifoPath);

//        int fifoFd = open(fifoPath, O_RDWR);
//        int fifoFd = open(fifoPath,  O_RDWR | O_NONBLOCK | O_CREAT);

//        log_msg(LOG_INFO, "Forking...");

        char * cmdArgs[MAX_ARGS];
        int i = 0;
        // split to args
        char *token = strtok(cmd, " ");
        for (i = 0; i < MAX_ARGS && token != NULL; i++) {
            cmdArgs[i] = token;
            token = strtok(NULL, " ");
        }
        cmdArgs[i] = NULL;


        int fifoFd = open(fifoPath,  O_RDWR);

        if (fifoFd == -1) {
            log_msg(LOG_ERROR, "Opening pipe failed: %s", fifoPath);
            perror("open");
//                    exit(EXIT_FAILURE);
        }

        // Fork a new process
        pid_t pid = fork();

        if (pid == -1) {
            // Error in fork
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
//            log_msg(LOG_INFO,
//                    "Child process (before exec) PID: %d\n", getpid());

            sem_wait(mySemaphore);
//            log_msg(LOG_INFO, "Entered the critical section in child");

//            log_msg(LOG_INFO, "Generating Image cmd: %s\n", cmd);

          /*  int fifoFd = open(fifoPath,  O_WRONLY);
            if (fifoFd == -1) {
                log_msg(LOG_ERROR, "Opening pipe failed: %s", fifoPath);
                perror("open");
//                    exit(EXIT_FAILURE);
            }*/
                // redir stdout to the named pipe
             if (dup2(fifoFd, STDOUT_FILENO) == -1) {
//                if (dup2(STDOUT_FILENO, fifoFd) == -1) {
//                    log_msg(LOG_ERROR, "Dumping data to pipe failed: %s", fifoPath);
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }

//            log_msg(LOG_INFO, "Dumping data to pipe: %s - done", fifoPath);
//            log_msg(LOG_INFO, "Generating Image cmd: %s", cmd);
            int res = execvp("/usr/bin/convert", cmdArgs);
//            SSL_write(ssl, buffer, outFileSize);
            // Remove the named pipe
            write(STDOUT_FILENO, "\0\0\0\0",4 );
//            write(STDOUT_FILENO, "\0",1 );
//            write(fifoFd, "\0",1 );
//            write(fifoFd, "\0",1 );\
//            write(fifoFd, "\0",1 );

//            close(fifoFd);
//            close(STDOUT_FILENO);
//            unlink(fifoPath);

//            log_msg(LOG_INFO, "Leaving the critical "
//                              "section in child 1, exec res: %d", res);

//                unlink(fifoPath);
//            close(fifoFd);

            sem_post(mySemaphore);

            // execl only returns if there is an error
//            perror("execl");
//            exit(EXIT_FAILURE);
        } else {
            // Parent process
            log_msg(LOG_INFO, "Parent process, child PID: %d\n", pid);

        /*    // Wait for the child to complete
            if (waitpid(pid, NULL, 0) == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }*/
            log_msg(LOG_INFO, "Parent waited for child PID: %d\n", pid);

//            char buffer[1024];
//            char buffer[4096];
//            char buffer[4096];
            char buffer[1024];
            int bytesRead;

            log_msg(LOG_INFO, "Read fifo");
//            fifoFd = open(fifoPath,  O_RDONLY | O_CREAT);

            if (fifoFd == -1) {
                log_msg(LOG_ERROR, "Opening pipe failed: %s", fifoPath);
                perror("open");
//                    exit(EXIT_FAILURE);
            }
            log_msg(LOG_INFO, "Read fifo 2 %d", fifoFd);
            // read fifo
            int bytesTotal = 0;
            while ((bytesRead = read(fifoFd, buffer, sizeof(buffer) - 1)) > 0) {
//                if(buffer[bytesRead-1] == '\0') {
//                    break;
//                }
                if ( bytesRead <= 0 || (bytesRead == 1 && buffer[0] == '\0' )) {
                    SSL_write(ssl, "\0", 1);
                    log_msg(LOG_INFO, "Rec2send: done");
                    break;
                }
                buffer[bytesRead] = '\0'; // Null-terminate the string
                log_msg(LOG_INFO, "Rec2send: %d", bytesRead);
                bytesTotal += bytesRead;
                SSL_write(ssl, buffer, bytesRead);
            }
            log_msg(LOG_INFO, "receiving done - total %d: ", bytesTotal);

            if (bytesRead == -1) {
                perror("fifo_read");
                close(fifoFd);
//                exit(EXIT_FAILURE);
            }
//            close(fifoFd);

            log_msg(LOG_INFO, "receiving done");

            // Close the FIFO
        }

        waitpid(pid, NULL, 0);
        close(fifoFd);
        unlink(fifoPath);
//        SSL_write(ssl, "\0", 1);
//        close(fifoFd);
        sem_close(mySemaphore);
        sem_destroy(mySemaphore);
        sem_unlink(semName);

    }
    close(sd);
    SSL_free(ssl);
}


