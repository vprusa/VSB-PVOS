/* serv.cpp  -  Minimal ssleay server for Unix
   30.9.1996, Sampo Kellomaki <sampo@iki.fi>

   Modified to accept multiple clients using fork()
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h> /* For signal() */

#include <openssl/rsa.h>       /* SSLeay stuff */
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>

#define HOME "./"
#define CERTF  HOME "my.crt"
#define KEYF  HOME  "my.key"

#define CHK_NULL(x) if ((x)==NULL) exit (1)
#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(1); }
#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }


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

void handle_client(int sd, SSL_CTX* ctx);
void* handle_client_thread(void* arguments);

void help();
void help() {
    printf("Usage: serv [t]\n");
    printf("  t - use threads instead fork for parallelism\n");
}

int main(int argc, char *argv[]) {

    int use_thread = 0;
    int use_fork = 1;
    int use_poll = 0;
    if (argc < 2) {
        help();
        return 2;
    } else {
        switch (argv[1][0]) {
            case 't':
                use_thread = 1;
                use_fork = 0;
                use_poll = 0;
                printf("Using threads instead forks or poll\n");
                break;
            case 'p':
                use_thread = 0;
                use_fork = 0;
                use_poll = 1;
                printf("Using poll instead forks or threads\n");
                break;
            default:
                help();
        }
    }

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

        memset (&sa_serv, '\0', sizeof(sa_serv));
        sa_serv.sin_family      = AF_INET;
        sa_serv.sin_addr.s_addr = INADDR_ANY;
        sa_serv.sin_port        = htons (1111);  /* Server Port number */

        // Enable the port number reusing
        int l_opt = 1;
        if ( setsockopt( listen_sd, SOL_SOCKET, SO_REUSEADDR,
                         &l_opt, sizeof( l_opt ) ) < 0 ) {
            log_msg( LOG_ERROR, "Unable to set socket option!" );
        }

        // err will be used as return variable for most of the functions, even `read` that may return size of read data
        int err = bind(listen_sd, (struct sockaddr*) &sa_serv, sizeof (sa_serv));
        CHK_ERR(err, "bind");
        err = listen (listen_sd, 5);
        CHK_ERR(err, "listen");

        // go!
        while ( 1 ) {

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
                    /*
                    sockaddr_in l_rsa;
                    int l_rsa_size = sizeof(l_rsa);
                    // new connection
                    l_sock_client = accept(l_sock_listen, (sockaddr *) &l_rsa, (socklen_t *) &l_rsa_size);
                    if (l_sock_client == -1) {
                        log_msg(LOG_ERROR, "Unable to accept new client.");
//                        close( l_sock_listen );
//                        exit( 1 );
                    }
                    */

                    client_len = sizeof(socketAddrClient);
                    client_fd = accept(listen_sd, (struct sockaddr*)&socketAddrClient, &client_len);
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

                    break;
                }

            } // while wait for client

            // poll done
        }
    } else {
        // Start listening
        listen_sd = socket (AF_INET, SOCK_STREAM, 0);
        CHK_ERR(listen_sd, "socket");

        memset (&sa_serv, '\0', sizeof(sa_serv));
        sa_serv.sin_family      = AF_INET;
        sa_serv.sin_addr.s_addr = INADDR_ANY;
        sa_serv.sin_port        = htons (1111);  /* Server Port number */

        // err will be used as return variable for most of the functions, even `read` that may return size of read data
        int err = bind(listen_sd, (struct sockaddr*) &sa_serv, sizeof (sa_serv));
        CHK_ERR(err, "bind");
        err = listen (listen_sd, 5);
        CHK_ERR(err, "listen");

        while (1) {
            // wait for client to connect
            client_len = sizeof(socketAddrClient);
            client_fd = accept(listen_sd, (struct sockaddr*)&socketAddrClient, &client_len);
            CHK_ERR(client_fd, "accept");

            // start parallel...
            if(use_fork) {
                printf("Starting new fork...\n");
                // ... fork
                int pid = fork();
                if (pid == 0) { // Child process
                    close(listen_sd);
                    handle_client(client_fd, ctx);
                    exit(0);
                } else if (pid > 0) { // Parent process
                    close(client_fd);
                } else {
                    perror("Fork failed");
                    exit(1);
                }
            } else if (use_thread) {
                // ... thread
                printf("Starting new thread...\n");
                pthread_t tid;
                thread_args* args = (thread_args*)malloc(sizeof(thread_args));
                if (args == NULL) {
                    perror("Failed to allocate memory for thread arguments");
                    exit(1);
                }
                args->sd = client_fd;
                args->ctx = ctx;

                int ret = pthread_create(&tid, NULL, handle_client_thread, (void*)args);
                if (ret != 0) {
                    perror("pthread_create failed");
                    exit(1);
                }

                pthread_detach(tid);  // Detach the thread so it runs independently
            }
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

    char buf[4096]; // may not be sufficient for long messages
    int read = SSL_read (ssl, buf, sizeof(buf) - 1);    CHK_SSL(err);
    buf[read] = '\0';
    printf ("Got %d chars:'%s'\n", read, buf);
    // Prepare a message with the size of the received message
    char message[32];
    sprintf(message, "%d[b]", read);

    /*
     * Pro kontrolu by bylo vhodné, aby server klientovi zpět potvrdil, kolik bajtů přijat.
     */
    err = SSL_write (ssl, message, strlen(message));  CHK_SSL(err);

    close(sd);
    SSL_free(ssl);
}


