/*
**
** Copyright (C) 1995, 1996, 1997, 1998 Hewlett-Packard Company
** Originally by Kevin Hughes, kev@kevcom.com, 3/11/94
**
** This program and library is free software; you can redistribute it and/or
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
** ** ** PATCHED 5/13/96, CJC
**
** Added code to countwords and countwordstr to disreguard the last char
** if requiered by the config.h
** G. Hill  3/12/97  ghill@library.berkeley.edu
**
** Changed addentry, countwords, countwordstr, parsecomment, rintindex
** added createMetaEntryList, getMeta, parseMetaData
** to support METADATA
** G. Hill 3/18/97 ghill@library.berkeley.edu
**
** Changed removestops to support printing of stop words
** G. Hill 4/7/97
**
** Changed countwords, countwrdstr, and parseMetaData to disreguard the
** first char if required by the config.h
** G.Hill 10/16/97  ghill@library.berkeley.edu
**
** Added stripIgnoreLastChars and isIgnoreLastChar routines which iteratively
** remove all ignore characters from the end of each word.
** P. Bergner  10/5/97  bergner@lcse.umn.edu
**
** Added stripIgnoreFirstChars and isIgnoreFirstChar to make stripping of
** the ignore first chars iterative.
** G. Hill 11/19/97 ghill@library.berkeley.edu
**
** Added possibility of use of quotes and brackets in meta CONTENT countwords, parsemetadata
** G. Hill 1/14/98
**
** Added regex for replace rule G.Hill 1/98
**
** REQMETANAME - don't index meta tags not specified in MetaNames
** 10/11/99 - Bill Moseley
**
** change sprintf to snprintf to avoid corruption, use MAXPROPLEN instead of literal "20",
** added include of merge.h - missing declaration caused compile error in prototypes,
** added word length arg to Stem() call for strcat overflow checking in stemmer.c
** added safestrcpy() macro to avoid corruption from strcpy overflow
** SRE 11/17/99
**
** fixed misc problems pointed out by "gcc -Wall"
** SRE 2/22/00
**
** Added code for storing word positions in index file 
** Jose Ruiz 3/00 jmruiz@boe.es
**
** 04/00 - Jose Ruiz
** Added code for a hash table in index file for searching words
** via getfileinfo in search.c (Lots of addons). Better perfomance
** with big databases and or heavy searchs (a* or b* or c*)
**
** 04/00 - Jose Ruiz
** Improved number compression function (compress)
** New number decompress function
** Both converted into macros for better performance
**
** 07/00 and 08/00 - Jose Ruiz
** Many modifications to make some functions thread safe
**
** 08/00 - Jose Ruiz
** New function indexstring. Up to now there were 4 functions doing almost
** the same thing: countwords, countwordstr, parseMetaData and parsecomment
** From now on, these 4 functions calls indexstring wich is the common part
** to all of them. In fact, countwordstr, parseMetaData and parsecomment
** are now simple frontends to indexstring
**
** 2000-11 - rasc
** some redesgin, place common index code into a common routine
** FileProp structures, routines
**
** --
** TODO
** $$ there has still to be some resesign to be done.
** $$ swish-e was originally designed to index html only. So the routines
** $$ are for historically reasons scattered
** $$ (e.g. isoktitle (), is ishtml() etc.)
**
** 2000-12 Jose Ruiz
** obsolete routine ishtml removed
** isoktitle moved to html.c
**
** 2001-03-02 rasc   Header: write translatecharacters
** 2001-03-14 rasc   resultHeaderOutput  -H n
** 2001-03-24 rasc   timeroutines rearranged
**
**
*/


#include "swish.h"
#include "string.h"
#include "index.h"
#include "hash.h"
#include "mem.h"
#include "check.h"
#include "search.h"
#include "merge.h"
#include "docprop.h"
#include "stemmer.h"
#include "soundex.h"
#include "error.h"
#include "file.h"
#include "compress.h"
/* Removed due to problems with patents
#include "deflate.h"
*/
#include "html.h"
#include "xml.h"
#include "txt.h"
#include "lst.h"
#include "metanames.h"
#include "result_sort.h"
#include "result_output.h"
#include "filter.h"
#include "date_time.h"
#include "db.h"


/* 
  -- init structures for this module
*/

void initModule_Index (SWISH  *sw)
{
	int i;
    struct MOD_Index *idx;

    idx = (struct MOD_Index *) emalloc(sizeof(struct MOD_Index));
    sw->Index = idx;

	idx->filenum = 0;
	idx->entryArray = NULL;

   	idx->len_compression_buffer = MAXSTRLEN;  /* For example */
	idx->compression_buffer=(unsigned char *)emalloc(idx->len_compression_buffer);

  	idx->len_worddata_buffer = MAXSTRLEN;  /* For example */
	idx->worddata_buffer=(unsigned char *)emalloc(idx->len_worddata_buffer);


		/* Init  entries hash table */
	for (i=0; i<SEARCHHASHSIZE; i++) 
		idx->hashentries[i] = NULL;

	idx->fp_loc_write=idx->fp_loc_read=idx->fp_file_write=idx->fp_file_read=NULL;

	idx->lentmpdir=idx->lenspiderdirectory=MAXSTRLEN;
	idx->tmpdir = (char *)emalloc(idx->lentmpdir + 1);idx->tmpdir[0]='\0';
	idx->spiderdirectory = (char *)emalloc(idx->lenspiderdirectory + 1);idx->spiderdirectory[0]='\0';

	        /* Initialize tmpdir */
	idx->tmpdir = SafeStrCopy(idx->tmpdir,TMPDIR,&idx->lentmpdir);

        /* Initialize spider directory */
    idx->spiderdirectory = SafeStrCopy(idx->spiderdirectory,SPIDERDIRECTORY,&idx->lenspiderdirectory);


	if(idx->tmpdir && idx->tmpdir[0] && isdirectory(idx->tmpdir))
	{
		idx->swap_file_name=tempnam(idx->tmpdir,"swfi");
		idx->swap_location_name=tempnam(idx->tmpdir,"swlo");
	} else {
		idx->swap_file_name=tempnam(NULL,"swfi");
		idx->swap_location_name=tempnam(NULL,"swlo");
	}
   
   return;
}


/* 
  -- release all wired memory for this module
  -- 2001-04-11 rasc
*/

void freeModule_Index (SWISH *sw)
{
  struct MOD_Index *idx = sw->Index;

  if (isfile(idx->swap_file_name))
  {
	  remove(idx->swap_file_name);
	  efree(idx->swap_file_name);
  }
  if (isfile(idx->swap_location_name))
  {
	  remove(idx->swap_location_name);
	  efree(idx->swap_location_name);
  }

  if(idx->lenspiderdirectory) efree(idx->spiderdirectory);		
  if(idx->lentmpdir) efree(idx->tmpdir);		

  		/* Free compression info */	
		/* Free compression buffer */	
  efree(idx->compression_buffer);
        /* free worddata buffer */
  efree(idx->worddata_buffer);

       /* free module data */
  efree (idx);
  sw->Index = NULL;

  return;
}



