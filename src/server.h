/* 
 * File:   server.h
 * Author: Le Borgne
 *
 * Created on 9 février 2017, 22:20
 */

#ifndef SERVER_H
#define SERVER_H

/**
 * Lancement du serveur http
 * @param port port d'ecoute
 * @return 0 serveur en ecoute
 */
int mjpegserver_server_start ( int port );

int mjpegserver_server_run ( void );

void mjpegserver_server_stop ( void );

#endif /* SERVER_H */
