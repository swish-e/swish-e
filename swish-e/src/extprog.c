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
**
** Mar 27, 2001 - created moseley
**
*/

#include "acconfig.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "swish.h"
#include "mem.h"
#include "swstring.h"
#include "index.h"
#include "file.h"
#include "error.h"
#include "parse_conffile.h"

struct MOD_Prog
{
    /* prog system specific configuration parameters */
    struct swline *progparameterslist;
};


/* 
  -- init structures for this module
*/

void initModule_Prog (SWISH  *sw)
{
    struct MOD_Prog *self;

    self = (struct MOD_Prog *) emalloc(sizeof(struct MOD_Prog));
    sw->Prog = self;

    /* initialize buffers used by indexstring */
    self->progparameterslist = (struct swline *) NULL;

    return;
}

void freeModule_Prog (SWISH *sw)
{
    struct MOD_Prog *self = sw->Prog;


    if ( self->progparameterslist )
        efree( self->progparameterslist );

    efree ( self );
    sw->Prog = NULL;

    return;
}

int configModule_Prog (SWISH *sw, StringList *sl)

{
    struct MOD_Prog *self = sw->Prog;
    char *w0    = sl->word[0];

    if (strcasecmp(w0, "SwishProgParameters") == 0)
    {
        if (sl->n > 1)
        {
            grabCmdOptions(sl, 1, &self->progparameterslist);
        }
        else
            progerr("%s: requires at least one value", w0);
    }

    else 
    {
        return 0;                   /* not a module directive */
    }

    return 1;
}



static FILE   *open_external_program(SWISH * sw, char *prog)
{
    char   *cmd;
    FILE   *fp;
    size_t  total_len;
    struct  stat stbuf;
    struct swline *progparameterslist = sw->Prog->progparameterslist;
    char *execdir = get_libexec();
	
    if ( ! strcmp( prog, "stdin") )
        return stdin;


    /* get total length of configuration parameters */
    total_len = 0;
    while (progparameterslist)
    {
        total_len += strlen(progparameterslist->line) + 1; /* separate by spaces */
        progparameterslist = progparameterslist->next;
    }

    total_len += strlen( execdir ) + 1;
    total_len += strlen(prog);

    cmd = emalloc(total_len + 20);
    cmd[0] = '\0';

/* Prefix libexecdir if path does not start with a ".", "/", "\", nor "X:"  */
/* $$$ the "." check is a bug, need to check for "./"  */

    if ( prog[0] != '/' && prog[0] != '.' && prog[0] != '\\' && prog[1] != ':' )
    {
        strcat( cmd, execdir );
        efree( execdir );
        if ( cmd[ strlen( cmd ) - 1 ]  != '/' ) 
           strcat( cmd, "/" );
    }

    strcat(cmd, prog);

    normalize_path( cmd );  /* for stat calls */


#ifndef NO_PROG_CHECK
    /* this should probably be in file.c so filters.c can check, too */
    /* note this won't catch errors in a shebang line, of course */

    if (stat(cmd, &stbuf))
        progerrno("External program '%s': ", cmd);

    if ( stbuf.st_mode & S_IFDIR)
        progerr("External program '%s' is a directory.", cmd);

#ifdef HAVE_ACCESS

    if ( access( cmd, R_OK|X_OK ) )
        progerrno("Cannot execute '%s': ", cmd);

#endif

#endif /* NO_PROG_CHECK */

#ifdef _WIN32

    make_windows_path( cmd );

#endif

        


    progparameterslist = sw->Prog->progparameterslist;
    while (progparameterslist)
    {
        strcat(cmd, " ");
        strcat(cmd, progparameterslist->line);
        progparameterslist = progparameterslist->next;
    }

    fp = popen(cmd, F_READ_TEXT);

    if (!fp)
        progerrno("Failed to spawn external program '%s': ", cmd);

    efree(cmd);
    return fp;
}

/* To make filters work with prog, need to write the file out to a temp file */
/* It will be faster to do the filtering from within the "prog" program */
/* This may not be safe if running as a threaded app, and I'm not clear on how portable this is */
/* This also uses read_stream to read in the file -- so the entire file is read into memory instead of chunked to the temp file */

static void    save_to_temp_file(SWISH *sw, FileProp *fprop)
{
    FILE   *out;
    char   *rd_buffer = NULL;   /* complete file read into buffer */
    size_t  bytes;
    struct FilterList *filter_save = fprop->hasfilter;
    

    /* slirp entire file into memory -- yuck */
    fprop->hasfilter = NULL;  /* force reading fprop->fsize bytes */
    rd_buffer = read_stream(sw, fprop, 0);

    fprop->hasfilter = filter_save;

    /* Save content to a temporary file */
    efree( fprop->work_path );
    out = create_tempfile(sw, F_WRITE_TEXT, "fltr", &fprop->work_path, 0 );

    bytes = fwrite( rd_buffer, 1, fprop->fsize, out );

    if ( bytes != (size_t)fprop->fsize )
        progerrno("Failed to write temporary filter file '%s': ", fprop->work_path);


    /* hide the fact that it's an external program */
    fprop->fp = (FILE *) NULL;


//***JMRUIZ    efree(rd_buffer);
    fclose( out );
   
}



