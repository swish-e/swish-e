/* httpserver.h
**/

#ifndef __HTTPSERVER_H
#define __HTTPSERVER_H

typedef struct httpserverinfo {
    char *baseurl;
    
    time_t lastretrieval;
    
    char *useragent;
    struct robotrules *robotrules;
    
    struct httpserverinfo *next;
} httpserverinfo;

typedef struct robotrules {
    char *disallow;
    struct robotrules *next;
} robotrules;


/*
** use _AP() for easier cross-compiler (non-ANSI) porting 
** <return value> <functionname> _AP( (<arg prototypes>) );
*/

httpserverinfo *getserverinfo _AP((SWISH *sw, char *url));
int urldisallowed _AP((SWISH *sw, char *url));
int equivalentserver _AP((SWISH *sw, char *url, char *baseurl));


#endif

