//***************************************************************************
//
// Program example for labs in subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2017
//
// Example of socket server.
//
// This program is example of socket server and it allows to connect and serve
// the only one client.
// The mandatory argument of program is port number for listening.
//
//***************************************************************************

// https://gist.github.com/alexandruc/2350954 - TODO fix ....

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define STR_CLOSE   "close"
#define STR_QUIT    "quit"

#define TMP_FILE "/tmp/test_sock"

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

//***************************************************************************
// help

void help( int t_narg, char **t_args ) {
    if ( t_narg <= 1 || !strcmp( t_args[ 1 ], "-h" ) ) {
        printf(
            "\n"
            "  Socket server example.\n"
            "\n"
            "  Use: %s [-u -4 -6 -h -d] port_number\n"
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
    if ( t_narg <= 1 ) { help( t_narg, t_args ); }

    int l_port = 0;
    char *filename = NULL;

    // parsing arguments
    for ( int i = 1; i < t_narg; i++ ) {

        if ( !strcmp( t_args[ i ], "-u" ) ) {
            g_socket = 1;
            filename = t_args[i + 1];
            i++; // Skip next arg
        }

        if ( !strcmp( t_args[ i ], "-4" ) ) {
            g_ipv4 = 1;
        }

        if ( !strcmp( t_args[ i ], "-6" ) ) {
            g_ipv6 = 1;
        }

        if ( !strcmp( t_args[ i ], "-d" ) ) {
            g_debug = LOG_DEBUG;
        }

        if ( !strcmp( t_args[ i ], "-h" ) ) {
            help( t_narg, t_args );
        }

        if ( *t_args[ i ] != '-' && !l_port ) {
            l_port = atoi( t_args[ i ] );
            break;
        }
    }

    if (g_socket == 1) {
        g_ipv4 = 0;
        g_ipv6 = 0;
    }
    if (g_ipv4 == 1) {
        g_ipv6 = 0;
    }

    if (!g_socket &&  l_port <= 0 ) {
        log_msg( LOG_INFO, "Bad or missing port number %d!", l_port );
        help( t_narg, t_args );
    }

    if(g_ipv4 || g_ipv6) {
        log_msg( LOG_INFO, "Server will listen on port: %d.", l_port );
    }

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

    if(g_socket == 1) {
        //create server side
        int s = 0;
        struct sockaddr_un local, remote;
        int len = 0;

        socket_sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if( -1 == s )
        {
            printf("Error on socket() call \n");
            return 1;
        }
//        socket_sock = s;
        l_sock_client = socket_sock;
        local.sun_family = AF_UNIX;
        strcpy( local.sun_path, filename );
        unlink(local.sun_path);
        len = strlen(local.sun_path) + sizeof(local.sun_family);
        if(bind(socket_sock, (struct sockaddr*)&local, len) != 0)
        {
            printf("Error on binding socket \n");
            return 1;
        }

        if( listen(socket_sock, 1) != 0 )
        {
            printf("Error on listen call \n");
        }

/*        unsigned int sock_len = 0;
        printf("Waiting for connection.... \n");
        if( (s2 = accept(s, (struct sockaddr*)&remote, &sock_len)) == -1 )
        {
            printf("Error on accept() call \n");
            return 1;
        }*/

        l_sock_client = s2;
        printf("Server connected \n");

    } else if (g_ipv4 || g_ipv6) {
        // TODO g_ipv4 || g_ipv6
        log_msg( LOG_DEBUG, "TODO g_ipv4 || g_ipv6" );

        if (g_ipv4 || g_ipv6) {
            struct sockaddr *l_addr_ptr;
            socklen_t l_addr_len;

            if (g_ipv6) {
                // IPv6 socket
                /* Create socket for listening (client requests) */
                listen_sock_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
                if(listen_sock_fd == -1) {
                    perror("socket()");
                    return EXIT_FAILURE;
                }

                /* Set socket to reuse address */
                flag = 1;
                ret = setsockopt(listen_sock_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
                if(ret == -1) {
                    perror("setsockopt()");
                    return EXIT_FAILURE;
                }

                server_addr.sin6_family = AF_INET6;
                server_addr.sin6_addr = in6addr_any;
                server_addr.sin6_port = htons(l_port);

                /* Bind address and socket together */
                ret = bind(listen_sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
                if(ret == -1) {
                    perror("bind()");
                    close(listen_sock_fd);
                    return EXIT_FAILURE;
                }

                /* Create listening queue (client requests) */
                ret = listen(listen_sock_fd, 10); // CLIENT_QUEUE_LEN
                if (ret == -1) {
                    perror("listen()");
                    close(listen_sock_fd);
                    return EXIT_FAILURE;
                }

                l_sock_listen = listen_sock_fd;

                client_addr_len = sizeof(client_addr);
            } else {
                // IPv4 socket
                l_sock_listen = socket(AF_INET, SOCK_STREAM, 0);
                if (l_sock_listen == -1) {
                    log_msg(LOG_ERROR, "Unable to create IPv4 socket.");
                    exit(1);
                }

                in_addr l_addr_any = { INADDR_ANY };
                sockaddr_in l_srv_addr;
                l_srv_addr.sin_family = AF_INET;
                l_srv_addr.sin_port = htons( l_port );
                l_srv_addr.sin_addr = l_addr_any;

                // Enable the port number reusing
                int l_opt = 1;
                if ( setsockopt( l_sock_listen, SOL_SOCKET, SO_REUSEADDR,
                                 &l_opt, sizeof( l_opt ) ) < 0 )
                    log_msg( LOG_ERROR, "Unable to set socket option!" );

                // assign port number to socket
                if ( bind( l_sock_listen, (const sockaddr * ) &l_srv_addr, sizeof( l_srv_addr ) ) < 0 )
                {
                    log_msg( LOG_ERROR, "Bind failed!" );
                    close( l_sock_listen );
                    exit( 1 );
                }

                // listenig on set port
                if ( listen( l_sock_listen, 1 ) < 0 )
                {
                    log_msg( LOG_ERROR, "Unable to listen on given port!" );
                    close( l_sock_listen );
                    exit( 1 );
                }

            }

        }
    }

    log_msg( LOG_INFO, "Enter 'quit' to quit server." );

    // go!
    while ( 1 ) {

        // list of fd sources
        pollfd l_read_poll[ 2 ];

        l_read_poll[ 0 ].fd = STDIN_FILENO;
        l_read_poll[ 0 ].events = POLLIN;
        l_read_poll[ 1 ].fd = l_sock_listen;
        l_read_poll[ 1 ].events = POLLIN;
        // https://www.man7.org/linux/man-pages/man7/unix.7.html - todo ...
        while ( 1 ) { // wait for new client
            // select from fds
            if (g_socket == 1) {
                // l_sock_client = accept(socket_sock, NULL, 0);
                /*
                unsigned int sock_len = 0;
                printf("Waiting for connection.... \n");
                if((l_sock_client = accept(socket_sock, (struct sockaddr*)&remote,
                        &sock_len)) == -1 ) {
                    printf("Error on accept() call \n");
                    return 1;
                }
                */

                unsigned int sock_len = 0;
                printf("Waiting for connection.... \n");
                if((s2 = accept(socket_sock, (struct sockaddr*)&remote, &sock_len)) == -1 ) {
                    printf("Error on accept() call \n");
                    return 1;
                }
            }

            int timeout = -1;
            if (g_socket == 1) {
                timeout = -1;
            }

            int l_poll = poll( l_read_poll, 2, timeout);

            if ( l_poll < 0 )
            {
                log_msg( LOG_ERROR, "Function poll failed!" );
                exit( 1 );
            }

            if ( l_read_poll[ 0 ].revents & POLLIN ) { // data on stdin
                char buf[ 128 ];
                int len = read( STDIN_FILENO, buf, sizeof( buf) );
                if ( len < 0 ) {
                    log_msg( LOG_DEBUG, "Unable to read from stdin!" );
                    exit( 1 );
                }

                log_msg( LOG_DEBUG, "Read %d bytes from stdin" );
                // request to quit?
                if ( !strncmp( buf, STR_QUIT, strlen(STR_QUIT)) ) {
                    log_msg( LOG_INFO, "Request to 'quit' entered.");
                    close( l_sock_listen );
                    exit( 0 );
                }
            }

            if ( l_read_poll[ 1 ].revents & POLLIN ) {
                // new client?
                sockaddr_in l_rsa;
                int l_rsa_size = sizeof( l_rsa );
                // new connection
                if (g_socket == 1) {
//                    l_sock_client = accept(socket_sock, NULL, 0);

                } else {
                    if(g_ipv6) {
                        client_sock_fd = accept(l_sock_listen,
                                                (struct sockaddr*)&client_addr,
                                                &client_addr_len);
                        l_sock_client = client_sock_fd;
                        if (client_sock_fd == -1) {
                            perror("accept()");
                            close(listen_sock_fd);
                            return EXIT_FAILURE;
                        }

                        inet_ntop(AF_INET6, &(client_addr.sin6_addr),
                                  str_addr, sizeof(str_addr));
                        printf("New connection from: %s:%d ...\n",
                               str_addr,
                               ntohs(client_addr.sin6_port));

                    } else if(g_ipv4) {
                        l_sock_client = accept(l_sock_listen, (sockaddr *) &l_rsa, (socklen_t *) &l_rsa_size);
                    }
                }
                if ( l_sock_client == -1 ) {
                    log_msg( LOG_ERROR, "Unable to accept new client." );
//                        close( l_sock_listen );
//                        exit( 1 );
                }
                if(g_ipv6) {
                } else if(g_ipv4) {
                    uint l_lsa = sizeof(l_srv_addr);
                    // my IP
                    getsockname(l_sock_client, (sockaddr *) &l_srv_addr, &l_lsa);
                    log_msg(LOG_INFO, "My IP: '%s'  port: %d",
                            inet_ntoa(l_srv_addr.sin_addr), ntohs(l_srv_addr.sin_port));
                    // client IP
                    getpeername(l_sock_client, (sockaddr *) &l_srv_addr, &l_lsa);
                    log_msg(LOG_INFO, "Client IP: '%s'  port: %d",
                            inet_ntoa(l_srv_addr.sin_addr), ntohs(l_srv_addr.sin_port));
                }
                break;
            }

        } // while wait for client

        // change source from sock_listen to sock_client
        if(g_socket != 1) {
            l_read_poll[ 1 ].fd = l_sock_client;
        }

        while ( 1  )
        { // communication
            char l_buf[ 256 ];

            // select from fds
            int l_poll = poll( l_read_poll, 2, -1 );

            if ( l_poll < 0 )
            {
                log_msg( LOG_ERROR, "Function poll failed!" );
                exit( 1 );
            }

            // data on stdin?
            if ( l_read_poll[ 0 ].revents & POLLIN )
            {
                // read data from stdin
                int l_len = read( STDIN_FILENO, l_buf, sizeof( l_buf ) );
                if ( l_len < 0 ) {
                    log_msg( LOG_ERROR, "Unable to read data from stdin." );
                } else {
                    log_msg( LOG_DEBUG, "Read %d bytes from stdin.", l_len );
                }

                if (g_socket) {
                    // TODO unix
                    l_len = write(s2, l_buf, l_len);
                    if (l_len < 0)
                        log_msg(LOG_ERROR, "Unable to send data to client.");
                    else
                        log_msg(LOG_DEBUG, "Sent %d bytes to client.", l_len);
                } else if (g_ipv4) {
                    // send data to client
                    l_len = write(l_sock_client, l_buf, l_len);
                    if (l_len < 0)
                        log_msg(LOG_ERROR, "Unable to send data to client.");
                    else
                        log_msg(LOG_DEBUG, "Sent %d bytes to client.", l_len);
                } else if (g_ipv6) {
                    // send data to client
                    l_len = write(l_sock_client, l_buf, l_len);
                    if (l_len < 0) {
                        log_msg(LOG_ERROR, "Unable to send data to client.");
                    } else {
                        log_msg(LOG_DEBUG, "Sent %d bytes to client.", l_len);
                    }
                }
            }
            // data from client?
            if ( l_read_poll[ 1 ].revents & POLLIN )
            {
                // read data from socket
                int l_len = read( l_sock_client, l_buf, sizeof( l_buf ) );
                if ( !l_len ) {
                    log_msg( LOG_DEBUG, "Client closed socket!" );
                    close( l_sock_client );
                    break;
                } else if ( l_len < 0 ) {
                    log_msg( LOG_ERROR, "Unable to read data from client." );
                    close( l_sock_client );
                    break;
                } else {
                    log_msg( LOG_DEBUG, "Read %d bytes from client.", l_len );
                }

                // write data to client
                l_len = write( STDOUT_FILENO, l_buf, l_len );
                if ( l_len < 0 ) {
                    log_msg( LOG_ERROR, "Unable to write data to stdout." );
                }

                // close request?
                if ( !strncasecmp( l_buf, "close", strlen( STR_CLOSE ) ) ) {
                    log_msg( LOG_INFO, "Client sent 'close' request to close connection.");
                    close( l_sock_client );
                    log_msg( LOG_INFO, "Connection closed. Waiting for new client.");
                    break;
                }
            }
            // request for quit
            if ( !strncasecmp( l_buf, "quit", strlen( STR_QUIT ) ) )
            {
                close( l_sock_listen );
                close( l_sock_client );
                log_msg( LOG_INFO, "Request to 'quit' entered" );
                exit( 0 );
            }
        } // while communication
    } // while ( 1 )

//    if( g_socket == 1) {
    close( socket_sock );
    close( listen_sock_fd );
    close( client_sock_fd );
//    close( sock );
    unlink( sun.sun_path );
//    }

    return 0;
}

