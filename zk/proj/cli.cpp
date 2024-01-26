/* cli.cpp  -  Minimal ssleay client for Unix
   30.9.1996, Sampo Kellomaki <sampo@iki.fi> */

/* mangled to work with SSLeay-0.9.0b and OpenSSL 0.9.2b
   Simplified to be even more minimal
   12/98 - 4/99 Wade Scholine <wades@mail.cybg.com> */

#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


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



#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


#define CHK_NULL(x) if ((x)==NULL) exit (1)
#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(1); }
#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }


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

void help() {
    printf("Usage: <retry_time> <dimensions>\n");
    printf("  retry_time - seconds\n");
    printf("  dimensions - <width>x<height>\n");
}

int main(int argc, char *argv[]) {

    int use_thread = 0;
    int use_fork = 1;
    int retry_time = 1;
    int dim_width = 0;
    int dim_height = 0;

    if (argc != 3) {
        help();
        return 2;
    } else {
        retry_time = atoi(argv[1]);
        char * dimensions = argv[2];
        int dim_str_len = strlen(dimensions);
        for(int i = 0 ; i < dim_str_len; i++) {
            if(dimensions[i] == 'x') {
                dimensions[i] = '\0';
                dim_width = atoi(dimensions);
                dimensions[i] = 'x';
                dimensions = dimensions + i + 1;
                dim_height = atoi(dimensions);
                break;
            }
        }
//        dim_width = atoi(argv[1]);
//        dim_height = atoi(argv[1]);
    }


    log_msg(LOG_INFO, "Parsed args, \nretry_time: %d, "
                      "\ndim_width: %d, \ndim_height: %d",
            retry_time, dim_width, dim_height);

//    exit(0);

  int err;
  int sd;
  struct sockaddr_in sa;
  SSL_CTX* ctx;
  SSL*     ssl;
  X509*    server_cert;
  char*    str;
  char     buf [4096];
  const SSL_METHOD *meth;

  SSLeay_add_ssl_algorithms();
  meth = SSLv23_client_method();
  SSL_load_error_strings();
  ctx = SSL_CTX_new (meth);                        CHK_NULL(ctx);

  CHK_SSL(err);

  /* ----------------------------------------------- */
  /* Create a socket and connect to server using normal socket calls. */

  sd = socket (AF_INET, SOCK_STREAM, 0);       CHK_ERR(sd, "socket");

  memset (&sa, '\0', sizeof(sa));
  sa.sin_family      = AF_INET;
  sa.sin_addr.s_addr = inet_addr ("127.0.0.1");   /* Server IP */
  sa.sin_port        = htons     (1111);          /* Server Port number */

  err = connect(sd, (struct sockaddr*) &sa,
		sizeof(sa));                   CHK_ERR(err, "connect");

  /* ----------------------------------------------- */
  /* Now we have TCP conncetion. Start SSL negotiation. */

  ssl = SSL_new (ctx);                         CHK_NULL(ssl);
  SSL_set_fd (ssl, sd);
  err = SSL_connect (ssl);                     CHK_SSL(err);

  /* Following two steps are optional and not required for
     data exchange to be successful. */

  /* Get the cipher - opt */

  printf ("SSL connection using %s\n", SSL_get_cipher (ssl));

  /* Get server's certificate (note: beware of dynamic allocation) - opt */

  server_cert = SSL_get_peer_certificate (ssl);       CHK_NULL(server_cert);
  printf ("Server certificate:\n");

  str = X509_NAME_oneline (X509_get_subject_name (server_cert),0,0);
  CHK_NULL(str);
  printf ("\t subject: %s\n", str);
  OPENSSL_free (str);

  str = X509_NAME_oneline (X509_get_issuer_name  (server_cert),0,0);
  CHK_NULL(str);
  printf ("\t issuer: %s\n", str);
  OPENSSL_free (str);

  /* We could do all sorts of certificate verification stuff here before
     deallocating the certificate. */

  X509_free (server_cert);

  /* --------------------------------------------------- */
  /* DATA EXCHANGE - Send a message and receive a reply. */

  // Prepare a message with the size of the received message

//    const char* message = "Hello World!";
//  err = SSL_write (ssl, message, strlen(message));  CHK_SSL(err);
//    printf("Wrote %d chars:'%s'\n", strlen(message), message);

//    const char* message = "Hello World!";
//    const char* message = "retry: ";
    char retry_and_dim_message[32];
    sprintf(retry_and_dim_message, "R:%dW:%dH:%d", retry_time, dim_width, dim_height);

//    char retry_time_str[10];
    // zk, send data about retry and image to server
    err = SSL_write (ssl, retry_and_dim_message, strlen(retry_and_dim_message));  CHK_SSL(err);
    printf("Wrote %d chars:'%s'\n", strlen(retry_and_dim_message), retry_and_dim_message);
//    err = SSL_write (ssl, retry_message, strlen(message));  CHK_SSL(err);
//    printf("Wrote %d chars:'%s'\n", strlen(message), message);

    // zk, send data about retry and image to server
//    err = SSL_write (ssl, message, strlen(message));  CHK_SSL(err);
//    printf("Wrote %d chars:'%s'\n", strlen(message), message);

    int imageSize = -1;
    int receivedSizeLastTime = 0;
    while(1) {

        if(receivedSizeLastTime != 0
            && imageSize != -1
        ) {
            log_msg(LOG_INFO, "Start download image...");
            // file ready to receive
            char * imgBuf = (char*) malloc(imageSize);

            err = SSL_read(ssl, imgBuf, sizeof(buf) - 1);

            receivedSizeLastTime = 0;
            log_msg(LOG_INFO, "Download image done");
            continue;
        }
        err = SSL_read(ssl, buf, sizeof(buf) - 1);
//        CHK_SSL(err);
        if(err < 1){
            continue;
        }

        buf[err] = '\0';
        printf("Got %d chars:'%s'\n", err, buf);

        receivedSizeLastTime = 0;
        if(buf[0] == 'S' && buf[1] == ':') {
            char * buf_i = buf + 2;
            imageSize = atoi(buf_i);
            printf("New image size will be: %d\n", imageSize);
            receivedSizeLastTime = 1;
        } else {
            receivedSizeLastTime = 0;
        }
        /* Clean up. */
    }
    SSL_shutdown(ssl);  /* send SSL/TLS close_notify */
  close (sd);
  SSL_free (ssl);
  SSL_CTX_free (ctx);
}
/* EOF - cli.cpp */
