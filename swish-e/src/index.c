/*
$Id$
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
** 2001-06-08 wsm    Store word after ENTRY to save memory
** 2001-08    jmruiz All locations stuff rewritten to save memory
**
*/

#include "swish.h"
#include "mem.h"
#include "string.h"
#include "index.h"
#include "hash.h"
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
#include "metanames.h"
#include "result_sort.h"
#include "result_output.h"
#include "filter.h"
#include "date_time.h"
#include "db.h"
#include "dump.h"
#include "swish_qsort.h"

static void index_path_parts( SWISH *sw, char *path, path_extract_list *list );



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
    {
        idx->hashentries[i] = NULL;
        idx->hashentriesdirty[i] = 0;
    }

    idx->fp_loc_write=idx->fp_loc_read=idx->fp_file_write=idx->fp_file_read=NULL;

    idx->lentmpdir=MAXSTRLEN;
    idx->tmpdir = (char *)emalloc(idx->lentmpdir + 1);idx->tmpdir[0]='\0';

            /* Initialize tmpdir */
    idx->tmpdir = SafeStrCopy(idx->tmpdir,TMPDIR,&idx->lentmpdir);

        /* Economic flag and temp files*/
    idx->swap_locdata = SWAP_LOC_DEFAULT;
    idx->swap_filedata = SWAP_FILE_DEFAULT;

    if(idx->tmpdir && idx->tmpdir[0] && isdirectory(idx->tmpdir))
    {
        idx->swap_file_name=tempnam(idx->tmpdir,"swfi");
        idx->swap_location_name=tempnam(idx->tmpdir,"swlo");
    } else {
        idx->swap_file_name=tempnam(NULL,"swfi");
        idx->swap_location_name=tempnam(NULL,"swlo");
    }

    for(i=0;i<BIGHASHSIZE;i++) idx->inode_hash[i]=NULL;

    /* initialize buffers used by indexstring */
    idx->word = (char *) emalloc((idx->lenword = MAXWORDLEN) + 1);
    idx->swishword = (char *) emalloc((idx->lenswishword = MAXWORDLEN) + 1);

    idx->plimit=PLIMIT;
    idx->flimit=FLIMIT;
    idx->nIgnoreLimitWords = 0;
    idx->IgnoreLimitPositionsArray = NULL;

       /* Swapping access file functions */
    idx->swap_tell = NULL;
    idx->swap_write = NULL;
    idx->swap_seek = NULL;
    idx->swap_read = NULL;
    idx->swap_close = NULL;
    idx->swap_getc = NULL;
    idx->swap_putc = NULL;

    /* Index in blocks of chunk_size documents */
    idx->chunk_size = INDEX_DEFAULT_CHUNK_SIZE;

    /* Use this value to avoid using big zones just as a temporary location storage */
    idx->optimalChunkLocZoneSize = INDEX_DEFAULT_OPTIMAL_CHUNK_ZONE_SIZE_FOR_LOCATIONS;

    idx->freeLocMemChain = NULL;

    /* memory zones for common structures */
    idx->perDocTmpZone = Mem_ZoneCreate("Per Doc Temporal Zone", 0, 0);
    idx->currentChunkLocZone = Mem_ZoneCreate("Current Chunk Locators", 0, 0);
    idx->totalLocZone = Mem_ZoneCreate("All Locators", 0, 0);
    idx->entryZone = Mem_ZoneCreate("struct ENTRY", 0, 0);

    /* table for storing which metaIDs to index */
    idx->metaIDtable.max = 200;  /* totally random guess */
    idx->metaIDtable.num = 0;
    idx->metaIDtable.array = (int *)emalloc( idx->metaIDtable.max * sizeof(int) );
    idx->metaIDtable.defaultID = -1;

    return;
}


/* 
  -- release all wired memory for this module
  -- 2001-04-11 rasc
*/

void freeModule_Index (SWISH *sw)
{
  struct MOD_Index *idx = sw->Index;
  int i;

/* we need to call the real free here */
#undef free

  if (isfile(idx->swap_file_name))
  {
    if (idx->fp_file_read)
        fclose(idx->fp_file_read);

    if (idx->fp_file_write)
        fclose(idx->fp_file_write);

    remove(idx->swap_file_name);

    /* tempnam internally calls malloc, so must use free not efree */
    free(idx->swap_file_name);
  }

  if (isfile(idx->swap_location_name))
  {
    if (idx->fp_loc_read)  
        idx->swap_close(idx->fp_loc_read);

    if (idx->fp_loc_write)
        idx->swap_close(idx->fp_loc_write);

    remove(idx->swap_location_name);

    /* tempnam internally calls malloc, so must use free not efree */
    free(idx->swap_location_name);
  }

  if(idx->lentmpdir) efree(idx->tmpdir);        

        /* Free compression buffer */    
  efree(idx->compression_buffer);
        /* free worddata buffer */
  efree(idx->worddata_buffer);

    /* free word buffers used by indexstring */
  efree(idx->word);
  efree(idx->swishword);

  /* free IgnoreLimit stuff */
  if(idx->IgnoreLimitPositionsArray)
  {
      for(i=0; i<sw->indexlist->header.totalfiles; i++)
      {
          if(idx->IgnoreLimitPositionsArray[i])
          {
              efree(idx->IgnoreLimitPositionsArray[i]->pos);
              efree(idx->IgnoreLimitPositionsArray[i]);
          }
      }
      efree(idx->IgnoreLimitPositionsArray);
  }

  /* should be free by now!!! But just in case... */
  if (idx->entryZone)
      Mem_ZoneFree(&idx->entryZone);

  if (idx->totalLocZone)
      Mem_ZoneFree(&idx->totalLocZone);
  if (idx->currentChunkLocZone)
      Mem_ZoneFree(&idx->currentChunkLocZone);
  if (idx->perDocTmpZone)
      Mem_ZoneFree(&idx->perDocTmpZone);


  efree( idx->metaIDtable.array );

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
  struct MOD_Index *idx = sw->Index;
  char *w0    = sl->word[0];
  int  retval = 1;

  if (strcasecmp(w0, "tmpdir") == 0)
  {
     if (sl->n == 2)
     {
        idx->tmpdir = SafeStrCopy(idx->tmpdir, sl->word[1],&idx->lentmpdir);
        if (!isdirectory(idx->tmpdir))
        {
           progerr("%s: %s is not a directory", w0, idx->tmpdir);
        }
        else
        {
              /* New names for temporal files */
           if (idx->swap_file_name)
              efree(idx->swap_file_name);
           if (idx->swap_location_name)
              efree(idx->swap_location_name);
           idx->swap_file_name = (unsigned char *)tempnam(idx->tmpdir, "swfi");
           idx->swap_location_name = (unsigned char *)tempnam(idx->tmpdir, "swlo");
        }
     }
     else
        progerr("%s: requires one value", w0);
  }
  else if (strcasecmp(w0, "IgnoreLimit") == 0)
  {
     if (sl->n == 3)
     {
        idx->plimit = atol(sl->word[1]);
        idx->flimit = atol(sl->word[2]);
     }
     else
        progerr("%s: requires two values", w0);
  }
  else 
  {
      retval = 0;                   /* not a module directive */
  }
  return retval;
}

/**************************************************************************
*  Index just the file name (or the title) for NoContents files
*
**************************************************************************/
static int index_no_content(SWISH * sw, FileProp * fprop, char *buffer)
{
    IndexFILE          *indexf = sw->indexlist;
    struct MOD_Index   *idx = sw->Index;
    char               *title = "";
    int                 n;
    int                 position = 1;       /* Position of word */
    int                 metaID = 1;         /* THIS ASSUMES that that's the default ID number */


    /* Look for title if HTML document */
    
    if (fprop->doctype == HTML)
    {
        title = parseHTMLtitle( sw , buffer );

        if (!isoktitle(sw, title))
            return -2;  /* skipped because of title */
    }
    
    idx->filenum++;
    addtofilelist(sw, indexf, fprop->real_path, NULL );

    addCommonProperties( sw, indexf, fprop->mtime, title, NULL, 0, fprop->fsize );

    n = indexstring( sw, *title == '\0' ? fprop->real_path : title , idx->filenum, IN_FILE, 1, &metaID, &position);

    addtofwordtotals(indexf, idx->filenum, n);
 
    return n;
}


