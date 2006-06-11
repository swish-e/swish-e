/*
$Id$
**


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


/* private module prototypes */

static FilterList *addfilter(FilterList *rp, char *FilterSuffix, char *FilterProg, char *options, char *FilterDir);
static char *filterCallCmdOptParam2(char *str, char param, FileProp * fp);
static char *filterCallCmdOptStr(char *opt_mask, FileProp * fprop);




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
                efree( fm->options );
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
        {
            normalize_path( sl->word[2] );  /* trying to keep these close to where there's input */
            md->filterlist = (FilterList *) addfilter(md->filterlist, sl->word[1], sl->word[2], sl->word[3], md->filterdir);
        }
        else
            progerr("%s: requires \"extension\" \"filter\" \"[options]\"", w0);

        return 1;
    }


    /* added March 16, 2002 - moseley */

    if ( strcasecmp( w0, "FileFilterMatch") == 0 )
    {
        FilterList *filter;

        if ( sl->n < 4 )
            progerr("%s requires at least three parameters: 'filterprog' 'options' 'regexp' ['regexp'...]\n");


        filter = (FilterList *)emalloc( sizeof( FilterList ) );
        memset( filter, 0, sizeof( FilterList ) );

        filter->prog    = estrdup( sl->word[1] );
        normalize_path( filter->prog );
        
        filter->options = estrdup( sl->word[2] );
        add_regex_patterns( w0, &filter->regex, &(sl->word[3]), 1 );

        if ( !md->filterlist )
            md->filterlist = filter;
        else
        {
            /* add to end of list */
            FilterList *f = md->filterlist;
            while ( f->next )
                f = f->next;

            f->next = filter;
        }

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


static FilterList *addfilter(FilterList *rp, char *suffix, char *prog, char *options, char *filterdir)
 {
    FilterList *newnode;
    char   *buf;


    newnode = (FilterList *) emalloc(sizeof(FilterList));
    memset( newnode, 0, sizeof( FilterList ) );

    newnode->suffix = (char *) estrdup(suffix);
    newnode->options = (options) ? (char *) estrdup(options) : NULL;

    if ( filterdir && (*prog != '/' ) )
    {
        buf = emalloc( strlen( filterdir ) + strlen( prog ) + 2 );
        *buf = '\0';
        strcat( buf, filterdir ); /* don't care about speed here */
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
    char   *filtercmd;
    FilterList *fi;
    char   *cmd_opts,
           *opt_mask;
    FILE   *fp;
    int     len;
    char   *prog;

    /*
       -- simple filter call "filter 'work_file' 'real_path_url'"
       -- or call filter "filter  user_param"
       --    > (decoded output =stdout)
     */

    fi = fprop->hasfilter;

// old code (rasc, leave it for checks and speed benchmarks)
//      len =  strlen(fi->prog)+strlen(fprop->work_path)+strlen(fprop->real_path);
//
//      filtercmd=emalloc(len + 6 +1);
//      sprintf(filtercmd, "%s \'%s\' \'%s\'", fi->prog,
//                 fprop->work_path, fprop->real_path);


    /* if no filter cmd param given, use default */

    opt_mask = (fi->options && *(fi->options) ) ? fi->options : "'%p' '%P'";
    cmd_opts = filterCallCmdOptStr(opt_mask, fprop);

    len = strlen(fi->prog) + strlen(cmd_opts);
    filtercmd = emalloc(len + 2);

    prog = estrdup( fi->prog );
#if defined(_WIN32) && !defined(__CYGWIN__)
    make_windows_path( prog );
#endif

    sprintf(filtercmd, "%s %s", prog, cmd_opts);

    fp = popen(filtercmd, F_READ_TEXT); /* Open stream */

    efree( prog );
    efree(filtercmd);
    efree(cmd_opts);

    return fp;
}


/* 
  -- Close filter stream
  -- return: errcode
*/

int     FilterClose(FILE * fp)
{
    return pclose(fp);
}





/*
  -- process Filter cmdoption parameters
  -- Replace variables with the file/document names.
  -- Variables: please see code below
  -- return: (char *) new string
  -- 2001-04-10 rasc
*/

static char *filterCallCmdOptStr(char *opt_mask, FileProp * fprop)
 {
    char   *cmdopt,
           *co,
           *om;
    int     max = MAXSTRLEN *3;


    cmdopt = (char *) emalloc(max);
    *cmdopt = '\0';

    co = cmdopt;
    om = opt_mask;
    while (*om) {

        /* Argh! no overflow checking. Fix $$$ - Mar 2002 - moseley */

        switch (*om) {

        case '\\':
            *(co++) = charDecode_C_Escape(om, &om);
            break;

        case '%':
            ++om;
            co = filterCallCmdOptParam2(co, *om, fprop);
            if (*om)
                om++;
            break;

        default:
            *(co++) = *(om++);
            break;
        }

    }

    *co = '\0';
    return cmdopt;
}


static char *filterCallCmdOptParam2(char *str, char param, FileProp * fprop)
 {
    static char *nul = "_NULL_"; // $$$ wouldn't "" be better?  Be easier to check for in the filter
    char   *x;
    

    switch (param) {

    case 'P':                  /* Full Doc Path/URL */
        strcpy(str, (fprop->real_path) ? fprop->real_path : nul);
        break;

    case 'p':                  /* Full Path TMP/Work path */
        strcpy(str, (fprop->work_path) ? fprop->work_path : nul);
#if defined(_WIN32) && !defined(__CYGWIN__)
        make_windows_path( str );
#endif
        break;

    case 'F':                  /* Basename Doc Path/URL */
        strcpy(str, (fprop->real_filename) ? fprop->real_filename : nul);
        break;

    case 'f':                  /* basename Path TMP/Work path */
        strcpy(str, (fprop->work_path) ? str_basename(fprop->work_path) : nul);
        break;

    case 'D':                  /* Dirname Doc Path/URL */
        x = (fprop->real_path) ? cstr_dirname(fprop->real_path) : nul;
        strcpy(str, x);
        efree(x);
        break;

    case 'd':                  /* Dirname TMP/Work Path */
        x = (fprop->work_path) ? cstr_dirname(fprop->work_path) : nul;
        strcpy(str, x);
        efree(x);
#if defined(_WIN32) && !defined(__CYGWIN__)
        make_windows_path( str );
#endif
        break;

    case '%':                  /* %% == print %    */
        *str = param;
        *(str + 1) = '\0';
        break;

    default:                   /* unknown, print  % and char */
        *str = '%';
        *(str + 1) = param;
        *(str + 2) = '\0';
        break;
    }

    while (*str)
        str++;                  /* Pos to end of string */
    return str;
}


