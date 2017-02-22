/* 
 * File:   log.h
 * Author: Le Borgne
 *
 * Created on 17 décembre 2016, 15:20
 */

#ifndef LOG_H
#define	LOG_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <stdint.h>
#include <errno.h>
#include <string.h>
    
#define LOG_ERROR_DETAILS(...) log_details(LOG_ERROR_LEVEL,__FILE__,__LINE__,__VA_ARGS__)
#define LOG_ERROR(...) log_msg(LOG_ERROR_LEVEL,__VA_ARGS__)
#define LOG_WARN(...) log_msg(LOG_WARN_LEVEL,__VA_ARGS__)
#define LOG_INFO(...) log_msg(LOG_INFO_LEVEL,__VA_ARGS__)
#define LOG_DEBUG(...) log_msg( LOG_DEBUG_LEVEL, __VA_ARGS__ )
    
    enum {
        LOG_NONE_LEVEL,       // ..
        LOG_ERROR_LEVEL,      // Message d'erreur
        LOG_WARN_LEVEL,       // message de mise en garde
        LOG_INFO_LEVEL,       // message d'information
        LOG_DEBUG_LEVEL       // message de debug
    };

    /**
     * Affichage d'un message
     * @param level niveau de log
     * @param format parametres variables
     * @param ...
     */
    void log_msg ( uint8_t level, const char* format, ...);

    /**
     * Affiche un log détaillé
     * @param level niveau du log
     * @param file nom du fichier
     * @param func function
     * @param line ligne
     * @param format parametres variables
     * @param ...
     */
    void log_details ( uint8_t level, const char *file, const int line, const char* format, ...);

    /**
     * Defini le niveau d'affichage du log
     * @param level
     */
    void log_set_level ( uint8_t level );
    
#ifdef	__cplusplus
}
#endif

#endif	/* LOG_H */

