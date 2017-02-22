/* 
 * File:   client_mjpeg.c
 * Author: Le Borgne
 * 
 * Created on 10 février 2017, 23:04
 */

#include <stdio.h>
#include <string.h>
#include <alloca.h>
#include <sys/inotify.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "client_mjpeg.h"
#include "http_tools.h"
#include "log.h"

#define DATA_SIZE       512
#define INOTIFY_FLAGS   (IN_CLOSE_WRITE | IN_DELETE_SELF | IN_MOVED_FROM)

static int is_file_existe ( const char* file ) {
    
    struct stat buf;
        
    if ( stat( file, &buf ) < 0 ) {
        return 0;
    }
    
    return 1;
}

static void write_file ( const char* prefix, const char *sufix, const char *value ) {
    
    FILE *pFile;
    size_t fileName_size = strlen(prefix) + strlen(sufix) + 2;
    char *sFileName = alloca(fileName_size + 1);
    
    snprintf ( sFileName, fileName_size, "%s.%s", prefix, sufix );
    
    if ( (pFile = fopen(sFileName, "w")) == NULL ) {
        LOG_ERROR_DETAILS("fopen error [%s]", strerror(errno));
        return;
    }
    
    fwrite ( value, sizeof(char), strlen(value), pFile );
    fwrite ( "\n", sizeof(char), 1, pFile );
    
    fclose ( pFile );
}

static void remove_file ( const char* prefix, const char *sufix ) {
    
    size_t fileName_size = strlen(prefix) + strlen(sufix) + 2;
    char *sFileName = alloca(fileName_size + 1);
    
    snprintf ( sFileName, fileName_size, "%s.%s", prefix, sufix );
    
    remove ( sFileName );
}

static int is_file_change ( int fd_inotify ) {
    
    char buf[512] __attribute__ ((aligned(__alignof__(struct inotify_event))));
    ssize_t len;
    char *ptr;
    const struct inotify_event *event;
    int ret = 0;
    
    len = read(fd_inotify, buf, sizeof buf);

    if (len == -1 && errno != EAGAIN) {
        return 0;
    }
    
    if (len <= 0)
        return 0;
    
    for (ptr = buf; ptr < buf + len;
                    ptr += sizeof(struct inotify_event) + event->len) {

            event = (const struct inotify_event *) ptr;

            ret |= event->mask;
    }    
    
    return ret;
}

static int send_file ( int sock, const char *file ) {
    
    struct stat buf;
    int hFile;
    char *data = NULL;
    ssize_t data_size = 0;
    char header_data[512];
        
    if ( stat( file, &buf ) < 0 ) {
        LOG_ERROR_DETAILS("stat error [%s] : %s", strerror(errno), file );
        return -1;
    }
    
    if ( buf.st_size <= 0 ) {
        LOG_ERROR("fichier vide %s", file );
        return -1;
    }
    
    if ( (data = (char*) malloc(buf.st_size + 1)) == NULL ) {
        LOG_ERROR_DETAILS("malloc error [%s]", strerror(errno) );
        return -1;
    }
    
    if ( (hFile = open(file, O_RDONLY)) < 0 ) {
        free (data);
        LOG_ERROR_DETAILS("open error [%s]", strerror(errno) );
        return -1;
    }
    
    if ( (data_size = read( hFile, data, buf.st_size )) < 0 ) {
        free (data);
        close( hFile );
        LOG_ERROR_DETAILS("read error [%s]", strerror(errno) );
        return -1;
    }
    
    snprintf ( header_data, 511, "Content-Type: image/jpeg\r\n" \
            "Content-Length: %d\r\n\r\n", data_size );
    
    if ( write( sock, header_data, strlen(header_data) ) < 0 ) {
        LOG_ERROR_DETAILS("write error [%s]", strerror(errno) );
    }
    else if ( write( sock, data, data_size ) < 0 ) {
        LOG_ERROR_DETAILS("write error [%s]", strerror(errno) );
    }
    else {
        snprintf ( header_data, 511, "\r\n--myboundary\r\n" );
        if ( write( sock, header_data, strlen(header_data) ) < 0 ) {
            LOG_ERROR_DETAILS("write error [%s]", strerror(errno) );
        }
    }
    
    
    free (data);
    close( hFile );
    
    return -1;
}

