/* 
 * File:   mjpegserver.c
 * Author: Le Borgne
 *
 * Created on 9 février 2017, 21:35
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#include "log.h"
#include "server.h"

static int stopmain = 0;

static void signalHandler(int signo) {
    fprintf ( stderr, "\n<Ctrl-C>\n");
    stopmain = 1;
}

static void usage ( char *name ) {
    
    fprintf(stderr, "Usage: %s [-p PORTS] [-v VERBOSE]\n", name);
    exit(EXIT_FAILURE);
}

/*
 * 
 */
int main(int argc, char** argv) {

    int opt;
    int port = 8888;
    
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGUSR1, signalHandler);

    while ((opt = getopt(argc, argv, "v:p:")) != -1) {
        switch (opt) {
            case 'v':
                log_set_level ( atoi(optarg) );
                break;
            case 'p':
                port = atoi(optarg);
                break;
            default: /* '?' */
                usage( argv[0] );
        }
    }
    
    LOG_INFO ("mjpegserver v%s", "0.0.1");

    if ( port <= 0 )
        usage( argv[0] );

    mjpegserver_server_start( port );
    
    for (;;) {
        
        mjpegserver_server_run();
        
        if ( stopmain )
            break;
    }
    
    mjpegserver_server_stop();
    
    return (EXIT_SUCCESS);
}

