/*
** Copyright (C) 1995, 1996, 1997, 1998 Hewlett-Packard Company
** Originally by Kevin Hughes, kev@kevcom.com, 3/11/94
**
** This program and library is free software; you can redistribute it and/or
** modify it under the terms of the GNU (Library) General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU (Library) General Public License for more details.
**
** You should have received a copy of the GNU (Library) General Public License
**  long with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
**--------------------------------------------------------------------
** All the code in this file added by Ron Klachko ron@ckm.ucsf.edu 9/98
** 
** change sprintf to snprintf to avoid corruption,
** test length of spiderdirectory before strcat to avoid corruption,
** added safestrcpy() macro to avoid corruption from strcpy overflow,
** define MAXPIDLEN instead of literal "32" - assumed return length from lgetpid()
** SRE 11/17/99
**
** added buffer size arg to grabStringValue - core dumping from overrun
** SRE 2/22/00
**
** 2000-11   jruiz,rasc  some redesign
*/

/*
** http.c
*/

#ifdef HAVE_CONFIG_H
#include "acconfig.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_PROCESS_H
#include <process.h>
#endif

#include <time.h>
#include <stdarg.h>

// for wait
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include "swish.h"
#include "mem.h"
#include "swstring.h"
#include "index.h"
#include "hash.h"
#include "file.h"
#include "check.h"
#include "error.h"
#include "list.h"

#include "http.h"
#include "httpserver.h"

#include "xml.h"
#include "txt.h"
#include "html.h"

#include "filter.h"
/*
  -- init structures for this module
*/

void    initModule_HTTP(SWISH * sw)
{
    struct MOD_HTTP *http;
    int     i;

    char *execdir = get_libexec();

    http = (struct MOD_HTTP *) emalloc(sizeof(struct MOD_HTTP));

    sw->HTTP = http;

    http->lenspiderdirectory = strlen(execdir); 
    http->spiderdirectory = (char *) emalloc(http->lenspiderdirectory + 1);
    strcpy( http->spiderdirectory, execdir );
    efree( execdir );

    for (i = 0; i < BIGHASHSIZE; i++)
        http->url_hash[i] = NULL;

    http->equivalentservers = NULL;

    /* http default system parameters */
    http->maxdepth = 0;
    http->delay = DEFAULT_HTTP_DELAY;
}

void    freeModule_HTTP(SWISH * sw)
{
    struct MOD_HTTP *http = sw->HTTP;

    if (http->spiderdirectory)
        efree(http->spiderdirectory);
    efree(http);
    sw->HTTP = NULL;
}

int     configModule_HTTP(SWISH * sw, StringList * sl)
{
    struct MOD_HTTP *http = sw->HTTP;
    char   *w0 = sl->word[0];
    int     retval = 1;

    int     i;
    struct multiswline *list;
    struct swline *slist;

    if (strcasecmp(w0, "maxdepth") == 0)
    {
        if (sl->n == 2)
        {
            retval = 1;
            http->maxdepth = atoi(sl->word[1]);
        }
        else
            progerr("MaxDepth requires one value");
    }
    else if (strcasecmp(w0, "delay") == 0)
    {
        if (sl->n == 2)
        {
            retval = 1;
            http->delay = atoi(sl->word[1]);
        }
        else
            progerr("Delay requires one value");
    }
    else if (strcasecmp(w0, "spiderdirectory") == 0)
    {
        if (sl->n == 2)
        {
            retval = 1;
            http->spiderdirectory = erealloc( http->spiderdirectory, strlen(sl->word[1])+2);
            strcpy( http->spiderdirectory, sl->word[1] );
            normalize_path( http->spiderdirectory );
            

            if (!isdirectory(http->spiderdirectory))
            {
                progerr("SpiderDirectory. %s is not a directory", http->spiderdirectory);
            }
        }
        else
            progerr("SpiderDirectory requires one value");
    }
    else if (strcasecmp(w0, "equivalentserver") == 0)
    {
        if (sl->n > 1)
        {
            retval = 1;
            /* Add a new list of equivalent servers */
            list = (struct multiswline *) emalloc(sizeof(struct multiswline));

            list->next = http->equivalentservers;
            list->list = 0;
            http->equivalentservers = list;

            for (i = 1; i < sl->n; i++)
            {
                /* Add a new entry to this list */
                slist = newswline(sl->word[i]);
                slist->next = list->list;
                list->list = slist;
            }

        }
        else
            progerr("EquivalentServers requires at least one value");
    }
    else
    {
        retval = 0;
    }

    return retval;
}
typedef struct urldepth
{
    char   *url;
    int     depth;
    struct urldepth *next;
}
urldepth;


