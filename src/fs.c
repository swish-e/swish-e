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
#include "string.h"
#include "index.h"
#include "hash.h"
#include "mem.h"
#include "file.h"
#include "list.h"
#include "fs.h"
#include "check.h"
#include "error.h"
#include "xml.h"
#include "txt.h"
#include "parse_conffile.h"


#define MAXKEYLEN 34            /* Hash key -- allow for 64 bit inodes */

/*
  -- init structures for this module
*/

void initModule_FS (SWISH  *sw)
{
    struct MOD_FS *fs;

    fs = (struct MOD_FS *) emalloc(sizeof(struct MOD_FS));
    sw->FS = fs;
            /* File system parameters */
    fs->pathconlist=fs->dirconlist=fs->fileconlist=fs->titconlist=fs->fileislist=NULL;
    fs->followsymlinks = 0;

}


/*
  -- release all wired memory for this module
*/

void freeModule_FS (SWISH *sw)
{
  struct MOD_FS *fs = sw->FS;

      /* Free fs parameters */
  if (fs->pathconlist) freeswline(fs->pathconlist);
  if (fs->dirconlist) freeswline(fs->dirconlist);
  if (fs->fileconlist) freeswline(fs->fileconlist);
  if (fs->titconlist) freeswline(fs->titconlist);
  if (fs->fileislist) freeswline(fs->fileislist);

       /* free module data */
  efree (fs);
  sw->FS = NULL;

  return;
}


/*
 -- Config Directives
 -- Configuration directives for this Module
 -- return: 0/1 = none/config applied
*/

int configModule_FS (SWISH *sw, StringList *sl)

{
  struct MOD_FS *fs = sw->FS;
  char *w0    = sl->word[0];
  int  retval = 1;

  if (strcasecmp(w0, "FileRules") == 0)
  {
     if (sl->n > 3)
     {
        char   *w1;
        int     is2_contains = !strcasecmp(sl->word[2], "contains");

        w1 = sl->word[1];

        if (!(strcasecmp(w1, "path") && strcasecmp(w1, "pathname")) && is2_contains)
        {
           grabCmdOptions(sl, 3, &fs->pathconlist);
           retval = 1;
        }
        else if (!strcasecmp(w1, "directory") && is2_contains)
        {
           grabCmdOptions(sl, 3, &fs->dirconlist);
           retval = 1;
        }
        else if (!strcasecmp(w1, "filename") && is2_contains)
        {
           grabCmdOptions(sl, 3, &fs->fileconlist);
           retval = 1;
        }
        else if (!strcasecmp(w1, "title") && is2_contains)
        {
           grabCmdOptions(sl, 3, &fs->titconlist);
           retval = 1;
        }
        else if (!strcasecmp(w1, "filename") && !strcasecmp(sl->word[2], "is"))
        {
           grabCmdOptions(sl, 3, &fs->fileislist);
           retval = 1;
        }
        else
           progerr("Bad parameter in \"FileRules %s %s\"", sl->word[1], sl->word[2]);
     }
     else
        progerr("Bad number of parameters in FileRules");
  }
  else if (strcasecmp(w0, "FollowSymLinks") == 0)
  {
     fs->followsymlinks = getYesNoOrAbort(sl, 1, 1);
  }
  else
  {
     retval = 0;                   /* not a module directive */
  }

  return retval;
}

/* Have we already indexed a file or directory?
** This function is used to avoid multiple index entries
** or endless looping due to symbolic links.
*/

int     fs_already_indexed(SWISH * sw, char *path)
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
    p = (struct dev_ino *) emalloc(sizeof(struct dev_ino));

    p->dev = buf.st_dev;
    p->ino = buf.st_ino;
    p->next = sw->Index->inode_hash[hashval];
    sw->Index->inode_hash[hashval] = p;
#endif

    return 0;
}


/* Recursively goes into a directory and calls the word-indexing
** functions for each file that's found.
*/

