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
#include "double_metaphone.h"
#include "error.h"
#include "file.h"
#include "compress.h"
/* Removed due to problems with patents
#include "deflate.h"
*/
#include "html.h"
#include "xml.h"
#include "parser.h"
#include "txt.h"
#include "metanames.h"
#include "result_sort.h"
#include "result_output.h"
#include "filter.h"
#include "date_time.h"
#include "db.h"
#include "dump.h"
#include "swish_qsort.h"
#include "swish_words.h"
#include "list.h"

static void index_path_parts( SWISH *sw, char *path, path_extract_list *list, INDEXDATAHEADER *header, docProperties **properties );
static void SwapLocData(SWISH *,ENTRY *,unsigned char *,int);
static void unSwapLocData(SWISH *,int, ENTRY *);
static void sortSwapLocData(SWISH * , ENTRY *);



/* 
  -- init structures for this module
*/


void initModule_Index (SWISH  *sw)
{
    int i;
    struct MOD_Index *idx;

    idx = (struct MOD_Index *) emalloc(sizeof(struct MOD_Index));
    memset( idx, 0, sizeof( struct MOD_Index ) );
    sw->Index = idx;

    idx->filenum = 0;
    idx->entryArray = NULL;

    idx->len_compression_buffer = MAXSTRLEN;  /* For example */
    idx->compression_buffer=(unsigned char *)emalloc(idx->len_compression_buffer);

    idx->len_worddata_buffer = MAXSTRLEN;  /* For example */
    idx->worddata_buffer=(unsigned char *)emalloc(idx->len_worddata_buffer);
    idx->sz_worddata_buffer = 0;

    /* Init  entries hash table */
    for (i=0; i<VERYBIGHASHSIZE; i++)
    {
        idx->hashentries[i] = NULL;
        idx->hashentriesdirty[i] = 0;
    }


        /* Economic flag and temp files*/
    idx->swap_locdata = SWAP_LOC_DEFAULT;


    for(i=0;i<BIGHASHSIZE;i++) idx->inode_hash[i]=NULL;

    /* initialize buffers used by indexstring */
    idx->word = (char *) emalloc((idx->lenword = MAXWORDLEN) + 1);
    idx->swishword = (char *) emalloc((idx->lenswishword = MAXWORDLEN) + 1);

    idx->plimit=PLIMIT;
    idx->flimit=FLIMIT;
    idx->nIgnoreLimitWords = 0;
    idx->IgnoreLimitPositionsArray = NULL;

       /* Swapping access file functions */
    idx->swap_tell = ftell;
    idx->swap_write = fwrite;
    idx->swap_close = fclose;
    idx->swap_seek = fseek;
    idx->swap_read = fread;
    idx->swap_getc = fgetc;
    idx->swap_putc = fputc;

    for( i = 0; i <MAX_LOC_SWAP_FILES ; i++)
    {
        idx->swap_location_name[i] = NULL;
        idx->fp_loc_write[i] = NULL;
        idx->fp_loc_read[i] = NULL;
    }
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


    /* $$$ this is only a fix while http.c and httpserver.c still exist */
    idx->tmpdir = estrdup(".");

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

  for( i = 0; i < MAX_LOC_SWAP_FILES ; i++)
  {
      if (idx->swap_location_name[i] && isfile(idx->swap_location_name[i]))
      {
          if (idx->fp_loc_read[i])  
             idx->swap_close(idx->fp_loc_read[i]);

          if (idx->fp_loc_write[i])
             idx->swap_close(idx->fp_loc_write[i]);

          remove(idx->swap_location_name[i]);
      }


      if (idx->swap_location_name[i])
          efree(idx->swap_location_name[i]);
  }

  if(idx->tmpdir) efree(idx->tmpdir);        

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


  if ( idx->entryArray )
    efree( idx->entryArray);


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
  char *env_tmp = NULL;

  if (strcasecmp(w0, "tmpdir") == 0)
  {
     if (sl->n == 2)
     {
        idx->tmpdir = erealloc( idx->tmpdir, strlen( sl->word[1] ) + 1 );
        strcpy( idx->tmpdir, sl->word[1] );
        normalize_path( idx->tmpdir );

        if (!isdirectory(idx->tmpdir))
           progerr("%s: %s is not a directory", w0, idx->tmpdir);

       if ( !( env_tmp = getenv("TMPDIR")) )
            if ( !(env_tmp = getenv("TMP")) )
                env_tmp = getenv("TEMP");

        if ( env_tmp )
            progwarn("Configuration setting for TmpDir '%s' will be overridden by environment setting '%s'", idx->tmpdir, env_tmp );

           
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
*   Remove a file from the index.  Used when the parser aborts
*   while indexing.  Typically because of FileRules.
*
**************************************************************************/


static void remove_last_file_from_list(SWISH * sw, IndexFILE * indexf)
{
    struct MOD_Index *idx = sw->Index;
    int i;
    ENTRY *ep, *prev_ep;
    LOCATION *l;


    /* Should be removed */
    if(idx->filenum == 0 || indexf->header.totalfiles == 0) 
        progerr("Internal error in remove_last_file_from_list");


    /* walk the hash list to remove words */
    for (i = 0; i < VERYBIGHASHSIZE; i++)
    {
        if (idx->hashentriesdirty[i])
        {
            idx->hashentriesdirty[i] = 0;
            for (ep = idx->hashentries[i], prev_ep =NULL; ep; ep = ep->next)
            {
                if(ep->currentChunkLocationList)
                {
                    if(ep->currentChunkLocationList->filenum == idx->filenum)
                    {
                        /* First of all - Adjust tfrequency */
                        ep->tfrequency--;
                        /* Now remove locations */
                        for(l = ep->currentChunkLocationList; l; l = l->next)
                        {
                            if(ep->currentlocation == l || l->filenum != idx->filenum)
                                break;
                        }
                        /* Remove locations */                 
                        /* Do not use efree, locations uses a MemZone (currentChunkLocZone) */
                        /* Will be freed later */
                        ep->currentChunkLocationList = l;
                    }
                    /* If there is no locations we must also remove the word */
                    /* Do not call efree to remove the entry, entries use
                    ** a MemZone (perDocTmpZone) - Will be freed later */
                    if(!ep->currentChunkLocationList && !ep->allLocationList)
                    {
                        if(!prev_ep)
                        {
                            idx->hashentries[i] = ep->next;
                        }
                        else
                        {
                            prev_ep->next = ep->next;
                        }
                        /* Adjust word counters */
                        idx->entryArray->numWords--;
                        indexf->header.totalwords--;
                    }
                } 
                else
                {
                    prev_ep = ep;
                }
            }
        }
    }
    /* Decrease filenum */
    idx->filenum--;
    indexf->header.totalfiles--;

}



/**************************************************************************
*  Index just the file name (or the title) for NoContents files
*  $$$ this can be removed if libxml2 is used full time
**************************************************************************/
static int index_no_content(SWISH * sw, FileProp * fprop, FileRec *fi, char *buffer)
{
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


#ifdef HAVE_LIBXML2
    if (fprop->doctype == HTML2 || !fprop->doctype)
        return parse_HTML( sw, fprop, fi, buffer );
#endif


    addCommonProperties( sw, fprop, fi, title, NULL, 0 );


    n = indexstring( sw, *title == '\0' ? fprop->real_path : title , idx->filenum, IN_FILE, 1, &metaID, &position);


    /** ??? $$$ doesn't look right -- check this ***/
    if ( *title != '\0' )
        efree( title );
 
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
**
** 2001-09 jmruiz Improved. Now unused space is recovered when asking
** for space. Free nlocks are maintained using a linked list
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
    int avail = 0;
    struct loc_chain *p_avail = NULL;

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
        {
            /* Just reserve it to be used if we do not find a match */
            big = tmp;
        }
        else
        {
            p_avail = tmp;
            avail = tmp->size;
            /* Check consecutive for consecutive blocks */
            while(((unsigned char *)tmp + tmp->size) == (unsigned char *)tmp->next)
            {
                avail += tmp->next->size;
                if(avail == size)
                {
                    if(!tmp2)
                       idx->freeLocMemChain = (LOCATION *)tmp->next->next;
                    else
                       tmp2->next = (LOCATION *)tmp->next->next;
                    return (LOCATION *)p_avail;
                }
                else if(avail > size)
                {
                    break;
                }
                else
                {
                    tmp = tmp->next;
                }
            }
        }
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
    int     (*countwords)(SWISH *sw,FileProp *fprop, FileRec *fi, char *buffer);
    IndexFILE   *indexf = sw->indexlist;
    int         wordcount;
    char        *rd_buffer = NULL;   /* complete file read into buffer */
    struct MOD_Index *idx = sw->Index;
    char        strType[30];
    int         i;
    FileRec     fi;  /* place to hold doc properties */

    memset( &fi, 0, sizeof( FileRec ) );


    wordcount = -1;



    /* skip file is the last_mod date is newer than the check date */

    if (sw->mtime_limit && fprop->mtime < sw->mtime_limit)
    {
        if (sw->verbose >= 3)
            progwarn("Skipping %s: last_mod date is too old\n", fprop->real_path);

        /* external program must seek past this data (fseek fails) */
        if (fprop->fp)
            flush_stream( fprop );

        return;
    }


    /* Upon entry, if fprop->fp is non-NULL then it's already opened and ready to be read from.
       This is the case with "prog" external programs, *except* when a filter is selected for the file type.
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
        fprop->fp = fopen(fprop->work_path, F_READ_TEXT );

        if ( !fprop->fp )
        {
            progwarnno("Failed to open: '%s': ", fprop->work_path);
            return;
        }
    }
    else  /* Already open - flag to prevent closing the stream used with "prog" */
        fprop->external_program++;




    /** Replace the path for ReplaceRules **/

    if ( sw->replaceRegexps )
    {
        int     matched = 0;
        fprop->real_path = process_regex_list( fprop->real_path, sw->replaceRegexps, &matched );
    }
        


    /** Read the buffer, if not a stream parser **/
    
#ifdef HAVE_LIBXML2
    if ( !fprop->doctype || fprop->doctype == HTML2 || fprop->doctype == XML2 || fprop->doctype == TXT2 )
        rd_buffer = NULL;
    else
#endif
    /* -- Read  all data, last 1 is flag that we are expecting text only */
    rd_buffer = read_stream(sw, fprop, 1);


    /* just for fun so we can show total bytes shown */
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

#ifdef HAVE_LIBXML2
    case XML2:
        strcpy(strType,"XML2");
        countwords = parse_XML;
        break;

    case HTML2:
        strcpy(strType,"HTML2");
        countwords = parse_HTML;
        break;

    case TXT2:
        strcpy(strType,"TXT2");
        countwords = parse_TXT;
        break;

    default:
        strcpy(strType,"DEFAULT (HTML2)");
        countwords = parse_HTML;
        break;

#else

    /* Default if libxml not installed */
    default:
        strcpy(strType,"DEFAULT (HTML)");
        countwords = countwords_HTML;
        break;
#endif
        
    }


    if (sw->verbose >= 3)
        printf(" - Using %s parser - ",strType);


    /* Check for NoContents flag and just save the path name */
    /* $$$ Note, really need to only read_stream if reading from a pipe. */
    /* $$$ waste of disk IO and memory if reading from file system */

    if (fprop->index_no_content)
        countwords = index_no_content;


    /* Make sure all meta flags are cleared (incase a parser aborts) */
    ClearInMetaFlags( &indexf->header );




    /* Now bump the file counter  */
    idx->filenum++;
    indexf->header.totalfiles++; /* why ??? is this needed */
    fi.filenum = idx->filenum;

    /** PARSE **/
    wordcount = countwords(sw, fprop, &fi, rd_buffer);





    if (!fprop->external_program)  /* external_program is not set if a filter is in use */
    {
        if (fprop->hasfilter)
            FilterClose(fprop->fp); /* close filter pipe - should the filter be flushed? */
        else
            fclose(fprop->fp); /* close file */
    }
    /* Else, it's -S prog so make sure we read all the bytes we are suppose to read! */
    /* Can remove the check for fprop->bytes_read once read_stream is no longer used */

    else if ( fprop->bytes_read && fprop->bytes_read < fprop->fsize )
        flush_stream( fprop );


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
        else if (wordcount == -3)
            printf(" (Skipped due to Robots Excluion Rule in meta tag)\n");
        fflush(stdout);
    }


    /* If indexing aborted, remove the last file entry */
    if ( wordcount == -3 || wordcount == -2 )
    {
        remove_last_file_from_list( sw, indexf );
        return;
    }


    /* Continue if a file was not indexed */
    if ( wordcount < 0 )
        return;


    if ( DEBUG_MASK & DEBUG_PROPERTIES )
        dump_file_properties( indexf, &fi );


    /* write properties to disk, and release docprop array (and the prop index array) */
    /* Currently this just passes sw, and assumes only one index file when indexing */
    WritePropertiesToDisk( sw , &fi );


    /* Save total words per file */
    if ( !indexf->header.ignoreTotalWordCountWhenRanking )
    {

        setTotalWordsPerFile(sw, indexf, fi.filenum - 1,wordcount);
    }

    


    /* Compress the entries */
    {
        ENTRY       *ep;

        /* walk the hash list, and compress entries */
        for (i = 0; i < VERYBIGHASHSIZE; i++)
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
            for (i = 0; i < VERYBIGHASHSIZE; i++)
                for (ep = idx->hashentries[i]; ep; ep = ep->next)
                    coalesce_word_locations(sw, indexf, ep);
            /* Make zone available for reuse */
            Mem_ZoneReset(idx->currentChunkLocZone);
            idx->freeLocMemChain = NULL;

        }
    }


    /* Make zone available for reuse */
    Mem_ZoneReset(idx->perDocTmpZone);


    return;
}