int client_mjpeg ( int sock, char *args ) {
    
    char data[DATA_SIZE];
    char *sFile = NULL;
    struct pollfd fds[2];
    int fd_inotify = -1;
    int wd_inotify = -1;
    int nb_fds = 1;
    
    LOG_INFO("client_mjpeg ( %d, '%s' )", sock, args );

    if ( http_args_get_value( args, "file", data, DATA_SIZE-1) == NULL ) {
        LOG_ERROR("Parametre file inexistant");
        return -1;
    }
    
    /** strdupa */
    int data_size = strlen(data);
    sFile = alloca( data_size+1 );
    strncpy(sFile, data, data_size );
    sFile[data_size] = '\0';

    LOG_INFO("file=%s", sFile );
    
    // Enregistre les informations dans un fichier
    write_file ( sFile, "mjpeg", args );

    fd_inotify = inotify_init();
    if (fd_inotify == -1) {
        LOG_ERROR_DETAILS("inotify_init1 error [%s]", strerror(errno));
        goto _exit;
    }
        
    /** send header */
    snprintf(data, DATA_SIZE-1, 
            "HTTP/1.0 200 OK\r\n" \
            "Cache-Control: no-cache\r\n" \
            "Pragma: no-cache\r\n" \
            "Expires: Thu, 01 Dec 1994 16:00:00 GMT\r\n" \
            "Connection: close\r\n" \
            "Content-Type: multipart/x-mixed-replace; boundary=myboundary\r\n\r\n" \
            "--myboundary\r\n");

    if ( write( sock, data, strlen(data)) < 0 ) {
        LOG_ERROR_DETAILS("write error [%s]", strerror(errno) );
        goto _exit;
    }
    
    /** si le fichier existe deja */
    if ( is_file_existe ( sFile ) ) {

        wd_inotify = inotify_add_watch(fd_inotify, sFile, INOTIFY_FLAGS );

        if (wd_inotify == -1) {
            LOG_ERROR_DETAILS("inotify_add_watch error [%s] : %s", strerror(errno), sFile );
            goto _exit;
        }

        LOG_DEBUG("File %s create", sFile );
        send_file ( sock, sFile );

        nb_fds = 2;
    }
        
    fds[0].fd = sock;
    fds[0].events = POLLIN;
    
    fds[1].fd = fd_inotify;
    fds[1].events = POLLIN;
    
    while (1) {
        
        int poll_num = poll(fds, nb_fds, 2000);

        if (poll_num == -1) {
            if (errno == EINTR)
                    continue;
            LOG_ERROR_DETAILS("pool error [%s] : %s", strerror(errno) );
            goto _exit;
        }
        else if (poll_num > 0) {

            if (fds[0].revents & POLLIN) {

                char buffer[64];
                int size = recv(sock, buffer, 64, 0);
                
                if ( size > 0 ) {
                }
                else if ( size < 0 ) {
                    LOG_ERROR_DETAILS("[%03d] recv error [%s]", sock, strerror(errno));
                    goto _exit;
                }
                else {
                    LOG_DEBUG("[%03d] fin de connexion", sock );
                    goto _exit;
                }
            }
            
            if (fds[1].revents & POLLIN) {

                int ret_inotify = is_file_change ( fd_inotify );
                
                /* Inotify events are available */
                if ( ret_inotify & (IN_CLOSE_WRITE | IN_MOVED_FROM) ) {
                    
                    LOG_DEBUG("File %s change", sFile );
                    send_file ( sock, sFile );
                }
                else if ( ret_inotify & IN_DELETE_SELF ) {
                    
                    LOG_DEBUG("File %s deleted", sFile );
                    if ( wd_inotify > 0 )
                        inotify_rm_watch( fd_inotify, wd_inotify );
                    
                    wd_inotify = -1;
                }
            }
        }
        
        if ( wd_inotify < 0 && is_file_existe ( sFile ) ) {
            
            wd_inotify = inotify_add_watch(fd_inotify, sFile, INOTIFY_FLAGS );

            if (wd_inotify == -1) {
                LOG_ERROR_DETAILS("inotify_add_watch error [%s] : %s", strerror(errno), sFile );
                goto _exit;
            }

            LOG_DEBUG("File %s create", sFile );
            send_file ( sock, sFile );
            
            nb_fds = 2;
        }
        
        write_file ( sFile, "mjpeg", args );
    }

_exit:
    
    if ( wd_inotify > 0 )
        inotify_rm_watch( fd_inotify, wd_inotify );
    
    if ( fd_inotify > 0 )
        close(fd_inotify);
    
    remove_file( sFile, "mjpeg" );
    
    return 0;
}
