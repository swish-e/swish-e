/*
$Id$
**
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
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
**-------------------------------------------------------------
** Changed getdefaults to allow metaNames in the user
** configuration file
** G.Hill 4/16/97 ghill@library.berkeley.edu
**
** change sprintf to snprintf to avoid corruption, and use MAXSTRLEN from swish.h
** added safestrcpy() macro to avoid corruption from strcpy overflow
** SRE 11/17/99
**
** added buffer size arg to grabStringValue - core dumping from overrun
** fixed logical OR and other problems pointed out by "gcc -Wall"
** SRE 2/22/00
** 
** counter modulo 128 had parens typo
** SRE 2/23/00
**
** read stopwords from file
** Rainer Scherg (rasc)  2000-06-15
** 
** 2000-11-15 rasc
** file_properties retrieves last mod date, filesize, and evals some swish
** config flags for this file!
**
** 2001-02-12 rasc   errormsg "print" changed...
** 2001-03-16 rasc   truncateDoc [read_stream] (if doc to large, truncate... )
** 2001-03-17 rasc   fprop enhanced by "real_filename"
**
*/

#ifdef HAVE_CONFIG_H
#include "acconfig.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "swish.h"
#include "mem.h"
#include "string.h"
#include "file.h"
#include "error.h"
#include "list.h"
#include "hash.h"
#include "check.h"
#include "index.h"
#include "filter.h"
#include "metanames.h"


/* Cough, hack, cough - convert slash to backslash for programs that are run via the shell */
#ifdef _WIN32
void make_windows_path( char *path )
{
    char    *c;

    for ( c = path; *c; c++ )
        if ( '/' == *c )
            *c = '\\';
}
#endif

/* Flip any backslashes to forward slashes, and remove trailing slash */


void normalize_path(char *path)
{
    int     len = strlen( path );
    char    *c;

    /* For windows users */
    for ( c = path; *c; c++ )
        if ( '\\' == *c )
            *c = '/';

    while( len > 1 && path[len-1] == '/' )
    {
        path[len-1] = '\0';
        len--;
    }
}

    

/* Is a file a directory?
*/

int     isdirectory(char *path)
{
    struct stat stbuf;

    if (stat(path, &stbuf))
        return 0;
    return ((stbuf.st_mode & S_IFMT) == S_IFDIR) ? 1 : 0;
}

/* Is a file a regular file?
*/

int     isfile(char *path)
{
    struct stat stbuf;

    if (stat(path, &stbuf))
        return 0;
    return ((stbuf.st_mode & S_IFMT) == S_IFREG) ? 1 : 0;
}

/* Is a file a link?
*/

int     islink(char *path)
{
#ifndef NO_SYMBOLIC_FILE_LINKS
    struct stat stbuf;

    if (lstat(path, &stbuf))
        return 0;
    return ((stbuf.st_mode & S_IFLNK) == S_IFLNK) ? 1 : 0;
#else
    return 0;
#endif
}

/* Get the size, in bytes, of a file.
** Return -1 if there's a problem.
*/

int     getsize(char *path)
{
    struct stat stbuf;

    if (stat(path, &stbuf))
        return -1;
    return stbuf.st_size;
}



FILE   *openIndexFILEForWrite(char *filename)
{
    return fopen(filename, F_WRITE_BINARY);
}

FILE   *openIndexFILEForRead(char *filename)
{
    return fopen(filename, F_READ_BINARY);
}

FILE   *openIndexFILEForReadAndWrite(char *filename)
{
    return fopen(filename, F_READWRITE_BINARY);
}

void    CreateEmptyFile(char *filename)
{
    FILE   *fp;

    if (!(fp = openIndexFILEForWrite(filename)))
    {
        progerrno("Couldn't write the file \"%s\": ", filename);
    }
    fclose(fp);
}

/*
 * Invoke the methods of the current Indexing Data Source
 */
void    indexpath(SWISH * sw, char *path)
{
    /* invoke routine to index a "path" */
    (*IndexingDataSource->indexpath_fn) (sw, path);
}


/*
  -- read file into a buffer
  -- truncate file if necessary (truncateDocSize)
  -- return: buffer
  -- 2001-03-16 rasc    truncateDoc
*/

/* maybe some day this could be chunked reading? */