/*********************************************************************
** 2001-08 jmruiz - A couple of specialized routines to be used with
** locations and MemZones. The main goal is avoid malloc/realloc/free
** wich produces a lot of fragmentation
**
** The memory will be allocated in blocks of 64 bytes inside a zone.
** (I have tried both 32 and 64. 32 looks fine
** In this way, there is some overhead because when a new block is
** requested from the MemZone, the space is not recovered. But this
** only true for the current document because the MemZone is reset
** onces the document is processed. Then, the space is recovered
** after a MemZoneReset is issued
********************************************************************/

#define LOC_BLOCK_SIZE 32  /* Must be greater than sizeof(LOCATION) and a power of 2 */
#define LOC_MIN_SIZE   ((sizeof(LOCATION) + LOC_BLOCK_SIZE - 1) & (~(LOC_BLOCK_SIZE - 1)))

struct  loc_chain {
    struct loc_chain *next;
    int size;
};

/********************************************************************
** 2001-08 jmruiz
** Routine to allocate memory inside a zone for a plain LOCATION
** (frequency is 1). Since we are asking for LOC_BLOCK_SIZE bytes, we 
** are loosing some of the space.
** The advantage is that we do not need to call realloc so often. In 
** fact, most realloc function work this way. They asks for more memory
** to avoid the overhead of the sequence malloc, memcpy, free.
********************************************************************/

LOCATION *alloc_location(struct MOD_Index *idx,int size)
{
    struct loc_chain *tmp = (struct loc_chain *) idx->freeLocMemChain;
    struct loc_chain *big = NULL;
    LOCATION *tmp2 = NULL;

    /* Search for a previously freed location of the same size */
    while(tmp)
    {
        if(tmp->size == size)
        {
            if(!tmp2)
                idx->freeLocMemChain = (LOCATION *)tmp->next;
            else
                tmp2->next = (LOCATION *)tmp->next;
            return (LOCATION *)tmp;
        }
        else if(tmp->size > size)
            big = tmp;
        tmp2 = (LOCATION *)tmp;
        tmp = tmp->next;
    }
    /* Perhaps we have a block with greater size */
    if(big)
    {
        /* Split it */
        while(big->size > size)
        {
            big->size >>= 1;
            tmp = (struct loc_chain *) ((unsigned char *)big + big->size);
            tmp->next = big->next;
            tmp->size = big->size;
            if(tmp->size == size)
                return (LOCATION *)tmp;
            big->next = tmp;
            big = tmp;
        }
    }
    /* NO memory in free chain of the same size - Asks for size */
    return (LOCATION *)Mem_ZoneAlloc(idx->currentChunkLocZone, size);
}


LOCATION *new_location(struct MOD_Index *idx)
{
    return (LOCATION *)alloc_location(idx, LOC_MIN_SIZE);
}


int is_location_full(int size)
{
    int i;

    /* Fast test. Since LOC_BLOCK_SIZE is the minimum size ... */
    if(size % LOC_BLOCK_SIZE)
        return 0;  /* it is not a power of two */
    /* Check if size is a power of 2 (32,64,128,256,...) in binary ..000100... */
    for(i=LOC_BLOCK_SIZE;;i <<= 1)
    {
        if(size>i)
        {
            continue;
        }
        if((size & i) == size)
        {
            return 1;
        } 
        else
        {
            break;
        }
    }
    return 0;
}

/********************************************************************
** 2001-08 jmruiz
** Routine to reallocate memory inside a zone for a previous allocated
** LOCATION (frequency > 1). 
** A new block is allocated only if the previous becomes full
********************************************************************/
LOCATION *add_position_location(void *oldp, struct MOD_Index *idx, int frequency)
{
        LOCATION *newp = NULL;
        struct loc_chain *tmp = NULL;
        int oldsize; 

        oldsize = sizeof(LOCATION) + (frequency - 1) * sizeof(int);

        /* Check for available size in block */
        if(is_location_full(oldsize))
        {
            /* Not enough size - Allocate a new block. Size rounded to LOC_BLOCK_SIZE */
            newp = (LOCATION *)alloc_location(idx,oldsize << 1);
            memcpy((void *)newp,(void *)oldp,oldsize);
            /* Add old zone to the free chain of blocks */
            tmp = (struct loc_chain *)oldp;
            tmp->next = (struct loc_chain *)idx->freeLocMemChain;
            tmp->size = oldsize;
            idx->freeLocMemChain = (LOCATION *) tmp;
        }
        else
            /* Enough size */
            newp = oldp;

        return newp;
}

/***********************************************************************
   -- Start the real indexing process for a file.
   -- This routine will be called by the different indexing methods
   -- (httpd, filesystem, etc.)
   -- The indexed file may be the
   --   - real file on filesystem
   --   - tmpfile or work file (shadow of the real file)
   -- Checks if file has to be send thru filter (file stream)
   -- 2000-11-19 rasc
***********************************************************************/

void    do_index_file(SWISH * sw, FileProp * fprop)
{
    int     wordcount;
    char   *rd_buffer = NULL;   /* complete file read into buffer */
    int     external_program = 0;
                               /* pointer to parsing routine */
    int     (*countwords)(SWISH *,FileProp *,char *);
    IndexFILE *indexf = sw->indexlist;
    struct MOD_Index *idx = sw->Index;
    char    strType[30];
    ENTRY  *ep;
    int     i;

    wordcount = -1;

    /* skip file is the last_mod date is newer than the check date */

    if (sw->mtime_limit && fprop->mtime < sw->mtime_limit)
    {
        if (sw->verbose >= 4)
            progwarn("Skipping %s: last_mod date is too old\n", fprop->real_path);

        /* external program must seek past this data (fseek fails) */
        if (fprop->fp)
        {
            rd_buffer = read_stream(sw,fprop->real_path, fprop->fp, fprop->fsize, 0);
//***JMRUIZ            efree(rd_buffer);
        }

        return;
    }


    /* Upon entry, if fprop->fp is non-NULL then it's already opened and ready to be read from.
       This is the case with "prog" external programs, except when a filter is selected for the file type.
       If a filter is used with "prog" a temporary file was created (fprop->work_file), and
       fprop->fp will be NULL (as is with http and fs access methods).
       2001-05-13 moseley
    */



    /* Get input file handle */
    if (fprop->hasfilter)
    {
        fprop->fp = FilterOpen(fprop);

        /* This should be checked in filteropen because the popen probably won't fail */
        if ( !fprop->fp )
            progerr("Failed to open filter for file '%s'",fprop->real_path);
    }

    else if ( !fprop->fp )
    {
        /* FIX jmruiz 02/20001 Changed "r" to FILEMODE_READ for WIN32 compatibility */
        fprop->fp = fopen(fprop->work_path, FILEMODE_READ);

        if ( !fprop->fp )
        {
            progwarnno("Failed to open: '%s': ", fprop->work_path);
            return;
        }
    }
    else  /* Already open - flag to preven closing the stream used with "prog" */
        external_program++;



    /* -- Read  all data  (len = 0 if filtered...) */
    rd_buffer = read_stream(sw, fprop->real_path, fprop->fp, (fprop->hasfilter) ? 0 : fprop->fsize, sw->truncateDocSize);

    /* just for fun */
    sw->indexlist->total_bytes += fprop->fsize;


    /* Set which parser to use */
    
    switch (fprop->doctype)
    {

    case TXT:
        strcpy(strType,"TXT");
        countwords = countwords_TXT;
        break;

    case HTML:
        strcpy(strType,"HTML");
        countwords = countwords_HTML;
        break;

    case XML:
        strcpy(strType,"XML");
        countwords = countwords_XML;
        break;

    case WML:
        strcpy(strType,"WML");
        countwords = countwords_HTML;
        break;

    default:
        strcpy(strType,"DEFAULT (HTML)");
        countwords = countwords_HTML;
        break;
    }

    if (sw->verbose >= 3)
        printf(" - Using %s parser - ",strType);


    /* Check for NoContents flag and just save the path name */
    /* $$$ Note, really need to only read_stream if reading from a pipe. */
    /* $$$ waste of disk IO and memory if reading from file system */

    if (fprop->index_no_content)
        countwords = index_no_content;


    wordcount = countwords(sw, fprop, rd_buffer);


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

//***JMRUIZ    efree(rd_buffer);

    if (sw->verbose >= 3)
    {
        if (wordcount > 0)
            printf(" (%d words)\n", wordcount);
        else if (wordcount == 0)
            printf(" (no words indexed)\n");
        else if (wordcount == -1)
            printf(" (not opened)\n");
        else if (wordcount == -2)
            printf(" (Skipped due to 'FileRules title' setting)\n");
        fflush(stdout);
    }


    /* Continue if a file was not indexed */
    if ( wordcount < 0 )
        return;


    if ( DEBUG_MASK & DEBUG_PROPERTIES )
    {
        IndexFILE *indexf = sw->indexlist;
        struct file *fi = indexf->filearray[ indexf->filearray_cursize -1 ];

        printf("\nProperties for file '%s':\n", fprop->real_path );
        dump_file_properties( indexf, fi );
    }

#ifdef PROPFILE
    /* write properties to disk, and release memory */
    WritePropertiesToDisk( sw );
#endif


    /* Swap file info if it set */
    if(idx->swap_filedata)
        SwapFileData(sw, indexf->filearray[idx->filenum-1]);


    /* walk the hash list, and compress entries */
    for (i = 0; i < SEARCHHASHSIZE; i++)
    {
        if (idx->hashentriesdirty[i])
        {
            idx->hashentriesdirty[i] = 0;
            for (ep = idx->hashentries[i]; ep; ep = ep->next)
                CompressCurrentLocEntry(sw, indexf, ep);
        }
    }

    /* Coalesce word positions int a more optimal schema to avoid maintain the location data contiguous */
    if(idx->filenum && ((!(idx->filenum % idx->chunk_size)) || (Mem_ZoneSize(idx->currentChunkLocZone) > idx->optimalChunkLocZoneSize)))
    {
        for (i = 0; i < SEARCHHASHSIZE; i++)
            for (ep = idx->hashentries[i]; ep; ep = ep->next)
                coalesce_word_locations(sw, indexf, ep);
        /* Make zone available for reuse */
        Mem_ZoneReset(idx->currentChunkLocZone);
        idx->freeLocMemChain = NULL;

    }

     /* Make zone available for reuse */
     Mem_ZoneReset(idx->perDocTmpZone);


    return;
}