int     http_already_indexed(SWISH * sw, char *url);
urldepth *add_url(SWISH * sw, urldepth * list, char *url, int depth, char *baseurl);


urldepth *add_url(SWISH * sw, urldepth * list, char *url, int depth, char *baseurl)
{
    urldepth *item;
    struct MOD_HTTP *http = sw->HTTP;


    if (!equivalentserver(sw, url, baseurl))
    {
        if (sw->verbose >= 3)
            printf("Skipping %s:  %s\n", url, "Wrong method or server.");


    }
    else if (http->maxdepth && (depth >= http->maxdepth))
    {
        if (sw->verbose >= 3)
            printf("Skipping %s:  %s\n", url, "Too deep.");
    }
    else if (sw->nocontentslist && isoksuffix(url, sw->nocontentslist))
    {
        if (sw->verbose >= 3)
            printf("Skipping %s: %s\n", url, "Wrong suffix.");

    }
    else if (urldisallowed(sw, url))
    {
        if (sw->verbose >= 3)
            printf("Skipping %s:  %s\n", url, "URL disallowed by robots.txt.");
    }
    else if (!http_already_indexed(sw, url))
    {
        item = (urldepth *) emalloc(sizeof(urldepth));
        item->url = estrdup(url);
        item->depth = depth;
#if 0
        /* Depth first searching
           * */
        item->next = list;
        list = item;
#else
        /* Breadth first searching
           * */
        item->next = 0;
        if (!list)
        {
            list = item;
        }
        else
        {
            urldepth *walk;

            for (walk = list; walk->next; walk = walk->next)
            {
            }
            walk->next = item;
        }
#endif
    }

    return list;
}


/* Have we already indexed a file or directory?
** This function is used to avoid multiple index entries
** or endless looping due to symbolic links.
*/

int     http_already_indexed(SWISH * sw, char *url)
{
    struct url_info *p;

    int     len;
    unsigned hashval;
    struct MOD_HTTP *http = sw->HTTP;

    /* Hash with via the uri alone.  Depending on the equivalent
       ** servers, we may or may not make the decision of the entire
       ** url or just the uri.
     */
    hashval = bighash(url_uri(url, &len)); /* Search hash for this file. */
    for (p = http->url_hash[hashval]; p != NULL; p = p->next)
        if ((strcmp(url, p->url) == 0) || (equivalentserver(sw, url, p->url) && (strcmp(url_uri(url, &len), url_uri(p->url, &len)) == 0)))
        {                       /* We found it. */
            if (sw->verbose >= 3)
                printf("Skipping %s:  %s\n", url, "Link already processed.");
            return 1;
        }

    /* Not found, make new entry. */
    p = (struct url_info *) emalloc(sizeof(struct url_info));

    p->url = estrdup(url);
    p->next = http->url_hash[hashval];
    http->url_hash[hashval] = p;

    return 0;
}


char   *url_method(char *url, int *plen)
{
    char   *end;

    if ((end = strstr(url, "://")) == NULL)
    {
        return NULL;
    }
    *plen = end - url;
    return url;
}


char   *url_serverport(char *url, int *plen)
{
    int     methodlen;
    char   *serverstart;
    char   *serverend;

    if (url_method(url, &methodlen) == NULL)
    {
        return NULL;
    }

    /* +3 for 
       * */
    serverstart = url + methodlen + 3;
    if ((serverend = strchr(serverstart, '/')) == NULL)
    {
        *plen = strlen(serverstart);
    }
    else
    {
        *plen = serverend - serverstart;
    }

    return serverstart;
}


