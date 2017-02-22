/* 
 * File:   clients.h
 * Author: Le Borgne
 *
 * Created on 10 février 2017, 21:37
 */

#ifndef CLIENTS_H
#define CLIENTS_H

typedef int (*client_handler_t) ( int, char * );

client_handler_t clients_find_client ( char *name );

#endif /* CLIENTS_H */