/*
** ----------------------------------------------
** 
**  Module config code starts here
**
** ----------------------------------------------
*/


/*
 -- Config Directives
 -- Configuration directives for this Module
 -- return: 0/1 = none/config applied
*/

int configModule_Index (SWISH *sw, StringList *sl)

{
  //struct MOD_Index *srch = sw->Index;
  //char *w0    = sl->word[0];
  int  retval = 1;

  return retval;
}

/*
   -- Start the real indexing process for a file.
   -- This routine will be called by the different indexing methods
   -- (httpd, filesystem, etc.)
   -- The indexed file may be the
   --   - real file on filesystem
   --   - tmpfile or work file (shadow of the real file)
   -- Checks if file has to be send thru filter (file stream)
   -- 2000-11-19 rasc
*/


void    do_index_file(SWISH * sw, FileProp * fprop)
{
    int     wordcount;
    char   *rd_buffer = NULL;   /* complete file read into buffer */
    int     external_program = 0;

    wordcount = -1;

    /* skip file is the last_mod date is newer than the check date */

    if (sw->mtime_limit && fprop->mtime < sw->mtime_limit)
    {
        if (sw->verbose >= 4)
            progwarn("Skipping %s: last_mod date is too old\n", fprop->real_path);

        /* external program must seek past this data (fseek fails) */
        if (fprop->fp)
        {
            rd_buffer = read_stream(fprop->real_path, fprop->fp, fprop->fsize, 0);
            efree(rd_buffer);
        }

        return;
    }


    /* Upon entry, if fprop->fp is non-NULL then it's already opened and ready to be read from.
       This is the case with "prog" external programs, except when a filter is selected for the file type.
       If a file is used with "prog" a temporary file was created (fprop->work_file), and will read from that.
       2001-05-13 moseley
    */


    /* to keep from closing the stream used with "prog" */
    if ( fprop->fp )
        external_program++;


    if (fprop->hasfilter)
    {
        fprop->fp = FilterOpen(fprop);

        /* This should be checked in filteropen */
        if ( !fprop->fp )
            progerr("Failed to open filter file!");
    }

    else if ( !fprop->fp )
    {
        /* FIX jmruiz 02/20001 Changed "r" to FILEMODE_READ for WIN32 compatibility */
        fprop->fp = fopen(fprop->work_path, FILEMODE_READ);

        if ( !fprop->fp )
        {
            progwarn("Failed to open: '%s': %s", fprop->work_path, strerror( errno ) );
            return;
        }
    }



    /* -- Read  all data  (len = 0 if filtered...) */
    rd_buffer = read_stream(fprop->real_path, fprop->fp, (fprop->hasfilter) ? 0 : fprop->fsize, sw->truncateDocSize);

    switch (fprop->doctype)
    {

    case TXT:
        if (sw->verbose >= 3)
            printf(" - Using TXT filter - ");
        if (sw->verbose >= 4)
            printf("\n");
        wordcount = countwords_TXT(sw, fprop, rd_buffer);
        break;

    case HTML:
        if (sw->verbose >= 3)
            printf(" - Using HTML filter - ");
        if (sw->verbose >= 4)
            printf("\n");
        wordcount = countwords_HTML(sw, fprop, rd_buffer);
        break;

    case XML:
        if (sw->verbose >= 3)
            printf(" - Using XML filter - ");
        if (sw->verbose >= 4)
            printf("\n");
        wordcount = countwords_XML(sw, fprop, rd_buffer);
        break;

    case LST:
        if (sw->verbose >= 3)
            printf(" - Using LST filter - ");
        if (sw->verbose >= 4)
            printf("\n");
        wordcount = countwords_LST(sw, fprop, rd_buffer);
        break;

    case WML:
        if (sw->verbose >= 3)
            printf(" - Using WML filter - ");
        if (sw->verbose >= 4)
            printf("\n");
        wordcount = countwords_HTML(sw, fprop, rd_buffer);
        break;

    default:
        if (sw->verbose >= 3)
            printf(" - Using DEFAULT filter - ");
        if (sw->verbose >= 4)
            printf("\n");
        wordcount = countwords_HTML(sw, fprop, rd_buffer);
        break;
    }

    if (!external_program)
    {
        if (fprop->hasfilter)
        {
            FilterClose(fprop->fp); /* close filter pipe */
        }
        else
        {
            fclose(fprop->fp); /* close file */
        }
    }

        efree(rd_buffer);

    if (sw->verbose >= 3)
    {
        if (wordcount > 0)
            printf(" (%d words)\n", wordcount);
        else if (wordcount == 0)
            printf(" (no words)\n");
        else if (wordcount == -1)
            printf(" (not opened)\n");
        else if (wordcount == -2)
            printf(" (title is not ok)\n");
        fflush(stdout);
    }

    return;
}









/* Stores file names in alphabetical order so they can be
** indexed alphabetically. No big whoop.
*/

DOCENTRYARRAY *addsortentry(DOCENTRYARRAY * e, char *filename)
{
    int     i,
            j,
            k,
            isbigger;

    isbigger = 0;
    if (e == NULL)
    {
        e = (DOCENTRYARRAY *) emalloc(sizeof(DOCENTRYARRAY));
        e->maxsize = SEARCHHASHSIZE; /* Put what you like */
        e->filenames = (char **) emalloc(e->maxsize * sizeof(char *));

        e->currentsize = 1;
        e->filenames[0] = (char *) estrdup(filename);
    }
    else
    {
        /* Look for the position to insert using a binary search */
        i = e->currentsize - 1;
        j = k = 0;
        while (i >= j)
        {
            k = j + (i - j) / 2;
            isbigger = strcmp(filename, e->filenames[k]);
            if (!isbigger)
                progerr("SWISHE: Congratulations. You have found a bug!! Contact the author");
            else if (isbigger > 0)
                j = k + 1;
            else
                i = k - 1;
        }

        if (isbigger > 0)
            k++;
        e->currentsize++;
        if (e->currentsize == e->maxsize)
        {
            e->maxsize += 1000;
            e->filenames = (char **) erealloc(e->filenames, e->maxsize * sizeof(char *));
        }
        /* for(i=e->currentsize;i>k;i--) e->filenames[i]=e->filenames[i-1]; */
        /* faster!! */
        memmove(e->filenames + k + 1, e->filenames + k, (e->currentsize - 1 - k) * sizeof(char *));

        e->filenames[k] = (char *) estrdup(filename);
    }
    return e;
}

/* Adds a word to the master index tree.
*/

