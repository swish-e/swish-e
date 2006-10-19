/*
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

** Mon May  9 15:51:39 CDT 2005
** added GPL

**
** 1998-07-04 rasc    original filter code
** 1999-08-07 rasc
** 2001-02-28 rasc    own module started for filters
**                    some functions rewritten and enhanced...
** 2001-04-09 rasc    options for filters  (%f, etc)
** 2001-05-31 rasc    fix for possible crashes (NULL checks)
**
*/

#include <string.h>
#include "swish.h"
#include "file.h"
#include "mem.h"
#include "swstring.h"
#include "error.h"
#include "filter.h"
#include <stdlib.h>

#ifdef HAVE_WORKING_FORK
#include <sys/types.h>
#include <unistd.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */
#endif /* HAVE_WORKING_FORK */





/* private module prototypes */

static FilterList *addfilter(FilterList *rp, char *FilterSuffix, char *FilterProg, char *options, char *FilterDir, char **regex);
static char *expand_options( FileProp * fprop, char * template );
static char *join_string( char **string_list );
static char *expand_percent(  FileProp * fprop, char escape_code );
#ifdef HAVE_WORKING_FORK
static FILE *fork_program( char **arg );
#endif

/*
  -- init structures for filters
*/

void    initModule_Filter(SWISH * sw)
 {
    struct MOD_Filter *md;

    md = (struct MOD_Filter *) emalloc(sizeof(struct MOD_Filter));

    memset( md, 0, sizeof( struct MOD_Filter ) );

    sw->Filter = md;
    return;
}


/*
  -- release structures for filters
  -- release all wired memory
  -- 2001-04-09 rasc
*/

void    freeModule_Filter(SWISH * sw)
 {
    struct MOD_Filter *md = sw->Filter;




    if (md->filterdir)
        efree(md->filterdir);   /* free FilterDir */


    /* Free the FileFilterMatch selections */
    if ( md->filterlist )
    {
        FilterList *fm = md->filterlist;
        FilterList *fm2;

        while( fm )
        {
            efree( fm->prog );
            free_regex_list( &fm->regex );

            if ( fm->options )
                freeStringList( fm->options );

            if ( fm->suffix )
                efree( fm->suffix );


            fm2 = fm;
            fm = fm->next;
            efree( fm2 );
        }

        md->filterlist = NULL;
    }


    efree(sw->Filter);          /* free modul data structure */
    sw->Filter = NULL;
    return;
}




/*
 -- Config Directives
 -- Configuration directives for this Module
 -- return: 0/1 = none/config applied
*/

int     configModule_Filter(SWISH * sw, StringList * sl)
 {
    struct MOD_Filter *md = sw->Filter;
    char   *w0 = sl->word[0];

    if (strcasecmp(w0, "FilterDir") == 0) { /* 1999-05-05 rasc */
        if (sl->n == 2) {
            md->filterdir = estrredup(md->filterdir, sl->word[1]);
            normalize_path( md->filterdir );

            if (!isdirectory(md->filterdir)) {
                progerr("%s: %s is not a directory", w0, md->filterdir);
            }
        } else
            progerr("%s: requires one value", w0);

        return 1;
    }


    if (strcasecmp(w0, "FileFilter") == 0) { /* 1999-05-05 rasc */
        /* FileFilter fileextension  filterprog  [options] */
        if (sl->n == 3 || sl->n == 4)
            md->filterlist = addfilter(md->filterlist, sl->word[1], sl->word[2], sl->word[3], md->filterdir, NULL);
        else
            progerr("%s: requires \"extension\" \"filter\" \"[options]\"", w0);

        return 1;
    }


    /* added March 16, 2002 - moseley */

    if ( strcasecmp( w0, "FileFilterMatch") == 0 )
    {
        if ( sl->n < 4 )
            progerr("%s requires at least three parameters: 'filterprog' 'options' 'regexp' ['regexp'...]\n");


        md->filterlist = addfilter(md->filterlist, NULL, sl->word[1], sl->word[2], md->filterdir, &(sl->word[3]) );

        return 1;
    }

    return 0;                   /* not a filter directive */
}




/*
 -- Add a filter to the filterlist (file ext -> filterprog [cmd-options])
 -- (filterdir may be NULL)
 -- 1999-08-07 rasc
 -- 2001-02-28 rasc
 -- 2001-04-09 rasc   options, maybe NULL
*/


