/*
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
** change sprintf to snprintf to avoid corruption,
** added safestrcpy() macro to avoid corruption from strcpy overflow,
** and use MAXKEYLEN as string length vs. literal "34"
** SRE 11/17/99
**
** 2000-11     jruiz,rasc  some redesign
** 2001-04-07  rasc        fixed FileRule pathname
**
*/

#include "swish.h"
#include "mem.h"
#include "swstring.h"
#include "index.h"
#include "hash.h"
#include "file.h"
#include "list.h"
#include "fs.h"
#include "check.h"
#include "error.h"
#include "xml.h"
#include "txt.h"
#include "parse_conffile.h"
#include "swish_qsort.h"

typedef struct
{
    int     currentsize;
    int     maxsize;
    char  **filenames;
}
DOCENTRYARRAY;



#define MAXKEYLEN 34            /* Hash key -- allow for 64 bit inodes */


static int get_rules( char *name, StringList *sl, PATH_LIST *pathlist );
static int check_FileTests( char *path, PATH_LIST *test );
static void indexadir(SWISH *, char *);
static void indexafile(SWISH *, char *);
static void printfile(SWISH *, char *);
static void printfiles(SWISH *, DOCENTRYARRAY *);
static void printdirs(SWISH *, DOCENTRYARRAY *);
static DOCENTRYARRAY *adddocentry(DOCENTRYARRAY * e, char *filename);
static void split_path(char *path, char **directory, char **file);

/*
  -- init structures for this module
*/

void    initModule_FS(SWISH * sw)
{
    struct MOD_FS *fs;

    fs = (struct MOD_FS *) emalloc(sizeof(struct MOD_FS));

    memset(fs, 0, sizeof(struct MOD_FS));
    sw->FS = fs;

}


/*
  -- release all wired memory for this module
*/

void    freeModule_FS(SWISH * sw)
{
    struct MOD_FS *fs = sw->FS;

    /* Free fs parameters */

    free_regex_list( &fs->filerules.pathname );
    free_regex_list( &fs->filerules.dirname );
    free_regex_list( &fs->filerules.filename );
    free_regex_list( &fs->filerules.dircontains );
    free_regex_list( &fs->filerules.title );

    free_regex_list( &fs->filematch.pathname );
    free_regex_list( &fs->filematch.dirname );
    free_regex_list( &fs->filematch.filename );
    free_regex_list( &fs->filematch.dircontains );
    free_regex_list( &fs->filematch.title );


    /* free module data */
    efree(fs);
    sw->FS = NULL;

    return;
}


/*
 -- Config Directives
 -- Configuration directives for this Module
 -- return: 0/1 = none/config applied
 Aug 1, 2001 -- these probably should be pre-compiled regular expressions,
                and their memory should be freed on exit.  moseley
*/



int     configModule_FS(SWISH * sw, StringList * sl)
{
    struct MOD_FS *fs = sw->FS;
    char   *w0 = sl->word[0];

    if (strcasecmp(w0, "FileRules") == 0)
        return get_rules( w0, sl, &fs->filerules );

    if (strcasecmp(w0, "FileMatch") == 0)
        return get_rules( w0, sl, &fs->filematch );


    if (strcasecmp(w0, "FollowSymLinks") == 0)
    {
        fs->followsymlinks = getYesNoOrAbort(sl, 1, 1);
        return 1;
    }

    return 0;                   /* not one of our parameters */

}

