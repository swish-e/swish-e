/* http.h
**/

#ifndef __HasSeenModule_HTTP
#define __HasSeenModule_HTTP       1

#define MAXPIDLEN 32 /* 32 is for the pid identifier and the trailing null */

/*
   -- module data
*/

struct MOD_HTTP
{
        /* spider directory for index (HTTP method) */
    int     lenspiderdirectory;
    char   *spiderdirectory;

        /* http system specific configuration parameters */
    int     maxdepth;
    int     delay;
    struct multiswline *equivalentservers;

    struct url_info *url_hash[BIGHASHSIZE];
};

void initModule_HTTP (SWISH *);
void freeModule_HTTP (SWISH *);
int  configModule_HTTP (SWISH *, StringList *);


char *url_method ( char *url, int *plen );
char *url_serverport (char *url, int *plen);
char *url_uri (char *url, int *plen);
int get (SWISH *sw, char *contenttype_or_redirect, time_t *plastretrieval, char *url);
int cmdf (int (*cmd)(const char *), char *fmt, char *,pid_t pid);
char *readline (FILE *fp);
pid_t lgetpid ();


#endif