char   *url_uri(char *url, int *plen)
{
    if ((url = url_serverport(url, plen)) == 0)
    {
        return 0;
    }
    url += *plen;
    *plen = strlen(url);
    return url;
}
/************************************************************
*
* Fork and exec a program, and wait for child to exit.
* Returns
*
*************************************************************/
#ifdef HAVE_WORKING_FORK
static void run_program(char* prog, char** args)
{
    pid_t pid = fork();
    int   status;

    /* In parent, wait for child */
#ifdef HAVE_SYS_WAIT_H
    if ( pid )
    {
        wait( &status );
        if ( WIFEXITED( status ) ) // exited normally if non-zero
            return;

        progerr("%s exited with non-zero status (%d)", prog, WEXITSTATUS(status) );
    }
#endif /* HAVE_SYS_WAIT_H */

    execvp (prog, args);
    progerrno("Failed to fork '%s'. Error: ", prog );
}
#endif

/************************************************************
*
* Fetch a URL
* Side effect that it appends to "response_file"
*  -- lazy programmer hoping that -S http will go away...
*
*  Under Windows system() is used to call "perl"
*  Otherwise, exec is called on the swishspider program
*
*************************************************************/

int get(SWISH * sw, char *contenttype_or_redirect, time_t *last_modified, time_t * plastretrieval, char *file_prefix, char *url)
{
    int     code = 500;
    FILE   *fp;
    struct MOD_HTTP *http = sw->HTTP;

    /* Build path to swishspider program */
    char   *spider_prog = emalloc( strlen(http->spiderdirectory) + strlen("swishspider+fill") );
    sprintf(spider_prog, "%s/swishspider", http->spiderdirectory ); // note that spiderdir MUST be set.  

    /* Sleep a little so we don't overwhelm the server */
    if (  *plastretrieval && (time(0) - *plastretrieval) < http->delay)
    {
        int     num_sec = http->delay - (time(0) - *plastretrieval);
        if ( sw->verbose >= 3 )
            printf("sleeping %d seconds before fetching %s\n", num_sec, url);
        sleep(num_sec);
    }

    *plastretrieval = time(0);

    if ( sw->verbose >= 3 )
        printf("Now fetching [%s]...", url );

    
#ifndef HAVE_WORKING_FORK
    /* Should be in autoconf or obsoleted by extprog. - DLN 2001-11-05  */
    {
        int     retval;
        char    commandline[] = "perl \"%s\" \"%s\" \"%s\"";
        char   *command = emalloc( strlen(commandline) + strlen(spider_prog) + strlen(file_prefix) + strlen(url) + 1 );

        sprintf(command, commandline, spider_prog, file_prefix, url);

        retval = system( command );
        efree( command );
        efree( spider_prog );

        if ( retval )
            return 500;
    }
#else
    {
        char *args[4];

        args[0] = spider_prog;
        args[1] = file_prefix;
        args[2] = url;
        args[3] = NULL;
        run_program( spider_prog, args );
        efree( spider_prog );
    }
#endif

    /* Probably better to have Delay be time between requests since some docs may take more than Delay seconds to fetch */
    *plastretrieval = time(0);
    

    /* NAUGHTY SIDE EFFECT */
    strcat( file_prefix, ".response" );
    
    if ( !(fp = fopen(file_prefix, F_READ_TEXT)) )
    {
        progerrno("Failed to open file '%s': ", file_prefix );
    }
    else
    {
        char buffer[500];
    
        fgets(buffer, 400, fp);
        code = atoi(buffer);
        if ((code == 200) || ((code / 100) == 3))
        {
            /* read content-type  redirect */
            fgets(contenttype_or_redirect, MAXSTRLEN, fp);  /* more yuck */
            *(contenttype_or_redirect + strlen(contenttype_or_redirect) - 1) = '\0';
        }


        if (code == 200)
        {
            /* read last-mod time */
            fgets(buffer, 400, fp);  /* more yuck */
            *last_modified = (time_t)strtol(buffer, NULL, 10);  // go away http.c -- no error checking
        }


        fclose(fp);
    }

    if ( sw->verbose >= 3 ) 
        printf("Status: %d. %s\n", code, contenttype_or_redirect );

    return code;
}