char   *read_stream(SWISH *sw, char *name, FILE * fp, long filelen, long max_size)
{
    long    c,
            offset;
    long    bufferlen;
    unsigned char *buffer, *tmp = NULL;
    size_t  bytes_read;


    if (filelen)
    {

        /* truncate doc? */
        if (max_size && (filelen > max_size))
        {
            filelen = max_size;
        }

        buffer = (unsigned char *)Mem_ZoneAlloc(sw->Index->perDocTmpZone, filelen + 1);
        *buffer = '\0';
        bytes_read = fread(buffer, 1, filelen, fp);


        buffer[filelen] = '\0';

        /* JFP - substitute null chars, VFC record may have null char in reclen word, try to discard them */
        if ( strlen( (char *)buffer ) < bytes_read )
        {
            int i;
            progwarn("Substituted possible embedded null character(s) in file '%s'\n", name);
            for (i = 0; i < bytes_read; ++i)
                if (buffer[i] == '\0') buffer[i] = '\n';
        }

    }
    else
    {                           /* if we are reading from a popen call, filelen is 0 */

        buffer = (unsigned char *)Mem_ZoneAlloc(sw->Index->perDocTmpZone,(bufferlen = RD_BUFFER_SIZE) + 1);
        *buffer = '\0';
        for (offset = 0; (c = fread(buffer + offset, 1, RD_BUFFER_SIZE, fp)) == RD_BUFFER_SIZE; offset += RD_BUFFER_SIZE)
        {
            /* truncate? break if to much read */
            if (max_size && (bufferlen > max_size))
            {
                break;
            }
            tmp = (unsigned char *)Mem_ZoneAlloc(sw->Index->perDocTmpZone, bufferlen + RD_BUFFER_SIZE + 1);
            memcpy(tmp,buffer,bufferlen+1);
            buffer = tmp;			
            bufferlen += RD_BUFFER_SIZE;
        }
        filelen = offset + c;

        if (max_size && (filelen > max_size))
        {
            filelen = max_size;
        }
        buffer[filelen] = '\0';
    }
    return (char *) buffer;
}

/* Sept 25, 2001 - moseley
 * Flush the file -- for use with -S prog, when either Truncate is in use, or 
 * the parser aborted for some reason (e.g. !isoktitle).
 */

void flush_stream( FileProp *fprop )
{
    char tmpbuf[4096];
    int  read;

    while ( fprop->bytes_read < fprop->fsize )
    {
        if ( ( fprop->fsize - fprop->bytes_read ) > 4096 )
        {
            if ( !(read = fread(tmpbuf, 1, 4096, fprop->fp)))
                break;

            fprop->bytes_read += read;
        }
        else
        {
            read = fread(tmpbuf, 1, fprop->fsize - fprop->bytes_read, fprop->fp);
            break;
        }
    }
}


/* Mar 27, 2001 - moseley
 * Separate out the creation of the file properties
 *
 */

FileProp *init_file_properties(SWISH * sw)
{
    FileProp *fprop;

    fprop = (FileProp *) emalloc(sizeof(FileProp));
    /* emalloc checks fail and aborts... */

    memset( fprop, 0, sizeof(FileProp) );

    return fprop;
}


/* Mar 27, 2001 - moseley
 * Separate out the adjusting of file properties by config settings
 * 2001-04-09  rasc changed filters
 */

void    init_file_prop_settings(SWISH * sw, FileProp * fprop)
{

    /* Basename of document path => document filename */
    fprop->real_filename = str_basename(fprop->real_path);


    /* -- get Doc Type as is in IndexContents or Defaultcontents
       -- doctypes by jruiz
     */

    /* Might already be set by a header in extpro.c */
    if ( !fprop->doctype )
    {
        /* Get the type by file extension -- or return NODOCTYPE */
        fprop->doctype = getdoctype(fprop->real_path, sw->indexcontents);

        /* If was not set by getdoctype() then assign it the default parser */
        /* This could still be NODOCTYPE, or it might be something set by DefaultContents */
        
        if (fprop->doctype == NODOCTYPE)
            fprop->doctype = sw->DefaultDocType;
    }


    /* -- index just the filename (or doc title tags)?
       -- this param was "wrongly" named indextitleonly */

    fprop->index_no_content = (sw->nocontentslist != NULL) && isoksuffix(fprop->real_path, sw->nocontentslist);

    /* -- Any filter for this file type?
       -- NULL = No Filter, (char *) path to filter prog.
     */

    fprop->hasfilter = hasfilter(sw, fprop->real_path);

    fprop->stordesc = hasdescription(fprop->doctype, sw->storedescription);

}



/*
  -- file_properties
  -- Get/eval information about a file and return it.
  -- Some flags are calculated from swish configs for this "real_path"
  -- Structure has to be freed using free_file_properties
  -- 2000-11-15 rasc
  -- return: (FileProp *)
  -- A failed stat returns an empty (default) structure

  -- 2000-12
  -- Added StoreDescription
*/

