//***************************************************************************
//
// Program example for subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2021
//
// Example of socket server/client.
//
// This program is example of socket client.
// The mandatory arguments of program is IP adress or name of server and
// a port number.
//
//***************************************************************************

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define STR_CLOSE               "close"
#define TMP_FILE "/tmp/sock"
#define BUFFER_SIZE 1024

//***************************************************************************
// log messages

#define LOG_ERROR               0       // errors
#define LOG_INFO                1       // information and notifications
#define LOG_DEBUG               2       // debug messages

// debug flag
int g_debug = LOG_INFO;
int g_socket = 0;
int g_ipv4 = 0;
int g_ipv6 = 0;

void log_msg( int t_log_level, const char *t_form, ... )
{
    const char *out_fmt[] = {
            "ERR: (%d-%s) %s\n",
            "INF: %s\n",
            "DEB: %s\n" };

    if ( t_log_level && t_log_level > g_debug ) return;

    char l_buf[ 1024 ];
    va_list l_arg;
    va_start( l_arg, t_form );
    vsprintf( l_buf, t_form, l_arg );
    va_end( l_arg );

    switch ( t_log_level )
    {
    case LOG_INFO:
    case LOG_DEBUG:
        fprintf( stdout, out_fmt[ t_log_level ], l_buf );
        break;

    case LOG_ERROR:
        fprintf( stderr, out_fmt[ t_log_level ], errno, strerror( errno ), l_buf );
        break;
    }
}

//***************************************************************************
// help

void help( int t_narg, char **t_args )
{
    if ( t_narg <= 1 || !strcmp( t_args[ 1 ], "-h" ) )
    {
        printf(
            "\n"
            "  Socket client example.\n"
            "\n"
            "  Use: %s [-u -4 -6 -h -d] ip_or_name port_number\n"
            "\n"
            "    -u  unix socket (default, priority 1)\n"
            "    -4  IPv4 (priority 2)\n"
            "    -6  IPv6 (priority 3)\n"
            "    -d  debug mode \n"
            "    -h  this help\n"
            "\n", t_args[ 0 ] );

        exit( 0 );
    }

    if ( !strcmp( t_args[ 1 ], "-d" ) )
        g_debug = LOG_DEBUG;
}

//***************************************************************************