static FilterList *addfilter(FilterList *rp, char *suffix, char *prog, char *options, char *filterdir, char **regex )
{
    FilterList *newnode;
    char   *buf;

    normalize_path( prog );  /* Don't really see how this is right */



    newnode = (FilterList *) emalloc(sizeof(FilterList));
    memset( newnode, 0, sizeof( FilterList ) );

    newnode->suffix = (char *) estrdup(suffix);

    /* Parse the filter options into tokens */
    newnode->options = parse_line( options ? options : "%p %P" );

    /* If this is a FileFilterMatch then add patterns */
    if ( regex )
        add_regex_patterns( prog, &newnode->regex, regex, 1 );


    /* Append the directory on */
    if ( filterdir && (*prog != '/' ) )
    {
        buf = emalloc( strlen( filterdir ) + strlen( prog ) + 2 );
        *buf = '\0';
        strcat( buf, filterdir );
        strcat( buf, "/" );
        strcat( buf, prog );
        newnode->prog = buf;
    }
    else
        newnode->prog = estrdup( prog );


    newnode->next = NULL;

    if (rp == NULL)
        rp = newnode;
    else
    {
        /* add to end of list */
        FilterList *f = rp;
        while ( f->next )
            f = f->next;

        f->next = (struct FilterList *)newnode;
    }
    return rp;
}



/*
 -- Check, if a filter is needed to retrieve information from a file
 -- Returns NULL or path to filter prog according conf file.
 -- 1999-08-07 rasc
 -- 2001-02-28 rasc  rewritten, now possible: search for ".pdf.gz", etc.
 -- 3002-05-04 rasc  adapted to new module design (wow, and way into the future!)
 -- 2002-03-16 moseley added regexp check (other code not reviewed)
*/

FilterList *hasfilter(SWISH * sw, char *filename)
{
    struct MOD_Filter *md = sw->Filter;
    FilterList *fl;
    char   *s,
           *fe;

    fl = md->filterlist;

    if (!fl)
        return (FilterList *) NULL;


    fe = (filename + strlen(filename));

    while (fl != NULL) {

        /* added regex check - moseley */
        if ( fl->regex )
        {
            if ( match_regex_list( filename, fl->regex, "Filter match" ) )
                return fl;
        }
        else
        {
            s = fe - strlen(fl->suffix);
            if (s >= filename) {    /* no negative overflow! */
                if (!strcasecmp(fl->suffix, s)) {
                    return fl;
                }
            }
        }
        fl = fl->next;
    }

    return (FilterList *) NULL;
}



/*
  -- open filter (in: file,  out: FILE *)
  -- params are in (FileProp *) - but should be adapted later
  -- Return: FILE *  (filter stream) or NULL
*/

FILE   *FilterOpen(FileProp * fprop)
{
    FilterList      *fi         = fprop->hasfilter;
    char            **options   = fi->options->word;
    int             num_options = fi->options->n;
    char            **arg;      /* where to store expanded arguments */
    int             arg_size;
    FILE            *fp;        /* stream to return */
#ifndef HAVE_WORKING_FORK
    char            *command;   /* command string used with popen */
#endif
    int             n;




    /* Create second argument list that gets updated during each run */
    arg_size = ( num_options + 2 ) * sizeof(char*);
    arg = emalloc( arg_size );
    memset( arg, 0, arg_size );

    arg[0] = estrdup( fi->prog );  /* arg[0] is the program name by convention */

#if defined(_WIN32) && !defined(__CYGWIN__)
    make_windows_path( arg[0] );
#endif


    for ( n = 0; n < num_options; n++ )
        arg[n+1] = expand_options( fprop, options[n] );

#ifdef HAVE_WORKING_FORK
    fp = fork_program( arg );

#else
    command = join_string( arg );
    fp = popen(command, F_READ_TEXT); /* Open stream */
    efree( command );
#endif /* HAVE_WORKING_FORK */


    /* Free up memory used by args list */
    options = arg;
    while ( *options ) {
        efree( *options );
        options++;
    }
    efree( arg );


    return fp;
}

static char *join_string( char **string_list )
{
    int     len = 0;  /* total size of strings */
    char    **cur_string = string_list;
    char    *outstr;
    int     first = 1;
    char    *quote_char = "\"";

    while ( *cur_string ) {
        char *str = *cur_string;
        len += strlen( str ) + 3; /* plus two for the quotes and one for the space */
        cur_string++;
    }

    outstr = (char *) emalloc( len + 1 );

    outstr[0] = '\0';

    while ( *string_list ) {
        if ( !first )
            strcat( outstr, " " );

        strcat( outstr, quote_char );
        strcat( outstr, *string_list );
        strcat( outstr, quote_char );

        first = 0;
        string_list++;
    }

    return outstr;
}