void    addentry(SWISH * sw, char *word, int filenum, int structure, int metaID, int position)
{
    int     l;
    ENTRY  *en,
           *efound;
    LOCATION *tp;
    int     hashval;
    IndexFILE *indexf = sw->indexlist;

    if (sw->verbose >= 4)
    {
        printf("    Adding:'%s' Pos:%d Meta:%d Stuct:%X (", word, position, metaID, structure);
        if ( structure & IN_EMPHASIZED ) printf(" EM");
        if ( structure & IN_HEADER ) printf(" HEADER");
        if ( structure & IN_COMMENTS ) printf(" COMMENT");
		if ( structure & IN_META ) printf(" META");
        if ( structure & IN_BODY ) printf(" BODY");
        if ( structure & IN_HEAD ) printf(" HEAD");
        if ( structure & IN_TITLE ) printf(" TITLE");
        if ( structure & IN_FILE ) printf(" FILE");
        printf(" )\n");
    }

    if (!sw->Index->entryArray)
    {
        sw->Index->entryArray = (ENTRYARRAY *) emalloc(sizeof(ENTRYARRAY));
        sw->Index->entryArray->numWords = 0;
        sw->Index->entryArray->elist = NULL;
    }
    /* Compute hash value of word */
    hashval = searchhash(word);
    /* Look for the word in the hash array */
    for (efound = sw->Index->hashentries[hashval]; efound; efound = efound->nexthash)
        if (strcmp(efound->word, word) == 0)
            break;


    if (!efound)
    {
        en = (ENTRY *) emalloc(sizeof(ENTRY));
        en->word = (char *) estrdup(word);
        en->tfrequency = 1;
        en->locationarray = (LOCATION **) emalloc(sizeof(LOCATION *));
        tp = (LOCATION *) emalloc(sizeof(LOCATION));
        en->currentlocation = 0;
        en->u1.max_locations = 1;
        en->nexthash = sw->Index->hashentries[hashval];
        sw->Index->hashentries[hashval] = en;
        tp->filenum = filenum;
        tp->frequency = 1;
        tp->structure = structure;
        tp->metaID = metaID;
        tp->position[0] = position;

        en->locationarray[0] = tp;

        sw->Index->entryArray->numWords++;
        indexf->header.totalwords++;
    }
    else
    {
        for (l = efound->currentlocation; l < efound->u1.max_locations; l++)
        {
            tp = efound->locationarray[l];
            if (tp->filenum == filenum && tp->metaID == metaID)
                break;
        }
        if (l == efound->u1.max_locations)
        {                       /* filenum and metaname not found */
            efound->locationarray = (LOCATION **) erealloc(efound->locationarray, (++efound->u1.max_locations) * sizeof(LOCATION *));
            tp = (LOCATION *) emalloc(sizeof(LOCATION));
            tp->filenum = filenum;
            tp->frequency = 1;
            tp->structure = structure;
            tp->metaID = metaID;
            tp->position[0] = position;
            efound->locationarray[l] = tp;
            if (efound->locationarray[efound->currentlocation]->filenum != filenum)
            {
                /* Compress previous location data */
                CompressPrevLocEntry(sw, indexf, efound);
                efound->tfrequency++;
                efound->currentlocation = l;
            }
        }
        else
        {                       /* Found filenum and metaname */
            tp = efound->locationarray[l];
            tp = erealloc(tp, sizeof(LOCATION) + tp->frequency * sizeof(int));

            tp->position[tp->frequency++] = position;
            tp->structure |= structure;
            efound->locationarray[l] = tp;
        }
    }
}

/* Adds a file to the master list of files and file numbers.
*/

void    addtofilelist(SWISH * sw, IndexFILE * indexf, char *filename, time_t mtime, char *title, char *summary, int start, int size,
                      struct file **newFileEntry)
{
    struct file *newnode;
    unsigned char *ruleparsedfilename,
           *p1,
           *p2,
           *p3;
    unsigned char c;
    struct metaEntry *q;
    unsigned long int tmp;

    if (!indexf->filearray || !indexf->filearray_maxsize)
    {
        indexf->filearray = (struct file **) emalloc((indexf->filearray_maxsize = BIGHASHSIZE) * sizeof(struct file *));
        indexf->header.filetotalwordsarray = (int *) emalloc(indexf->filearray_maxsize * sizeof(int));

        indexf->filearray_cursize = 0;
    }
    if (indexf->filearray_maxsize == indexf->filearray_cursize)
    {
        indexf->filearray = (struct file **) erealloc(indexf->filearray, (indexf->filearray_maxsize += 1000) * sizeof(struct file *));
        indexf->header.filetotalwordsarray = (int *) erealloc(indexf->header.filetotalwordsarray, indexf->filearray_maxsize * sizeof(long));
    }

    newnode = (struct file *) emalloc(sizeof(struct file));

    if (newFileEntry != NULL)
    {
        *newFileEntry = newnode; /* pass object pointer up to caller */
    }
    ruleparsedfilename = ruleparse(sw, filename);
    /* look for last DIRDELIMITER (FS) and last / (HTTP) */
    p1 = strrchr(ruleparsedfilename, DIRDELIMITER);
    p2 = strrchr(ruleparsedfilename, '/');
    if (p1 && p2)
    {
        if (p1 >= p2)
            p3 = p1;
        else
            p3 = p2;
    }
    else if (p1 && !p2)
        p3 = p1;
    else if (!p1 && p2)
        p3 = p2;
    else
        p3 = NULL;
    if (!p3)
    {
        newnode->fi.lookup_path = get_lookup_path(&indexf->header.pathlookup, "");
    }
    else
    {
        c = *++p3;
        *p3 = '\0';
        newnode->fi.lookup_path = get_lookup_path(&indexf->header.pathlookup, ruleparsedfilename);
        *p3 = c;
        ruleparsedfilename = p3;
    }

    newnode->fi.filenum = indexf->filearray_cursize + 1; /* filenum starts in 1 */
    newnode->fi.filename = (char *) estrdup(ruleparsedfilename);
    /* Not used in indexing mode - They are in properties */
    /* NULL must be set to not get a segfault in freefileinfo */
    newnode->fi.title = newnode->fi.summary = NULL;
    /* Init docproperties */
    newnode->docProperties = NULL;

/* #### Added summary,filename, title and mtime as properties if specified */