static int get_rules( char *name, StringList *sl, PATH_LIST *pathlist )
{
    char   *w1;
    char   *both;
    int     regex_pattern = 0;


    if (sl->n < 4)
    {
        printf("err: Wrong number of parameters in %s\n", name);
        return 0;
    }


    /* For "is" make sure it matches the entire pattern */
    /* A bit ugly */

    if ( strcasecmp(sl->word[2], "is") == 0 )
    {
        int    i;
        /* make patterns match the full string */
        for ( i = 3; i < sl->n; i++ )
        {
            int len = strlen( sl->word[i] );
            char *new;
            char *old = sl->word[i];

            if ( (strcasecmp( old, "not" ) == 0) && i < sl->n-1 )
                continue;

            new = emalloc( len + 3 );
            
            if ( sl->word[i][0] != '^' )
            {
                strcpy( new, "^" );
                strcat( new, sl->word[i] );
            }
            else
                strcpy( new, sl->word[i] );

            if ( sl->word[i][len-1] != '$' )
                strcat( new, "$" );

            sl->word[i] = new;
            efree( old );
        }
                
    }

    else if ( strcasecmp(sl->word[2], "regex") == 0 )
        regex_pattern++;

    
    else if ( !(strcasecmp(sl->word[2], "contains") == 0) )
    {
        printf("err: %s must be followed by [is|contains|regex]\n", name);
        return 0;
    }

    w1 = sl->word[1];

    both = emalloc( strlen( name ) + strlen( w1 ) + 2 );
    strcpy( both, name );
    strcat( both, " ");
    strcat( both, w1 );

    

    if ( strcasecmp(w1, "pathname") == 0 )
        add_regex_patterns( both, &pathlist->pathname, &(sl->word)[3], regex_pattern );

    else if ( strcasecmp(w1, "filename") == 0 )
        add_regex_patterns( both, &pathlist->filename, &(sl->word)[3], regex_pattern );        
        
    else if ( strcasecmp(w1, "dirname") == 0 )
        add_regex_patterns( both, &pathlist->dirname, &(sl->word)[3], regex_pattern );

    else if ( strcasecmp(w1, "title") == 0 )
        add_regex_patterns( both, &pathlist->title, &(sl->word)[3], regex_pattern );        

    else if ( strcasecmp(w1, "directory") == 0 )
        add_regex_patterns( both, &pathlist->dircontains, &(sl->word)[3], regex_pattern );

    else
    {
        printf("err: '%s' - invalid parameter '%s'\n", both, w1 );
        return 0;
    }


    efree( both );
    return 1;
}    


        
        

        


/* Have we already indexed a file or directory?
** This function is used to avoid multiple index entries
** or endless looping due to symbolic links.
*/

static int     fs_already_indexed(SWISH * sw, char *path)
{
#ifndef NO_SYMBOLIC_FILE_LINKS
    struct dev_ino *p;
    struct stat buf;
    char    key[MAXKEYLEN];     /* Hash key -- allow for 64 bit inodes */
    unsigned hashval;

    if (stat(path, &buf))
        return 0;

    /* Create hash key:  string contains device and inode. */
    /* Avoid snprintf -> MAXKEYLEN is big enough for two longs
       snprintf( key, MAXKEYLEN, "%lx/%lx", (unsigned long)buf.st_dev,
       (unsigned long)buf.st_ino  );
     */
    sprintf(key, "%lx/%lx", (unsigned long) buf.st_dev, (unsigned long) buf.st_ino);

    hashval = bighash(key);     /* Search hash for this file. */
    for (p = sw->Index->inode_hash[hashval]; p != NULL; p = p->next)
        if (p->dev == buf.st_dev && p->ino == buf.st_ino)
        {                       /* We found it. */
            if (sw->verbose >= 3)
                printf("Skipping %s:  %s\n", path, "Already indexed.");
            return 1;
        }

    /* Not found, make new entry. */
    p = (struct dev_ino *) Mem_ZoneAlloc(sw->Index->entryZone,sizeof(struct dev_ino));

    p->dev = buf.st_dev;
    p->ino = buf.st_ino;
    p->next = sw->Index->inode_hash[hashval];
    sw->Index->inode_hash[hashval] = p; /* Aug 1, 2001 -- this is not freed */
#endif

    return 0;
}


/* Recursively goes into a directory and calls the word-indexing
** functions for each file that's found.
*/