/* Adds a word to the master index tree.
*/

void    addentry(SWISH * sw, char *word, int filenum, int structure, int metaID, int position)
{
    int     found;
    ENTRY  *en,
           *efound;
    LOCATION *tp, *newtp, *prevtp;
    int     hashval;
    IndexFILE *indexf = sw->indexlist;
    struct MOD_Index *idx = sw->Index;

    // if (sw->verbose >= 4)
    if ( DEBUG_MASK & DEBUG_WORDS )
    {
        struct metaEntry *m = getMetaNameByID(&indexf->header, metaID);

        printf("    Adding:[%s:%d]   '%s'   Pos:%d  Stuct:0x%0X (", m ? m->metaName : "UNKNOWN", metaID, word, position, structure);
        
        if ( structure & IN_EMPHASIZED ) printf(" EM");
        if ( structure & IN_HEADER ) printf(" HEADING");
        if ( structure & IN_COMMENTS ) printf(" COMMENT");
        if ( structure & IN_META ) printf(" META");
        if ( structure & IN_BODY ) printf(" BODY");
        if ( structure & IN_HEAD ) printf(" HEAD");
        if ( structure & IN_TITLE ) printf(" TITLE");
        if ( structure & IN_FILE ) printf(" FILE");
        printf(" )\n");
    }

    if (!idx->entryArray)
    {
        idx->entryArray = (ENTRYARRAY *) emalloc(sizeof(ENTRYARRAY));
        idx->entryArray->numWords = 0;
        idx->entryArray->elist = NULL;
    }
    /* Compute hash value of word */
    hashval = searchhash(word);


    /* Look for the word in the hash array */
    for (efound = idx->hashentries[hashval]; efound; efound = efound->next)
        if (strcmp(efound->word, word) == 0)
            break;

    /* flag hash entry used this file, so that the locations can be "compressed" in do_index_file */
    idx->hashentriesdirty[hashval] = 1;


    /* Word not found, so create a new word */

    if (!efound)
    {
        en = (ENTRY *) Mem_ZoneAlloc(idx->entryZone, sizeof(ENTRY) + strlen(word) + 1);
        strcpy(en->word, word);
        en->tfrequency = 1;
        en->u1.last_filenum = filenum;
        en->next = idx->hashentries[hashval];
        idx->hashentries[hashval] = en;

        /* create a location record */
        tp = (LOCATION *) new_location(idx);
        tp->filenum = filenum;
        tp->frequency = 1;
        tp->structure = structure;
        tp->metaID = metaID;
        tp->position[0] = position;

        tp->next = NULL;
        en->currentlocation = NULL;
        en->currentChunkLocationList = tp;
        en->allLocationList = NULL;

        idx->entryArray->numWords++;
        indexf->header.totalwords++;

        return;  /* all done here */
    }

    /* Word found -- look for same metaID and filename */
    /* $$$ To do it right, should probably compare the structure, too */
    /* Note: filename not needed due to compress we are only looking at the current file */

    tp = efound->currentChunkLocationList;
    found = 0;
    while (tp != efound->currentlocation)
    {
        if(tp->metaID == metaID)
        {
            found =1;
            break;
        }
        tp = tp->next;
    }

    /* matching metaID NOT found.  So, add a new LOCATION record onto the word */
    /* This expands the size of the location array for this word by one */
    
    if(!found)
    {
        /* create the new LOCATION entry */
        tp = (LOCATION *) new_location(idx);
        tp->filenum = filenum;
        tp->frequency = 1;            /* count of times this word in this file:metaID */
        tp->structure = structure;
        tp->metaID = metaID;
        tp->position[0] = position;

        /* add the new LOCATION onto the array */
        tp->next = efound->currentChunkLocationList;
        efound->currentChunkLocationList = tp;

        /* Count number of different files that this word is used in */
        if ( efound->u1.last_filenum != filenum )
        {
            efound->tfrequency++;
            efound->u1.last_filenum = filenum;
        }

        return; /* all done */
    }


    /* Otherwise, found matching LOCATION record (matches filenum and metaID) */
    /* Just add the position number onto the end by expanding the size of the LOCATION record */

    /* 2001/08 jmruiz - Much better memory usage occurs if we use MemZones */
    /* MemZone will be reset when the doc is completely proccesed */

    newtp = add_position_location(tp, idx, tp->frequency);

    if(newtp != tp)
    {
        if(efound->currentChunkLocationList == tp)
            efound->currentChunkLocationList = newtp;
        else
            for(prevtp = efound->currentChunkLocationList;;prevtp = prevtp->next)
            {
                if(prevtp->next == tp)
                {
                    prevtp->next = newtp;
                    break;
                }
            }
        tp = newtp;
    }

    tp->position[tp->frequency++] = position;
    tp->structure |= structure;  /* Just merged the structure elements! */

}


/*******************************************************************
*   Adds common file properties to the last entry in the file array
*   (which should be the current one)
*
*
*   Call with:
*       *SWISH      - need for indexing words
*       *IndexFILE  - to get at the cached common metaEntries
*
*       mtime       - last modified date
*       *title      - title of document
*       *summary    - document summary (why here?)
*       start       - start position of a sub-document
*       size        - size in bytes of document
*
*   Returns:
*       void
*
*   Note:
*       Uses cached meta entries (created in metanames.c) to save the
*       metaEntry lookup by name costs
*
********************************************************************/

