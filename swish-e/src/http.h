/* http.h
**/

#ifndef __HTTP_H
#define __HTTP_H

#define MAXPIDLEN 32 /* 32 is for the pid identifier and the trailing null */


char *url_method ( char *url, int *plen );
char *url_serverport (char *url, int *plen);
char *url_uri (char *url, int *plen);
int get (SWISH *sw, char *contenttype_or_redirect, time_t *plastretrieval, char *url);
int cmdf (int (*cmd)(const char *), char *fmt, char *,pid_t pid);
char *readline (FILE *fp);
pid_t lgetpid ();


#endif

