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



httpserverinfo *getserverinfo (SWISH *sw, char *url);
int urldisallowed (SWISH *sw, char *url);
int equivalentserver (SWISH *sw, char *url, char *baseurl);


#endif