    if (filename)
    {
        /* Check if filename is internal swish metadata */
        if ((q = getMetaNameData(&indexf->header, AUTOPROPERTY_DOCPATH)))
        {
            /* Check if it is also a property (META_PROP flag) */
            if (is_meta_property(q))
            {
                addDocProperty(&newnode->docProperties, q->metaID, filename, strlen(filename));
            }
            /* Perhaps we want it to be indexed ... */
            if (is_meta_index(q))
            {
                int     metaID[1],
                        positionMeta[1];

                metaID[0] = q->metaID;
                positionMeta[0] = 1;
                indexstring(sw, filename, sw->filenum, IN_FILE, 1, metaID, positionMeta);
            }
        }
    }
    if (title)
    {
        /* Check if title is internal swish metadata */
        if ((q = indexf->header.titleProp))
        {
            /* Check if it is also a property (META_PROP flag) */
            if (is_meta_property(q))
            {
                addDocProperty(&newnode->docProperties, q->metaID, title, strlen(title));
            }
            /* Perhaps we want it to be indexed ... */
            if (is_meta_index(q))
            {
                int     metaID[1],
                        positionMeta[1];

                metaID[0] = q->metaID;
                positionMeta[0] = 1;
                indexstring(sw, title, sw->filenum, IN_FILE, 1, metaID, positionMeta);
            }
        }
    }
    if (summary)
    {
        /* Check if summary is internal swish metadata */
        if ((q = indexf->header.summaryProp))
        {
            /* Check if it is also a property (META_PROP flag) */
            if (is_meta_property(q))
            {
                addDocProperty(&newnode->docProperties, q->metaID, summary, strlen(summary));
            }
            /* Perhaps we want it to be indexed ... */
            if (is_meta_index(q))
            {
                int     metaID[1],
                        positionMeta[1];

                metaID[0] = q->metaID;
                positionMeta[0] = 1;
                indexstring(sw, summary, sw->filenum, IN_FILE, 1, metaID, positionMeta);
            }
        }
    }
    /* Check if filedate is an internal swish metadata */
    if ((q = indexf->header.filedateProp))
    {
        /* Check if it is also a property (META_PROP flag) */
        if (is_meta_property(q))
        {
            tmp = mtime;
            PACKLONG(tmp);      /* make it portable */
            addDocProperty(&newnode->docProperties, q->metaID, (unsigned char *) &tmp, sizeof(tmp));
        }
    }
    /* Check if size is internal swish metadata */
    if ((q = indexf->header.sizeProp))
    {
        /* Check if it is also a property (META_PROP flag) */
        if (is_meta_property(q))
        {
            tmp = size;
            PACKLONG(tmp);      /* make it portable */
            addDocProperty(&newnode->docProperties, q->metaID, (unsigned char *) &tmp, sizeof(tmp));
        }
    }

    /* Check if size is internal swish metadata */
    if ((q = indexf->header.startProp))
    {
        /* Check if it is also a property (META_PROP flag) */
        if (is_meta_property(q))
        {
            tmp = start;
            PACKLONG(tmp);      /* make it portable */
            addDocProperty(&newnode->docProperties, q->metaID, (unsigned char *) &tmp, sizeof(tmp));
        }
    }
/* #### */
    indexf->filearray[indexf->filearray_cursize++] = newnode;
	indexf->header.totalfiles++;

}

/* Just goes through the master list of files and
** counts 'em.
*/

int     getfilecount(IndexFILE * indexf)
{
    return indexf->filearray_cursize;
}


/* Indexes the words in a string, such as a file name or an
** HTML title.
*/

int     countwordstr(SWISH * sw, char *s, int filenum)
{
    int     position = 1;       /* Position of word */
    int     metaID = 1;
    int     structure = IN_FILE;

    return indexstring(sw, s, filenum, structure, 1, &metaID, &position);
}



/* Removes words that occur in over _plimit_ percent of the files and
** that occur in over _flimit_ files (marks them as stopwords, that is).
*/
/* 05/00 Jose Ruiz
** Recompute positions when a stopword is removed from lists
** This piece of code is terrorific because the first goal
** was getting the best possible performace. So, the code is not
** very clear.
** The main problem is to recalculate word positions for all
** the words after removing the automatic stop words. This means
** looking at all word's positions for each automatic stop word
** and decrement its position
*/
/* 2001/02 jose ruiz - rewritten - all the proccess is made in one pass to achieve
better performance */


int     removestops(SWISH * sw)
{
    struct filepos
    {
        int     n;              /* Number of entries per file */
        int    *pos;            /* Store metaID1,position1, metaID2,position2 ..... */
    };
    int     i,
            j,
            k,
            m,
            n,
            percent,
            stopwords;
    LOCATION *lp;
    ENTRY  *e;
    ENTRY  *ep,
           *ep2;
    ENTRY **estop = NULL;
    int     estopsz = 0,
            estopmsz = 0;
    int     hashval;
    struct swline *sp;
    int     modified,
            totalwords;
    IndexFILE *indexf = sw->indexlist;
    int     totalfiles = getfilecount(indexf);
    struct filepos **filepos = NULL;
    struct filepos *fpos;


    /* Now let's count the current number of stopwords!!
     */

    for (stopwords = 0, hashval = 0; hashval < HASHSIZE; hashval++)
        for (sp = indexf->header.hashstoplist[hashval]; sp; sp = sp->next)
            stopwords++;

    totalwords = indexf->header.totalwords;

    if (!totalwords || sw->plimit >= NO_PLIMIT)
        return stopwords;

    if (sw->verbose)
        printf("Warning: This proccess can take some time. For a faster one, use IgnoreWords instead of IgnoreLimit\n");

    if (!estopmsz)
    {
        estopmsz = 1;
        estop = (ENTRY **) emalloc(estopmsz * sizeof(ENTRY *));
    }
    /* this is the easy part: Remove the automatic stopwords from
       ** the hash array */
    for (i = 0; i < SEARCHHASHSIZE; i++)
    {
        for (ep2 = NULL, ep = sw->Index->hashentries[i]; ep; ep = ep->nexthash)
        {
            percent = (ep->tfrequency * 100) / totalfiles;
            if (percent >= sw->plimit && ep->tfrequency >= sw->flimit)
            {
                addStopList(&indexf->header, ep->word);
                addstophash(&indexf->header, ep->word);
                stopwords++;
                /* Remove entry from  the hash array */
                if (ep2)
                    ep2->nexthash = ep->nexthash;
                else
                    sw->Index->hashentries[i] = ep->nexthash;
                totalwords--;
                sw->Index->entryArray->numWords--;
                indexf->header.totalwords--;
                if (estopsz == estopmsz)
                {               /* More memory? */
                    estopmsz *= 2;
                    estop = (ENTRY **) erealloc(estop, estopmsz * sizeof(ENTRY *));
                }
                /* estop is an array for storing the
                   ** automatic stopwords */
                estop[estopsz++] = ep;
            }
            else
                ep2 = ep;
        }
    }

    /* If we have automatic stopwords we have to recalculate
       ** word positions */
    if (estopsz)
    {
        /* Build an array with all the files positions to be removed */
        filepos = (struct filepos **) emalloc(totalfiles * sizeof(struct filepos *));

        for (i = 0; i < totalfiles; i++)
            filepos[i] = NULL;
        for (i = 0; i < estopsz; i++)
        {
            e = estop[i];
            for (j = 0; j < e->u1.max_locations; j++)
            {
                if (j < e->currentlocation)
                    lp = uncompress_location(sw, indexf, (unsigned char *) e->locationarray[j]);
                else
                    lp = e->locationarray[j];
                if (!filepos[lp->filenum - 1])
                {
                    fpos = (struct filepos *) emalloc(sizeof(struct filepos));

                    fpos->n = lp->frequency;
                    fpos->pos = (int *) emalloc(fpos->n * 2 * sizeof(int));

                    for (k = 0; k < fpos->n; k++)
                    {
                        m = k * 2;
                        fpos->pos[m++] = lp->metaID;
                        fpos->pos[m] = lp->position[k];
                    }
                    filepos[lp->filenum - 1] = fpos;
                }
                else
                {
                    fpos = filepos[lp->filenum - 1];
                    fpos->pos = (int *) erealloc(fpos->pos, (fpos->n + lp->frequency) * 2 * sizeof(int));

                    for (k = 0; k < lp->frequency; k++)
                    {
                        m = (fpos->n + k) * 2;
                        fpos->pos[m++] = lp->metaID;
                        fpos->pos[m] = lp->position[k];
                    }
                    fpos->n += lp->frequency;
                }
            }

            /* Free Memory used by stopword */
            efree(e->word);
            for (m = 0; m < e->u1.max_locations; m++)
            {
                efree(e->locationarray[m]);
            }
            efree(e);
        }
        /* sort each file entries by metaname/position */
        for (i = 0; i < totalfiles; i++)
        {
            if (filepos[i])
                qsort(filepos[i]->pos, filepos[i]->n, 2 * sizeof(int), &icomp2);
        }

        /* Now we need to recalculate all positions
           ** of words because we have removed the
           ** word in the index array */
        /* Sorry for the code but it is the fastest
           ** I could achieve!! */
        for (i = 0; i < SEARCHHASHSIZE; i++)
        {
            for (ep = sw->Index->hashentries[i]; ep; ep = ep->nexthash)
            {
                if (sw->verbose >= 3)
                {
                    printf("Computing new positions for %s (%d occurrences)                        \r", ep->word, ep->u1.max_locations);
                    fflush(stdout);
                }
                for (j = 0, modified = 0; j < ep->u1.max_locations; j++)
                {
                    if (j < ep->currentlocation)
                        lp = uncompress_location(sw, indexf, (unsigned char *) ep->locationarray[j]);
                    else
                        lp = ep->locationarray[j];
                    if (filepos[lp->filenum - 1])
                    {
                        fpos = filepos[lp->filenum - 1];
                        for (m = 0, n = 0; m < lp->frequency; m++)
                        {
                            /* Search metaID */
                            for (; n < fpos->n; n++)
                            {
                                if (fpos->pos[n * 2] >= lp->metaID)
                                    break;
                            }
                            if (n == fpos->n || fpos->pos[n * 2] > lp->metaID)
                                break; /* Not found */
                            k = n;
                            for (; n < fpos->n; n++)
                            {
                                if (fpos->pos[n * 2] != lp->metaID)
                                    break;
                                if (fpos->pos[n * 2 + 1] >= lp->position[m])
                                    break;
                            }
                            if (n > k)
                            {
                                modified = 1;
                                lp->position[m] -= (n - k);
                            }
                        }
                    }

                    /* Restore array of positions to its original state */
                    if (j < ep->currentlocation)
                    {
                        if (modified)
                        {
                            efree(ep->locationarray[j]);
                            ep->locationarray[j] = (LOCATION *) compress_location(sw, indexf, lp);
                        }
                        else
                            efree(lp);
                    }
                }
            }
        }
        /* Free the memory used by the table of files */
        for (i = 0; i < totalfiles; i++)
        {
            if (filepos[i])
            {
                efree(filepos[i]->pos);
                efree(filepos[i]);
            }
        }

    }
    efree(estop);

    return stopwords;
}


