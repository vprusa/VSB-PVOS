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


#define HOME "./"
#define CERTF  HOME "my.crt"
#define KEYF  HOME  "my.key"

#define IMG_OUT "./img/out.jpg"

#define CHK_NULL(x) if ((x)==NULL) exit (1)
#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(1); }
#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }

// TODO
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
                    handle_client(client_fd, ctx);

/*
                    pid_t pid = fork();
                    if(pid == 0) {
                        // child
                        close(listen_sd);
                        close(client_fd);
                        exit(0);
                    } else {
                        // parent
                        close(client_fd);
                    }
*/

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
    char cmd[2048];
    char buf[4096]; // may not be sufficient for long messages

//    int read_ok_dim = 0;
    int read_ok_size = 0;
    int sent_size = 0;
    int outFileSize = -1;

    pollfd l_read_poll[1];
    l_read_poll[0].fd = sd;
    l_read_poll[0].events = POLLIN;

    while(1) {
        int read = -1;
        int l_poll = poll(l_read_poll, 1, 5000);
//        if (l_read_poll[1].revents & POLLIN) {
        if (l_read_poll[0].revents & POLLIN) {
            read = SSL_read (ssl, buf, sizeof(buf) - 1);    CHK_SSL(err);
            if(read > 0) {
                buf[read] = '\0';
            } else {
                SSL_write(ssl, "E:0", 3);
            }
        } else {
            SSL_write(ssl, "E:1", 3);
        }
//        int read = SSL_read (ssl, buf, sizeof(buf) - 1);    CHK_SSL(err);
//        buf[read] = '\0';
        printf ("Got %d chars:'%s'\n", read, buf);
        // contains img info?
        if(read > 1) {
            if(buf[0] == 'O' && buf[1] == 'K') {
                log_msg(LOG_INFO, "Received OK...\n");
//                read_ok_dim = 1;
                read_ok_size = 1;
                buf[0] = '\0';
            } else if(buf[0] == 'R' && buf[1] == ':') {
                char * buf_i = buf + 2;
                int buf_idx = 1;
                while(buf[buf_idx] != 'W') {
                    buf_idx++;
                }
                buf_i[buf_idx-1] = '\0';
                retry_time = atoi(buf_i);
                log_msg(LOG_INFO,"Retry: %d", retry_time);

                char * buf_i_w = buf + buf_idx + 2;
                buf_idx = 0;
                while(buf_i_w[buf_idx] != 'H') {
                    buf_idx++;
                }
                buf_i_w[buf_idx] = '\0';
                dim_width = atoi(buf_i_w);
                log_msg(LOG_INFO,"Width: %d", dim_width);

                char * buf_i_h = buf_i_w + buf_idx + 2;
                buf_idx = 0;
                while(buf_i_h[buf_idx] != '\0') {
                    buf_idx++;
                }
    //            buf_i_h[buf_idx] = '\0';
                dim_height = atoi(buf_i_h);
                log_msg(LOG_INFO,"Height: %d", dim_height);
                buf[0] = '\0';

//                continue;
            }

        int t_sec = -1;
        int t_min = -1;
        int t_hours = -1;

        if(retry_time != -1
            && dim_width != -1
            && dim_height != -1
            && read_ok_size == 0
        ) {
            // get curren time
            struct tm* ptr;
            time_t t;
            t = time(NULL);
            ptr = localtime(&t);
            log_msg(LOG_INFO, "Time: %s", asctime(ptr));
//            log_msg(LOG_INFO, "Time: %s", asctime(ptr));
            t_sec = ptr->tm_sec;
            t_min = ptr->tm_min;
            t_hours = ptr->tm_hour;

            int t_h1 = t_hours / 10;
            int t_h2 = t_hours % 10;
            int t_m1 = t_min / 10;
            int t_m2 = t_min % 10;
            int t_s1 = t_sec / 10;
            int t_s2 = t_sec % 10;

            char * imgData[10] = {IMG_0,
                                IMG_1,
                                IMG_2,
                                IMG_3,
                                IMG_4,
                                IMG_5,
                                IMG_6,
                                IMG_7,
                                IMG_8,
                                IMG_9};

            char * ts_sep = IMG_PUNC;
            char * ts_h1 = imgData[t_h1];
            char * ts_h2 = imgData[t_h2];
            char * ts_m1 = imgData[t_m1];
            char * ts_m2 = imgData[t_m2];
            char * ts_s1 = imgData[t_s1];
            char * ts_s2 = imgData[t_s2];

//            char time_string_msg[32];
//            sprintf(time_string_msg, "%d%d:%d%d:%d%d",
//                     t_h1, t_h2, t_m1, t_m2, t_s1, t_s2);
//            SSL_write(ssl, time_string_msg, strlen(time_string_msg));

            // select images
            // generate whole image
            log_msg(LOG_INFO, "Generating Image %s ...\n", IMG_OUT);


    //    const char * p_name = "/usr/bin/convert";
    //    const char * p_args = "./img/jedna.png ./img/jedna.png ./img/jedna.png -background grey -alpha remove +append -resize 500x70! out.jpg";
    //    char const * p_args = {}; //"./img/jedna.png ./img/jedna.png ./img/jedna.png -background grey -alpha remove +append -resize 500x70! out.jpg";
    //    char* p_args[] = {"ls", "-l", NULL};
    /*

            const char * p_name = "/usr/bin/convert";
            char * dims_str = "500x70!";
            char* p_args[] = {"./img/jedna.png","./img/jedna.png","./img/jedna.png","-background","grey","-alpha","remove","+append","-resize",
    //                          dims_str, // TODO replace dimension
                              "500x70!", // TODO replace dimension
    //                          IMG_OUT,
                              "./img/out.jpg",
                              NULL};
    //                        __null};
            // convert jedna.png jedna.png jedna.png -background grey -alpha remove +append -resize 500x70! out.jpg
    //        execv(p_name, p_args);
    //        execv(p_name, p_args); // TODO crashing ... :/
            execv(p_name, p_args); // TODO crashing ... :/
    */

            /*
            const char * p_name = "echo";
    //        char * dims_str = "500x70!";
            char* p_args[] = {IMG_OUT,
                              NULL};
            execv(p_name, p_args); // TODO crashing ... :/
            */
            // send size of image to client
    //        sprintf(cmd,
    //                "convert "
            sprintf(cmd,
    //                "convert "
                    "%s " // h1
                    "%s " // h2
                    "%s " // :
                    "%s " // m1
                    "%s " // m2
                    "%s " // :
                    "%s " // s1
                    "%s " // s2
                    "-background grey -alpha remove +append -resize %dx%d! %s",
                    ts_h1, ts_h2, ts_sep, ts_m1, ts_m2, ts_sep, ts_s1, ts_s2,
                    dim_width, dim_height,
                    IMG_OUT);
            log_msg(LOG_INFO, "Generating Image cmd: %s\n", cmd);

    //        int status = system(cmd);
    //        int status = system(cmd);
            sem_t *mySemaphore;

            // Create and initialize a named semaphore
            const char *semName = "/mysemaphore";
            mySemaphore = sem_open(semName, O_CREAT, 0644, 1);
            if (mySemaphore == SEM_FAILED) {
                perror("sem_open");
                exit(EXIT_FAILURE);
            }

            // Fork a new process
            pid_t pid = fork();

            if (pid == -1) {
                // Error in fork
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                // Child process
                log_msg(LOG_INFO, "Child process (before exec) PID: %d\n", getpid());

                // Replace the child process with a new program
    //            execl("/bin/ls", "ls", NULL);
                sem_wait(mySemaphore);
                printf("Entered the critical section in child 1\n");
//                sleep(1);  // Simulate some critical work
                execl("convert ", cmd, NULL);

                printf("Leaving the critical section in child 1\n");
                sem_post(mySemaphore);
                exit(EXIT_SUCCESS);

                // execl only returns if there is an error
                perror("execl");
                exit(EXIT_FAILURE);
            } else {
                // Parent process
                log_msg(LOG_INFO, "Parent process, child PID: %d\n", pid);

                // Wait for the child to complete
                if (waitpid(pid, NULL, 0) == -1) {
                    perror("waitpid");
                    exit(EXIT_FAILURE);
                }
            }

//            sem_destroy(mySemaphore);
            waitpid(pid, NULL, 0);
            sem_close(mySemaphore);
            sem_unlink(semName);

//            waitpid(pid1, NULL, 0);
//            waitpid(pid2, NULL, 0);

            log_msg(LOG_INFO, "Generating Image %s done\n", IMG_OUT);

            outFileSize = findSize(IMG_OUT);
            log_msg(LOG_INFO, "Image %s with size: %d\n", IMG_OUT, outFileSize);

            // send size of image to client
            char size_string_msg[32];
            sprintf(size_string_msg, "S:%d",
                    outFileSize);
            SSL_write(ssl, size_string_msg, sizeof(size_string_msg));
            log_msg(LOG_INFO, "Image size, sent: %d\n", IMG_OUT, outFileSize);
            sent_size = 1;
            continue;

        } else {
            // TODO print problem
        }
            if(
//                    read_ok_dim == 1 &&
                    read_ok_size == 1 &&
                    outFileSize > 0) {
                log_msg(LOG_INFO, "Will write image to client...\n");

                char * buffer = 0;
                FILE * f = fopen (IMG_OUT, "rb");

                if (f) {
                    buffer = (char*) malloc(outFileSize);
                    if (buffer) {
                        fread (buffer, 1, outFileSize, f);
                    }
                    fclose(f);
                }

                // send image to client
                log_msg(LOG_INFO, "Writing image to client...\n");
                SSL_write(ssl, buffer, outFileSize);
                log_msg(LOG_INFO, "Image sent\n");
                read_ok_size = 0;
                sent_size = 0;
                // wait for response
                // wait <retry_time> seconds to send data again
            }
        }

    }

    close(sd);
    SSL_free(ssl);
}


