/* 
 * File:   clients.c
 * Author: Le Borgne
 * 
 * Created on 10 février 2017, 21:37
 */

#include <string.h>

#include "clients.h"
#include "log.h"
#include "client_mjpeg.h"

struct {
    char *name;
    client_handler_t handler;
} m_clients[] = {
    {"mjpeg", client_mjpeg},
    {"rtsp", NULL},
    {NULL, NULL}
};

client_handler_t clients_find_client ( char *name ) {
    
    int i = 0;
    
    if ( name == NULL )
        return NULL;
    
    for ( i=0; m_clients[i].name!=NULL; i++ ) {
        
        if ( strcmp( name, m_clients[i].name) == 0 ) {
            return m_clients[i].handler;
        }
    }
    
    return NULL;
}