static void    indexadir(SWISH * sw, char *dir)
{
    int             allgoodfiles = 0;
    DIR             *dfd;

#ifdef NEXTSTEP
    struct direct   *dp;
#else
    struct dirent   *dp;
#endif
    int             pathbuflen;
    char            *pathname;
    DOCENTRYARRAY   *sortfilelist = NULL;
    DOCENTRYARRAY   *sortdirlist = NULL;
    int             dirlen = strlen( dir );
    struct MOD_FS   *fs = sw->FS;

    if (fs_already_indexed(sw, dir))
        return;

    /* and another stat if not set to follow symlinks */
    if (!fs->followsymlinks && islink(dir))
        return;


    /* logic is not well defined - here we only check dirname */
    /* but that bypasses pathname checks -- but that's checked per-file */
    /* This allows one to override File* directory checks */
    /* but allows a pathname check to be limited to full paths */
    /* This also means you can avoid indexing an entire directory tree with FileRules dirname, */
    /* but using a FileRules pathname allows recursion into the directory */

    /* Reject entire directory due to FileRules dirname */
    if ( *dir && match_regex_list( dir, fs->filerules.dirname, "FileRules dirname" ) )
        return;




    /* Handle "FileRules directory" directive */
    /*  - Check all files within the directory before proceeding -- means reading the directory twice */
    /*  - All files are checked. */

    if (fs->filematch.dircontains || fs->filerules.dircontains )
    {
        if ((dfd = opendir(dir)) == NULL)
        {
            if ( sw->verbose )
                progwarnno("Failed to open dir '%s' :", dir );
            return;
        }

        while ((dp = readdir(dfd)) != NULL)
        {
            if ( match_regex_list( dp->d_name, fs->filerules.dircontains,"FileRules dircontains" ) )
            {
                closedir( dfd );
                return;  /* doesn't recurse */
            }

            if ( match_regex_list( dp->d_name, fs->filematch.dircontains, "FileMatch dircontains" ) )
            {
                allgoodfiles++;
                break;
            }

        }
        closedir(dfd);
    }


    /* Now, build list of files and directories */

    pathbuflen = MAXFILELEN;
    pathname = (char *) emalloc( pathbuflen + 1 ); 

    if ((dfd = opendir(dir)) == NULL)
    {
        if ( sw->verbose )
            progwarnno("Failed to open dir '%s' :", dir );
        return;
    }


    if ( dirlen == 1 && *dir == '/' ) /* case of root dir */
        dirlen = 0;

    while ((dp = readdir(dfd)) != NULL)
    {
        int filelen = strlen( dp->d_name );
        
        /* For security reasons, don't index dot files */
        /* Check for hidden under Windows? */

        if ((dp->d_name)[0] == '.')
            continue;


        /* Build full path to file */

        /* reallocate filename buffer, if needed (dir + path + '/' ) */
        if ( dirlen + filelen + 1 > pathbuflen )
        {
            pathbuflen = dirlen + filelen + 200;
            pathname = (char *) erealloc(pathname, pathbuflen + 1);
        }

        if ( dirlen )
            memcpy(pathname, dir, dirlen);

        pathname[dirlen] = '/';  // Add path separator
        memcpy(pathname + dirlen + 1, dp->d_name, filelen);
        pathname[ dirlen + filelen + 1] = '\0';

        /* Check if the path is a symlink */
        if ( !fs->followsymlinks && islink( pathname ) )
            continue;


        if ( isdirectory(pathname) )
        {
            sortdirlist = (DOCENTRYARRAY *) adddocentry(sortdirlist, pathname);
        }
        else        
        {
            if (fs_already_indexed(sw, pathname))
                continue;

            if ( allgoodfiles || check_FileTests( pathname, &fs->filematch ) ) 
            {
                sortfilelist = (DOCENTRYARRAY *) adddocentry(sortfilelist, pathname);
                continue;
            }


            if (!isoksuffix(dp->d_name, sw->suffixlist))
                continue;


            /* Check FileRules for rejects  */
            if ( check_FileTests( pathname, &fs->filerules ) )
                continue;

            sortfilelist = (DOCENTRYARRAY *) adddocentry(sortfilelist, pathname);
        }
    }

    efree(pathname);

    closedir(dfd);

    printfiles(sw, sortfilelist);
    printdirs(sw, sortdirlist);
}