void    addCommonProperties( SWISH *sw, IndexFILE *indexf, time_t mtime, char *title, char *summary, int start, int size )
{
    struct metaEntry *q;
    docProperties   **properties;
    unsigned long   tmp;
    int             metaID;


    properties = &indexf->filearray[indexf->filearray_cursize -1 ]->docProperties;

    /* Check if title is internal swish metadata */
    if ( title )
    {
        if ( (q = getPropNameByName(&indexf->header, AUTOPROPERTY_TITLE)))
            addDocProperty(properties, q, title, strlen(title),0);


         /* Perhaps we want it to be indexed ... */
        if ( (q = getMetaNameByName(&indexf->header, AUTOPROPERTY_TITLE)))
        {
            int     positionMeta;

            metaID = q->metaID;
            positionMeta = 1;
            indexstring(sw, title, sw->Index->filenum, IN_FILE, 1, &metaID, &positionMeta);
        }
    }


    if ( summary )
    {
        if ( (q = getPropNameByName(&indexf->header, AUTOPROPERTY_SUMMARY)))
            addDocProperty(properties, q, summary, strlen(summary),0);

        
        if ( (q = getMetaNameByName(&indexf->header, AUTOPROPERTY_SUMMARY)))
        {
            int     metaID,
                    positionMeta;

            metaID = q->metaID;
            positionMeta = 1;
            indexstring(sw, summary, sw->Index->filenum, IN_FILE, 1, &metaID, &positionMeta);
        }
    }



    /* Currently don't allow indexing by date or size or position */

    if ( (q = getPropNameByName(&indexf->header, AUTOPROPERTY_LASTMODIFIED)))
    {
        tmp = (unsigned long) mtime;
        tmp = PACKLONG(tmp);      /* make it portable */
        addDocProperty(properties, q, (unsigned char *) &tmp, sizeof(tmp),1);
    }

    if ( (q = getPropNameByName(&indexf->header, AUTOPROPERTY_DOCSIZE)))
    {
        tmp = (unsigned long) size;
        tmp = PACKLONG(tmp);      /* make it portable */
        addDocProperty(properties, q, (unsigned char *) &tmp, sizeof(tmp),1);
    }


    if ( (q = getPropNameByName(&indexf->header, AUTOPROPERTY_STARTPOS)))
    {
        tmp = (unsigned long) start;
        tmp = PACKLONG(tmp);      /* make it portable */
        addDocProperty(properties, q, (unsigned char *) &tmp, sizeof(tmp),1);
    }

}

/*******************************************************************
*   Adds a file name to a struct file entry, and also
*   optionally adds it to the index and/or as a real property
*
*   File name is converted with ReplaceRules
*
*   Call with:
*       *SWISH - for ReplaceRules
*       *struct file
*       *filename
*
*   Returns:
*       void
*
*   ToDo:
*       still saves hash value and file name, which is not currently used
*       and a waste of space
*
********************************************************************/

static void save_pathname( SWISH *sw, IndexFILE * indexf, struct file *newnode, char *filename )        
{
    unsigned char *ruleparsedfilename;
    struct metaEntry *q;
    int     matched = 0;  /* flag if any patterns matched */
    unsigned char   *directory;
    unsigned char   *file;

    /* Run ReplaceRules on file name */
    ruleparsedfilename = process_regex_list( estrdup(filename), sw->replaceRegexps, &matched );


    /* $$$ To be removed! */
    split_path( ruleparsedfilename, &directory, &file );
    newnode->lookup_path = get_lookup_path(&indexf->header.pathlookup, directory);
    efree( directory );
    newnode->filename = file;  /* This isn't freed at this point */


    /* Check if filename is internal swish metadata -- should be! */

    if ((q = getPropNameByName(&indexf->header, AUTOPROPERTY_DOCPATH)))
        addDocProperty(&newnode->docProperties, q, ruleparsedfilename, strlen(ruleparsedfilename),0);


    /* Perhaps we want it to be indexed ... */
    if ((q = getMetaNameByName(&indexf->header, AUTOPROPERTY_DOCPATH)))
    {
        int     metaID,
                positionMeta;

        metaID = q->metaID;
        positionMeta = 1;
        indexstring(sw, ruleparsedfilename, sw->Index->filenum, IN_FILE, 1, &metaID, &positionMeta);
    }


    efree(ruleparsedfilename);


    /* This allows extracting out parts of a path and indexing as a separate meta name */
    if ( sw->pathExtractList )
        index_path_parts( sw, filename, sw->pathExtractList );
        

}
/*******************************************************************
*   extracts out parts from a path name and indexes that part
*
********************************************************************/
static void index_path_parts( SWISH *sw, char *path, path_extract_list *list )
{
    int metaID;
    int positionMeta = 1;
    int matched = 0;  /* flag if any patterns matched */
    
    while ( list )
    {
        char *str = process_regex_list( estrdup(path), list->regex, &matched );

        if ( !matched )  /* only index if the pattern matched */
            return;
        
        metaID = list->meta_entry->metaID;
        indexstring(sw, str, sw->Index->filenum, IN_FILE, 1, &metaID, &positionMeta);
        efree( str );
        list = list->next;
    }
}


/*******************************************************************
*   Adds a file to the file list
*   Creates or expands file list, as needed
*
*   File name is converted with ReplaceRules
*
*   Call with:
*       *SWISH
*       *IndexFILE
*       *filename
*       **struct file - to return the new file entry created
*
*   Returns:
*       void
*
*
********************************************************************/

void    addtofilelist(SWISH * sw, IndexFILE * indexf, char *filename,  struct file **newFileEntry)
{
    struct file *newnode;

    /* Create the file array */

    if (!indexf->filearray || !indexf->filearray_maxsize)
    {
        indexf->filearray = (struct file **) emalloc((indexf->filearray_maxsize = BIGHASHSIZE) * sizeof(struct file *));
        indexf->header.filetotalwordsarray = (int *) emalloc(indexf->filearray_maxsize * sizeof(int));
        indexf->filearray_cursize = 0;

    }

    /* Extend the file array, if needed */

    if (indexf->filearray_maxsize == indexf->filearray_cursize)
    {
        indexf->filearray = (struct file **) erealloc(indexf->filearray, (indexf->filearray_maxsize += 1000) * sizeof(struct file *));
        indexf->header.filetotalwordsarray = (int *) erealloc(indexf->header.filetotalwordsarray, indexf->filearray_maxsize * sizeof(long));
    }


    /* Create a new file entry */

    newnode = (struct file *) emalloc(sizeof(struct file));

    if (newFileEntry != NULL)
        *newFileEntry = newnode; /* pass object pointer up to caller */


    /* Init structure elements */
    newnode->docProperties   = NULL;
    newnode->currentSortProp = NULL;
    newnode->filenum = indexf->filearray_cursize + 1; /* filenum starts in 1 */


    /* And save it in the file array */

    indexf->filearray[indexf->filearray_cursize++] = newnode;
    indexf->header.totalfiles++;

    /* path name is special and is always required.  Save it. */

    save_pathname( sw, indexf, newnode, filename );

}




/* Just goes through the master list of files and
** counts 'em.
*/