/* This is somewhat similar to the rank calculation algorithm
** from WAIS (I think). Any suggestions for improvements?
** Note that ranks can't be smaller than 1.
*/

int getrank(SWISH * sw, int freq, int tfreq, int words, int structure, int ignoreTotalWordCountWhenRanking)
{
    double  rank;
    int     irank;
    double  emphasized;

    if ((EMPHASIZECOMMENTS && (structure & IN_COMMENTS)) || (structure & (IN_HEADER | IN_TITLE | IN_META)))
        emphasized = 5.0;
    else
        emphasized = 1.0;

    rank = 1.0 / (double) tfreq;
    rank = (log((double) freq) + 10.0) * rank;

    if (ignoreTotalWordCountWhenRanking)
		rank /= 100.0;
    else
        rank /= words;

    rank = rank * 10000.0 * 100.0 * emphasized + 0.5;

    irank = (int) rank;
    if (irank <= 0)
        irank = 1;

    return irank;
}




/* -------------------------------------------- */
/* -------------------------------------------- */
/* Sorts the words and compress the last entry data
*/

void    sort_words(SWISH * sw, IndexFILE * indexf)
{
    int     i;
    ENTRYARRAY *ep;
    ENTRY  *epi;
    int     totalwords;


    BuildSortedArrayOfWords(sw, indexf);
    ep = sw->Index->entryArray;
    if (ep)
    {
        totalwords = ep->numWords;
        for (i = 0; i < totalwords; i++)
        {
            epi = ep->elist[i];
            if (!isstopword(&indexf->header, epi->word))
            {
                /* Compress remaining data */
                CompressCurrentLocEntry(sw, indexf, epi);
            }
        }
    }
}



/* Sort entry by MetaName, FileNum */
void    sortentry(SWISH * sw, IndexFILE * indexf, ENTRY * e)
{
    int     i,
            j,
            k,
            num;
    unsigned char *ptmp,
           *ptmp2,
           *compressed_data;
    int    *pi = NULL;

    /* Very trivial case */
    if (!e)
        return;

    if (!(i = e->u1.max_locations))
    {
        return;
    }
    /* Compute array wide */
    j = 2 * sizeof(int) + sizeof(void *);

    /* Compute array size */
    ptmp = (void *) emalloc(j * i);
    /* Build an array with the elements to compare
       and pointers to data */
    for (k = 0, ptmp2 = ptmp; k < i; k++)
    {
        pi = (int *) ptmp2;
        if (sw->swap_flag)
            e->locationarray[k] = (LOCATION *) unSwapLocData(sw, (long) e->locationarray[k]);
        compressed_data = (char *) e->locationarray[k];
        uncompress2(num, compressed_data); /* index to lookuptable */
        pi[0] = indexf->header.locationlookup->all_entries[num - 1]->val[0];
        uncompress2(num, compressed_data); /* filenum */
        pi[1] = num;
        ptmp2 += 2 * sizeof(int);

        memcpy((char *) ptmp2, (char *) &e->locationarray[k], sizeof(LOCATION *));
        ptmp2 += sizeof(void *);
    }
    /* Sort them */
    qsort(ptmp, i, j, &icomp2);
    /* Store results */
    for (k = 0, ptmp2 = ptmp; k < i; k++)
    {
        ptmp2 += 2 * sizeof(int);

        memcpy((char *) &e->locationarray[k], (char *) ptmp2, sizeof(LOCATION *));
        ptmp2 += sizeof(void *);
    }
    /* Free the memory of the array */
    efree(ptmp);
}


/* Write the index entries that hold the word, rank, and other information.
*/