/* Calls the word-indexing function for a single file.
*/

static void    indexafile(SWISH * sw, char *path)
{
    struct MOD_FS *fs = sw->FS;


    if (!fs->followsymlinks && islink(path))
        return;


    /* This only means "IndexDir test.html test.html test.html" will only index test.html once */
    if (fs_already_indexed(sw, path))
        return;

    /* Check for File|Pathmatch, and index if any match */
    if ( check_FileTests( path, &fs->filematch ) ) 
    {
        printfile(sw, path);
        return;
    }
    
    /* This is likely faster, so do it first */
    if (!isoksuffix(path, sw->suffixlist))
        return;

    /* Check FileRules for rejects  */
    if ( check_FileTests( path, &fs->filerules ) )
        return;


    /* Passed all tests, so index */
    printfile(sw, path);
}

/**********************************************************
* Process FileTests
*
* Returns 1 = something matched
*
**********************************************************/
static int check_FileTests( char *path, PATH_LIST *test )
{
    char *dir;
    char *file;


    if ( match_regex_list( path, test->pathname, "File[Rules|Match] pathname"  ) )
        return 1;

    if ( !( test->dirname || test->filename ) )
        return 0;

    split_path( path, &dir, &file );

    if ( *dir && match_regex_list( dir, test->dirname, "File[Rules|Match] dirname" ) )
    {
        efree( dir );
        efree( file );
        return 1;
    }
    
    if ( *file && match_regex_list( file, test->filename, "File[Rules|Match] filename" ) )
    {
        efree( dir );
        efree( file );
        return 1;
    }

    efree( dir );
    efree( file );
    return 0;
}

/***************************************************
*  Note that this is mostly a duplicate of above,
*  but was designed to work with both path and URLs
*
*  Probably should settle on one
*  Also, this returns "" on empty dirs, where above returns " "
*  Mar 2002 -- and is only called by fs.c...
*  May 2002, moved to fs.c.  Why isn't basename library used for this?
***************************************************/

static void    split_path(char *path, char **directory, char **file)
{
    char   *p1,
           *p2,
           *p3;

    /* look for last DIRDELIMITER (FS) and last / (HTTP) */
    //p1 = strrchr( path, DIRDELIMITER);
    p1 = strrchr( path, '/');
    p2 = strrchr( path, '/');

    if (p1 && p2)
    {                           /* if both are found, use the longest. */
        if (p1 >= p2)
            p3 = p1;
        else
            p3 = p2;
    } else if (p1 && !p2)
        p3 = p1;
    else if (!p1 && p2)
        p3 = p2;
    else
        p3 = NULL;

    /* Set directory */
    if (!p3)
        *directory = (char *) estrdup((char *) "");
    else
    {
        char c = *++p3;

        *p3 = '\0';
        *directory = (char *) estrdup((char *) path);
        *p3 = c;
        path = p3;
    }

    *file = (char *) estrdup((char *) path);
}




/* Indexes the words in the file
*/

static void    printfile(SWISH * sw, char *filename)
{
    char   *s;
    FileProp *fprop;


    if (filename)
    {
        if (sw->verbose >= 3)
        {
            /* Only display file name */
            if ((s = (char *) strrchr(filename, '/')) == NULL)
                printf("  %s", filename);
            else
                printf("  %s", s + 1);
            fflush(stdout);
        }


        fprop = file_properties(filename, filename, sw);

        do_index_file(sw, fprop);

        free_file_properties(fprop);
    }
}