int     getfilecount(IndexFILE * indexf)
{
    return indexf->header.totalfiles;
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
/* 2001/02 jmruiz - rewritten - all the proccess is made in one pass to achieve
better performance */
/* 2001-08 jmruiz - rewritten - adapted to new locations and zone schema */


int getNumberOfIgnoreLimitWords(SWISH *sw)
{
    return sw->Index->nIgnoreLimitWords;
}

void getPositionsFromIgnoreLimitWords(SWISH * sw)
{
    int     i,
            j,
            k,
            m,
            stopwords,
            percent,
            bytes_size,
            chunk_size,
            metaID,
            frequency,
            structure,
            tmpval,
            filenum;
    int    *positions;
    int     local_positions[MAX_STACK_POSITIONS];

    LOCATION *l, *next;
    ENTRY  *ep,
           *ep2;
    ENTRY **estop = NULL;
    int     estopsz = 0,
            estopmsz = 0;
    int     totalwords;
    IndexFILE *indexf = sw->indexlist;
    int     totalfiles = getfilecount(indexf);
    struct IgnoreLimitPositions **filepos = NULL;
    struct IgnoreLimitPositions *fpos;
    struct MOD_Index *idx = sw->Index;
    unsigned char *p, *q, *compressed_data, flag;

    stopwords = 0;
    totalwords = indexf->header.totalwords;

    idx->nIgnoreLimitWords = 0;
    idx->IgnoreLimitPositionsArray = NULL;

    if (!totalwords || idx->plimit >= NO_PLIMIT)
        return;

    if (sw->verbose)
    {
        printf("\r  Getting IgnoreLimit stopwords: ...");
        fflush(stdout);
    }


    if (!estopmsz)
    {
        estopmsz = 1;
        estop = (ENTRY **) emalloc(estopmsz * sizeof(ENTRY *));
    }

    
    /* this is the easy part: Remove the automatic stopwords from the hash array */
    /* Builds a list estop[] of ENTRY's that need to be removed */

    for (i = 0; i < SEARCHHASHSIZE; i++)
    {
        for (ep2 = NULL, ep = sw->Index->hashentries[i]; ep; ep = ep->next)
        {
            percent = (ep->tfrequency * 100) / totalfiles;
            if (percent >= idx->plimit && ep->tfrequency >= idx->flimit)
            {
                addStopList(&indexf->header, ep->word); /* For printing list of words */
                addstophash(&indexf->header, ep->word); /* Lookup hash */
                stopwords++;
                /* unlink the ENTRY from the hash */
                if (ep2)
                    ep2->next = ep->next;
                else
                    sw->Index->hashentries[i] = ep->next;

                totalwords--;
                sw->Index->entryArray->numWords--;
                indexf->header.totalwords--;

                /* Reallocte if more space is needed */
                if (estopsz == estopmsz)
                {
                    estopmsz *= 2;
                    estop = (ENTRY **) erealloc(estop, estopmsz * sizeof(ENTRY *));
                }

                /* estop is an array of ENTRY's that need to be removed */
                estop[estopsz++] = ep;
            }
            else
                ep2 = ep;
        }
    }



    /* If we have automatic stopwords we have to recalculate word positions */

    if (estopsz)
    {
        /* Build an array with all the files positions to be removed */
        filepos = (struct IgnoreLimitPositions **) emalloc(totalfiles * sizeof(struct IgnoreLimitPositions *));

        for (i = 0; i < totalfiles; i++)
            filepos[i] = NULL;

        /* Compute bytes required for chunk location size. Eg: 4096 -> 2 bytes, 65535 -> 2 bytes */
        for(bytes_size = 0, i = COALESCE_BUFFER_MAX_SIZE; i; i >>= 8)
            bytes_size++;

        /* Process each automatic stop word */
        for (i = 0; i < estopsz; i++)
        {
            ep = estop[i];

            if (sw->verbose)
            {
                printf("\r  Getting IgnoreLimit stopwords: %25s",ep->word);
                fflush(stdout);
            }

            if(sw->Index->swap_locdata)
                unSwapLocDataEntry(sw,ep);

            /* Run through location list to get positions */
            for(l=ep->allLocationList;l;)
            {
                 compressed_data = (unsigned char *) l;
                 /* Preserve next element */
                 next = *(LOCATION **)compressed_data;
                 /* Jump pointer to next element */
                 p = compressed_data + sizeof(LOCATION *);

                 metaID = uncompress2(&p);

                 for(chunk_size = 0, k = 0, j = bytes_size - 1; k < bytes_size; k++, j--)
                     chunk_size |= p[k] << (j * 8);
                 p += bytes_size;

                 filenum = 0;
                 while(chunk_size)
                 {                   /* Read on all items */
                     q = p;
                     uncompress_location_values(&p,&flag,&tmpval,&structure,&frequency);
                     filenum += tmpval;

                     if(frequency > MAX_STACK_POSITIONS)
                         positions = (int *) emalloc(frequency * sizeof(int));
                     else
                         positions = local_positions;

                     uncompress_location_positions(&p,flag,frequency,positions);

                     chunk_size -= (p-q);
         
                     /* Now build the list by file name of meta/position info */

                     if (!filepos[filenum - 1])
                     {
                         fpos = (struct IgnoreLimitPositions *) emalloc(sizeof(struct IgnoreLimitPositions));
                         fpos->pos = (int *) emalloc(frequency * 2 * sizeof(int));
                         fpos->n = 0;
                         filepos[filenum - 1] = fpos;
                     }
                     else /* file exists in array.  just append the meta and position data */
                     {
                         fpos = filepos[filenum - 1];
                         fpos->pos = (int *) erealloc(fpos->pos, (fpos->n + frequency) * 2 * sizeof(int));
                     }

                     for (m = fpos->n * 2, k = 0; k < frequency; k++)
                     {
                         fpos->pos[m++] = metaID;
                         fpos->pos[m++] = positions[k];
                     }

                     fpos->n += frequency;

                     if(positions != local_positions)
                         efree(positions);
                }
                l = next;
            }
            if(sw->Index->swap_locdata)
                Mem_ZoneReset(idx->totalLocZone);
        }

        /* sort each file sort entries by metaname/position */
        for (i = 0; i < totalfiles; i++)
        {
            if (filepos[i])
                swish_qsort(filepos[i]->pos, filepos[i]->n, 2 * sizeof(int), &icomp2);
        }
    }

    idx->nIgnoreLimitWords = estopsz;
    idx->IgnoreLimitPositionsArray = filepos;

    if (sw->verbose)
    {
        printf("\r  Getting IgnoreLimit stopwords: Complete                            \n");
        fflush(stdout);
    }


}

/* 2001-08 jmruiz - Adjust positions if there was IgnoreLimit stopwords
** In all cases, removes null end of chunk marks */
void adjustWordPositions(unsigned char *worddata, int *sz_worddata, int n_files, struct IgnoreLimitPositions **ilp)
{
    int     frequency,
            metaID,
            structure,
            tmpval,
            r_filenum, 
            w_filenum,
           *positions;
    int     i,j,k;
    unsigned long    r_nextposmeta;
    unsigned char   *w_nextposmeta;
    int     local_positions[MAX_STACK_POSITIONS];
    unsigned char r_flag, *w_flag;
    unsigned char *p, *q;

    p = worddata;

    tmpval = uncompress2(&p);     /* tfrequency */
    metaID = uncompress2(&p);     /* metaID */
    r_nextposmeta =  UNPACKLONG2(p); 
	w_nextposmeta = p;
    p += sizeof(long);

    q = p;
    r_filenum = w_filenum = 0;
    while(1)
    {                   /* Read on all items */
        uncompress_location_values(&p,&r_flag,&tmpval,&structure,&frequency);
        r_filenum += tmpval;
       
        if(frequency <= MAX_STACK_POSITIONS)
            positions = local_positions;
        else
            positions = (int *) emalloc(frequency * sizeof(int));

        uncompress_location_positions(&p,r_flag,frequency,positions);

        if(n_files && ilp && ilp[r_filenum - 1])
        {
            for(i = 0; i < ilp[r_filenum - 1]->n; i++)
            {
                tmpval = ilp[r_filenum - 1]->pos[2 * i];
                if( tmpval >= metaID)
                    break;
            }
            if(tmpval == metaID)
            {
                for(j = 0; j < frequency ; j++)
                {
                    for(k = i; k < ilp[r_filenum - 1]->n ; k++)
                    {
                        if(ilp[r_filenum - 1]->pos[2 * k] != metaID || 
                           ilp[r_filenum - 1]->pos[2 * k + 1] > positions[j])
                            break;  /* End */
                    }
                    positions[j] -= (k-i);
                }
            } 
        }
               /* Store the filenum incrementally to save space */
        compress_location_values(&q,&w_flag,r_filenum - w_filenum,structure,frequency, positions[0]);
		w_filenum = r_filenum;

               /* store positions */
        compress_location_positions(&q,w_flag,frequency,positions);

		if(positions != local_positions)
            efree(positions);

        if(!p[0])       /* End of chunk mark */
        {
            r_filenum = 0;  /* reset filenum */
            p++;
        }
        if ((p - worddata) == *sz_worddata)
             break;   /* End of worddata */

        if ((unsigned long)(p - worddata) == r_nextposmeta)
        {
            if(q != p)
                PACKLONG2(q - worddata, w_nextposmeta);

            metaID = uncompress2(&p);
			q = compress3(metaID,q);

            r_nextposmeta = UNPACKLONG2(p); 
            p += sizeof(long);

			w_nextposmeta = q;
			q += sizeof(long);

            w_filenum = 0;
        }
    }
    *sz_worddata = q - worddata;
    PACKLONG2(*sz_worddata, w_nextposmeta);
}


typedef struct {
    long    mask;
    double    rank;
} RankFactor;

static RankFactor ranks[] = {
    {IN_TITLE,        RANK_TITLE},
    {IN_HEADER,        RANK_HEADER},
    {IN_META,        RANK_META},
    {IN_COMMENTS,    RANK_COMMENTS},
    {IN_EMPHASIZED,    RANK_EMPHASIZED}
};

#define numRanks (sizeof(ranks)/sizeof(ranks[0]))

/*
** This is an all new ranking algorithm. I can't say it is based on anything,
** but it does seem to be better than what was used before!
** 2001/05 wsm
**
** Parameters:
**    sw
**        Pointer to SWISH structure
**
**    freq
**        Number of times this word appeared in this file
**
**    tfreq
**        Number of files this word appeared in this index (not used for ranking)
**
**    words
**        Number of owrds in this file
**
**    structure
**        Bit mask of context where this word appeared
**
**    ignoreTotalWordCount
**        Ignore total word count when ranking (config file parameter)
*/

int getrank(SWISH * sw, int freq, int tfreq, int words, int structure, int ignoreTotalWordCount)
{
    double    factor;
    double  rank;
    double    reduction;
    int        i;

    factor = 1.0;

    /* add up the multiplier factor based on where the word occurs */
    for (i = 0; i < numRanks; i++)
        if (ranks[i].mask & structure)
            factor += ranks[i].rank;

    rank = log((double)freq) + 10.0;

    /* if word count is significant, reduce rank by a number between 1.0 and 5.0 */
    if (!ignoreTotalWordCount)
    {
        if (words < 10) words = 10;
        reduction = log10((double)words);
        if (reduction > 5.0) reduction = 5.0;
        rank /= reduction;
    }

    /* multiply by the weighting factor, and scale to be sure we don't loose
       precision when converted to an integer. The rank will be normalized later */
    rank = rank * factor * 100.0 + 0.5;

    return (int)rank;
}


int     entrystructcmp(const void *e1, const void *e2)
{
    const ENTRY *ep1 = *(ENTRY * const *) e1;
    const ENTRY *ep2 = *(ENTRY * const *) e2;

    return (strcmp(ep1->word, ep2->word));
}


/* Sorts the words */
void    sort_words(SWISH * sw, IndexFILE * indexf)
{
    int     i,
            j;
    ENTRY  *e;


    if (!sw->Index->entryArray || !sw->Index->entryArray->numWords)
        return;


    if (sw->verbose)
    {
        printf("Sorting %d words alphabetically\n", sw->Index->entryArray->numWords );
        fflush(stdout);
    }

    /* Build the array with the pointers to the entries */
    sw->Index->entryArray->elist = (ENTRY **) emalloc(sw->Index->entryArray->numWords * sizeof(ENTRY *));

    /* Fill the array with all the entries */
    for (i = 0, j = 0; i < SEARCHHASHSIZE; i++)
        for (e = sw->Index->hashentries[i]; e; e = e->next)
            sw->Index->entryArray->elist[j++] = e;

    /* Sort them */
    swish_qsort(sw->Index->entryArray->elist, sw->Index->entryArray->numWords, sizeof(ENTRY *), &entrystructcmp);
}



/* Sort chunk locations of entry e by metaID, filenum */
void    sortChunkLocations(SWISH * sw, IndexFILE * indexf, ENTRY * e)
{
    int     i,
            j,
            k,
            filenum,metaID,structure,frequency;
    unsigned char flag;
    unsigned char *ptmp,
           *ptmp2,
           *compressed_data;
    int    *pi = NULL;
    LOCATION *l, *prev = NULL, **lp;

    /* Very trivial case */
    if (!e)
        return;

    if(!e->currentChunkLocationList)
        return;

    /* Get the number of locations in chunk */
    for(i = 0, l = e->currentChunkLocationList; l; i++)
        l=*(LOCATION **)l;    /* Get next location */

    /* Compute array wide */
    j = 2 * sizeof(int) + sizeof(void *);

    /* Compute array size */
    ptmp = (void *) emalloc(j * i);

    /* Build an array with the elements to compare
       and pointers to data */

    for(l = e->currentChunkLocationList, ptmp2 = ptmp; l; )
    {
        pi = (int *) ptmp2;

        compressed_data = (unsigned char *)l;
        /* Jump next offset */
        compressed_data += sizeof(LOCATION *);

        metaID = uncompress2(&compressed_data);
        uncompress_location_values(&compressed_data,&flag,&filenum,&structure,&frequency);
        pi[0] = metaID;
        pi[1] = filenum;
        ptmp2 += 2 * sizeof(int);

        lp = (LOCATION **)ptmp2;
        *lp = l;
        ptmp2 += sizeof(void *);
          /* Get next location */
        l=*(LOCATION **)l;    /* Get next location */
    }

    /* Sort them */
    swish_qsort(ptmp, i, j, &icomp2);

    /* Store results */
    for (k = 0, ptmp2 = ptmp; k < i; k++)
    {
        ptmp2 += 2 * sizeof(int);

        l = *(LOCATION **)ptmp2;
        if(!k)
            e->currentChunkLocationList = l;
        else
            prev->next =l;
        ptmp2 += sizeof(void *);
        prev = l;
    }
    l->next =NULL;

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
    int     percent, lastPercent, n;

#define DELTA 10


    if ( !(ep = sw->Index->entryArray ))
        return;  /* nothing to do */


    totalwords = ep->numWords;

    DB_InitWriteWords(sw, indexf->DB);

    if (sw->verbose)
    {
        printf("  Writing word text: ...");
        fflush(stdout);
    }

    /* This is not longer needed. So free it as soon as possible */
    Mem_ZoneFree(&sw->Index->perDocTmpZone);

    for (i = 0; i < totalwords; i++)
        coalesce_word_locations(sw, indexf, ep->elist[i]);

    /* This is not longer needed. So free it as soon as possible */
    Mem_ZoneFree(&sw->Index->currentChunkLocZone);

    /* If we are swaping locs to file, reset memory zone */
    if(sw->Index->swap_locdata)
        Mem_ZoneReset(sw->Index->totalLocZone);

    n = lastPercent = 0;
    for (i = 0; i < totalwords; i++)
    {
        if ( sw->verbose && totalwords > 10000 )  // just some random guess
        {
            n++;
            percent = (n * 100)/totalwords;
            if (percent - lastPercent >= DELTA )
            {
                printf("\r  Writing word text: %3d%%", percent );
                fflush(stdout);
                lastPercent = percent;
            }
        }

        epi = ep->elist[i];

        /* why check for stopwords here?  removestopwords could have remove them */
        if (!isstopword(&indexf->header, epi->word))
        {
            /* Write word to index file */
            write_word(sw, epi, indexf);
        }
        else
            epi->u1.wordID = -1;  /* flag as a stop word */
    }    

    if (sw->verbose)
    {
        printf("\r  Writing word text: Complete\n" );
        printf("  Writing word hash: ...");
        fflush(stdout);
    }



    n = lastPercent = 0;
    for (i = 0; i < SEARCHHASHSIZE; i++)
    {
        if ( sw->verbose )
        {
            n++;
            percent = (n * 100)/SEARCHHASHSIZE;
            if (percent - lastPercent >= DELTA )
            {
                printf("\r  Writing word hash: %3d%%", percent );
                fflush(stdout);
                lastPercent = percent;
            }
        }


        if ((epi = sw->Index->hashentries[i]))
        {
            while (epi)
            {
                /* If it is not a stopword write it */
                if (epi->u1.wordID > 0)  
                    DB_WriteWordHash(sw, epi->word,epi->u1.wordID,indexf->DB);
                epi = epi->next;
            }
        }
    }

    if (sw->verbose)
    {
        printf("\r  Writing word hash: Complete\n" );
        printf("  Writing word data: ...");
        fflush(stdout);
    }


    n = lastPercent = 0;
    for (i = 0; i < totalwords; i++)
    {
        if ( sw->verbose && totalwords > 10000 )  // just some random guess
        {
            n++;
            percent = (n * 100)/totalwords;
            if (percent - lastPercent >= DELTA )
            {
                printf("\r  Writing word data: %3d%%", percent );
                fflush(stdout);
                lastPercent = percent;
            }
        }

        epi = ep->elist[i];

        if (epi->u1.wordID > 0)   /* Not a stopword */
            write_worddata(sw, epi, indexf);
    }


    if (sw->verbose)
        printf("\r  Writing word data: Complete\n" );


    DB_EndWriteWords(sw, indexf->DB);

       /* free all ENTRY structs at once */
    Mem_ZoneFree(&sw->Index->entryZone);

       /* free all location compressed data */
    Mem_ZoneFree(&sw->Index->totalLocZone);

    efree(ep->elist);
}



unsigned char *buildFileEntry(struct file *filep, int *sz_buffer)
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
    char    *filename = filep->filename;
    int lookup_path = filep->lookup_path;
#ifndef PROPFILE    
    docProperties **docProperties = &filep->docProperties;
#endif    

    len_filename = strlen(filename) + 1;
    lenbuffer1 = len_filename + 2 * 6;
    buffer1 = emalloc(lenbuffer1);
    p = buffer1;
    lookup_path++;              /* To avoid the 0 problem in compress increase 1 */
    p = compress3(lookup_path, p);

    /* We store length +1 to avoid problems with 0 length - So 
       it also writes the null terminator */

    p = compress3(len_filename, p);
    memcpy(p, filename, len_filename);
    p += len_filename;
    datalen1 = p - buffer1;


#ifdef PROPFILE
    buffer2 = PackPropLocations( filep, &datalen2 );
    buffer3 = emalloc((datalen3 = datalen1 + datalen2));
#else    
    buffer2 = storeDocProperties(*docProperties, &datalen2);
    buffer3 = emalloc((datalen3 = datalen1 + datalen2 + 1));
#endif


    memcpy(buffer3, buffer1, datalen1);

    if (datalen2)
    {
        memcpy(buffer3 + datalen1, buffer2, datalen2);
        efree(buffer2);
    }

#ifndef PROPFILE
    buffer3[datalen1 + datalen2] = '\0';
#endif    

    efree(buffer1);

    *sz_buffer = datalen3;
    return (buffer3);
}