FileProp *file_properties(char *real_path, char *work_file, SWISH * sw)
{
    FileProp *fprop;
    struct stat stbuf;

    /* create an initilized fprop structure */
    
    fprop = init_file_properties(sw);


    /* Dup these, since the real_path may be reallocated by FileRules */
    fprop->real_path = estrdup( real_path );
    fprop->work_path = estrdup( work_file ? work_file : real_path );
    fprop->orig_path = estrdup( real_path );


    /* Stat the file */
    /* This is really the wrong place for this, as it's really only useful for fs.c method */
    /* for http.c it means the last mod date is the temp file date */
    /* Probably this entire function isn't needed - moseley */

    if (!stat(fprop->work_path, &stbuf))
    {
        fprop->fsize = (long) stbuf.st_size;
        fprop->mtime = stbuf.st_mtime;
    }


    /* Now set various fprop settings based mostly on file name */
    
    init_file_prop_settings(sw, fprop);



#ifdef DEBUG
    fprintf(stderr, "file_properties: path=%s, (workpath=%s), fsize=%ld, last_mod=%ld Doctype: %d Filter: %p\n",
            fprop->real_path, fprop->work_path, (long) fprop->fsize, (long) fprop->mtime, fprop->doctype, fprop->filterprog);
#endif

    return fprop;
}


/* -- Free FileProp structure
   -- unless no alloc for strings simple free structure
*/

void    free_file_properties(FileProp * fprop)
{
    efree( fprop->real_path );
    efree( fprop->work_path );
    efree( fprop->orig_path );
    efree(fprop);
}


static char *temp_file_template = "XXXXXX";
/***********************************************************************
*  Create a temporary file
*
*   Call With:
*       *SWISH              = to get at the TmpDir config setting which I don't like
*       *prefix             = chars to prepend to the file name
*       **file_name_buffer  = where to store address of file name
*       unlink              = if true, will unlink file
*                             if not unlinked, then caller must free the name
*   Return:
*       *FILE
*       modified file_name_buffer
*
*   Will create temp files in the directory specified by environment vars
*   TMPDIR and TMP, and by the config.h setting of TMPDIR in that order.
*
*   Note:
*       It's expected that swish is not run suid, so
*           (getuid()==geteuid()) && (getgid()==getegid())
*       if not checked.  I'm not sure if that would choke on other platforms.
*     
*
*   Source:
*       http://www.linuxdoc.org/HOWTO/Secure-Programs-HOWTO/avoid-race.html
*
*   Questions:
*       Can non-unix OS unlink the file and continue to hold the fd?
*
***********************************************************************/

FILE *create_tempfile(SWISH *sw, const char *f_mode, char *prefix, char **file_name_buffer, int remove_file_name )
{
    int         temp_fd;
    mode_t      old_mode;
    FILE        *temp_file;
    char        *file_name;
    int         file_name_len;
    struct MOD_Index *idx = sw->Index;
    char        *tmpdir = NULL;
    file_name_len = (prefix ? strlen(prefix) : 0) + strlen( temp_file_template ) + strlen( TEMP_FILE_PREFIX );

    

    /* Perl is nice sometimes */
    if ( !( tmpdir = getenv("TMPDIR")) )
        if ( !(tmpdir = getenv("TMP")) )
            if( !(tmpdir = getenv("TEMP")) )
                tmpdir = idx->tmpdir;

    if ( tmpdir && !*tmpdir )
        tmpdir = NULL;     // just in case it's the empty string

    if ( tmpdir )
        file_name_len += strlen( tmpdir ) + 1;  // for path separator

 

    file_name = emalloc( file_name_len + 1 );

    *file_name = '\0';

    if ( tmpdir )
    {
        strcat( file_name, tmpdir );
        normalize_path( file_name );
        strcat( file_name, "/" );
    }

    strcat( file_name, TEMP_FILE_PREFIX );
    
    if ( prefix )
        strcat( file_name, prefix );

    strcat( file_name, temp_file_template );
        
    old_mode = umask(077);  /* Create file with restrictive permissions */

    temp_fd = mkstemp( file_name );

    (void) umask(old_mode);

    if (temp_fd == -1)
        progerrno("Couldn't open temporary file '%s': ", file_name );

    if (!(temp_file = fdopen(temp_fd, f_mode)))
        progerrno("Couldn't create temporary file '%s' file descriptor: ", file_name);

    if ( remove_file_name )
    {
        if ( remove( file_name ) == -1 )
            progerrno("Couldn't unlink temporary file '%s' :", file_name);

        efree( file_name );            
    }
    else
        *file_name_buffer = file_name;

        
    return temp_file;
}