/*
 * Expands the % escapes.
 * Pass in:
 *      fprop - for file names
 *      template - string with possibly unexpanded % escapes
 * Returns:
 *      pointer to a newly allocated string
 *
 */

static char *expand_options( FileProp * fprop, char * template )
{
    int     cur_size    = strlen( template );
    char    *outstr     = (char *) emalloc( cur_size + 1 );
    char    *cur_char   = outstr;
    char    *tmpstr;


    while( *template ) {
        switch (*template) {

        case '\\':
            /* convert encoded char to char */
            *(cur_char++) = charDecode_C_Escape(template, &template);
            break;

        case '%':
            template++;
            tmpstr = expand_percent( fprop, *template );
            template++;

            if ( tmpstr ) {
                *cur_char = '\0';  /* terminate */

#if defined(_WIN32) && !defined(__CYGWIN__)
                make_windows_path( tmpstr );
#endif
                /* expand string to hold new string */
                cur_size += strlen( tmpstr );
                outstr = erealloc( outstr, cur_size + 1 );

                /* cat new string on to output string */
                strcat( outstr, tmpstr );

                efree( tmpstr );

                /* Set current char pointer */
                cur_char = outstr + strlen( outstr );
            }
            break;

        default:
            *(cur_char++) = *(template++);
            break;
        }

    }

    *cur_char = '\0';
    return outstr;
}

/*
 *  expand_percent -- expands escape code into string
 *  returns a string which must be freed.
 */


static char *expand_percent(  FileProp * fprop, char escape_code )
{
    switch( escape_code ) {

        case 'P':
            return estrdup( fprop->real_path ? fprop->real_path : "" );

        case 'p':
            return estrdup( fprop->work_path ? fprop->work_path : "" );


        case 'F':
            return estrdup( fprop->real_filename ? fprop->real_filename : "" );

        case 'f':
            return estrdup( fprop->work_path ? str_basename( fprop->work_path ) : "" );


        /* cstr_dirname allocates memory */
        case 'D':
            return fprop->real_path ? cstr_dirname( fprop->real_path ) : estrdup("");

        case 'd':
            return fprop->work_path ? cstr_dirname( fprop->work_path ) : estrdup("");


        case '%':
            return estrdup( "%" );

        default:
            progerr("Failed to decode percent escape in FileFilter* directive [%%%c]", escape_code );
    }

    return NULL;
}


/*
  -- Close filter stream
  -- return: errcode
*/

int     FilterClose(FileProp *fprop)
{
#ifdef HAVE_WORKING_FORK
    FilterList  *fl = fprop->hasfilter;

    if ( fclose( fprop->fp ) != 0 )
        progwarnno("Error closing filter '%s'", fl->options->word[0] );

    return 0;

#else
    return pclose(fprop->fp);
#endif
}

/* This should be elsewhere, but at this time this is the only place
 * it is used.
 */

#ifdef HAVE_WORKING_FORK
static FILE *fork_program( char **arg )
{
    pid_t   pid;
    int     pipe_fd[2];
    FILE    *fi;


    if ( pipe( pipe_fd ) )
        progerrno( "failed to create pipe for running [%s]: ", *arg );


    if ( (pid = fork()) == -1 )
        progerrno( "failed to fork for running [%s]: ", *arg );



    if ( !pid ) {   /* child process */

        close( pipe_fd[0] );    /* close the reading end of the pipe */

        /* Make child's stdout go to pipe */
        if ( dup2( pipe_fd[1], 1 ) == -1 )
            progerrno( "failed to dup stdout in child process [%s]: ", arg[0] );


        /* Set non-buffered output */
        /* setvbuf(stdout,(char*)NULL,_IONBF,0); */

        /* Now exec */
        execvp( arg[0], arg );
        progerrno("Failed to exec program [%s]: ", arg[0] );
    }


    /* in parent */
    close( pipe_fd[1] );  /* close writing end of pipe */

    fi = fdopen( pipe_fd[0], "r" );

    if ( fi == NULL ) 
        progerrno( "failed fdopen for filter program [%s]: ", arg[0] );

    return fi;

}
#endif /* HAVE_WORKING_FORK */