struct file *readFileEntry(SWISH *sw, IndexFILE * indexf, int filenum)
{
    int     total_len,
            len1,
            len4,
            lookup_path;
    unsigned char   *buffer,
           *p;
    char   *buf1;
    struct file *fi;

    fi = indexf->filearray[filenum - 1];
    if (fi)
        return fi;              /* Read it previously */

    fi = (struct file *) emalloc(sizeof(struct file));

    fi->filename = NULL;
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


    lookup_path = uncompress2(&p); /* Index to lookup table of paths */
    lookup_path--;
    len1 = uncompress2(&p);       /* Read length of filename */
    buf1 = emalloc(len1);       /* Includes NULL terminator */
    memcpy(buf1, p, len1);      /* Read filename */
    p += len1;

    fi->lookup_path = lookup_path;

    /* Add the path to filename */
    len4 = strlen(indexf->header.pathlookup->all_entries[lookup_path]->val);
    len1 = strlen(buf1);
    fi->filename = emalloc(len4 + len1 + 1);
    memcpy(fi->filename, indexf->header.pathlookup->all_entries[lookup_path]->val, len4);
    memcpy(fi->filename + len4, buf1, len1);
    fi->filename[len1 + len4] = '\0';
    efree(buf1);


    fi->filenum = filenum - 1;


#ifdef PROPFILE
    p = UnPackPropLocations( fi, p );
#else    
    /* read the document properties section  */
    p = fetchDocProperties(fi, p);
#endif    



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
    struct  MOD_Index *idx = sw->Index;

