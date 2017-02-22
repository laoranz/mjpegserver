/* 
 * File:   server.c
 * Author: Le Borgne
 * 
 * Created on 9 février 2017, 22:20
 */

#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>
#include <sys/poll.h>
#include <stdlib.h>

#include "server.h"
#include "clients.h"
#include "log.h"

#define BUFFER_SIZE     1024

static int socket_sock = -1;

static void send_error_not_found ( int sock ) {
    
    char data[] = "HTTP/1.1 404 Not Found\r\n" \
                  "Content-Length: 0\r\n" \
                  "Connection: close\r\n\r\n";
    
    write ( sock, data, sizeof(data) );
}

static int get_infos_url ( int sock, char **client, char **args ) {
    
    char buffer[BUFFER_SIZE];
    size_t buffer_size = 0;
    char *pcr = NULL;
    
    LOG_DEBUG("[%03d] client", sock );
    
    // Attente de l'URL
    for (;;) {
        
        struct pollfd pfds[2];
        int rv;

        pfds[0].fd = sock;
        pfds[0].events = POLLIN;

        // Connexion d'un equipement
        if ((rv = poll(pfds, 1, 2000)) > 0) {

            if (pfds[0].revents & POLLIN) {
                
                int size = recv(sock, &buffer[buffer_size], BUFFER_SIZE-1-buffer_size, 0);
                
                LOG_DEBUG("[%03d] receive %d bytes", sock, size );
                if ( size > 0 ) {
                    
                    buffer_size += size;
                    buffer[buffer_size] = '\0';
                    
                    if ( (pcr = strstr( buffer, "\n" )) != NULL ) {
                        *pcr = '\0';
                        break;
                    }
                }
                else if ( size < 0 ) {
                    LOG_ERROR_DETAILS("[%03d] recv error [%s]", sock, strerror(errno));
                    break;
                }
                else {
                    LOG_DEBUG("[%03d] fin de connexion", sock );
                    break;
                }
            }
        }
    }
    
    if ( pcr != NULL ) {
        
        char *p = buffer;
        char *_client = NULL;
        char *_args = NULL;
        
        if ( (p = strstr( p, "/" )) == NULL ) 
            return -1;
        
        if ( *(p+1)==' ' ) 
            return -2;
        
        _client = p+1;

        if ( (p = strstr( _client, "?" )) != NULL ) {
            *p++='\0';
            _args = p;
        }
        
        p=_client;
        if ( _args!= NULL ) {
            p=_args;
        }
        
        if ( (p = strstr( p, " " )) != NULL ) {
            *p='\0';
        }
                
        if ( _client != NULL ) {
            *client = calloc( strlen(_client)+1, sizeof(char) );
            strncpy( *client, _client, strlen(_client) );
        }
        
        if ( _args != NULL ) {
            char * pch;
            *args = calloc( strlen(_args)+1, sizeof(char) );
            strncpy( *args, _args, strlen(_args) );
            pch = strchr( *args ,'&');
            while (pch!=NULL) {
                *pch=' ';
                pch=strchr(pch+1,'&');
            }
        }
    }
    
    return 0;
}

static void *connection_handler(void *socket_desc) {
    
    pthread_detach(pthread_self());
    
    int sock = *(int*)socket_desc;
    char *client = NULL, *args = NULL;
    client_handler_t client_handler = NULL;

    get_infos_url ( sock, &client, &args );
    
    client_handler = clients_find_client ( client );
    
    if ( client_handler != NULL ) {
        
        LOG_DEBUG("[%03d] client_handler:%p, client:%s, args:%s", sock, client_handler, client, args );
        
        (*client_handler) ( sock, args );
    }
    else {
        send_error_not_found ( sock );
    }
    
    LOG_DEBUG("[%03d] close", sock );
    
    close ( sock );
    
    free (socket_desc);
    
    if ( client != NULL ) {
        free ( client );
    }
    
    if ( args != NULL ) {
        free ( args );
    }
    
    return NULL;
}

int mjpegserver_server_start(int port) {

    struct sockaddr_in server;
    int on = 1;

    socket_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_sock == -1) {
        LOG_ERROR_DETAILS("socket error [%s]", strerror(errno));
        return -1;
    }

    if (setsockopt(socket_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) < 0) {
        LOG_ERROR_DETAILS("setsockopt error [%s]", strerror(errno));
        return -2;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    if (bind(socket_sock, (struct sockaddr *) &server, sizeof (server)) < 0) {
        LOG_ERROR_DETAILS("bind error [%s]", strerror(errno));
        return -3;
    }

    if (listen(socket_sock, 10) < 0) {
        LOG_ERROR_DETAILS("listen error [%s]", strerror(errno));
        return -4;
    }

    return 0;
}

int mjpegserver_server_run ( void ) {
    
    int sockaddr_in_size = sizeof (struct sockaddr_in);
    struct sockaddr_in client;
    int client_sock;

    struct pollfd pfds[2];
    int rv;

    pfds[0].fd = socket_sock;
    pfds[0].events = POLLIN;

    // Connexion d'un equipement
    if ((rv = poll(pfds, 1, 2000)) > 0) {

        if (pfds[0].revents & POLLIN) {

            client_sock = accept(socket_sock, (struct sockaddr *) &client, (socklen_t*) & sockaddr_in_size);

            // si le socket est valide
            if (client_sock > 0) {
                pthread_t sniffer_thread;

                int *new_sock = malloc(sizeof (int));
                (*new_sock) = client_sock;

                if (pthread_create(&sniffer_thread, NULL, connection_handler, (void*) new_sock) < 0) {
                    LOG_ERROR_DETAILS("pthread_create error [%s]", strerror(errno));
                    return -1;
                }
            }
        }
    }

    return 0;
}

void mjpegserver_server_stop ( void ) {
    
    if ( socket_sock < 0 )
        return;
    
    close(socket_sock);
}

