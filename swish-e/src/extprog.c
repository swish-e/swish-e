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

#include "swish.h"
#include "index.h"
#include "hash.h"
#include "mem.h"
#include "string.h"
#include "file.h"
#include "list.h"
#include "fs.h"
#include "check.h"
#include "error.h"
#include "xml.h"
#include "txt.h"
#include "parse_conffile.h"
#include <unistd.h>

FILE   *open_external_program(SWISH * sw, char *prog)
{
    char   *cmd;
    FILE   *fp;
    size_t  total_len;
    struct swline *tmplist;

    /* get total length of configuration parameters */

    total_len = strlen(prog);

    tmplist = sw->progparameterslist;
    while (tmplist)
    {
        total_len += strlen(tmplist->line) + 1; /* separate by spaces */
        tmplist = tmplist->next;
    }

    cmd = emalloc(total_len + 20);
    strcpy(cmd, prog);

    tmplist = sw->progparameterslist;
    while (tmplist)
    {
        strcat(cmd, " ");
        strcat(cmd, tmplist->line);
        tmplist = tmplist->next;
    }


    fp = popen(cmd, FILEMODE_READ);
    efree(cmd);

    if (!fp)
        progerr("Failed to spawn external program");

    return fp;
}

/* To make filters work with prog, need to write the file out to a temp file */
/* It will be faster to do the filtering from within the "prog" program */
/* This may not be save if running as a threaded app, and I'm not clear on how portable this is */
/* This also uses read_stream to read in the file -- so the entire file is read into memory instead of chunked to the temp file */

void    save_to_temp_file(FileProp *fprop)
{
    FILE   *out;
    char   *rd_buffer = NULL;   /* complete file read into buffer */
    size_t  bytes;
    
    fprop->work_path = tmpnam( (char *) NULL );
    if ( !fprop->work_path )
        progerr("Failed to create a temporary file for filtering");


    /* slirp entire file into memory -- yuck */
    rd_buffer = read_stream(fprop->fp, fprop->fsize, 0);
        

    out = fopen( fprop->work_path, "w" );  /* is "w" portable? */        

    if ( !out )
         /* ok, how do I easily get the filename and errno into the string? */
        progerr("Failed to open temporary filter file");


    bytes = fwrite( rd_buffer, 1, fprop->fsize, out );

    if ( bytes != (size_t)fprop->fsize )
        progerr("Failed to write temporary filter file");


    /* hide the fact that it's an external program */
    fprop->fp = (FILE *) NULL;


    efree(rd_buffer);
    fclose( out );
   
}



void    extprog_indexpath(SWISH * sw, char *prog)
{
    FileProp *fprop;
    FILE   *fp;
    char   *line;
    char   *ln;
    char   *x;
    char   *real_path;
    long    fsize;
    time_t  mtime;
    int     index_no_content;
    long    truncate_doc_size;
    int     has_filter = 0;

    mtime = 0;
    fsize = 0;
    index_no_content = 0;
    real_path = NULL;

    fp = open_external_program(sw, prog);

    ln = emalloc(MAXSTRLEN + 1);

    truncate_doc_size = sw->truncateDocSize;
    sw->truncateDocSize = 0;    /* can't truncate -- prog should make sure doc is not too large */

    /* loop on headers */
    while (fgets(ln, MAXSTRLEN, fp) != NULL)
    {

        line = str_skip_ws(ln); /* skip leading white space */
        x = strrchr(line, '\n'); /* replace \n with null -- better to remove trailing white space */
        if (x)
            x[0] = '\0';


        if (strlen(line) == 0)
        {                       /* blank line indicates body */

            if (fsize && real_path)
            {

                fprop = init_file_properties(sw);
                fprop->real_path = real_path;
                fprop->work_path = real_path;

                /* set real_path, doctype, index_no_content, filter, stordesc */
                init_file_prop_settings(sw, fprop);

                fprop->fp = fp; /* stream to read from */
                fprop->fsize = fsize; /* how much to read */
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
                    save_to_temp_file( fprop );
                    has_filter++; /* save locally, in case it gets reset somewhere else */
                }

                if (sw->verbose >= 3)
                {
                    printf("%s", real_path);
                }
                else if (sw->verbose >= 2)
                {
                    printf("Processing %s...\n", real_path);
                }

                do_index_file(sw, fprop);

                if ( has_filter )
                    unlink( fprop->work_path );

                free_file_properties(fprop);
                efree(real_path);
                real_path = NULL;
                mtime = 0;
                fsize = 0;
                index_no_content = 0;

            }
            else
            {
                /* now this could be more helpful */
                progerr("External program failed to return required headers");
            }

        }
        else
        {


            if (strncasecmp(line, "Content-Length", 14) == 0)
            {
                x = strchr(line, ':');
                if (!x)
                    progerr("Failed to parse Content-Length header");
                fsize = strtol(++x, NULL, 10);
                continue;
            }

            if (strncasecmp(line, "Last-Mtime", 10) == 0)
            {
                x = strchr(line, ':');
                if (!x)
                    progerr("Failed to parse Last-Mtime header");
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
                x = strchr(line, ':');
                if (!x)
                    progerr("Failed to parse Path-Name header");

                x = str_skip_ws(++x);
                if (!*x)
                    progerr("Failed to find path name in Path-Name header");

                real_path = emalloc(strlen(x) + 1);
                strcpy(real_path, x);
                continue;
            }

            printf("Warning: Failed to parse header line: '%s' from program %s\n", line, prog);

        }
    }

    efree(ln);

    /* restore the setting */
    sw->truncateDocSize = truncate_doc_size;

    pclose(fp);                 /* progerr("Failed to properly close external program"); */
}





/* Don't have any specific configuration values to check */
int     extprog_parseconfline(SWISH * sw, void *l)
{
    return 0;
}

struct _indexing_data_source_def ExternalProgramDataSource = {
    "External-Program",
    "prog",
    extprog_indexpath,
    extprog_parseconfline
};
