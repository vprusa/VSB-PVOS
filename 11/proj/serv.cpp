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

void handle_client(int sd, SSL_CTX* ctx); // Function to handle client communication

int main () {
    // First lets setup listening for incoming connections
    int listen_sd;
    int client_fd;
    struct sockaddr_in socketAddrClient;
    socklen_t client_len;

    int sd;
    struct sockaddr_in sa_serv;
    const SSL_METHOD *meth;
    SSL_CTX* ctx;

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

    listen_sd = socket (AF_INET, SOCK_STREAM, 0);   CHK_ERR(listen_sd, "socket");

    int use_thread = 0;
    int use_fork = 0;
    if(use_fork) {
        int fid = fork();
        if (fid == 0) { // Child process
            close(listen_sd); // Close listening socket in child
            handle_client(sd, ctx); // Process the client's requests
            exit(0);
        } else if (fid > 0) { // Parent process
            close(sd); // Close connected socket in parent
        } else {
            perror("Fork failed");
            exit(1);
        }
    }

    memset (&sa_serv, '\0', sizeof(sa_serv));
    sa_serv.sin_family      = AF_INET;
    sa_serv.sin_addr.s_addr = INADDR_ANY;
    sa_serv.sin_port        = htons (1111);          /* Server Port number */

    int err = bind(listen_sd, (struct sockaddr*) &sa_serv, sizeof (sa_serv)); CHK_ERR(err, "bind");
    err = listen (listen_sd, 5);                    CHK_ERR(err, "listen");

    /*
    while (1) {
        struct sockaddr_in sa_cli;
        client_len = sizeof(sa_cli);
        sd = accept (listen_sd, (struct sockaddr*) &sa_cli, &client_len);
        CHK_ERR(sd, "accept");
        printf ("Connection from %x, port %x\n", sa_cli.sin_addr.s_addr, sa_cli.sin_port);

        int pid = fork();
        if (pid == 0) { // Child process
            close(listen_sd); // Close listening socket in child
            handle_client(sd, ctx); // Process the client's requests
            exit(0);
        } else if (pid > 0) { // Parent process
            close(sd); // Close connected socket in parent
        } else {
            perror("Fork failed");
            exit(1);
        }
    }
    */

    while (1) {
        client_len = sizeof(socketAddrClient);
        client_fd = accept(listen_sd, (struct sockaddr*)&socketAddrClient, &client_len);
        CHK_ERR(client_fd, "accept");

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
    }

    close (listen_sd);
    SSL_CTX_free (ctx); // Release context
}

void handle_client(int sd, SSL_CTX* ctx) {
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

    char buf[4096];
    int read = SSL_read (ssl, buf, sizeof(buf) - 1);    CHK_SSL(err);
    buf[read] = '\0';
    printf ("Got %d chars:'%s'\n", read, buf);
    // Prepare a message with the size of the received message
    char message[1024];
//    sprintf(message, "Received %d bytes: %s", read, buf);
    sprintf(message, "%d[b]", read);

    /*
     * Pro kontrolu by bylo vhodné, aby server klientovi zpět potvrdil, kolik bajtů přijat.
     */
    err = SSL_write (ssl, message, strlen(message));  CHK_SSL(err);

    close(sd);
    SSL_free(ssl);
}