void    write_index(SWISH * sw, IndexFILE * indexf)
{
    int     i;
    ENTRYARRAY *ep;
    ENTRY  *epi;
    int     totalwords;


    ep = sw->Index->entryArray;
    if (ep)
    {
        totalwords = ep->numWords;

        DB_InitWriteWords(sw, indexf->DB);

        for (i = 0; i < totalwords; i++)
        {
            epi = ep->elist[i];
            if (!isstopword(&indexf->header, epi->word))
            {
                /* Sort locationlist by MetaName, Filenum
                   ** for faster search */
                sortentry(sw, indexf, epi);
                /* Write word to index file */
                write_word(sw, epi, indexf);
            }
            else
                epi->u1.fileoffset = -1L;
        }	
		
		
	    for (i = 0; i < SEARCHHASHSIZE; i++)
		{
			if ((epi = sw->Index->hashentries[i]))
			{
				while (epi)
				{
						/* If it is not a stopword write it */
					if (epi->u1.fileoffset >= 0L)  
						DB_WriteWordHash(sw, epi->word,epi->u1.fileoffset,indexf->DB);
					epi = epi->nexthash;
				}
			}
		}


        for (i = 0; i < totalwords; i++)
        {
            epi = ep->elist[i];
            if (epi->u1.fileoffset >= 0L)
            {
                write_worddata(sw, epi, indexf);
            }
            efree(epi->word);
            efree(epi);
        }
		
	DB_EndWriteWords(sw, indexf->DB);

    }
}



unsigned char *buildFileEntry(char *filename, struct docPropertyEntry **docProperties, int lookup_path, int *sz_buffer)
{
    int     len_filename;
    unsigned char *buffer1,
           *buffer2,
           *buffer3,
           *p;
    int     lenbuffer1;
    int     datalen1,
            datalen2,
            datalen3;

    len_filename = strlen(filename) + 1;
    lenbuffer1 = len_filename + 2 * 6;
    buffer1 = emalloc(lenbuffer1);
    p = buffer1;
    lookup_path++;              /* To avoid the 0 problem in compress increase 1 */
    compress3(lookup_path, p);
    /* We store length +1 to avoid problems with 0 length - So 
       it also writes the null terminator */
    compress3(len_filename, p);
    memcpy(p, filename, len_filename);
    p += len_filename;
    datalen1 = p - buffer1;
    buffer2 = storeDocProperties(*docProperties, &datalen2);
    buffer3 = emalloc((datalen3 = datalen1 + datalen2 + 1));
    memcpy(buffer3, buffer1, datalen1);
    if (datalen2)
        memcpy(buffer3 + datalen1, buffer2, datalen2);
    buffer3[datalen1 + datalen2] = '\0';
    efree(buffer1);
    efree(buffer2);
    *sz_buffer = datalen3;
    return (buffer3);
}

struct file *readFileEntry(SWISH *sw, IndexFILE * indexf, int filenum)
{
    int     total_len,
            len1,
            len4,
            lookup_path;
    char   *buffer,
           *p;
    char   *buf1;
    struct file *fi;

    fi = indexf->filearray[filenum - 1];
    if (fi)
        return fi;              /* Read it previously */

    fi = (struct file *) emalloc(sizeof(struct file));

    fi->fi.filename = NULL;
    fi->fi.title = NULL;
    fi->fi.summary = NULL;
    fi->fi.start = 0;
    fi->fi.size = 0;
    fi->docProperties = NULL;

    indexf->filearray[filenum - 1] = fi;



	/* Removed due to problems with patents
    if (indexf->header.applyFileInfoCompression)
    {
        buffer = p = zfread(indexf->dict, &total_len, fp);
    }
    else
    {
	*/
		DB_ReadFile(sw, filenum, &buffer, &total_len, indexf->DB);
		p = buffer;
	/* } */

    uncompress2(lookup_path, p); /* Index to lookup table of paths */
    lookup_path--;
    uncompress2(len1, p);       /* Read length of filename */
    buf1 = emalloc(len1);       /* Includes NULL terminator */
    memcpy(buf1, p, len1);      /* Read filename */
    p += len1;

    fi->fi.lookup_path = lookup_path;
    /* Add the path to filename */
    len4 = strlen(indexf->header.pathlookup->all_entries[lookup_path]->val);
    len1 = strlen(buf1);
    fi->fi.filename = emalloc(len4 + len1 + 1);
    memcpy(fi->fi.filename, indexf->header.pathlookup->all_entries[lookup_path]->val, len4);
    memcpy(fi->fi.filename + len4, buf1, len1);
    fi->fi.filename[len1 + len4] = '\0';
    efree(buf1);

    fi->fi.filenum = filenum - 1;
    /* read the document properties section  */
    fi->docProperties = fetchDocProperties(p);

    /* Read internal swish properties */
    /* first init them */
    fi->fi.mtime = (unsigned long) 0L;
    fi->fi.title = NULL;
    fi->fi.summary = NULL;
    fi->fi.start = 0;
    fi->fi.size = 0;

    /* Read values */
    getSwishInternalProperties(fi, indexf);

    /* Add empty strings if NULL */
    if (!fi->fi.title)
        fi->fi.title = estrdup("");
    if (!fi->fi.summary)
        fi->fi.summary = estrdup("");

    efree(buffer);
    return fi;
}

/* Writes the list of files, titles, and sizes into the DB index
Also sorts properties
*/

void    write_file_list(SWISH * sw, IndexFILE * indexf)
{
    int     i;
    struct file *filep;
    unsigned char *buffer;
    int     sz_buffer;

    /* Deflate studd removed ...
    struct buffer_pool *bp = NULL;
    */

	DB_InitWriteFiles(sw, indexf->DB);

    for (i = 0; i < indexf->filearray_cursize; i++)
    {
        if (sw->swap_flag)
        {
            filep = unSwapFileData(sw);
            filep->fi.filenum = i + 1;
            indexf->filearray[i] = filep;
        }
        else
            filep = indexf->filearray[i];
        buffer = buildFileEntry(filep->fi.filename, &filep->docProperties, filep->fi.lookup_path, &sz_buffer);
		/* Deflate stuff removed due to patents 
        if (indexf->header.applyFileInfoCompression)
        {
            bp = zfwrite(bp, buffer, sz_buffer, &indexf->fileoffsetarray[i], fp);
        }
        else
        {
		*/
			DB_WriteFile(sw, i, buffer, sz_buffer, indexf->DB);
        /* } */
        efree(buffer);
    }
	/* Deflate stuff removed due to patents 
    if (indexf->header.applyFileInfoCompression)
    {
        zfflush(bp, fp);
        printdeflatedictionary(bp, indexf);
    } */
    /* Sort properties -> Better search performance */
    sortFileProperties(indexf);
    /* Free memory */
    for (i = 0; i < indexf->filearray_cursize; i++)
    {
        freefileinfo(indexf->filearray[i]);
        indexf->filearray[i] = NULL;
    }
	DB_EndWriteFiles(sw, indexf->DB);
}


/* Writes the sorted indexes to DB index */
void    write_sorted_index(SWISH * sw, IndexFILE * indexf)
{
	int i,j,val;
    struct metaEntry *m;
	unsigned char  *CompressedSortFileProps , *s;
	
	CompressedSortFileProps = (unsigned char *)emalloc(5 * indexf->filearray_cursize);

	DB_InitWriteSortedIndex(sw, indexf->DB);
    /* Execute for each property */
    for ( j = 0; j < indexf->header.metaCounter; j++)
    {
		m = getMetaIDData(&indexf->header, indexf->header.metaEntryArray[j]->metaID);
		if (m->sorted_data)
		{
			s = CompressedSortFileProps;
			for(i=0;i<indexf->filearray_cursize;i++)
			{
				val = m->sorted_data[i];
				compress3(val,s);
			}
			DB_WriteSortedIndex(sw, m->metaID,CompressedSortFileProps, s - CompressedSortFileProps, indexf->DB);
		}
	}
	efree(CompressedSortFileProps);
	DB_EndWriteSortedIndex(sw, indexf->DB);

}