int main( int t_narg, char **t_args )
{

    if ( t_narg <= 2 ) help( t_narg, t_args );

    int l_port = 0;
    char *l_host = nullptr;

    // parsing arguments
    for ( int i = 1; i < t_narg; i++ )
    {

        if ( !strcmp( t_args[ i ], "-u" ) ) {
            g_socket = 1;
        }

        if ( !strcmp( t_args[ i ], "-4" ) ) {
            g_ipv4 = 1;
        }

        if ( !strcmp( t_args[ i ], "-6" ) ) {
            g_ipv6 = 1;
        }

        if ( !strcmp( t_args[ i ], "-d" ) )
            g_debug = LOG_DEBUG;

        if ( !strcmp( t_args[ i ], "-h" ) )
            help( t_narg, t_args );

        if ( *t_args[ i ] != '-' )
        {
            if ( !l_host )
                l_host = t_args[ i ];
            else if ( !l_port )
                l_port = atoi( t_args[ i ] );
        }

    }

    if (g_socket == 1) {
        g_ipv4 = 0;
        g_ipv6 = 0;
    }
    if (g_ipv4 == 1) {
        g_ipv6 = 0;
    }

    if ( g_socket != 1 && ( !l_host || !l_port ))
    {
        log_msg( LOG_INFO, "Host or port is missing!" );
        help( t_narg, t_args );
        exit( 1 );
    }

    log_msg( LOG_INFO, "Connection to '%s':%d.", l_host, l_port );


        addrinfo l_ai_req, *l_ai_ans;
        bzero(&l_ai_req, sizeof(l_ai_req));
//    l_ai_req.ai_family = AF_INET;
        int domain = AF_UNIX;
        if(g_socket == 1) { // TODO if(g_socket)
            domain = AF_UNIX;
        } else if (g_ipv4 == 1) {
            domain = AF_INET;
        } else if (g_ipv6 == 1) {
            domain = AF_INET6;
        }

        if ( g_socket != 1) {
            l_ai_req.ai_family = domain;
            l_ai_req.ai_socktype = SOCK_STREAM;

            int l_get_ai = getaddrinfo(l_host, nullptr, &l_ai_req, &l_ai_ans);
            if (l_get_ai) {
                log_msg(LOG_ERROR, "Unknown host name!");
                exit(1);
            }
        }
        sockaddr_in l_cl_addr;
        int l_sock_server;
//        int l_sock_listen;
        struct sockaddr *l_addr_ptr;
        socklen_t l_addr_len;

    struct sockaddr_un addr;
    int ret;
    int data_socket;
    char buffer[BUFFER_SIZE];

        if(g_socket == 1) {

            /*
//            sockaddr_in l_cl_addr = *(sockaddr_in *) l_ai_ans->ai_addr;
            l_cl_addr = *(sockaddr_in *) l_ai_ans->ai_addr;
            l_cl_addr.sin_port = htons(l_port);
            freeaddrinfo(l_ai_ans);

            // socket creation
            // https://www.man7.org/linux/man-pages/man2/socket.2.html
            // AF_UNIX | AF_INET | AF_INET6
//            int l_sock_server = socket(AF_INET, SOCK_STREAM, 0);
            l_sock_server = socket(AF_LOCAL, SOCK_STREAM, 0);
            if (l_sock_server == -1) {
                log_msg(LOG_ERROR, "Unable to create socket.");
                exit(1);
            }

            // connect to server
            if (connect(l_sock_server, (sockaddr * ) & l_cl_addr, sizeof(l_cl_addr)) < 0) {
                log_msg(LOG_ERROR, "Unable to connect server.");
                exit(1);
            }

            uint l_lsa = sizeof(l_cl_addr);
            // my IP
            getsockname(l_sock_server, (sockaddr * ) & l_cl_addr, &l_lsa);
            log_msg(LOG_INFO, "My IP: '%s'  port: %d",
                    inet_ntoa(l_cl_addr.sin_addr), ntohs(l_cl_addr.sin_port));
            // server IP
            getpeername(l_sock_server, (sockaddr * ) & l_cl_addr, &l_lsa);
            log_msg(LOG_INFO, "Server IP: '%s'  port: %d",
                    inet_ntoa(l_cl_addr.sin_addr), ntohs(l_cl_addr.sin_port));
            */


            /* Create local socket. */

            data_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
            if (data_socket == -1) {
                perror("socket");
                exit(EXIT_FAILURE);
            }

            /*
             * For portability clear the whole structure, since some
             * implementations have additional (nonstandard) fields in
             * the structure.
             */

            memset(&addr, 0, sizeof(addr));

            /* Connect socket to socket address. */

            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, TMP_FILE, sizeof(addr.sun_path) - 1);

            ret = connect(data_socket, (const struct sockaddr *) &addr,
                          sizeof(addr));
            if (ret == -1) {
                fprintf(stderr, "The server is down.\n");
                exit(EXIT_FAILURE);
            }

//            /* Send arguments. */
/*
            for (size_t i = 1; i < argc; ++i) {
                ret = write(data_socket, argv[i], strlen(argv[i]) + 1);
                if (ret == -1) {
                    perror("write");
                    break;
                }
            }
*/

            /* Request result. */
            strcpy(buffer, "END");
            ret = write(data_socket, buffer, strlen(buffer) + 1);
            if (ret == -1) {
                perror("write");
                exit(EXIT_FAILURE);
            }

            /* Receive result. */

            ret = read(data_socket, buffer, sizeof(buffer));
            if (ret == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }
            /* Ensure buffer is 0-terminated. */
//            buffer[sizeof(buffer) - 1] = 0;
//            printf("Result = %s\n", buffer);

            /* Close socket. */
        }


        if (g_ipv4 || g_ipv6) {
            // TODO g_ipv4 || g_ipv6
            log_msg( LOG_DEBUG, "TODO g_ipv4 || g_ipv6" );

            if (g_ipv6) {
                // IPv6 socket
                l_sock_server = socket(AF_INET6, SOCK_STREAM, 0);
                if (l_sock_server == -1) {
                    log_msg(LOG_ERROR, "Unable to create IPv6 socket.");
                    exit(1);
                }

                struct sockaddr_in6 l_srv_addr6 = {};
                l_srv_addr6.sin6_family = AF_INET6;
                l_srv_addr6.sin6_port = htons(l_port);
                l_srv_addr6.sin6_addr = in6addr_any;

                l_addr_ptr = (struct sockaddr *)&l_srv_addr6;
                l_addr_len = sizeof(l_srv_addr6);

                if (g_ipv4) {
                    // Dual-stack socket
                    int l_opt = 0;
                    if (setsockopt(l_sock_server, IPPROTO_IPV6, IPV6_V6ONLY, &l_opt, sizeof(l_opt)) < 0) {
                        log_msg(LOG_ERROR, "Unable to set dual-stack socket option!");
                    }
                }
            } else {
                // IPv4 socket
                l_sock_server = socket(AF_INET, SOCK_STREAM, 0);
                if (l_sock_server == -1) {
                    log_msg(LOG_ERROR, "Unable to create IPv4 socket.");
                    exit(1);
                }

                struct sockaddr_in l_srv_addr = {};
                l_srv_addr.sin_family = AF_INET;
                l_srv_addr.sin_port = htons(l_port);
                l_srv_addr.sin_addr.s_addr = INADDR_ANY;

                l_addr_ptr = (struct sockaddr *)&l_srv_addr;
                l_addr_len = sizeof(l_srv_addr);
            }

            // The rest of your socket setup code goes here...
            // For example, setsockopt for SO_REUSEADDR, bind, listen, etc.
            // Use l_sock_listen, l_addr_ptr, and l_addr_len for these operations
        }

        log_msg(LOG_INFO, "Enter 'close' to close application.");

        // list of fd sources
        pollfd l_read_poll[2];

        l_read_poll[0].fd = STDIN_FILENO;
        l_read_poll[0].events = POLLIN;
        l_read_poll[1].fd = l_sock_server;
        l_read_poll[1].events = POLLIN;

        // go!
        while (1) {
            char l_buf[128];

            // select from fds
            if (poll(l_read_poll, 2, -1) < 0) break;

            // data on stdin?s
            if (l_read_poll[0].revents & POLLIN) {
                //  read from stdin
                int l_len = read(STDIN_FILENO, l_buf, sizeof(l_buf));
                if (l_len < 0)
                    log_msg(LOG_ERROR, "Unable to read from stdin.");
                else
                    log_msg(LOG_DEBUG, "Read %d bytes from stdin.", l_len);

                // send data to server
                l_len = write(l_sock_server, l_buf, l_len);
                if (l_len < 0)
                    log_msg(LOG_ERROR, "Unable to send data to server.");
                else
                    log_msg(LOG_DEBUG, "Sent %d bytes to server.", l_len);
            }

            // data from server?
            if (l_read_poll[1].revents & POLLIN) {
                // read data from server
//                int l_len = read(l_sock_server, l_buf, sizeof(l_buf));
                int l_len = read(l_sock_server, l_buf, sizeof(l_buf));
                if(g_socket == 1) {
                } else if (g_ipv4) {
                    log_msg( LOG_DEBUG, "g_ipv4" );
                    l_len = read(l_sock_server, l_buf, sizeof(l_buf));
                }
                if (!l_len) {
                    log_msg(LOG_DEBUG, "Server closed socket.");
                    break;
                } else if (l_len < 0) {
                    log_msg(LOG_ERROR, "Unable to read data from server.");
                    break;
                } else
                    log_msg(LOG_DEBUG, "Read %d bytes from server.", l_len);

                // display on stdout
                l_len = write(STDOUT_FILENO, l_buf, l_len);
                if (l_len < 0)
                    log_msg(LOG_ERROR, "Unable to write to stdout.");

                // request to close?
                if (!strncasecmp(l_buf, STR_CLOSE, strlen(STR_CLOSE))) {
                    log_msg(LOG_INFO, "Connection will be closed...");
                    break;
                }
            }
        }

        // close socket
        close(l_sock_server);

        if(g_socket == 1) {
            close(data_socket);
        }

    return 0;
  }
