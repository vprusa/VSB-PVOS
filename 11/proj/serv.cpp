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

#define HOME "./"
#define CERTF  HOME "my.crt"
#define KEYF  HOME  "my.key"

#define CHK_NULL(x) if ((x)==NULL) exit (1)
#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(1); }
#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }

typedef struct {
    int sd;
    SSL_CTX* ctx;
} thread_args;

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
    if (argc < 2) {
        help();
        return 2;
    } else {
        switch (argv[1][0]) {
            case 't':
                use_thread = 1;
                use_fork = 0;
                printf("Using threads instead forks\n");
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

    // Start listening
    listen_sd = socket (AF_INET, SOCK_STREAM, 0);   CHK_ERR(listen_sd, "socket");

    memset (&sa_serv, '\0', sizeof(sa_serv));
    sa_serv.sin_family      = AF_INET;
    sa_serv.sin_addr.s_addr = INADDR_ANY;
    sa_serv.sin_port        = htons (1111);          /* Server Port number */

    // err will be used as return variable for most of the functions, even `read` that may return size of read data
    int err = bind(listen_sd, (struct sockaddr*) &sa_serv, sizeof (sa_serv)); CHK_ERR(err, "bind");
    err = listen (listen_sd, 5);                    CHK_ERR(err, "listen");

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