void    indexadir(SWISH * sw, char *dir)
{
    int     badfile;
    DIR    *dfd;

#ifdef NEXTSTEP
    struct direct *dp;
#else
    struct dirent *dp;
#endif
    int     lens;
    char   *s;
    DOCENTRYARRAY *sortfilelist,
           *sortdirlist;
    struct swline *tmplist;
    int     ilen1,
            ilen2;
    struct MOD_FS *fs =sw->FS;

    sortfilelist = sortdirlist = NULL;

    if (!fs->followsymlinks && islink(dir))
        return;

    if (fs_already_indexed(sw, dir))
        return;

    if (dir[strlen(dir) - 1] == '/')
        dir[strlen(dir) - 1] = '\0';


    /* Handle "FileRules directory contains" directive */

    if (fs->dirconlist != NULL)
    {
        if ((dfd = opendir(dir)) == NULL)
            return;

        badfile = 0;

        while ((dp = readdir(dfd)) != NULL)
        {
            tmplist = fs->dirconlist;
            while (tmplist != NULL)
            {
                if (matchARegex(dp->d_name, tmplist->line))
                {
                    badfile = 1;
                    break;
                }
                tmplist = tmplist->next;
            }
            if (badfile)
            {
                closedir(dfd);
                return;
            }
        }
        closedir(dfd);
    }

    s = (char *) emalloc((lens = MAXFILELEN) + 1);

    if ((dfd = opendir(dir)) == NULL)
        return;

    while ((dp = readdir(dfd)) != NULL)
    {

        if ((dp->d_name)[0] == '.')
            continue;

        /* This is stating the file name not the path, and is checked later on.
           * if ( !fs->followsymlinks && islink(dp->d_name) )
           * continue;
         */

        /* Handle "FileRules filename is" */
        badfile = 0;
        tmplist = fs->fileislist;
        while (tmplist != NULL)
        {
            if (matchARegex(dp->d_name, tmplist->line))
            {
                badfile = 1;
                break;
            }
            tmplist = tmplist->next;
        }
        if (badfile)
            continue;

        /* Handle "FileRules filename contains" */
        badfile = 0;
        tmplist = fs->fileconlist;
        while (tmplist != NULL)
        {
            if (matchARegex(dp->d_name, tmplist->line))
            {
                badfile = 1;
                break;
            }
            tmplist = tmplist->next;
        }
        if (badfile)
            continue;

        /* Build full path to file */

        ilen1 = strlen(dir);
        ilen2 = strlen(dp->d_name);
        if ((ilen1 + 1 + ilen2) >= lens)
        {
            lens = ilen1 + 1 + ilen2 + 200;
            s = (char *) erealloc(s, lens + 1);
        }
        memcpy(s, dir, ilen1);
        if (dir[ilen1 - 1] != '/')
            s[ilen1++] = '/';
        memcpy(s + ilen1, dp->d_name, ilen2);
        s[ilen1 + ilen2] = '\0';

        /* Check if the path is a symlink */
        if (!fs->followsymlinks && islink(s))
            continue;


        /* FileRules pathname contains */
        badfile = 0;
        tmplist = fs->pathconlist;
        while (tmplist != NULL)
        {
            if (matchARegex(s, tmplist->line))
            {
                badfile = 1;
                break;
            }
            tmplist = tmplist->next;
        }
        if (badfile)
            continue;

        if (!isdirectory(s))
        {
            if (fs_already_indexed(sw, s))
                continue;

            if (!isoksuffix(dp->d_name, sw->suffixlist))
                continue;

            sortfilelist = (DOCENTRYARRAY *) addsortentry(sortfilelist, s);
        }
        else
        {
            sortdirlist = (DOCENTRYARRAY *) addsortentry(sortdirlist, s);
        }
    }

    efree(s);

    closedir(dfd);

    printfiles(sw, sortfilelist);
    printdirs(sw, sortdirlist);
}

/* Calls the word-indexing function for a single file.
*/

void    indexafile(SWISH * sw, char *path)
{
    int     badfile;
    char   *filename;
    struct swline *tmplist;
    struct MOD_FS *fs = sw->FS;

    if (!fs->followsymlinks && islink(path))
        return;

    if (fs_already_indexed(sw, path))
        return;

    if (path[strlen(path) - 1] == '/')
        path[strlen(path) - 1] = '\0';

    badfile = 0;
    tmplist = fs->fileislist;
    while (tmplist != NULL)
    {
        if (!matchARegex(path, tmplist->line))
        {
            badfile = 1;
            break;
        }
        tmplist = tmplist->next;
    }
    if (badfile)
        return;

    badfile = 0;
    tmplist = fs->fileconlist;
    while (tmplist != NULL)
    {
        if (matchARegex(path, tmplist->line))
        {
            badfile = 1;
            break;
        }
        tmplist = tmplist->next;
    }
    if (badfile)
        return;

    badfile = 0;
    tmplist = fs->pathconlist;
    while (tmplist != NULL)
    {
        if (matchARegex(path, tmplist->line))
        {
            badfile = 1;
            break;
        }
        tmplist = tmplist->next;
    }
    if (badfile)
        return;

    if (!isoksuffix(path, sw->suffixlist))
        return;


    filename = (char *) estrdup(path);

    printfile(sw, filename);
}


/* Indexes the words in the file
*/

void    printfile(SWISH * sw, char *filename)
{
    char   *s;
    FileProp *fprop;


    if (filename)
    {
        if (sw->verbose >= 3)
        {
            if ((s = (char *) strrchr(filename, '/')) == NULL)
                printf("  %s", filename);
            else
                printf("  %s", s + 1);
            fflush(stdout);
        }


        fprop = file_properties(filename, filename, sw);
        do_index_file(sw, fprop);


        free_file_properties(fprop);
        efree(filename);
    }
}

/* Indexes the words in all the files in the array of files
** The array is sorted alphabetically
*/

void    printfiles(SWISH * sw, DOCENTRYARRAY * e)
{
    int     i;

    if (e)
    {
        for (i = 0; i < e->currentsize; i++)
            printfile(sw, e->filenames[i]);
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



/********************************************************/
/*					"Public" functions					*/
/********************************************************/

void    fs_indexpath(SWISH * sw, char *path)
{

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
        progwarn("Invalid path '%s'\n", path );
}




struct _indexing_data_source_def FileSystemIndexingDataSource = {
    "File-System",
    "fs",
    fs_indexpath,
    configModule_FS
};