static void    extprog_indexpath(SWISH * sw, char *prog)
{
    FileProp *fprop;
    FILE   *fp;
    char   *ln;
    char   *real_path;
    long    fsize;
    time_t  mtime;
    int     index_no_content;
    long    truncate_doc_size;
    int     docType = 0;

    mtime = 0;
    fsize = -1;
    index_no_content = 0;
    real_path = NULL;

    fp = open_external_program(sw, prog);

    ln = emalloc(MAXSTRLEN + 1);

    truncate_doc_size = sw->truncateDocSize;
    sw->truncateDocSize = 0;    /* can't truncate -- prog should make sure doc is not too large */
    // $$$ This is no longer true with libxml push parser

    // $$$ next time, break out the header parsing in its own function, please

    /* loop on headers */
    while (fgets(ln, MAXSTRLEN, fp) != NULL)
    {
        char    *end;
        char    *line;
        int     has_filter = 0;
        
        line = str_skip_ws(ln); /* skip leading white space */
        end = strrchr(line, '\n'); /* replace \n with null -- better to remove trailing white space */

        /* trim white space */
        if (end)
        {
            while ( end > line && isspace( (int)*(end-1) ) )
                end--;

            *end = '\0';
        }

        if (strlen(line) == 0) /* blank line indicates body */
        {
            if (!real_path)
                progerr("External program failed to return required headers Path-Name:");

            if ( fsize == -1)
                progerr("External program failed to return required headers Content-Length: when processing file '%s'", real_path);

            if ( fsize == 0 && sw->verbose >= 2)
                progwarn("External program returned zero Content-Length when processing file'%s'", real_path);

            

            /* Create the FileProp entry to describe this "file" */

            /* This is not great -- really should make creating a fprop more generic */
            /* this was done because file.c assumed that the "file" was on disk */
            /* which has changed over time due to filters, http, and prog */
            
            fprop = init_file_properties();  
            fprop->real_path = real_path;
            fprop->work_path = estrdup( real_path );
            fprop->orig_path = estrdup( real_path );

            /* Set the doc type from the header */
            if ( docType ) 
            {
                fprop->doctype   = docType;
                docType = 0;
            } 


            /* set real_path, doctype, index_no_content, filter, stordesc */
            init_file_prop_settings(sw, fprop);

            fprop->fp = fp; /* stream to read from */
            fprop->fsize = fsize; /* how much to read */
            fprop->source_size = fsize;  /* original size of input document - should be an extra header! */
            fprop->mtime = mtime;

            /* header can force index_no_content */
            if (index_no_content)
                fprop->index_no_content++;


            /*  the quick hack to make filters work is for FilterOpen
             *  to see that fprop->fp is set, read it into a buffer
             *  write it to a temporary file, then call the filter
             *  program as noramlly is done.  But much smarter to
             *  simply filter in the prog, after all.  Faster, too.
             */

            if (fprop->hasfilter)
            {
                save_to_temp_file( sw , fprop );
                has_filter++; /* save locally, in case it gets reset somewhere else */
            }

            if (sw->verbose >= 3)
                printf("%s", real_path);
            else if (sw->verbose >= 2)
                printf("Processing %s...\n", real_path);


            do_index_file(sw, fprop);

            if ( has_filter && remove( fprop->work_path ) )
                progwarnno("Error removing temporary file '%s': ", fprop->work_path);

            free_file_properties(fprop);
            // efree(real_path); free_file_properties will free the paths
            real_path = NULL;
            mtime = 0;
            fsize = 0;
            index_no_content = 0;
            
        }


        else /* we are reading headers */
        {
            if (strncasecmp(line, "Content-Length", 14) == 0)
            {
                char *x = strchr(line, ':');
                if (!x)
                    progerr("Failed to parse Content-Length header '%s'", line);
                fsize = strtol(++x, NULL, 10);
                continue;
            }

            if (strncasecmp(line, "Last-Mtime", 10) == 0)
            {
                char *x = strchr(line, ':');
                if (!x)
                    progerr("Failed to parse Last-Mtime header '%s'", line);
                mtime = strtol(++x, NULL, 10);
                continue;
            }

            if (strncasecmp(line, "No-Contents:", 12) == 0)
            {
                index_no_content++;
                continue;
            }


            if (strncasecmp(line, "Path-Name", 9) == 0)
            {
                char *x = strchr(line, ':');
                if (!x)
                    progerr("Failed to parse Path-Name header '%s'", line);

                x = str_skip_ws(++x);
                if (!*x)
                    progerr("Failed to find path name in Path-Name header '%s'", line);

                real_path = emalloc(strlen(x) + 1);
                strcpy(real_path, x);
                continue;
            }

            if (strncasecmp(line, "Document-Type", 13) == 0)
            {
                char *x = strchr(line, ':');
                if (!x)
                    progerr("Failed to parse Document-Type '%s'", line);

                x = str_skip_ws(++x);
                if (!*x)
                    progerr("Failed to documnet type in Document-Type header '%s'", line);

                if ( !(docType = strtoDocType( x )) )
                    progerr("documnet type '%s' not a valid Swish-e document type in Document-Type header '%s'", x, line);

                continue;
            }

            progwarn("Unknown header line: '%s' from program %s", line, prog);

        }
    }

    efree(ln);

    /* restore the setting */
    sw->truncateDocSize = truncate_doc_size;

    if ( fp != stdin )
        if ( pclose(fp) == -1 )
            progwarnno("Failed to properly close external program: ");
    
}





/* Don't use old method of config checking */
static int     extprog_parseconfline(SWISH * sw, StringList *l)
{
    return 0;
}



struct _indexing_data_source_def ExternalProgramDataSource = {
    "External-Program",
    "prog",
    extprog_indexpath,
    extprog_parseconfline
};