ENTRY  *getentry(SWISH * sw, char *word)
{
    IndexFILE *indexf = sw->indexlist;
    struct MOD_Index *idx = sw->Index;
    int     hashval;
    ENTRY *e;

    if (!idx->entryArray)
    {
        idx->entryArray = (ENTRYARRAY *) emalloc(sizeof(ENTRYARRAY));
        idx->entryArray->numWords = 0;
        idx->entryArray->elist = NULL;
    }
    /* Compute hash value of word */
    hashval = verybighash(word);


    /* Look for the word in the hash array */
    for (e = idx->hashentries[hashval]; e; e = e->next)
        if (strcmp(e->word, word) == 0)
            break;

    /* flag hash entry used this file, so that the locations can be "compressed" in do_index_file */
    idx->hashentriesdirty[hashval] = 1;


    /* Word found, return it */
    if (e)
        return e;

    /* Word not found, so create a new word */

    e = (ENTRY *) Mem_ZoneAlloc(idx->entryZone, sizeof(ENTRY) + strlen(word));
    strcpy(e->word, word);
    e->next = idx->hashentries[hashval];
    idx->hashentries[hashval] = e;

    /* Init values */
    e->tfrequency = 0;  
    e->u1.last_filenum = 0; 
    e->currentlocation = NULL;
    e->currentChunkLocationList = NULL;  
    e->allLocationList = NULL;

    idx->entryArray->numWords++;
    indexf->header.totalwords++;

    return e;
}