int     cmdf(int (*cmd) (const char *), char *fmt, char *string, pid_t pid)
{
    int     rc;
    char   *buffer;

    buffer = emalloc(strlen(fmt) + strlen(string) + sizeof(pid_t) * 8 + 1);

    sprintf(buffer, fmt, string, pid);

    rc = cmd(buffer);
    efree(buffer);
    return rc;
}

char   *readline(FILE * fp)
{
    static char *buffer = 0;
    static int buffersize = 512;

    if (buffer == 0)
    {
        buffer = (char *) emalloc(buffersize);
    }
    /*
       *Try to read in the line
     */

    if (fgets(buffer, buffersize, fp) == NULL)
    {
        return NULL;
    }

    /*
       * Make sure we read the entire line.  If not, double the buffer
       * size and try to read the rest
     */
    while (buffer[strlen(buffer) - 1] != '\n')
    {
        buffer = (char *) erealloc(buffer, buffersize * 2);

        /*
           * The easiest way to verify that this line is okay is to consider
           * the situation where the buffer is 2 bytes longs.  Since fgets()
           * always guarantees to put the trailing NULL, it will have essentially
           * used only 1 bytes.  We double it to four, so we now have the left
           * over byte (that currently contains NULL) in addition to the doubling
           * which gets us to read buffersize + 1.
         */
        if (fgets(buffer + buffersize - 1, buffersize + 1, fp) == 0)
        {
            break;
        }
        buffersize *= 2;
    }

    return buffer;
}


/* A local version of getpid() so that we don't have to suffer
** a system call each time we need it.
*/
pid_t   lgetpid()
{
    static pid_t pid = -1;

    if (pid == -1)
    {
        pid = getpid();
    }
    return pid;
}

#if 0

/* Testing the robot rules parsing code...
**/
void    http_indexpath(char *url)
{
    httpserverinfo *server = getserverinfo(url);
    robotrules *robotrule;

    printf("User-agent: %s\n", server->useragent ? server->useragent : "(none)");
    for (robotrule = server->robotrules; robotrule; robotrule = robotrule->next)
    {
        printf("Disallow: %s\n", robotrule->disallow);
    }
}

#else

/********************************************************/
/*					"Public" functions					*/
/********************************************************/