/* Parses lines according to the ReplaceRules directives.
*/

char   *ruleparse(SWISH * sw, char *line)
{
    static int lenrule = 0;
    static char *rule = NULL;
    static int lentmpline = 0;
    static char *tmpline = NULL;
    static int lennewtmpline = 0;
    static char *newtmpline = NULL;
    static int lenline1 = 0;
    static char *line1 = NULL;
    static int lenline2 = 0;
    static char *line2 = NULL;
    struct swline *tmplist = NULL;
    int     ilen1,
            ilen2;

    if (!lenrule)
        rule = (char *) emalloc((lenrule = MAXSTRLEN) + 1);
    if (!lentmpline)
        tmpline = (char *) emalloc((lentmpline = MAXSTRLEN) + 1);
    if (!lennewtmpline)
        newtmpline = (char *) emalloc((lennewtmpline = MAXSTRLEN) + 1);
    if (!lenline1)
        line1 = (char *) emalloc((lenline1 = MAXSTRLEN) + 1);
    if (!lenline2)
        line2 = (char *) emalloc((lenline2 = MAXSTRLEN) + 1);

    if (sw->replacelist == NULL)
        return line;

    tmplist = sw->replacelist;
    tmpline = SafeStrCopy(tmpline, line, &lentmpline);
    while (1)
    {
        if (tmplist == NULL)
            return tmpline;
        rule = SafeStrCopy(rule, tmplist->line, &lenrule);
        tmplist = tmplist->next;
        if (tmplist == NULL)
            return tmpline;
        if (rule == NULL)
        {
            sw->replacelist = tmplist;
            return tmpline;
        }
        else
        {
            if (lstrstr(rule, "replace"))
            {
                line1 = SafeStrCopy(line1, tmplist->line, &lenline1);
                tmplist = tmplist->next;
                if (tmplist)
                {
                    line2 = SafeStrCopy(line2, tmplist->line, &lenline2);
                    tmplist = tmplist->next;
                }
                else
                {
                    /* Handle case where 2nd part of replace rule
                       ** is an empty string. Config-file parsing
                       ** idiosyncrasies cause a replace of "x" to ""
                       ** to incompletely represent the rule.
                     */
                    line2[0] = '\0';
                }
                newtmpline = SafeStrCopy(newtmpline, (char *) matchAndChange(tmpline, line1, line2), &lennewtmpline);
            }
            else if (lstrstr(rule, "append"))
            {
                ilen1 = strlen(tmpline);
                ilen2 = strlen(tmplist->line);
                if ((ilen1 + ilen2) >= lennewtmpline)
                {
                    lennewtmpline = ilen1 + ilen2 + 200;
                    newtmpline = erealloc(newtmpline, lennewtmpline + 1);
                }
                memcpy(newtmpline, tmpline, ilen1);
                memcpy(newtmpline + ilen1, tmplist->line, ilen2);
                newtmpline[ilen1 + ilen2] = '\0';
                tmplist = tmplist->next;
            }
            else if (lstrstr(rule, "prepend"))
            {
                ilen1 = strlen(tmpline);
                ilen2 = strlen(tmplist->line);
                if ((ilen1 + ilen2) >= lennewtmpline)
                {
                    lennewtmpline = ilen1 + ilen2 + 200;
                    newtmpline = erealloc(newtmpline, lennewtmpline + 1);
                }
                memcpy(newtmpline, tmplist->line, ilen2);
                memcpy(newtmpline + ilen2, tmpline, ilen1);
                newtmpline[ilen1 + ilen2] = '\0';
                tmplist = tmplist->next;
            }
            else if (lstrstr(rule, "remove"))
            {
                newtmpline = SafeStrCopy(newtmpline, (char *) matchAndChange(tmpline, tmplist->line, ""), &lennewtmpline);
            }
            tmpline = SafeStrCopy(tmpline, newtmpline, &lentmpline);
        }
    }
}


/*  These 2 routines fix the problem when a word ends with mutiple
**  IGNORELASTCHAR's (eg, qwerty'. ).  The old code correctly deleted
**  the ".", but didn't check if the new last character ("'") is also
**  an ignore character.
*/
void     stripIgnoreLastChars(INDEXDATAHEADER *header, char *word)
{
    int     k,j,i = strlen(word);

    /* Get rid of specified last char's */
    /* for (i=0; word[i] != '\0'; i++); */
    /* Iteratively strip off the last character if it's an ignore character */
    while ((i > 0) && (isIgnoreLastChar(header, word[--i])))
    {
        word[i] = '\0';

        /* We must take care of the escaped characeters */
        /* Things like hello\c hello\\c hello\\\c can appear */
        for(j=0,k=i-1;k>=0 && word[k]=='\\';k--,j++);
		
        /* j contains the number of \ */
        if(j%2)   /* Remove the escape if even */
        {
             word[--i]='\0';    
        }
    }
}

void    stripIgnoreFirstChars(INDEXDATAHEADER *header, char *word)
{
    int     j,
            k;
    int     i = 0;

    /* Keep going until a char not to ignore is found */
	/* We must take care of the escaped characeters */
	/* Things like \chello \\chello can appear */

    while (word[i])
    {
        if(word[i]=='\\')   /* Jump escape */
            k=i+1;
        else
            k=i;
        if(!word[k] || !isIgnoreFirstChar(header, word[k]))
            break;
        else
            i=k+1;
    }

    /* If all the char's are valid, just return */
    if (0 == i)
        return;
    else
    {
        for (k = i, j = 0; word[k] != '\0'; j++, k++)
        {
            word[j] = word[k];
        }
        /* Add the NULL */
        word[j] = '\0';
    }
}



int     entrystructcmp(const void *e1, const void *e2)
{
    const ENTRY *ep1 = *(ENTRY * const *) e1;
    const ENTRY *ep2 = *(ENTRY * const *) e2;

    return (strcmp(ep1->word, ep2->word));
}


/* Builds a sorted array with all of the words */
void    BuildSortedArrayOfWords(SWISH * sw, IndexFILE * indexf)
{
    int     i,
            j;
    ENTRY  *e;

    if (sw->verbose)
    {
        printf("Sorting Words alphabetically\n");
        fflush(stdout);
    }
    if (!sw->Index->entryArray || !sw->Index->entryArray->numWords)
        return;

    /* Build the array with the pointers to the entries */
    sw->Index->entryArray->elist = (ENTRY **) emalloc(sw->Index->entryArray->numWords * sizeof(ENTRY *));
    /* Fill the array with all the entries */
    for (i = 0, j = 0; i < SEARCHHASHSIZE; i++)
        for (e = sw->Index->hashentries[i]; e; e = e->nexthash)
            sw->Index->entryArray->elist[j++] = e;
    /* Sort them */
    qsort(sw->Index->entryArray->elist, sw->Index->entryArray->numWords, sizeof(ENTRY *), &entrystructcmp);
}


