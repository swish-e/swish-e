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
** removed Copyright HP statement, since moseley created this file from scratch.

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
#include "bash.h" /* for locating a program */

static char *find_command_in_path(const char *name, const char *path_list, int *path_index);
static char *get_env_path_with_libexecdir( void );


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
    char   *full_path;
    FILE   *fp;
    size_t  total_len;
    struct swline *progparameterslist = sw->Prog->progparameterslist;
    int    path_index = 0;  /* index into $PATH */
    char  *env_path = get_env_path_with_libexecdir();
	
    if ( ! strcmp( prog, "stdin") )
        return stdin;

    normalize_path( prog );  /* flip backslashes to forward slashes */

    full_path = find_command_in_path( (const char *)prog, env_path, &path_index );
    if ( !full_path )
        progerr("Failed to find program '%s' in PATH: %s ", prog, env_path );

    efree( env_path );

    if ( sw->verbose )
        printf("External Program found: %s\n", full_path );



    /* get total length of configuration parameters */
    total_len = 0;
    while (progparameterslist)
    {
        total_len += strlen(progparameterslist->line) + 1; /* separate by spaces */
        progparameterslist = progparameterslist->next;
    }

    cmd = emalloc(total_len + strlen( full_path ) + 1);
    strcpy(cmd, full_path);
    efree( full_path );


#if defined(_WIN32) && !defined(__CYGWIN__)

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

/* Notice that the data is read out in TEXT mode -- this is because it's read from the */
/* external program in TEXT mode.  Binary files will be modified while in memory */
/* (under Windows) but writing back in TEXT mode should restore the file to its */
/* original binary format for use by the filter.  Really, don't use FileFilter with -S prog */

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
    char   *x;
    long    fsize;
    time_t  mtime;
    int     index_no_content;
    long    truncate_doc_size;
    int     docType = 0;
    int     original_update_mode = sw->Index->update_mode;

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

            /* Reset Update Mode */
            sw->Index->update_mode = original_update_mode;


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

           /* new Update-Mode: [Update|Remove|Index] header
            * for this to work, swish-e has to be compiled with incremental option and
            * in update mode (-u) so that index is opened in read/write mode
            * dpavlin 2004-12-09
            */

            if (strncasecmp(line, "Update-Mode", 11) == 0)
            {
#ifndef USE_BTREE
                progerr("Cannot use Update-Mode header with this version of Swish-e.  Rebuild with --enable-incremental.");
#endif
                /* April 8, 2005 - If update mode is set on the initial indexing job
                 * then the btree code fails.  So for now restrict this feature for only
                 * when -r or -r specified on the command line */
                if ( MODE_UPDATE != original_update_mode && MODE_REMOVE != original_update_mode )
                    progerr("Cannot use 'Update-Mode' header without -u or -r");


                x = strchr(line, ':');
                if (!x)
                    progerr("Failed to parse Update-Mode '%s'", line);

                x = str_skip_ws(++x);
                if (!*x)
                    progerr("Failed to parse Update-Mode header '%s'", line);

               /* should we dump error here? It seem to work without update mode! - dpavlin
                * I say just let it run. Without -u or -r the index is recreated, though
                * that may be more of an issue.
                * In fact, maybe with USE_BTREE need a way to explicitly say to 
                * clear the index.  Forget using -u or -r can be a big mistake. - moseley
                * 
               if (sw->Index->update_mode != MODE_UPDATE && sw->Index->update_mode != MODE_REMOVE)
                       progwarn("Update-Mode header is supported only if swish-e is invoked in update (-u) mode");
                */



               if ( strncasecmp(x, "Update", 6) == 0 ) {
                       sw->Index->update_mode = MODE_UPDATE;
                       if ( sw->verbose >= 4 ) printf( "Input file selected: %s (MODE_UPDATE)\n", x );
               } else if ( strncasecmp(x, "Remove", 6) == 0 ) {
                       sw->Index->update_mode = MODE_REMOVE;
                       if ( sw->verbose >= 4 ) printf( "Input file selected: %s (MODE_REMOVE)\n", x );
               } else if ( strncasecmp(x, "Index", 5) == 0 ) {
                       sw->Index->update_mode = MODE_UPDATE;
                       if ( sw->verbose >= 4 ) printf( "Input file selecte: %s (MODE_UPDATE)\n", x );
               } else {
                       progerr("Unknown Update-Mode: %s", x);
               }

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


/* From GNU Which */
static char *find_command_in_path(const char *name, const char *path_list, int *path_index)
{
  char *found = NULL, *full_path;
  int status, name_len;
  int absolute_path_given = 0;
  char *abs_path = NULL;
  name_len = strlen(name);

  if (!absolute_program(name))
    absolute_path_given = 0;
  else
  {
    char *p;
    absolute_path_given = 1;

    if (abs_path)
      xfree(abs_path);

    if (*name != '.' && *name != '/' && *name != '~')
    {
      abs_path = (char *)xmalloc(3 + name_len);
      strcpy(abs_path, "./");
      strcat(abs_path, name);
    }
    else
    {
      abs_path = (char *)xmalloc(1 + name_len);
      strcpy(abs_path, name);
    }

    path_list = abs_path;
    p = strrchr(abs_path, '/');
    *p++ = 0;
    name = p;
  }

  while (path_list && path_list[*path_index])
  {
    char *path;

    if (absolute_path_given)
    {
      path = savestring(path_list);
      *path_index = strlen(path);
    }
    else
      path = get_next_path_element(path_list, path_index);

    if (!path)
      break;

#ifdef SKIPTHIS
    if (*path == '~')
    {
      char *t = tilde_expand(path);
      xfree(path);
      path = t;

      if (skip_tilde)
      {
        xfree(path);
        continue;
      }
    }

    if (skip_dot && *path != '/')
    {
      xfree(path);
      continue;
    }

    found_path_starts_with_dot = (*path == '.');
#endif

    full_path = make_full_pathname(path, name, name_len);
    xfree(path);

    status = file_status(full_path);

    /* This is different from "where" because it stops at the first found file */
    /* but where (and shells) continue to find first executable program in path */
    if (status & FS_EXISTS) 
    {
      if (status & FS_EXECABLE)
      {
        found = full_path;
        break;
      }
      else
          progwarn("Found '%s' in PATH but is not executable", full_path);
    }
    xfree(full_path);
  }

  return (found);
}


static char *get_env_path_with_libexecdir( void )
{
    char *pathbuf;
    char *path = getenv("PATH");
    char *execdir = get_libexec();  /* Should free */

    if ( !path )
        return execdir;

    pathbuf = (char *)emalloc( strlen( path ) + strlen( execdir ) + strlen( PATH_SEPARATOR ) + 1 );

    sprintf(pathbuf, "%s%s%s", path, PATH_SEPARATOR, execdir );
    return pathbuf;
}