/* The main entry point for the module.  For fs.c, decides whether this
** is a file or directory and routes to the correct routine.
*/
void    http_indexpath(SWISH * sw, char *url)
{
    urldepth *urllist = 0;
    urldepth *item;
    static int lentitle = 0;
    static char *title = NULL;
    char   *tmptitle;
    static int lencontenttype = 0;
    static char *contenttype = NULL;
    int     code;
    time_t  last_modified = 0;

    httpserverinfo *server;
    char   *link;
    char   *p;
    FileProp *fprop;
    FILE   *fp;
    struct MOD_Index *idx = sw->Index;

    char   *file_prefix;  // prefix for use with files written by swishspider -- should just be on the stack!
    char   *file_suffix;  // where to copy the suffix


    /* Initialize buffers */


    file_prefix = emalloc( strlen(idx->tmpdir) + MAXPIDLEN + strlen("/swishspider@.contents+fill") );
    sprintf(file_prefix, "%s/swishspider@%ld", idx->tmpdir, (long) lgetpid());
    file_suffix = file_prefix + strlen( file_prefix );
    

    if (!lentitle)
        title = emalloc((lentitle = MAXSTRLEN) + 1);

    if (!lencontenttype)
        contenttype = emalloc((lencontenttype = MAXSTRLEN) + 1);



    /* prime the pump with the first url */
    urllist = add_url(sw, urllist, url, 0, url);



    /* retrieve each url and add urls to a certain depth */

    while (urllist)
    {
        item = urllist;
        urllist = urllist->next;

        if (sw->verbose >= 2)
        {
            printf("retrieving %s (%d)...\n", item->url, item->depth);
            fflush(stdout);
        }

        /* We don't check if this url is legal here, because we do that before adding to the list. */
        server = getserverinfo(sw, item->url);

        strcpy( file_suffix, "" );  // reset to just the prefix

        if ((code = get(sw, contenttype, &last_modified, &server->lastretrieval, file_prefix, item->url)) == 200)
        {
            FilterList *filter_list = hasfilter(sw, item->url);  /* check to see if there's a filter */
            
            /* Set the file_prefix to be the path to "contents" */
            strcpy( file_suffix, ".contents" );

            
            /* Patch from Steve van der Burg */
            /* change from strcmp to strncmp */


            /* Fetch title from doc if it's HTML */

            if (strncmp(contenttype, "text/html", 9) == 0)
                title = SafeStrCopy(title, (char *) (tmptitle = parseHTMLtitle(sw , file_prefix)), &lentitle);
            else
                if ((p = strrchr(item->url, '/')))
                    title = SafeStrCopy(title, p + 1, &lentitle);
                else
                    title = SafeStrCopy(title, item->url, &lentitle);


            /* Now index the file */

            /* What to do with non text files?? */
            /* This never worked correctly.  Used to set fprop->index_no_content if it wasn't a text type of file. */
            /* That forced indexing of only the path name for say a PDF file.  But although that also allowed files */
            /* to be processed by FileFilter filters, the index_no_content still forced indexing of only file names, */
            /* thus making the filters worthless.  But without the index_no_contents it would index all files, includeing binary files. */
            /* Two solutions: 1: set a flag that only should index the file if a filters is setup for it, or */
            /*                2: do filtering in swishspider.  That's a better option. */

            /* Nov 14, 2002 - well, do both */

            if ( filter_list || strncmp(contenttype, "text/", 5) == 0 )            {
                if (sw->verbose >= 4)
                    printf("Indexing %s:  Content type: %s. %s\n",
                        item->url, contenttype, filter_list ? "(filtered)" : "");
 
                
                fprop = file_properties(item->url, file_prefix, sw);
                fprop->mtime = last_modified;

                

                /* only index contents of text docs */
                // this would just index the path name
                // but also tossed away output from filters.
                // fprop->index_no_content = strncmp(contenttype, "text/", 5);

                do_index_file(sw, fprop);
                free_file_properties(fprop);
            }
            else if (sw->verbose >= 3)
                printf("Skipping %s:  Wrong content type: %s.\n", item->url, contenttype);
            



            /* add new links as extracted by the spider */

            if (strncmp(contenttype, "text/html", 9) == 0)
            {
                strcpy( file_suffix, ".links" );
            
                if ((fp = fopen(file_prefix, F_READ_TEXT)) != NULL)
                {
                    /* URLs can get quite large so don't depend on a fixed size buffer */
                
                    while ((link = readline(fp)) != NULL)
                    {
                        *(link + strlen(link) - 1) = '\0';
                        urllist = add_url(sw, urllist, link, item->depth + 1, url);
                    }
                    fclose(fp);
                }
            }

        }
        else if ((code / 100) == 3)
        {
            if ( *contenttype )
                urllist = add_url(sw, urllist, contenttype, item->depth, url);
            else
                if (sw->verbose >= 3)
                    printf("URL '%s' returned redirect code %d without a Location.\n", url, code);
        }



        /* Clean up the files left by swishspider */
        cmdf(unlink, "%s/swishspider@%ld.response", idx->tmpdir, lgetpid());
        cmdf(unlink, "%s/swishspider@%ld.contents", idx->tmpdir, lgetpid());
        cmdf(unlink, "%s/swishspider@%ld.links", idx->tmpdir, lgetpid());
    }
    efree(file_prefix);
}

#endif




struct _indexing_data_source_def HTTPIndexingDataSource = {
    "HTTP-Crawler",
    "http",
    http_indexpath,
    configModule_HTTP
};
