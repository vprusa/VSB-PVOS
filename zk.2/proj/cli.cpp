/* cli.cpp  -  Minimal ssleay client for Unix
   30.9.1996, Sampo Kellomaki <sampo@iki.fi> */

/* mangled to work with SSLeay-0.9.0b and OpenSSL 0.9.2b
   Simplified to be even more minimal
   12/98 - 4/99 Wade Scholine <wades@mail.cybg.com> */

#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <sys/stat.h>
#include <csignal>
#include <sys/poll.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHK_NULL(x) if ((x)==NULL) exit (1)
#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(1); }
#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }


#define CHK_NULL(x) if ((x)==NULL) exit (1)
#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(1); }
#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }


#define LOG_ERROR               0       // errors
#define LOG_INFO                1       // information and notifications
#define LOG_DEBUG               2       // debug messages


#define OUT_FILE "./img/recOut.jpg"
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
//    printf("Usage: <retry_time> <dimensions>\n");
//    printf("  retry_time - seconds\n");
//    printf("  dimensions - <width>x<height>\n");
}

int main(int argc, char *argv[]) {
//    exit(0);
    int dim_width = -1;
    int dim_height = -1;
    int res = 0;
//        help();
//        return 2;
    if (argc == 2) {
//    } else {
//        retry_time = atoi(argv[1]);
    /*    char * dimensions = argv[1];
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
        }*/
//        int width, height;
        // TODO parse dimensions 10x10 and 100x10, etc..
        sscanf(argv[1], "%3dx%3d", &dim_width, &dim_height);
//        if (strlen(argv[1]) != 7 || argv[1][3] != 'x'
//            ) {
//            sscanf(argv[1], "%3dx%3d", &dim_width, &dim_height);
//            fprintf(stderr, "Invalid format. Please use WWWxHHH format.\n");
//            return 1;
//        } else {
//            help();
//            exit(2);
//        }
    } else if (argc > 2) {
        help();
        exit(2);
    }

    if(dim_width > 999 || dim_height > 999) {
        log_msg(LOG_ERROR, "Dimensions are too big, max is 999x999");
//        return 2;
        exit(2);
    }

    log_msg(LOG_INFO, "Parsed args,\n"
                      "  dim_width: %d, dim_height: %d, res: %d",
            dim_width, dim_height, res);

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
  ctx = SSL_CTX_new (meth);
  CHK_NULL(ctx);
//  CHK_SSL(err);

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

    int imageSize = -1;
    int sent_dim = 0;
    int sent_ok = 0;
    int receivedSizeLastTime = 0;
    int request_new_time = 0;


    pollfd l_read_poll[1];
    l_read_poll[0].fd = sd;
    l_read_poll[0].events = POLLIN;

    char time_and_dim_message[32];
    pid_t pid = -1;
    int OK_size = 10;
    char okBuff[OK_size];


    struct tm* ptr;
    time_t t;
    t = time(NULL);
    ptr = localtime(&t);
    log_msg(LOG_INFO, "Time: %s", asctime(ptr));

    int t_hour = ptr->tm_hour;
    int t_min = ptr->tm_min;

    while(1) {
//        if (sent_dim == 0) {
        if( dim_width > 0 && dim_height > 0) {
            sprintf(time_and_dim_message,
//                    "TIME %d:%d SIZE %dx%d",
                    "TIME %d:0%d SIZE yyyx%d",
                t_hour, t_min, dim_width, dim_height);
        } else {
            sprintf(time_and_dim_message,
                    "TIME %d:%d",
                    t_hour, t_min);
        }
        // zk, send data about retry and image to server
        err = SSL_write(ssl, time_and_dim_message,
                         strlen(time_and_dim_message));  CHK_SSL(err);
        printf("Wrote %d chars:'%s'\n", strlen(time_and_dim_message),
               time_and_dim_message);
        sent_dim = 1;
//        }
        int imageSize = 4096;
        char * imgBuf = (char*) malloc(imageSize);
        log_msg(LOG_INFO, "Start download image...");
        FILE * f = fopen (OUT_FILE, "wb");
        while(1) {
            err = SSL_read(ssl, imgBuf, sizeof(imgBuf) - 1);
//            log_msg(LOG_INFO, "Read %d bytes", err);
            imgBuf[err] = '\0';
            if(err > 0) {
                fwrite(imgBuf, 1, err, f);
            } else {break;}
            if(imgBuf[0] == '\0'
               && imgBuf[1] == '\0'
                  && imgBuf[2] == '\0'
                     && imgBuf[3] == '\0'
            ) {
                break;
            }

        }
        fclose (f);
        log_msg(LOG_INFO, "File closed");

        break;
        int read = -1;
        int l_poll = poll(l_read_poll, 1, 5000);
        if (l_read_poll[0].revents & POLLIN) {
            read = SSL_read(ssl, buf, sizeof(buf) - 1);
            CHK_SSL(err);
            if(read > 0) {
                buf[read] = '\0';
            }
        }
        if (err < 1) {
            continue;
        }
        if (read > 1) {
            if(buf[0] == 'E' && buf[1] == ':') {
                sent_ok = 0;
                sent_dim = 0;
                printf("Got %d chars:'%s'\n", err, buf);
                continue;
            }
        }

        buf[err] = '\0';
        printf("Got %d chars:'%s'\n", err, buf);

        receivedSizeLastTime = 0;
        if(buf[0] == 'S' && buf[1] == ':') {
            char *buf_i = buf + 2;
            imageSize = atoi(buf_i);
            printf("New image size will be: %d\n", imageSize);
            SSL_write(ssl, "OK", 2);
            sent_ok = 1;

//        }
//        if(sent_ok == 1) {
            log_msg(LOG_INFO, "Start download image...");
            char * imgBuf = (char*) malloc(imageSize);

            err = SSL_read(ssl, imgBuf, sizeof(buf) - 1);
            if(err > 1 && imgBuf[0] == 'E' && imgBuf[1] == ':' ) {
                sent_ok = 0;
                sent_dim = 0;
                continue;
            }
            FILE * f = fopen (OUT_FILE, "wb");

            if (f) {
                fwrite(imgBuf, 1, imageSize, f);
                fclose (f);
            }

            receivedSizeLastTime = 0;
            log_msg(LOG_INFO, "Download image done");
            log_msg(LOG_INFO, "Killing child if necessary, pid: $d...", pid);
            if(pid >= 0) {
                kill(pid, SIGKILL);
//                kill(pid, SIGTERM);
            }

            pid = fork();
            if(pid == 0) {
                // child
                log_msg(LOG_INFO, "Opening file under child...");
//                int status = system("display ./img/recOut.jpg");

//                execl("/usr/bin/display", {"./img/recOut.jpg"}, NULL);
//                execl("/usr/bin/display", {"/home/vprusa/workspace/school/VSB/1ZS/PVOS/ukoly/zk/proj/img/recOut.jpg"}, NULL);
                char* args[] = {"display", "/home/vprusa/workspace/school/VSB/1ZS/PVOS/ukoly/zk/proj/img/recOut.jpg", NULL};
                execv("/usr/bin/display", args);

                exit(0);
            } else {
                log_msg(LOG_INFO, "Parent fork child pid: %d...", pid);
                // parent
            }

            log_msg(LOG_INFO, "opening file %s done, status: %d\n", OUT_FILE);
            request_new_time = 1;

            log_msg(LOG_INFO, "Sleeping seconds ...");


        } else {
            receivedSizeLastTime = 0;
        }
//            SSL_shutdown(ssl);
        int sleep_time = 2;
        sleep(sleep_time);
        sent_dim = 0;
        log_msg(LOG_INFO, "woke up, requests time again ...", sleep_time);

    }
    SSL_shutdown(ssl);  /* send SSL/TLS close_notify */
  close (sd);
  SSL_free (ssl);
  SSL_CTX_free (ctx);
}
/* EOF - cli.cpp */
