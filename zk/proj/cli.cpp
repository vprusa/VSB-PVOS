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
    printf("Usage: <retry_time> <dimensions>\n");
    printf("  retry_time - seconds\n");
    printf("  dimensions - <width>x<height>\n");
}

int main(int argc, char *argv[]) {
//    exit(0);

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
    }

    log_msg(LOG_INFO, "Parsed args, \nretry_time: %d, "
                      "\ndim_width: %d, \ndim_height: %d",
            retry_time, dim_width, dim_height);

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

    char retry_and_dim_message[32];
    pid_t pid = -1;
    int OK_size = 10;
    char okBuff[OK_size];

    while(1) {
        if(sent_dim == 0) {
            sprintf(retry_and_dim_message, "R:%dW:%dH:%d", retry_time, dim_width, dim_height);
            // zk, send data about retry and image to server
            err = SSL_write (ssl, retry_and_dim_message, strlen(retry_and_dim_message));  CHK_SSL(err);
            printf("Wrote %d chars:'%s'\n", strlen(retry_and_dim_message), retry_and_dim_message);
            sent_dim = 1;
        }

        int read = -1;
        int l_poll = poll(l_read_poll, 1, 5000);
        if (l_read_poll[0].revents & POLLIN) {
            read = SSL_read(ssl, buf, sizeof(buf) - 1);
            CHK_SSL(err);
            if(read > 0) {
                buf[read] = '\0';
            }
        }
        if(err < 1) {
            continue;
        }
        if(read > 1) {
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
//            receivedSizeLastTime = 1;
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
            // opening image
//            const char * p_name = "/usr/bin/xdg-open";
//            char* p_args[] = {OUT_FILE, NULL};
//            execv(p_name, p_args);

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

       /*     int status = system("ps aux | grep 'display ./img/recOut.jpg'  | grep -v 'grep' > tmp.ps.log");
            const char *filename = "tmp.ps.log";
            struct stat st;

            if (stat(filename, &st) != 0) {
                return EXIT_FAILURE;
            }
            log_msg(LOG_INFO, "file size: %zd\n", st.st_size);
            if(st.st_size == 0) {
//                status = system("/usr/bin/xdg-open ./img/recOut.jpg");

                status = system("display ./img/recOut.jpg & disown");
//                status = system("/usr/bin/xdg-open ./img/recOut.jpg");
            } else {
                status = system("display ./img/recOut.jpg & disown");
                log_msg(LOG_INFO, "display already running");
            }*/
            log_msg(LOG_INFO, "opening file %s done, status: %d\n", OUT_FILE);
            request_new_time = 1;

            log_msg(LOG_INFO, "Sleeping %d seconds ...", retry_time);


/*

            sprintf(retry_and_dim_message, "R:%dW:%dH:%d", retry_time, dim_width, dim_height);
            err = SSL_write (ssl, retry_and_dim_message, strlen(retry_and_dim_message));  CHK_SSL(err);
            printf("Wrote %d chars:'%s'\n", strlen(retry_and_dim_message), retry_and_dim_message);
*/

        } else {
            receivedSizeLastTime = 0;
        }
//            SSL_shutdown(ssl);
        sleep(retry_time);
        sent_dim = 0;
        log_msg(LOG_INFO, "woke up, requests time again ...", retry_time);


        /*
        if(request_new_time) {
            log_msg(LOG_INFO, "Sleeping %d seconds ...", retry_time);

//            SSL_shutdown(ssl);
            sleep(retry_time);
            log_msg(LOG_INFO, "woke up, requests time again ...", retry_time);


            sprintf(retry_and_dim_message, "R:%dW:%dH:%d", retry_time, dim_width, dim_height);
            err = SSL_write (ssl, retry_and_dim_message, strlen(retry_and_dim_message));  CHK_SSL(err);
            printf("Wrote %d chars:'%s'\n", strlen(retry_and_dim_message), retry_and_dim_message);

            int OK_size = 10;
            char okBuff[OK_size];
            err = SSL_read(ssl, okBuff, sizeof(buf) - 1);
            if(err < 1 || ( okBuff[0] != 'O' && okBuff[1] != 'K')) {
                continue;
            }
            request_new_time = 0;


            receivedSizeLastTime = 0;
            imageSize = -1;
        }
        if(receivedSizeLastTime != 0
            && imageSize != -1
        ) {
            log_msg(LOG_INFO, "Start download image...");
            char * imgBuf = (char*) malloc(imageSize);

            err = SSL_read(ssl, imgBuf, sizeof(buf) - 1);

            FILE * f = fopen (OUT_FILE, "wb");

            if (f) {
                fwrite(imgBuf, 1, imageSize, f);
                fclose (f);
            }

            receivedSizeLastTime = 0;
            log_msg(LOG_INFO, "Download image done");
            // opening image
//            const char * p_name = "/usr/bin/xdg-open";
//            char* p_args[] = {OUT_FILE, NULL};
//            execv(p_name, p_args);

            int status = system("ps aux | grep 'display ./img/recOut.jpg'  | grep -v 'grep' > tmp.ps.log");
            const char *filename = "tmp.ps.log";
            struct stat st;

            if (stat(filename, &st) != 0) {
                return EXIT_FAILURE;
            }
            log_msg(LOG_INFO, "file size: %zd\n", st.st_size);
            if(st.st_size == 0) {
//                status = system("/usr/bin/xdg-open ./img/recOut.jpg");
                status = system("display ./img/recOut.jpg & disown");
//                status = system("/usr/bin/xdg-open ./img/recOut.jpg");
            } else {
                status = system("display ./img/recOut.jpg & disown");
                log_msg(LOG_INFO, "display already running");
            }
*//*
            int status = system("ps aux | grep ./img/recOut.jpg  | grep -v 'grep' > tmp.ps.log");
            const char *filename = "tmp.ps.log";
            struct stat st;

            if (stat(filename, &st) != 0) {
                return EXIT_FAILURE;
            }
            log_msg(LOG_INFO, "file size: %zd\n", st.st_size);
            if(st.st_size == 0) {
//                status = system("/usr/bin/xdg-open ./img/recOut.jpg");
                status = system("/usr/bin/xdg-open ./img/recOut.jpg");
            }
*//*
            log_msg(LOG_INFO, "opening file %s done, status: %d\n", OUT_FILE, status);
            request_new_time = 1;
            continue;
        }
        err = SSL_read(ssl, buf, sizeof(buf) - 1);
        if(err < 1) {
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
        }*/
        /* Clean up. */
    }
    SSL_shutdown(ssl);  /* send SSL/TLS close_notify */
  close (sd);
  SSL_free (ssl);
  SSL_CTX_free (ctx);
}
/* EOF - cli.cpp */
