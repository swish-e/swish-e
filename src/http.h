/* http.h
**/

#ifndef __HTTP_H
#define __HTTP_H

#define MAXPIDLEN 32 /* 32 is for the pid identifier and the trailing null */

/*
** use _AP() for easier cross-compiler (non-ANSI) porting 
** <return value> <functionname> _AP( (<arg prototypes>) );
*/

char *url_method _AP(( char *url, int *plen ));
char *url_serverport _AP((char *url, int *plen));
char *url_uri _AP((char *url, int *plen));
int get _AP((SWISH *sw, char *contenttype_or_redirect, time_t *plastretrieval, char *url));
int cmdf _AP((int (*cmd)(const char *), char *fmt, char *,pid_t pid));
char *readline _AP((FILE *fp));
pid_t lgetpid _AP(());


#endif

