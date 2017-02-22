/* 
 * File:   http_tools.c
 * Author: Le Borgne
 * 
 * Created on 10 février 2017, 23:11
 */

#include "http_tools.h"
#include <string.h>
#include <stdio.h>

char *http_args_get_value ( const char *args, const char *parameter, char *data, int data_size ) {
    
    const char *p = args;
    const char *s = NULL;
    
    if ( (p = strstr( p , parameter)) == NULL )
        return NULL;
    
    p+=strlen(parameter);
    if ( *p != '=' ) {
        return NULL;
    }
    
    s = p+1;
    
    size_t l;
    if ( (l = strcspn(s, "& ")) == 0 ) 
        return NULL;
    
    if ( l < data_size )
        data_size = l;
    
    strncpy ( data, s, data_size );
    data[data_size] = '\0';
    
    return data;
}