/* Adds a word to the master index tree.
*/

void   addentry(SWISH * sw, ENTRY *e, int filenum, int structure, int metaID, int position)
{
    int     found;
    LOCATION *tp, *newtp, *prevtp;
    IndexFILE *indexf = sw->indexlist;
    struct MOD_Index *idx = sw->Index;


    indexf->total_word_positions++;

    if ( DEBUG_MASK & DEBUG_WORDS )
    {
        struct metaEntry *m = getMetaNameByID(&indexf->header, metaID);

        printf("    Adding:[%d:%s(%d)]   '%s'   Pos:%d  Stuct:0x%0X (", filenum, m ? m->metaName : "PROP_UNKNOWN", metaID, e->word, position, structure);
        
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


    /* Check for first time */
    if(!e->tfrequency)
    {
        /* create a location record */
        tp = (LOCATION *) new_location(idx);
        tp->filenum = filenum;
        tp->frequency = 1;
        tp->metaID = metaID;
        tp->posdata[0] = SET_POSDATA(position,structure);
        tp->next = NULL;

        e->currentChunkLocationList = tp;
        e->tfrequency = 1;
        e->u1.last_filenum = filenum;

        return;
    }

    /* Word found -- look for same metaID and filename */
    /* $$$ To do it right, should probably compare the structure, too */
    /* Note: filename not needed due to compress we are only looking at the current file */
    /* Oct 18, 2001 -- filename is needed since merge adds words in non-filenum order */

    tp = e->currentChunkLocationList;
    found = 0;

    while (tp != e->currentlocation)
    {
        if(tp->metaID == metaID && tp->filenum == filenum  )
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
        tp->metaID = metaID;
        tp->posdata[0] = SET_POSDATA(position,structure);

        /* add the new LOCATION onto the array */
        tp->next = e->currentChunkLocationList;
        e->currentChunkLocationList = tp;

        /* Count number of different files that this word is used in */
        if ( e->u1.last_filenum != filenum )
        {
            e->tfrequency++;
            e->u1.last_filenum = filenum;
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
        if(e->currentChunkLocationList == tp)
            e->currentChunkLocationList = newtp;
        else
            for(prevtp = e->currentChunkLocationList;;prevtp = prevtp->next)
            {
                if(prevtp->next == tp)
                {
                    prevtp->next = newtp;
                    break;
                }
            }
        tp = newtp;
    }

    tp->posdata[tp->frequency++] = SET_POSDATA(position,structure);

}


/*******************************************************************
*   Adds common file properties to the last entry in the file array
*   (which should be the current one)
*
*
*   Call with:
*       *SWISH      - need for indexing words
*       *fprop
*       *fi
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

void    addCommonProperties( SWISH *sw, FileProp *fprop, FileRec *fi, char *title, char *summary, int start )
{
    struct metaEntry *q;
    docProperties   **properties = &fi->docProperties;
    unsigned long   tmp;
    int             metaID;
    INDEXDATAHEADER *header = &sw->indexlist->header;
    char            *filename = fprop->real_path;  /* should always have a path */
    int             filenum = fi->filenum;
    


    /* Check if filename is internal swish metadata -- should be! */

    if ((q = getPropNameByName(header, AUTOPROPERTY_DOCPATH)))
        addDocProperty( properties, q, (unsigned char *)filename, strlen(filename),0);


    /* Perhaps we want it to be indexed ... */
    if ((q = getMetaNameByName(header, AUTOPROPERTY_DOCPATH)))
    {
        int     metaID,
                positionMeta;

        metaID = q->metaID;
        positionMeta = 1;
        indexstring(sw, filename, filenum, IN_FILE, 1, &metaID, &positionMeta);
    }


    /* This allows extracting out parts of a path and indexing as a separate meta name */
    if ( sw->pathExtractList )
        index_path_parts( sw, fprop->orig_path, sw->pathExtractList, header, properties );
        


    /* Check if title is internal swish metadata */
    if ( title )
    {
        if ( (q = getPropNameByName(header, AUTOPROPERTY_TITLE)))
            addDocProperty(properties, q, (unsigned char *)title, strlen(title),0);


         /* Perhaps we want it to be indexed ... */
        if ( (q = getMetaNameByName(header, AUTOPROPERTY_TITLE)))
        {
            int     positionMeta;

            metaID = q->metaID;
            positionMeta = 1;
            indexstring(sw, title, filenum, IN_FILE, 1, &metaID, &positionMeta);
        }
    }


    if ( summary )
    {
        if ( (q = getPropNameByName(header, AUTOPROPERTY_SUMMARY)))
            addDocProperty(properties, q, (unsigned char *)summary, strlen(summary),0);

        
        if ( (q = getMetaNameByName(header, AUTOPROPERTY_SUMMARY)))
        {
            int     metaID,
                    positionMeta;

            metaID = q->metaID;
            positionMeta = 1;
            indexstring(sw, summary, filenum, IN_FILE, 1, &metaID, &positionMeta);
        }
    }



    /* Currently don't allow indexing by date or size or position */

    /* mtime is a time_t, but we don't have an entry for NOT A TIME.  Does anyone care about the first second of 1970? */

    if ( fprop->mtime && (q = getPropNameByName(header, AUTOPROPERTY_LASTMODIFIED)))
    {
        tmp = (unsigned long) fprop->mtime;
        tmp = PACKLONG(tmp);      /* make it portable */
        addDocProperty(properties, q, (unsigned char *) &tmp, sizeof(tmp),1);
    }

    if ( (q = getPropNameByName(header, AUTOPROPERTY_DOCSIZE)))
    {
        /* Use the disk size, if available */
        tmp = (unsigned long) ( fprop->source_size ? fprop->source_size : fprop->fsize);
        tmp = PACKLONG(tmp);      /* make it portable */
        addDocProperty(properties, q, (unsigned char *) &tmp, sizeof(tmp),1);
    }


    if ( (q = getPropNameByName(header, AUTOPROPERTY_STARTPOS)))
    {
        tmp = (unsigned long) start;
        tmp = PACKLONG(tmp);      /* make it portable */
        addDocProperty(properties, q, (unsigned char *) &tmp, sizeof(tmp),1);
    }

}


/*******************************************************************
*   extracts out parts from a path name and indexes that part
*
********************************************************************/
static void index_path_parts( SWISH *sw, char *path, path_extract_list *list, INDEXDATAHEADER *header, docProperties **properties )
{
    int metaID;
    int positionMeta = 1;
    int matched = 0;  /* flag if any patterns matched */
    
    while ( list )
    {
        char *str = process_regex_list( estrdup(path), list->regex, &matched );

        if ( !matched )
        {
            /* use default? */
            if ( list->meta_entry->extractpath_default )
            {
                metaID = list->meta_entry->metaID;
                indexstring(sw, list->meta_entry->extractpath_default, sw->Index->filenum, IN_FILE, 1, &metaID, &positionMeta);
            }
        }
        else
        {
            struct metaEntry *q;

            metaID = list->meta_entry->metaID;
            indexstring(sw, str, sw->Index->filenum, IN_FILE, 1, &metaID, &positionMeta);

            if ((q = getPropNameByName(header, list->meta_entry->metaName )))
                addDocProperty( properties, q, (unsigned char *)str, strlen(str),0);
            

            efree( str );
        }

        matched = 0;
        list = list->next;
    }
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
/* 2002-07 jmruiz - rewritten - adapted to new -e schema */

int getNumberOfIgnoreLimitWords(SWISH *sw)
{
    return sw->Index->nIgnoreLimitWords;
}


void getPositionsFromIgnoreLimitWords(SWISH * sw)
{
    int     i,
            k,
            m,
            stopwords,
            percent,
            chunk_size,
            metaID,
            frequency,
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
    int     last_loc_swap;

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

    for (i = 0; i < VERYBIGHASHSIZE; i++)
    {
        for (ep2 = NULL, ep = sw->Index->hashentries[i]; ep; ep = ep->next)
        {
            percent = (ep->tfrequency * 100) / totalfiles;
            if (percent >= idx->plimit && ep->tfrequency >= idx->flimit)
            {
                add_word_to_hash_table( &indexf->header.hashstoplist, ep->word);

                /* for printing words removed at the end */
                idx->IgnoreLimitWords = addswline( idx->IgnoreLimitWords, ep->word);
                

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
            {
                /* jmruiz - Be careful with this lines!!!! If we have a lot of words,
                ** probably this code can be very slow and may be rethought.
                ** Fortunately, only a few words must usually raise a IgnoreLimit option
                */
                last_loc_swap = (verybighash(ep->word) * (MAX_LOC_SWAP_FILES - 1)) / (VERYBIGHASHSIZE - 1);
                unSwapLocData(sw, last_loc_swap, ep );
            }
      
            /* Run through location list to get positions */
            for(l=ep->allLocationList;l;)
            {
                 compressed_data = (unsigned char *) l;
                 /* Preserve next element */
                 next = *(LOCATION **)compressed_data;
                 /* Jump pointer to next element */
                 p = compressed_data + sizeof(LOCATION *);

                 metaID = uncompress2(&p);

                 memcpy((char *)&chunk_size,(char *)p,sizeof(chunk_size));
                 p += sizeof(chunk_size);

                 filenum = 0;
                 while(chunk_size)
                 {                   /* Read on all items */
                     q = p;
                     uncompress_location_values(&p,&flag,&tmpval,&frequency);
                     filenum += tmpval;

                     if(frequency > MAX_STACK_POSITIONS)
                         positions = (int *) emalloc(frequency * sizeof(int));
                     else
                         positions = local_positions;

                     uncompress_location_positions(&p,flag,frequency,positions);

                     chunk_size -= (p-q);
         
                     /* Now build the list by filenum of meta/position info */

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
                         fpos->pos[m++] = GET_POSITION(positions[k]);
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
            tmpval,
            r_filenum, 
            w_filenum,
           *posdata;
    int     i,j,k;
    unsigned long    r_nextposmeta;
    unsigned char   *w_nextposmeta;
    int     local_posdata[MAX_STACK_POSITIONS];
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
        uncompress_location_values(&p,&r_flag,&tmpval,&frequency);
        r_filenum += tmpval;
       
        if(frequency <= MAX_STACK_POSITIONS)
            posdata = local_posdata;
        else
            posdata = (int *) emalloc(frequency * sizeof(int));

        uncompress_location_positions(&p,r_flag,frequency,posdata);

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
                            ilp[r_filenum - 1]->pos[2 * k + 1] > GET_POSITION(posdata[j]))
                            break;  /* End */
                    }
                    posdata[j] = SET_POSDATA(GET_POSITION(posdata[j]) - (k-i), GET_STRUCTURE(posdata[j]));
                }
            } 
        }
               /* Store the filenum incrementally to save space */
        compress_location_values(&q,&w_flag,r_filenum - w_filenum,frequency, posdata);
        w_filenum = r_filenum;

               /* store positions */
        compress_location_positions(&q,w_flag,frequency,posdata);

        if(posdata != local_posdata)
            efree(posdata);

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
    for (i = 0, j = 0; i < VERYBIGHASHSIZE; i++)
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
            filenum,metaID,frequency;
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
        uncompress_location_values(&compressed_data,&flag,&filenum,&frequency);
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

void    coalesce_all_word_locations(SWISH * sw, IndexFILE * indexf)
{
    int     i;
    ENTRY  *epi;

    for (i = 0; i < VERYBIGHASHSIZE; i++)
    {
        if ((epi = sw->Index->hashentries[i]))
        {
            while (epi)
            {
                coalesce_word_locations(sw, indexf, epi);
                epi = epi->next;
            }
        }
    }

}

/* Write the index entries that hold the word, rank, and other information.
*/


#ifndef USE_BTREE
void    write_index(SWISH * sw, IndexFILE * indexf)
{
    int     i;
    ENTRYARRAY *ep;
    ENTRY  *epi;
    int     totalwords;
    int     percent, lastPercent, n;
    int     last_loc_swap;

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
        if ( !is_word_in_hash_table( indexf->header.hashstoplist, epi->word ) )
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
    for (i = 0; i < VERYBIGHASHSIZE; i++)
    {
        if ( sw->verbose )
        {
            n++;
            percent = (n * 100)/VERYBIGHASHSIZE;
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


    n = lastPercent = last_loc_swap = -1;
    for (i = 0; i < VERYBIGHASHSIZE; i++)
    {
         /* If we are in economic mode -e restore locations */
        if(sw->Index->swap_locdata)
        {
            if (((i * (MAX_LOC_SWAP_FILES - 1)) / (VERYBIGHASHSIZE - 1)) != last_loc_swap)
            {
                /* Free not longer needed memory */
                Mem_ZoneReset(sw->Index->totalLocZone);
                last_loc_swap = (i * (MAX_LOC_SWAP_FILES - 1)) / (VERYBIGHASHSIZE - 1);
                unSwapLocData(sw, last_loc_swap, NULL );
            }
        }
        if ((epi = sw->Index->hashentries[i]))
        {
            while (epi)
            {
                /* If we are in economic mode -e we must sort locations by metaID, filenum */
                if(sw->Index->swap_locdata)
                {
                    sortSwapLocData(sw, epi);
                }
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
                if (epi->u1.wordID > 0)   /* Not a stopword */
                {
                    build_worddata(sw, epi, indexf);
                    write_worddata(sw, epi, indexf);
                }
                epi = epi->next;
            }
        }
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

#else

void    write_index(SWISH * sw, IndexFILE * indexf)
{
    int     i;
    ENTRYARRAY *ep;
    ENTRY  *epi;
    int     totalwords;
    int     percent, lastPercent, n;
    long    old_wordid;
    unsigned char *buffer =NULL;
    int     sz_buffer = 0;
#define DELTA 10


    if ( !(ep = sw->Index->entryArray ))
        return;  /* nothing to do */

    totalwords = ep->numWords;


    /* Write words */
    DB_InitWriteWords(sw, indexf->DB);

    if (sw->verbose)
    {
        printf("  Writing word text: ...");
        fflush(stdout);
    }

    /* This is not longer needed. So free it as soon as possible */
    Mem_ZoneFree(&sw->Index->perDocTmpZone);


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

        if ( !is_word_in_hash_table( indexf->header.hashstoplist, epi->word ) )
        {
            /* Build worddata buffer */
            build_worddata(sw, epi, indexf);
            /* let's see if word is already in the index */
            old_wordid = read_worddata(sw, epi, indexf, &buffer, &sz_buffer);
            /* If exists, we have to add the new worddata buffer to the old one */
            if(old_wordid)
            {
                 add_worddata(sw, epi, indexf, buffer, sz_buffer);
                 efree(buffer);
                 buffer = NULL;
                 sz_buffer = 0;
                 delete_worddata(sw, old_wordid, indexf);
                 write_worddata(sw, epi, indexf);
                 update_wordID(sw, epi, indexf);
            }
            else
            {
                 /* Reset last error. It was set in read_worddata if
                 ** word was not found */
                 sw->lasterror = RC_OK;
                 /* Write word to index file */
                 write_worddata(sw, epi, indexf);
                 write_word(sw, epi, indexf);
            }
        }
    }    

    if (sw->verbose)
    {
        printf("\r  Writing word text: Complete\n" );
        fflush(stdout);
    }


    DB_EndWriteWords(sw, indexf->DB);

       /* free all ENTRY structs at once */
    Mem_ZoneFree(&sw->Index->entryZone);

       /* free all location compressed data */
    Mem_ZoneFree(&sw->Index->totalLocZone);

    efree(ep->elist);
}


#endif




static void addword( char *word, SWISH * sw, int filenum, int structure, int numMetaNames, int *metaID, int *word_position)
{
    int     i;

    /* Add the word for each nested metaname. */
    for (i = 0; i < numMetaNames; i++)
        (void) addentry(sw, getentry(sw,word), filenum, structure, metaID[i], *word_position);

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
    if ( !metas->num && metas->defaultID  )
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
        if ( indexf->header.hashuselist.count )
        {
            if ( is_word_in_hash_table( indexf->header.hashuselist, word ) )
            {
                addword(word, sw, filenum, structure, numMetaNames, metaID, position );
                wordcount++;
            }

            continue;                
        }


        /* Check for buzzwords */
        if ( indexf->header.hashbuzzwordlist.count )
        {
            /* only strip when buzzwords are being used since stripped again as a "swish word" */
            stripIgnoreLastChars(&indexf->header, word);
            stripIgnoreFirstChars(&indexf->header, word);
            if ( !*word ) /* stripped clean? */
                continue;

            if ( is_word_in_hash_table( indexf->header.hashbuzzwordlist, word ) )
            {
                addword(word, sw, filenum, structure, numMetaNames, metaID, position );
                wordcount++;
                continue;
            }
        }


            
        

        /* Translate chars */
        TranslateChars(indexf->header.translatecharslookuptable, (unsigned char *)word);

        cur_pos = word;



        /* Now split the word up into "swish words" */

        while ( next_swish_word( sw, &cur_pos, &swishword, &lenswishword, position ) )
        {

            /* Weed out Numbers - or anything that's all the listed chars */
            if ( indexf->header.numberchars_used_flag )
            {
                unsigned char *c = (unsigned char *)swishword;

                /* look for any char that's NOT in the lookup table */
                while ( *c ) {
                    if ( !indexf->header.numbercharslookuptable[(int) *c ] )
                        break;
                    c++;
                }

                /* if got all the way through the string then it's only those chars */
                if ( !*c )
                    continue; /* skip this word */
            }

            
            /* Check Begin & EndCharacters */
            if (!indexf->header.begincharslookuptable[(int) ((unsigned char) swishword[0])])
                continue;

            if (!indexf->header.endcharslookuptable[(int) ((unsigned char) swishword[strlen(swishword) - 1])])
                continue;


            /* limit by stopwords, min/max length, max number of digits, ... */
            if (!isokword(sw, swishword, indexf))
                continue;

            /* Now translate word if fuzzy mode */                    

            switch ( indexf->header.fuzzy_mode )
            {
                case FUZZY_NONE:
                    addword(swishword, sw, filenum, structure, numMetaNames, metaID, position );
                    wordcount++;
                    break;

                case FUZZY_STEMMING:
                    stem_return = Stem(&swishword, &lenswishword);

                    /* === 
                    if ( stem_return == STEM_NOT_ALPHA ) printf("Stem: not alpha in '%s'\n", swishword );
                    if ( stem_return == STEM_TOO_SMALL ) printf("Stem: too small in '%s'\n", swishword );
                    if ( stem_return == STEM_WORD_TOO_BIG ) printf("Stem: too big to stem in '%s'\n", swishword );
                    if ( stem_return == STEM_TO_NOTHING ) printf("Stem: stems to nothing '%s'\n", swishword );
                    === */

                    addword(swishword, sw, filenum, structure, numMetaNames, metaID, position );
                    wordcount++;
                    break;

                    
                case FUZZY_SOUNDEX:
                    soundex(swishword);
                    addword(swishword, sw, filenum, structure, numMetaNames, metaID, position );
                    wordcount++;
                    break;

                case FUZZY_METAPHONE:
                case FUZZY_DOUBLE_METAPHONE:
                    {
                        char *codes[2];
                        DoubleMetaphone(swishword, codes);
                        
                        if ( !(*codes[0]) )
                        {
                            efree( codes[0] );
                            efree( codes[1] );
                            addword(swishword, sw, filenum, structure, numMetaNames, metaID, position );
                            wordcount++;
                            break;
                        }
                        addword(codes[0], sw, filenum, structure, numMetaNames, metaID, position );
                        wordcount++;

                        if ( indexf->header.fuzzy_mode == FUZZY_DOUBLE_METAPHONE &&  *(codes[1]) && strcmp(codes[0], codes[1]) )
                        {
                            (*position)--; /* at same position as first word */
                            addword(codes[1], sw, filenum, structure, numMetaNames, metaID, position );
                            wordcount++;
                        }

                        efree( codes[0] );
                        efree( codes[1] );
                    }
                    
                    break;
                    

                default:
                   progerr("Invalid FuzzyMode '%d'", (int)indexf->header.fuzzy_mode );
            }
        }
    }

           /* Buffers can be reallocated - So, reasign them */
    idx->word = word;
    idx->lenword = lenword;
    idx->swishword = swishword;
    idx->lenswishword = lenswishword;

    return wordcount;
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
        *tmploc = (LOCATION *)e;   /* Preserve e in buffer */
                                   /* The cast is for avoiding the warning */                                 
        SwapLocData(sw, e, coalesced, sz_coalesced);
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
             frequency,
             num_locs,
             worst_case_size;
    unsigned int tmp;
    unsigned char *p, *q, *size_p = NULL;
    unsigned char uflag, *cflag;
    LOCATION *loc, *next;
    static unsigned char static_buffer[COALESCE_BUFFER_MAX_SIZE];
    unsigned char *buffer;
    unsigned int sz_buffer;
    unsigned char *coalesced_buffer;
    int     *posdata;
    int      local_posdata[MAX_STACK_POSITIONS];


    /* Check for new locations in the current chunk */
    if(!e->currentChunkLocationList)
        return;

    /* Sort all pending word locations by metaID, filenum */
    sortChunkLocations(sw, indexf, e);

    /* Init buffer to static buffer */
    buffer = static_buffer;
    sz_buffer = COALESCE_BUFFER_MAX_SIZE;

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
                tmp = q - (size_p + sizeof(unsigned int));

                /* Write the size */
                memcpy(size_p, (char *)&tmp, sizeof(tmp) );

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
            q += sizeof(unsigned int);     /* Make room for size */
            num_locs = 0;
        }
        uncompress_location_values(&p,&uflag,&filenum,&frequency);
        worst_case_size = sizeof(unsigned char *) + (3 + frequency) * MAXINTCOMPSIZE;

        while ((q + worst_case_size) - buffer > (int)sz_buffer)
        {
            if(!num_locs)
            {
                //progerr("Buffer too short in coalesce_word_locations. Increase COALESCE_BUFFER_MAX_SIZE in config.h and rebuild.");
                /* Allocate new buffer */
                unsigned char * new_buffer = emalloc(sz_buffer * 2 + worst_case_size);
                memcpy(new_buffer,buffer,sz_buffer);
                sz_buffer = sz_buffer * 2 +worst_case_size;
                if(buffer != static_buffer)
                    efree(buffer);

                /* Adjust pointers */
                q = new_buffer + (q - buffer);
                size_p = new_buffer + (size_p -buffer);
                /* Asign buffer and continue */
                buffer = new_buffer;
                break;
            }
            /* add to the linked list and reset values */
            /* Update the size of chunk's data in *size_p */
            tmp = q - (size_p + sizeof(unsigned int));  /* tmp contains the size */

            /* Write the size */
            memcpy(size_p,(char *)&tmp,sizeof(tmp));

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
            q += sizeof(unsigned int);     /* Make room for size */
            num_locs = 0;
        }

        if(frequency > MAX_STACK_POSITIONS)
            posdata = emalloc(frequency * sizeof(int));
        else
            posdata = local_posdata;

        uncompress_location_positions(&p,uflag,frequency,posdata);

                /* Store the filenum incrementally to save space */
        compress_location_values(&q,&cflag,filenum - curfilenum,frequency, posdata);

        curfilenum = filenum;

        compress_location_positions(&q,cflag,frequency,posdata);

        if(frequency > MAX_STACK_POSITIONS)
            efree(posdata);

        num_locs++;

        loc = next;
    }
    if (num_locs)
    {
        /* add to the linked list and reset values */

        /* Update the size of chunk's data in *size_p */
        tmp = q - (size_p + sizeof(unsigned int));  /* tmp contains the size */
        /* Write the size */
        memcpy(size_p,(char *)&tmp,sizeof(tmp));

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


    /* Free buffer if not using static buffer */
    if(buffer != static_buffer)
        efree(buffer);
}



/* 09/00 Jose Ruiz
** Function to swap location data to a temporal file or ramdisk to save
** memory. Unfortunately we cannot use it with IgnoreLimit option
** enabled.
** The data has been compressed previously in memory.
** Returns the pointer to the file.
*/
static void SwapLocData(SWISH * sw, ENTRY *e, unsigned char *buf, int lenbuf)
{
    int     idx_swap_file;
    struct  MOD_Index *idx = sw->Index;

    /* 2002-07 jmruiz - Get de corrsponding swap file */
    /* Get the corresponding hash value to this word  */
    /* IMPORTANT!!!! - The routine being used here to compute the hash */  
    /* must be the same used to store the words */
    /* Then we must get the corresponding swap file index */
    /* Since we cannot have so many swap files (VERYBIGHASHSIZE for verybighash */
    /* routine) we must scale the hash into SWAP_FILES */
    idx_swap_file = (verybighash(e->word) * (MAX_LOC_SWAP_FILES -1))/(VERYBIGHASHSIZE -1);

    if (!idx->fp_loc_write[idx_swap_file])
    {
        idx->fp_loc_write[idx_swap_file] = create_tempfile(sw, F_WRITE_BINARY, "loc", &idx->swap_location_name[idx_swap_file], 0 );
    }

    compress1(lenbuf, idx->fp_loc_write[idx_swap_file], idx->swap_putc);
    if (idx->swap_write(buf, 1, lenbuf, idx->fp_loc_write[idx_swap_file]) != (unsigned int) lenbuf)
    {
        progerr("Cannot write location to swap file");
    }
}

/* 2002-07 jmruiz - New -e schema */
/* Get location data from swap file */
/* If e is null, all data will be restored */
/* If e si not null, only the location for this data will be readed */
static void unSwapLocData(SWISH * sw, int idx_swap_file, ENTRY *ep)
{
    unsigned char *buf;
    int     lenbuf;
    struct MOD_Index *idx = sw->Index;
    ENTRY *e;
    LOCATION *l;
    FILE *fp;

    /* Check if some swap file is being used */
    if(!idx->fp_loc_write[idx_swap_file] && !idx->fp_loc_read[idx_swap_file])
       return;

    /* Check if the file is opened for write and close it */
    if(idx->fp_loc_write[idx_swap_file])
    {
        /* Write a 0 to mark the end of locations */
        idx->swap_putc(0,idx->fp_loc_write[idx_swap_file]);
        idx->swap_close(idx->fp_loc_write[idx_swap_file]);
        idx->fp_loc_write[idx_swap_file] = NULL;
    }

    /* Reopen in read mode for (for faster reads, I suppose) */
    if(!idx->fp_loc_read[idx_swap_file])
    {
        if (!(idx->fp_loc_read[idx_swap_file] = fopen(idx->swap_location_name[idx_swap_file], F_READ_BINARY)))
            progerrno("Could not open temp file %s: ", idx->swap_location_name[idx_swap_file]);
    }
    else
    {
        /* File already opened for read -> reset pointer */
        fseek(idx->fp_loc_read[idx_swap_file],0,SEEK_SET);
    }

    fp = idx->fp_loc_read[idx_swap_file];
    while((lenbuf = uncompress1(fp, idx->swap_getc)))
    {
        if(ep == NULL)
        {
            buf = (unsigned char *) Mem_ZoneAlloc(idx->totalLocZone,lenbuf);
            idx->swap_read(buf, lenbuf, 1, fp);
            e = *(ENTRY **)buf;
            /* Store the locations in reverse order - Faster. They will be
            ** sorted later */
            l = (LOCATION *) buf;
            l->next = e->allLocationList;
            e->allLocationList = l;
        }
        else
        {
            idx->swap_read(&e,sizeof(ENTRY *),1,fp);
            if(ep == e)
            {
                buf = (unsigned char *) Mem_ZoneAlloc(idx->totalLocZone,lenbuf);             
                memcpy(buf,&e,sizeof(ENTRY *));
                idx->swap_read(buf + sizeof(ENTRY *),lenbuf - sizeof(ENTRY *),1,fp);
                /* Store the locations in reverse order - Faster. They will be
                ** sorted later */
                l = (LOCATION *) buf;
                l->next = e->allLocationList;
                e->allLocationList = l;
            }
            else
            {
                /* Just advance file pointer */
                idx->swap_seek(fp,lenbuf - sizeof(ENTRY *),SEEK_CUR);
            }
        }
    }
}

/* 2002-07 jmruiz - Sorts unswaped location data by metaname, filenum */
static void sortSwapLocData(SWISH * sw, ENTRY *e)
{
    int i, j, k, metaID;
    int    *pi = NULL;
    unsigned char *ptmp,
           *ptmp2,
           *compressed_data;
    LOCATION **tmploc;
    LOCATION *l, *prev=NULL, **lp;

    /* Count the locations */
    for(i = 0, l = e->allLocationList; l;i++, l = l->next);
    
    /* Very trivial case */
    if(i < 2)
        return;

    /* */
    /* Now, let's sort by metanum, offset in file */

    /* Compute array wide for sort */
    j = 2 * sizeof(int) + sizeof(void *);

    /* Compute array size */
    ptmp = (void *) emalloc(j * i);

    /* Build an array with the elements to compare
       and pointers to data */

    /* Very important to remind - data was read from the loc
    ** swap file in reverse order, so, to get data sorted
    ** by filenum we just need to use a reverse counter (i - k)
    ** as the other value for sorting (it has the same effect
    ** as filenum)
    */
    for(k=0, ptmp2 = ptmp, l = e->allLocationList ; k < i; k++, l = l->next)
    {
        pi = (int *) ptmp2;

        compressed_data = (unsigned char *)l;
        /* Jump fileoffset */
        compressed_data += sizeof(LOCATION *);

        metaID = uncompress2(&compressed_data);
        pi[0] = metaID;
        pi[1] = i-k;
        ptmp2 += 2 * sizeof(int);

        lp = (LOCATION **)ptmp2;
        *lp = l;
        ptmp2 += sizeof(void *);
    }

    /* Sort them */
    swish_qsort(ptmp, i, j, &icomp2);

    /* Store results */
    for (k = 0, ptmp2 = ptmp; k < i; k++)
    {
        ptmp2 += 2 * sizeof(int);

        l = *(LOCATION **)ptmp2;
        if(!k)
            e->allLocationList = l;
        else
        {
            tmploc = (LOCATION **)prev;
            *tmploc = l;
        }
        ptmp2 += sizeof(void *);
        prev = l;
    }
    tmploc = (LOCATION **)l;
    *tmploc = NULL;

    /* Free the memory of the sorting array */
    efree(ptmp);
                 
}