    /* Deflate studd removed ...
    struct buffer_pool *bp = NULL;
    */

    DB_InitWriteFiles(sw, indexf->DB);

    for (i = 0; i < indexf->filearray_cursize; i++)
    {
        if (idx->swap_filedata)
        {
            filep = unSwapFileData(sw);
            filep->filenum = i + 1;
            indexf->filearray[i] = filep;
        }
        else
            filep = indexf->filearray[i];

        buffer = buildFileEntry(filep, &sz_buffer);


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

#ifdef PROPFILE
    /* First reopen the property file in read only mode for seek speed */
    DB_Reopen_PropertiesForRead( sw, indexf->DB  );
#endif    

    sortFileProperties(sw,indexf);


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
        m = getMetaNameByID(&indexf->header, indexf->header.metaEntryArray[j]->metaID);

        if (m && m->sorted_data)
        {
            s = CompressedSortFileProps;
            for(i=0;i<indexf->filearray_cursize;i++)
            {
                val = m->sorted_data[i];
                s = compress3(val,s);
            }
            DB_WriteSortedIndex(sw, m->metaID,CompressedSortFileProps, s - CompressedSortFileProps, indexf->DB);
        }
    }

    efree(CompressedSortFileProps);
    DB_EndWriteSortedIndex(sw, indexf->DB);
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



static void addword( char *word, SWISH * sw, int filenum, int structure, int numMetaNames, int *metaID, int *word_position)
{
    int     i;

    /* Add the word for each nested metaname. */
    for (i = 0; i < numMetaNames; i++)
        addentry(sw, word, filenum, structure, metaID[i], *word_position);

    (*word_position)++;
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
int next_swish_word(SWISH * sw, char **buf, char **word, int *lenword, int *word_position )
{
    int     i;
    IndexFILE *indexf = sw->indexlist;
    int     bump_flag = 0;

    /* skip non-wordchars and check for bump chars */
    while ( **buf && !iswordchar(indexf->header, **buf ) )
    {
        if (!bump_flag && isBumpPositionCounterChar(&indexf->header, (int) **buf))
            bump_flag++;
            
        (*buf)++;
    }

    i = 0;
    while ( **buf && iswordchar(indexf->header, **buf) )
    {
        /* It doesn't really make sense to have a WordChar that's also a bump char */
        if (!bump_flag && isBumpPositionCounterChar(&indexf->header, (int) **buf))
            bump_flag++;


        /* reallocate buffer, if needed */
        if ( i == *lenword )
        {
            *lenword *= 2;
            *word = erealloc(*word, *lenword + 1);
        }

        (*word)[i++] = **buf;
        (*buf)++;
    }

    /* If any bump chars were found then bump to prevent phrase matching */
    if ( bump_flag )
        (*word_position)++;

    if ( i )
    {
        (*word)[i] = '\0';
        stripIgnoreLastChars(&indexf->header, *word);
        stripIgnoreFirstChars(&indexf->header, *word);

        return *word ? 1 : 0;
    }

    return 0;
}

/******************************************************************
*  Build the list of metaIDs that need to be indexed
*
*  Returns number of IDs found
*
*
******************************************************************/
static int build_metaID_list( SWISH *sw )
{
    struct  MOD_Index   *idx = sw->Index;
    METAIDTABLE         *metas = &idx->metaIDtable;
    IndexFILE           *indexf = sw->indexlist;
    INDEXDATAHEADER     *header = &indexf->header;
    struct metaEntry    *m;
    int                 i;


    /* cache the default metaID for speed */
    if ( metas->defaultID == -1 )
    {
        m = getMetaNameByName( header, AUTOPROPERTY_DEFAULT );
        metas->defaultID = m ? m->metaID : 0;
    }    


    metas->num = 0;


    /* Would be smart to track number of metas flagged so not to loop through all for every lookup */
    
    for ( i = 0; i < header->metaCounter; i++)
    {
        m = header->metaEntryArray[i];

        if ( (m->metaType & META_INDEX) && m->in_tag )
        {
            if ( ++metas->num > metas->max )
                metas->array = (int *)erealloc( metas->array, (metas->max = metas->num + 200) );

            metas->array[metas->num - 1] = m->metaID;
        }
    }

    /* If no metas found to index, then add default metaID */
    if ( !metas->num && !sw->ReqMetaName && metas->defaultID  )
        metas->array[metas->num++] = metas->defaultID;

    return metas->num;
}
    

/******************************************************************
*  Index a string
*
*
******************************************************************/

/* 05/2001 Jose Ruiz - Changed word and swishword buffers to make this routine ** thread safe */


int     indexstring(SWISH * sw, char *s, int filenum, int structure, int numMetaNames, int *metaID, int *position)
{
    int     wordcount = 0;

    IndexFILE *indexf = sw->indexlist;

    char   *buf_pos;        /* pointer to current position */
    char   *cur_pos;        /* pointer to position with a word */

    int     stem_return;    /* return value of stem operation */

    struct  MOD_Index *idx = sw->Index;

                            /* Assign word buffers */
    char   *word = idx->word;
    int     lenword = idx->lenword;
    char   *swishword = idx->swishword;
    int     lenswishword = idx->lenswishword;



    /* Generate list of metaIDs to index unless passed in */
    if ( !metaID )
    {
        if ( !(numMetaNames = build_metaID_list( sw )) )
            return 0;
        else
            metaID = idx->metaIDtable.array;
    }

    /* current pointer into buffer */
    buf_pos = s;


    /* get the next word as defined by whitespace */
    while ( next_word( &buf_pos, &word, &lenword ) )
    {
        if ( DEBUG_MASK & DEBUG_PARSED_WORDS )
            printf("White-space found word '%s'\n", word );

    
        strtolower(word);

        /* is this a useful feature? */
        if ( indexf->header.is_use_words_flag )
        {
            if  ( isuseword(&indexf->header, word) )
            {
                addword(word, sw, filenum, structure, numMetaNames, metaID, position );
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
                addword(word, sw, filenum, structure, numMetaNames, metaID, position );
                wordcount++;
                continue;
            }
        }
        

        /* Translate chars */
        TranslateChars(indexf->header.translatecharslookuptable, word);

        cur_pos = word;



        /* Now split the word up into "swish words" */

        while ( next_swish_word( sw, &cur_pos, &swishword, &lenswishword, position ) )
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

            addword(swishword, sw, filenum, structure, numMetaNames, metaID, position );
            wordcount++;            
        }
    }

           /* Buffers can be reallocated - So, reasign them */
    idx->word = word;
    idx->lenword = lenword;
    idx->swishword = swishword;
    idx->lenswishword = lenswishword;

    return wordcount;
}



void    addtofwordtotals(IndexFILE * indexf, int filenum, int ftotalwords)
{
    if (filenum > indexf->filearray_cursize)
        progerr("Internal error in addtofwordtotals");
    else
        indexf->header.filetotalwordsarray[filenum - 1] = ftotalwords;
}

/* Coalesce word current word location into the linked list */
void add_coalesced(SWISH *sw, ENTRY *e, unsigned char *coalesced, int sz_coalesced, int metaID)
{
    int        tmp;
    LOCATION  *tloc, *tprev;
    LOCATION **tmploc, **tmploc2;
    unsigned char *tp;

    /* Check for economic mode (-e) and swap data to disk */
    if(sw->Index->swap_locdata)
    {
        tmploc = (LOCATION **)coalesced;
        *tmploc = e->allLocationList;
        e->allLocationList = (LOCATION *)SwapLocData(sw, coalesced, sz_coalesced);
        return;
    }

    /* Add to the linked list keeping the data sorted by metaname, filenum */
    for(tprev =NULL, tloc = e->allLocationList; tloc; )
    {
        tp = (unsigned char *)tloc + sizeof(void *);
        tmp = uncompress2(&tp); /* Read metaID */
        if(tmp > metaID)
             break;
        tprev = tloc;
        tmploc = (LOCATION **)tloc;
        tloc = *tmploc;
    }

    if(! tprev)
    {
        tmploc = (LOCATION **)coalesced;
        *tmploc = e->allLocationList;
        e->allLocationList = (LOCATION *)coalesced;
    }
    else
    {
        tmploc = (LOCATION **)coalesced;
        tmploc2 = (LOCATION **)tprev;
        *tmploc = *tmploc2;
        *tmploc2 = (LOCATION *)coalesced;
    }
}


void    coalesce_word_locations(SWISH * sw, IndexFILE * indexf, ENTRY *e)
{
    int      curmetaID, metaID,
             curfilenum, filenum,
             structure,
             frequency,
             num_locs,
             bytes_size,
             worst_case_size;
    int      i, j, tmp;
    unsigned char *p, *q, *size_p = NULL;
    unsigned char uflag, *cflag;
    LOCATION *loc, *next;
    unsigned char buffer[COALESCE_BUFFER_MAX_SIZE];
    unsigned char *coalesced_buffer;
    int     *positions;
    int      local_positions[MAX_STACK_POSITIONS];


    /* Check for new locations in the current chunk */
    if(!e->currentChunkLocationList)
        return;

    /* Compute bytes required for size. Eg: 4096 -> 2 bytes, 65535 -> 2 bytes */
    for(bytes_size = 0, tmp = COALESCE_BUFFER_MAX_SIZE; tmp; tmp >>= 8)
        bytes_size++;

    /* Sort all pending word locations by metaID, filenum */
    sortChunkLocations(sw, indexf, e);

    /* Init vars */
    curmetaID = 0;
    curfilenum = 0;
    q = buffer;     /* Destination buffer */
    num_locs = 0;   /* Number of coalesced LOCATIONS */

    /* Run on all locations */
    for(loc = e->currentChunkLocationList; loc; )
    {
        p = (unsigned char *) loc;

        /* get next LOCATION in linked list*/
        next = * (LOCATION **) loc;
        p += sizeof(LOCATION *);

        /* get metaID of LOCATION */
        metaID = uncompress2(&p);

        /* Check for new metaID */
        if(metaID != curmetaID)
        {
            /* If exits previous data add it to the linked list */
            if(curmetaID)
            {
                /* add to the linked list and reset values */
                /* Update the size of chunk's data in *size_p */
                tmp = q - (size_p + bytes_size);  /* tmp contains the size */
                /* Write the size */
                for(i = 0, j = bytes_size - 1; i < bytes_size; i++, j--)
                    size_p[i] = tmp >> (j * 8);
                /* Add to the linked list keeping the data sorted by metaname, filenum */
                /* Allocate memory space */
                coalesced_buffer = (unsigned char *)Mem_ZoneAlloc(sw->Index->totalLocZone,q-buffer);
               /* Copy content to it */
                memcpy(coalesced_buffer,buffer,q-buffer);
                /* Add to the linked list */
                add_coalesced(sw, e, coalesced_buffer, q - buffer, curmetaID);
            }
        /* Reset values */
            curfilenum = 0;
            curmetaID = metaID;
            q = buffer + sizeof(void *);   /* Make room for linked list pointer */        
            q = compress3(metaID,q);  /* Add metaID */
            size_p = q;      /* Preserve position for size */
            q += bytes_size;     /* Make room for size */
            num_locs = 0;
        }
        uncompress_location_values(&p,&uflag,&filenum,&structure,&frequency);
        worst_case_size = sizeof(unsigned char *) + (3 + frequency) * 5;

        while ((q + worst_case_size) - buffer > sizeof(buffer))
        {
            if(!num_locs)
                progerr("Buffer too short in coalesce_word_locations. Increase COALESCE_BUFFER_MAX_SIZE in config.h and rebuild.");
            /* add to the linked list and reset values */
            /* Update the size of chunk's data in *size_p */
            tmp = q - (size_p + bytes_size);  /* tmp contains the size */
            /* Write the size */
            for(i = 0, j = bytes_size - 1; i < bytes_size; i++, j--)
                size_p[i] = tmp >> (j * 8);
            /* Add to the linked list keeping the data sorted by metaname, filenum */
            /* Allocate memory space */
            coalesced_buffer = (unsigned char *)Mem_ZoneAlloc(sw->Index->totalLocZone,q-buffer);
            /* Copy content to it */
            memcpy(coalesced_buffer,buffer,q-buffer);
            /* Add to the linked list */
            add_coalesced(sw, e, coalesced_buffer, q - buffer, curmetaID);

            /* Reset values */
            curfilenum = 0;
            curmetaID = metaID;
            q = buffer + sizeof(void *);   /* Make room for linked list pointer */
            q = compress3(metaID,q);
            size_p = q;      /* Preserve position for size */
            q += bytes_size;     /* Make room for size */
            num_locs = 0;
        }

        if(frequency > MAX_STACK_POSITIONS)
            positions = emalloc(frequency * sizeof(int));
        else
            positions = local_positions;

        uncompress_location_positions(&p,uflag,frequency,positions);

                /* Store the filenum incrementally to save space */
        compress_location_values(&q,&cflag,filenum - curfilenum,structure,frequency, positions[0]);

        curfilenum = filenum;

        compress_location_positions(&q,cflag,frequency,positions);

        if(frequency > MAX_STACK_POSITIONS)
            efree(positions);

        num_locs++;

        loc = next;
    }
    if (num_locs)
    {
        /* add to the linked list and reset values */
        /* Update the size of chunk's data in *size_p */
        tmp = q - (size_p + bytes_size);  /* tmp contains the size */
        /* Write the size */
        for(i = 0, j = bytes_size - 1; i < bytes_size; i++, j--)
            size_p[i] = tmp >> (j * 8);
        /* Add to the linked list keeping the data sorted by metaname, filenum */
        /* Allocate memory space */
        coalesced_buffer = (unsigned char *)Mem_ZoneAlloc(sw->Index->totalLocZone,q-buffer);
        /* Copy content to it */
        memcpy(coalesced_buffer,buffer,q-buffer);
        /* Add to the linked list */
        add_coalesced(sw, e, coalesced_buffer, q - buffer, curmetaID);
    }
    e->currentChunkLocationList = NULL;
    e->currentlocation = NULL;

        /* If we are swaping locs to file, reset also correspondant memory zone */
        if(sw->Index->swap_locdata)
            Mem_ZoneReset(sw->Index->totalLocZone);

}

