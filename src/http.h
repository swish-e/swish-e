/* http.h

$Id$


    This file is part of Swish-e.

    Swish-e is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Swish-e is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along  with Swish-e; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    
    See the COPYING file that accompanies the Swish-e distribution for details
    of the GNU GPL and the special exception available for linking against
    the Swish-e library.
    
** Mon May  9 18:19:34 CDT 2005
** added GPL


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
int get(SWISH * sw, char *contenttype_or_redirect, time_t *last_modified, time_t * plastretrieval, char *file_prefix, char *url);
int cmdf (int (*cmd)(const char *), char *fmt, char *,pid_t pid);       // no rw64. This is passed unlink()
char *readline (FILE *fp);
pid_t lgetpid ();


#endif