/* 
** Jose Ruiz 04/00
** Store a portable long with just four bytes
*/
void    printlong(FILE * fp, long num)
{
    PACKLONG(num);              /* Make the number portable */
    fwrite(&num, MAXLONGLEN, 1, fp);
}

/* 
** Jose Ruiz 04/00
** Read a portable long (just four bytes)
*/
long    readlong(FILE * fp)
{
    long    num;

    fread(&num, MAXLONGLEN, 1, fp);
    UNPACKLONG(num);            /* Make the number readable */
    return (num);
}


void addword( char *word, int *bump_position_flag, SWISH * sw, int filenum, int structure, int numMetaNames, int *metaID, int *position)
{
    int     i;

    if ( *bump_position_flag )
    {
        for (i = 0; i < numMetaNames; i++)
            position[i]++;

        *bump_position_flag = 0;
    }


    for (i = 0; i < numMetaNames; i++)
    {
        addentry(sw, word, filenum, structure, metaID[i], position[i]);
        position[i]++;
    }
}

/* Gets the next white-space delimited word */
int next_word( char **buf, char **word, int *lenword )
{
    int     i;

    /* skip any whitespace */
    while ( **buf && isspace( (unsigned char) **buf) )
        (*buf)++;

    i = 0;
    while ( **buf && !isspace( (unsigned char) **buf) )
    {
        /* reallocate buffer, if needed */
        if ( i == *lenword )
        {
            *lenword *= 2;
            *word = erealloc(*word, *lenword + 1);
        }

        (*word)[i++] = **buf;
        (*buf)++;
    }

    if ( i )
    {
        (*word)[i] = '\0';
        return 1;
    }

    return 0;
}

/* Gets the next non WordChars delimited word */
/* Bumps position if needed */
int next_swish_word(SWISH * sw, char **buf, char **word, int *lenword, int *bump_position_flag )
{
    int     i;
    IndexFILE *indexf = sw->indexlist;

    /* skip non-wordchars */
    while ( **buf && !iswordchar(indexf->header, **buf ) )
    {
        if (isBumpPositionCounterChar(&indexf->header, (int) **buf))
            (*bump_position_flag)++;
            
        (*buf)++;
    }

    i = 0;
    while ( **buf && iswordchar(indexf->header, **buf) )
    {
        /* It doesn't really make sense to have a WordChar that's also a bump char */
        if (isBumpPositionCounterChar(&indexf->header, (int) **buf))
            (*bump_position_flag)++;


        /* reallocate buffer, if needed */
        if ( i == *lenword )
        {
            *lenword *= 2;
            *word = erealloc(*word, *lenword + 1);
        }

        (*word)[i++] = **buf;
        (*buf)++;
    }

    if ( i )
    {
        (*word)[i] = '\0';
        stripIgnoreLastChars(&indexf->header, *word);
        stripIgnoreFirstChars(&indexf->header, *word);

        return *word ? 1 : 0;
    }

    return 0;
}
        


int     indexstring(SWISH * sw, char *s, int filenum, int structure, int numMetaNames, int *metaID, int *position)
{
    static int lenswishword = 0;
    static char *swishword = NULL;

    static int lenword = 0;
    static char *word = NULL;

    int     wordcount = 0;
    int     bump_position_flag = 0;

    IndexFILE *indexf = sw->indexlist;

    char   *buf_pos;        /* pointer to current position */
    char   *cur_pos;        /* pointer to position with a word */

    int     stem_return;    /* return value of stem operation */



    /* initialize buffers */
    if (!lenword)
        word = (char *) emalloc((lenword = MAXWORDLEN) + 1);

    if (!lenswishword)
        swishword = (char *) emalloc((lenswishword = MAXWORDLEN) + 1);


    if (!numMetaNames)
        numMetaNames = 1;

        

    /* current pointer into buffer */
    buf_pos = s;


    /* get the next word as defined by whitespace */
    while ( next_word( &buf_pos, &word, &lenword ) )
    {
        strtolower(word);

        /* is this a useful feature? */
        if ( indexf->header.is_use_words_flag )
        {
            if  ( isuseword(&indexf->header, word) )
            {
                addword(word, &bump_position_flag, sw, filenum, structure, numMetaNames, metaID, position );
                wordcount++;
            }

            continue;                
        }

        /* Check for buzzwords */
        if ( indexf->header.buzzwords_used_flag )
        {
            /* only strip when buzzwords are being used since stripped again as a "swish word" */
            stripIgnoreLastChars(&indexf->header, word);
            stripIgnoreFirstChars(&indexf->header, word);
            if ( !*word ) /* stripped clean? */
                continue;

        
            if ( isbuzzword(&indexf->header, word) )
            {
                addword(word, &bump_position_flag, sw, filenum, structure, numMetaNames, metaID, position );
                wordcount++;
                continue;
            }
        }
        

        /* Translate chars */
        TranslateChars(indexf->header.translatecharslookuptable, word);

        cur_pos = word;



        /* Now split the word up into "swish words" */

        while ( next_swish_word( sw, &cur_pos, &swishword, &lenswishword, &bump_position_flag ) )
        {
            /* Check Begin & EndCharacters */
            if (!indexf->header.begincharslookuptable[(int) ((unsigned char) swishword[0])])
                continue;

            if (!indexf->header.endcharslookuptable[(int) ((unsigned char) swishword[strlen(swishword) - 1])])
                continue;


            /* limit by stopwords, min/max length, max number of digits, ... */
            if (!isokword(sw, swishword, indexf))
                continue;

            if (indexf->header.applyStemmingRules)
            {
                stem_return = Stem(&swishword, &lenswishword);

                /* === 

                if ( stem_return == STEM_NOT_ALPHA ) printf("Stem: not alpha in '%s'\n", swishword );
                if ( stem_return == STEM_TOO_SMALL ) printf("Stem: too small in '%s'\n", swishword );
                if ( stem_return == STEM_WORD_TOO_BIG ) printf("Stem: too big to stem in '%s'\n", swishword );
                if ( stem_return == STEM_TO_NOTHING ) printf("Stem: stems to nothing '%s'\n", swishword );

                === */
            }

            /* This needs fixing, no?  The soundex could might be longer than the string */
            if (indexf->header.applySoundexRules)
                soundex(swishword);

            addword(swishword, &bump_position_flag, sw, filenum, structure, numMetaNames, metaID, position );
            wordcount++;            
        }
    }
    return wordcount;
}



void    addtofwordtotals(IndexFILE * indexf, int filenum, int ftotalwords)
{
    if (filenum > indexf->filearray_cursize)
        progerr("Internal error in addtofwordtotals");
    else
        indexf->header.filetotalwordsarray[filenum - 1] = ftotalwords;
}


void    addsummarytofile(IndexFILE * indexf, int filenum, char *summary)
{
    if (filenum > indexf->filearray_cursize)
        progerr("Internal error in addsummarytofile");
    else
        indexf->filearray[filenum - 1]->fi.summary = estrdup(summary);
}


