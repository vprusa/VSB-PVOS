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
#define TMP_FILE "/tmp/test_sock"
#define BUFFER_SIZE 1024

//***************************************************************************
// log messages

#define LOG_ERROR               0       // errors
#define LOG_INFO                1       // information and notifications
#define LOG_DEBUG               2       // debug messages


static const char* socket_path = "/tmp/mysocket";

// debug flag
int g_debug = LOG_INFO;
int g_socket = 0;
int g_ipv4 = 0;
int g_ipv6 = 0;

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

//***************************************************************************
// help

void help( int t_narg, char **t_args ) {
    if ( t_narg <= 1 || !strcmp( t_args[ 1 ], "-h" ) ) {
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

int main( int t_narg, char **t_args ) {

    if ( t_narg <= 2 ) { help( t_narg, t_args ); }

    int l_port = 0;
    char *l_host = nullptr;

    // parsing arguments
    for ( int i = 1; i < t_narg; i++ ) {

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

    if ( g_socket != 1 && ( !l_host || !l_port )) {
        log_msg( LOG_INFO, "Host or port is missing!" );
        help( t_narg, t_args );
        exit( 1 );
    }

//    log_msg( LOG_INFO, "Connection to '%s':%d.", l_host, l_port );

    if (g_socket != 1) {

        addrinfo l_ai_req, *l_ai_ans;
        bzero(&l_ai_req, sizeof(l_ai_req));
        int domain = AF_UNIX;
        if(g_socket == 1) { // TODO if(g_socket)
            domain = AF_UNIX;
        } else if (g_ipv4 == 1) {
            domain = AF_INET;
        } else if (g_ipv6 == 1) {
            domain = AF_INET6;
        }

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
    struct sockaddr *l_addr_ptr;
    socklen_t l_addr_len;

    struct sockaddr_un addr;
    int data_socket;

    int sock_fd = -1;
    struct sockaddr_in6 server_addr;
    int ret;
    sockaddr_un sun;

    if(g_socket == 1) {
        /*
//            int sock = socket( AF_UNIX, SOCK_STREAM , 0 );
        data_socket = socket( AF_UNIX, SOCK_STREAM , 0 );
//        data_socket = socket( AF_UNIX, SOCK_DGRAM, 0 );

        int retry = 10;
        sun. sun_family = AF_UNIX;
        strcpy( sun. sun_path , TMP_FILE );
        while(retry-- > 0) {
            int con_res = connect(data_socket, (sockaddr *) &sun, sizeof(sun));
            if (con_res == -1) {
                log_msg(LOG_DEBUG, "Unable to connect to server.");
            } else {
                break;
            }
        }
        l_sock_server = data_socket;
// communication by sock
//            close( sock );
         */
/*
        sun.sun_family = AF_UNIX;
        strcpy( sun.sun_path, TMP_FILE );
        int data_len = strlen(sun.sun_path) + sizeof(sun.sun_family);

        printf("Client: Trying to connect... \n");
        if( connect(data_socket, (struct sockaddr*)&sun, data_len) == -1 ) {
            printf("Client: Error on connect call \n");
            return 1;
        }
        */
/*
        int retry = 10;
        sun. sun_family = AF_UNIX;
//        strcpy( sun. sun_path , "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
        strcpy( sun. sun_path , TMP_FILE );
        while(--retry > 0) {
            int data_len = strlen(sun.sun_path) + sizeof(sun.sun_family);
//            int con_res = connect(data_socket, (struct sockaddr *) &sun, sizeof(sun));
            int con_res = connect(data_socket, (struct sockaddr *) &sun, data_len);
            if (con_res == -1) {
                log_msg(LOG_DEBUG, "Unable to connect to server.");
            } else {
                printf("Unable to connect, quitting\n");
            }
        }
        if(retry <= 0) {
            printf("Unable to connect, quitting\n");
            exit(1);
        }*/
//        static const char* socket_path = "/tmp/sock_test";
        static const unsigned int s_recv_len = 200;
        static const unsigned int s_send_len = 100;
        int sock = 0;
        int data_len = 0;
        struct sockaddr_un remote;
        char recv_msg[s_recv_len];
        char send_msg[s_send_len];

        memset(recv_msg, 0, s_recv_len*sizeof(char));
        memset(send_msg, 0, s_send_len*sizeof(char));

        if( (sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1  )
        {
            printf("Client: Error on socket() call \n");
            return 1;
        }

        remote.sun_family = AF_UNIX;
        strcpy( remote.sun_path, socket_path );
        data_len = strlen(remote.sun_path) + sizeof(remote.sun_family);

        printf("Client: Trying to connect... \n");
        if( connect(sock, (struct sockaddr*)&remote, data_len) == -1 )
        {
            printf("Client: Error on connect call \n");
            return 1;
        }
        l_sock_server = sock;

        printf("Client: Connected \n");

    } else if (g_ipv4 || g_ipv6) {
        // TODO g_ipv4 || g_ipv6
        log_msg( LOG_DEBUG, "TODO g_ipv4 || g_ipv6" );

        if (g_ipv6) {
            // IPv6 socket

            /* Create socket for communication with server */
            sock_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
            if (sock_fd == -1) {
                perror("socket()");
                return EXIT_FAILURE;
            }

            /* Connect to server running on localhost */
            server_addr.sin6_family = AF_INET6;
            inet_pton(AF_INET6, l_host, &server_addr.sin6_addr);
            server_addr.sin6_port = htons(l_port);

            /* Try to do TCP handshake with server */
            ret = connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
            if (ret == -1) {
                perror("connect()");
                close(sock_fd);
                return EXIT_FAILURE;
            }
            l_sock_server = sock_fd;

        } else {
            // IPv4 socket

            addrinfo l_ai_req, *l_ai_ans;
            bzero( &l_ai_req, sizeof( l_ai_req ) );
            l_ai_req.ai_family = AF_INET;
            l_ai_req.ai_socktype = SOCK_STREAM;

            int l_get_ai = getaddrinfo( l_host, nullptr, &l_ai_req, &l_ai_ans );
            if ( l_get_ai ) {
                log_msg( LOG_ERROR, "Unknown host name!" );
                exit( 1 );
            }

            sockaddr_in l_cl_addr =  *( sockaddr_in * ) l_ai_ans->ai_addr;
            l_cl_addr.sin_port = htons( l_port );
            freeaddrinfo( l_ai_ans );

            // socket creation
            l_sock_server = socket( AF_INET, SOCK_STREAM, 0 );
            if ( l_sock_server == -1 ) {
                log_msg( LOG_ERROR, "Unable to create socket.");
                exit( 1 );
            }

            // connect to server
            if ( connect( l_sock_server, ( sockaddr * ) &l_cl_addr, sizeof( l_cl_addr ) ) < 0 ) {
                log_msg( LOG_ERROR, "Unable to connect server." );
                exit( 1 );
            }

            uint l_lsa = sizeof( l_cl_addr );
            // my IP
            getsockname( l_sock_server, ( sockaddr * ) &l_cl_addr, &l_lsa );
            log_msg( LOG_INFO, "My IP: '%s'  port: %d",
                     inet_ntoa( l_cl_addr.sin_addr ), ntohs( l_cl_addr.sin_port ) );
            // server IP
            getpeername( l_sock_server, ( sockaddr * ) &l_cl_addr, &l_lsa );
            log_msg( LOG_INFO, "Server IP: '%s'  port: %d",
                     inet_ntoa( l_cl_addr.sin_addr ), ntohs( l_cl_addr.sin_port ) );

        }

    }

    log_msg(LOG_INFO, "Enter 'close' to close application.");

    // list of fd sources
    pollfd l_read_poll[2];
    pollfd l_read_poll1[1];
    pollfd l_read_poll2[1];

    l_read_poll1[0].fd = STDIN_FILENO;
    l_read_poll1[0].events = POLLIN;

    l_read_poll[0].fd = STDIN_FILENO;
    l_read_poll[0].events = POLLIN;
    l_read_poll[1].fd = l_sock_server;
    l_read_poll[1].events = POLLIN;

    l_read_poll2[0].fd = l_sock_server;
    l_read_poll2[0].events = POLLIN;

    // go!
    while ( 1 )
    {
        char l_buf[ 128 ];

        int timeout = -1;
        if (g_socket == 1) {
            timeout = 1000;
        }
/*        if (g_socket == 1) {

            // select from fds
            if (poll(l_read_poll1, 2, timeout) < 0) {
                log_msg( LOG_ERROR, "Idk. poll 1" );
//                continue;
            };
            if (poll(l_read_poll2, 2, timeout) < 0) {
                log_msg( LOG_ERROR, "Idk. poll 2" );
//                continue;
            };
//            log_msg( LOG_ERROR, "Idk." );

        } else {
            if (poll(l_read_poll, 2, -1) < 0) {
            }
        }*/

        if (poll(l_read_poll, 2, -1) < 0) {
            log_msg( LOG_ERROR, "Idk." );
        }


        // data on stdin?
        if ( l_read_poll[ 0 ].revents & POLLIN ) {
            //  read from stdin
//            int l_len = read( STDIN_FILENO, l_buf, sizeof( l_buf ) );
            int l_len = read( 0, l_buf, sizeof( l_buf ) );
            if ( l_len < 0 ) {
                log_msg( LOG_ERROR, "Unable to read from stdin." );
            } else {
                log_msg( LOG_DEBUG, "Read %d bytes from stdin.", l_len );
            }

            if(g_socket) {
                // TODO
                l_len = write(l_sock_server, l_buf, l_len);
                if (l_len < 0) {
                    log_msg(LOG_ERROR, "Unable to send data to server.");
                } else {
                    log_msg(LOG_DEBUG, "Sent %d bytes to server.", l_len);
                }
            } else if(g_ipv4) {
                // send data to server
                l_len = write(l_sock_server, l_buf, l_len);
                if (l_len < 0) {
                    log_msg(LOG_ERROR, "Unable to send data to server.");
                } else {
                    log_msg(LOG_DEBUG, "Sent %d bytes to server.", l_len);
                }
            } else if (g_ipv6) {
                /* Send data to server */
                l_len = write(sock_fd, l_buf, l_len);
                if (l_len < 0) {
                    log_msg(LOG_ERROR, "Unable to send data to server.");
                } else {
                    log_msg(LOG_DEBUG, "Sent %d bytes to server.", l_len);
                }

            }
        }

        // data from server?
        if ( l_read_poll[ 1 ].revents & POLLIN )
        {
            // read data from server
            int l_len = read( l_sock_server, l_buf, sizeof( l_buf ) );
            if ( !l_len )
            {
                log_msg( LOG_DEBUG, "Server closed socket." );
                break;
            }
            else if ( l_len < 0 )
            {
                log_msg( LOG_ERROR, "Unable to read data from server." );
                break;
            }
            else
                log_msg( LOG_DEBUG, "Read %d bytes from server.", l_len );

            // display on stdout
            l_len = write( STDOUT_FILENO, l_buf, l_len );
            if ( l_len < 0 )
                log_msg( LOG_ERROR, "Unable to write to stdout." );

            // request to close?
            if ( !strncasecmp( l_buf, STR_CLOSE, strlen( STR_CLOSE ) ) )
            {
                log_msg( LOG_INFO, "Connection will be closed..." );
                break;
            }
        }
    }

/*

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
*/

    // close socket
    close(l_sock_server);

    if(g_socket == 1) {
        close(data_socket);
    }

    return 0;
  }
