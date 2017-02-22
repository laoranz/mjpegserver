/* 
 * File:   log.h
 * Author: Le Borgne
 *
 * Created on 17 décembre 2016, 15:20
 */


#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <libgen.h>
#include <alloca.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#define LOG_BUFFER_SIZE 512

#ifndef LOG_FIFO
# define LOG_FIFO        "/var/log/logger"
#endif

char *log_string[] = {"NONE", "ERROR", "WARN", "INFO", "DEBUG"};
static uint8_t log_level = LOG_INFO_LEVEL;
static struct sigaction new_sa, old_sa;
static int log_fifo = -1;
static int log_fifo_create = 1;

static void signalSIGPIPE(int signo) {
    close(log_fifo);
    log_fifo = -1;
}

/**
 * Affichage d'un message
 * @param level niveau de log
 * @param format parametres variables
 * @param ...
 */
void log_msg ( uint8_t level, const char* format, ...) {
    
    va_list args;
    va_start (args, format);    
    struct timeval tv;
    struct tm * ptm;
    char s_date[64];
    FILE *dst = stdout;
    char buffer[LOG_BUFFER_SIZE];
    int buffer_size = 0;

    
    if ( !level )
        return;

    if ( level > log_level )
        return;
    
    gettimeofday(&tv, 0);
    ptm = localtime(&tv.tv_sec);
    
    snprintf(s_date, sizeof (s_date), "%02d/%02d %02d:%02d:%02d.%03d",
            ptm->tm_mday, ptm->tm_mon + 1,
            ptm->tm_hour, ptm->tm_min, ptm->tm_sec, (int)(tv.tv_usec / 1000));
    
    if ( level == 1 ) {
        dst = stderr;
    }
    
    buffer_size += snprintf ( buffer+buffer_size, LOG_BUFFER_SIZE-buffer_size-1, "%s | ", s_date ); 
    buffer_size += snprintf ( buffer+buffer_size, LOG_BUFFER_SIZE-buffer_size-1, "%-5s | ", log_string[level] );    
    buffer_size += vsnprintf ( buffer+buffer_size, LOG_BUFFER_SIZE-buffer_size-1, format, args);
    buffer_size += snprintf ( buffer+buffer_size, LOG_BUFFER_SIZE-buffer_size-1, "\n" );
    buffer[buffer_size] = '\0';
    
    va_end (args);
    
    fprintf  (dst, "%s", buffer );    
    
    if ( log_fifo < 0 ) {
        
        if ( log_fifo_create ) {
            
            struct stat s_stat;

            log_fifo_create = 0;
            if ( stat( LOG_FIFO, &s_stat ) != 0 ) {

                if ( mkfifo( LOG_FIFO, 0777) != 0 ) {
                    fprintf ( stderr, "%s:%d mkfifo error [%s] %s\n", __func__, __LINE__, strerror(errno), LOG_FIFO );
                    log_fifo_create = 1;
                }
            }
        }
        
        if ( (log_fifo = open(LOG_FIFO, O_WRONLY | O_NONBLOCK)) == -1 ) {
            //fprintf ( stderr, "%s:%d open error [%s] %s\n", __func__, __LINE__, strerror(errno), LOG_FIFO );
        }
        else if ( new_sa.sa_handler != signalSIGPIPE ) {
            
            new_sa.sa_handler = signalSIGPIPE;
            new_sa.sa_flags = 0;
            sigemptyset(&new_sa.sa_mask);
            sigaction(SIGPIPE, &new_sa, &old_sa);
        }
    }
    
    if ( log_fifo > 0) {
        
        if ( write( log_fifo, buffer, buffer_size) == 0 ) {
            fprintf ( stderr, "%s:%d write error [%s]\n", __func__, __LINE__, strerror(errno) );
            close(log_fifo);
            log_fifo = -1;
        }
    }
    
}

/**
 * Affiche un log détaillé
 * @param level niveau du log
 * @param file nom du fichier
 * @param func function
 * @param line ligne
 * @param format parametres variables
 * @param ...
 */
void log_details ( 
        uint8_t level, 
        const char *file, 
        const int line, 
        const char* format, ...) {
    
    char buffer[LOG_BUFFER_SIZE];
    size_t file_size = strlen(file);
    char *pfile = alloca ( file_size );
    va_list args;
    va_start (args, format);
    vsnprintf (buffer,LOG_BUFFER_SIZE,format, args);
    va_end (args);
    
    if ( pfile!= NULL ) {
        strncpy ( pfile, file, file_size );
    }
    
    log_msg ( level, "%s:%d %s", basename(pfile), line, buffer );
}

/**
 * Defini le niveau d'affichage du log
 * @param level
 */
void log_set_level ( uint8_t level ) {
    log_level = level;
}