/* 2001-08 Jose Ruiz */
/* function for comparing filenames to get all filenames in a dir sorted
** Original addsortentry used strcmp - So, I use the same routine here
** What about Win32?
*/
int     compfilenames(const void *s1, const void *s2)
{
    char *r1 = *(char * const *) s1;
    char *r2 = *(char * const *) s2;

    return strcmp(r1,r2);
}

/* Indexes the words in all the files in the array of files
** The array is sorted alphabetically
*/

static void    printfiles(SWISH * sw, DOCENTRYARRAY * e)
{
    int     i;

    if (e)
    {
        /* 2001-08 sorting of filenames moved here - Do we really
        ** need to sort them?  - Adjust it in config.h */
        if(e->currentsize)
        {
            if(SORT_FILENAMES)
            {
                swish_qsort(e->filenames, e->currentsize, sizeof(char *), compfilenames);
            }
        }

        for (i = 0; i < e->currentsize; i++)
        {
            printfile(sw, e->filenames[i]);
            efree( e->filenames[i] );
        }

        /* free the array and filenames */
        efree(e->filenames);
        efree(e);
    }
}

/* Prints out the directory names as things are getting indexed.
** Calls indexadir() so directories in the array are indexed,
** in alphabetical order...
*/

void    printdirs(SWISH * sw, DOCENTRYARRAY * e)
{
    int     i;

    if (e)
    {
        /* 2001-08 sorting of dirs moved here - Do we really
        ** need to sort them? - Adjust it in config.h */
        if(e->currentsize)
        {
            if(SORT_FILENAMES)
            {
                swish_qsort(e->filenames, e->currentsize, sizeof(char *), compfilenames);
            }
        }

        for (i = 0; i < e->currentsize; i++)
        {
            if (sw->verbose >= 3)
                printf("\nIn dir \"%s\":\n", e->filenames[i]);
            else if (sw->verbose >= 2)
                printf("Checking dir \"%s\"...\n", e->filenames[i]);

            indexadir(sw, e->filenames[i]);
            efree(e->filenames[i]);
        }
        efree(e->filenames);
        efree(e);
    }
}

/* Stores file names in alphabetical order so they can be
** indexed alphabetically. No big whoop.
*/

static DOCENTRYARRAY *adddocentry(DOCENTRYARRAY * e, char *filename)
{
    if (e == NULL)
    {
        e = (DOCENTRYARRAY *) emalloc(sizeof(DOCENTRYARRAY));
        e->maxsize = VERYBIGHASHSIZE; /* Put what you like */
        e->filenames = (char **) emalloc(e->maxsize * sizeof(char *));

        e->currentsize = 1;
        e->filenames[0] = (char *) estrdup(filename);
    }
    else
    {
        if ((e->currentsize + 1) == e->maxsize)
        {
            e->maxsize += 1000;
            e->filenames = (char **) erealloc(e->filenames, e->maxsize * sizeof(char *));
        }
        e->filenames[e->currentsize++] = (char *) estrdup(filename);
    }
    return e;
}




/********************************************************/
/*					"Public" functions					*/
/********************************************************/

void    fs_indexpath(SWISH * sw, char *path)
{

    normalize_path( path );  /* flip backslashes and remove trailing slash */


    if (isdirectory(path))
    {
        if (sw->verbose >= 2)
            printf("\nChecking dir \"%s\"...\n", path);

        indexadir(sw, path);
    }

    else if (isfile(path))
    {
        if (sw->verbose >= 2)
            printf("\nChecking file \"%s\"...\n", path);
        indexafile(sw, path);
    }
    else
        progwarnno("Invalid path '%s': ", path);
}




struct _indexing_data_source_def FileSystemIndexingDataSource = {
    "File-System",
    "fs",
    fs_indexpath,
    configModule_FS
};
