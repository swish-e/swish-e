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
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
**-----------------------------------------------------------------
** Fixed the merge option -M
** G. Hill 3/7/97
**
** Changed readindexline, mergeindexentries, printindexentry and
** added marknumMerge, addtoresultlistMerge, markentrylistMerge,
** ismarkedMerge to add support for METADATA
** G. Hill 3/26/97 ghill@library.berkeley.edu
**
** change sprintf to snprintf to avoid corruption
** added safestrcpy() macro to avoid corruption from strcpy overflow
** SRE 11/17/99
**
** 2001-02-12 rasc   errormsg "print" changed...
**
**
*/

#include <assert.h>             /* for bug hunting */
#include "swish.h"
#include "mem.h"
#include "string.h"
#include "merge.h"
#include "error.h"
#include "search.h"
#include "index.h"
#include "hash.h"
#include "file.h"
#include "docprop.h"
#include "list.h"
#include "compress.h"
#include "metanames.h"
#include "db.h"
#include "dump.h"


/* Private structures, and data */

struct mapentry
{
    struct mapentry *next;
    struct mapentry *next2;
    int     oldnum;
    int     newnum;
};
struct mapentry *mapentrylist[BIGHASHSIZE];
struct mapentry *mapentrylist2[BIGHASHSIZE];

struct mergeindexfileinfo
{
    struct mergeindexfileinfo *next;
    int     filenum;
    int     start;
    int     size;
    time_t  mtime;
    IndexFILE *indexf;
    char    path[0];
};

typedef struct {
    struct metaEntry *filename;
    struct metaEntry *size;
    struct metaEntry *mtime;
    struct metaEntry *start;
} METAS;

struct mergeindexfileinfo *indexfilehashlist[BIGHASHSIZE];



struct markentry
{
    struct markentry *next;
    int     num;
};
struct markentry *markentrylist[BIGHASHSIZE];


struct markentryMerge
{
    struct markentryMerge *next;
    int     num;
    int     metaName;
};
struct markentryMerge *markentrylistMerge[BIGHASHSIZE];



static int     getnew(int num);




/* 2001-09 jmruiz - I do not know why is this code in merge.c
**                  It should be moved to search.c
**                  Also seems that it is not thread safe 
**                  It uses global vars...
*/
/*** These three routines are public, and used by search.c */

/* This marks a number as having been printed.
*/

void    marknum(int num)
{
    unsigned hashval;
    struct markentry *mp;

    mp = (struct markentry *) emalloc(sizeof(struct markentry));

    mp->num = num;

    hashval = bignumhash(num);
    mp->next = markentrylist[hashval];
    markentrylist[hashval] = mp;
}


/* Has a number been printed?
*/

int     ismarked(int num)
{
    unsigned hashval;
    struct markentry *mp;

    hashval = bignumhash(num);
    mp = markentrylist[hashval];

    while (mp != NULL)
    {
        if (mp->num == num)
            return 1;
        mp = mp->next;
    }
    return 0;
}

/* Initialize the marking list.
*/

void    initmarkentrylist()
{
    int     i;
    struct markentry *mp;

    for (i = 0; i < BIGHASHSIZE; i++)
    {
        mp = markentrylist[i];  /* minor optimization */
        if (mp != NULL)
            efree(mp);
        markentrylist[i] = NULL;
    }
}

/* Private Routines */

static int get_numeric_prop(struct docProperties *docProperties, struct metaEntry *meta_entry)
{
    unsigned long i;
    propEntry *p;

    if ( !meta_entry )
        return 0;

    if ((meta_entry->metaID < docProperties->n) && (p = docProperties->propEntry[meta_entry->metaID]))
    {
        i = *(unsigned long *) p->propValue;
        return (int) UNPACKLONG(i);
    }

    return 0;
}

static char *get_string_prop(struct docProperties *docProperties, struct metaEntry *meta_entry)
{
    propEntry *p;

    if ( !meta_entry )
        return NULL;

    if ((meta_entry->metaID < docProperties->n) && (p = docProperties->propEntry[meta_entry->metaID]))
        return DecodeDocProperty( meta_entry, p );

    return NULL;        
}





/* This reads the next line in the index file and puts the results
** in a result structure.
*/

static ENTRY  *readindexline(SWISH * sw, IndexFILE * indexf, struct metaMergeEntry *metaFile, int c, int *is_first, int num)
{
    int     tfrequency,
            filenum,
            structure,
            metaID,
            metaID2,
            frequency,
           *position,
            tmpval;
    LOCATION *loc, *prev_loc;
    ENTRY  *ip;
    struct metaMergeEntry *tmp = NULL;
    unsigned long nextposmetaname;
    unsigned char word[2];
    char   *resultword;
    long    wordID;
    unsigned char *worddata,
           *s,
            flag;
    int     sz_worddata;

    word[0] = (unsigned char) c;
    word[1] = '\0';

    tfrequency = filenum = structure = metaID = frequency = 0;
    position = NULL;
    nextposmetaname = 0L;

    loc = NULL;

    if (*is_first)
    {
        DB_ReadFirstWordInvertedIndex(sw, word, &resultword, &wordID, indexf->DB);
        *is_first = 0;
    }
    else
        DB_ReadNextWordInvertedIndex(sw, word, &resultword, &wordID, indexf->DB);

    if (!wordID)                /* No more words */
        return NULL;

    ip = (ENTRY *) Mem_ZoneAlloc(sw->Index->entryZone,sizeof(ENTRY) + strlen(resultword) + 1);
    strcpy(ip->word, resultword);

    ip->allLocationList = ip->currentChunkLocationList = ip->currentlocation = NULL;

    /* read Word data */
    DB_ReadWordData(sw, wordID, &worddata, &sz_worddata, indexf->DB);
    s = worddata;

    /* parse word data to add it to the structure */
    tfrequency = uncompress2(&s);
    ip->tfrequency = tfrequency;

    metaID = uncompress2(&s);

    if (metaID)
    {
        nextposmetaname = UNPACKLONG2(s); s += sizeof(long);
    }
    filenum = 0;

    prev_loc = NULL;

    while(1)
    {                   /* Read on all items */
        uncompress_location_values(&s,&flag,&tmpval,&structure,&frequency);
        filenum += tmpval;

        /* Adjust filenum based on map */
        filenum = (int) getnew(filenum + num);

        loc = (LOCATION *) Mem_ZoneAlloc(sw->Index->perDocTmpZone, sizeof(LOCATION) + (frequency - 1) * sizeof(int));

        loc->filenum = filenum;

        loc->structure = structure;
        loc->frequency = frequency;
        uncompress_location_positions(&s,flag,frequency,loc->position);

        if(filenum)
        {
            /* we also need to modify metaID with new list */
            metaID2 = 1;
            if (metaID != 1)
            {
                for (tmp = metaFile; tmp; tmp = tmp->next)
                {
                    if (tmp->oldMetaID == metaID)
                    {                
                        metaID2 = tmp->newMetaID;
                        break;
                    }
                }
            }
     
            /* Severe bug if metaID not found */
            if (!tmp && metaID != 1)
                progerr("Merge internal error: Could not translate metaname");

            loc->metaID = metaID2;
        
            loc->next = NULL;
        }

        /* Add only if filenum is not null */
        if(filenum)
        {
            if(!prev_loc)
                ip->currentChunkLocationList = loc;
            else
                prev_loc->next = loc;
        
            prev_loc = loc;        
        }

        if ((s - worddata) == sz_worddata)
            break;   /* End of worddata */

        if ((unsigned long)(s - worddata) == nextposmetaname)
        {
            filenum = 0;
            metaID = uncompress2(&s);
            if (metaID)
            {
                nextposmetaname = UNPACKLONG2(s); 
                s += sizeof(long);
            }
            else
                nextposmetaname = 0L;
        }
    }
    return ip;
}

/* This puts all the file info into a hash table so that it can
** be looked up by its pathname and filenumber. This is how
** we find redundant file information.
*/



/* This returns the file number corresponding to a pathname.
*/

static struct mergeindexfileinfo *lookupindexfilepath(char *path, int start, int size)
{
    unsigned hashval;
    struct mergeindexfileinfo *ip;

    hashval = bighash(path);
    ip = indexfilehashlist[hashval];

    while (ip != NULL)
    {
        if ((strcmp(ip->path, path) == 0) && ip->start == start && ip->size == size)
            return ip;
        ip = ip->next;
    }
    return NULL;
}

#ifndef PROPFILE

static struct mergeindexfileinfo *lookupindexfilepath2(char *path, int filenum)
{
    unsigned hashval;
    struct mergeindexfileinfo *ip;

    hashval = bighash(path);
    ip = indexfilehashlist[hashval];

    while (ip != NULL)
    {
        if ((strcmp(ip->path, path) == 0) && ip->filenum == filenum)
            return ip;
        ip = ip->next;
    }
    return NULL;
}

#endif


void freemergeindexfileinfo()
{
    struct mergeindexfileinfo *ip , *next;
    int i;

    for(i=0;i<BIGHASHSIZE;i++)
    {
        ip = indexfilehashlist[i];
        indexfilehashlist[i] = NULL;
        while (ip != NULL)
        {
            next = ip->next;
            efree(ip);
            ip = next;
        }
    }
}

/* This simply concatenates two location lists that correspond
** to a word found in both index files.
*/

static ENTRY  *mergeindexentries(ENTRY * ip1, ENTRY * ip2, int num)
{
    LOCATION *lap;

    for (lap = ip1->currentChunkLocationList; ; lap = lap->next)
        if(!lap->next)
             break;

    lap->next = ip2->currentChunkLocationList;
    lap = lap->next;

    ip1->tfrequency += ip2->tfrequency;

    return ip1;
}

/* This associates a number with a new number.
** This function is used to remap file numbers from index
** files to a new merged index file.
*/

static void    remap(int oldnum, int newnum)
{
    unsigned hashval;
    struct mapentry *mp;

    mp = (struct mapentry *) emalloc(sizeof(struct mapentry));

    mp->oldnum = oldnum;
    mp->newnum = newnum;

    hashval = bignumhash(oldnum);
    mp->next = mapentrylist[hashval];
    mapentrylist[hashval] = mp;

    hashval = bignumhash(newnum);
    mp->next2 = mapentrylist2[hashval];
    mapentrylist2[hashval] = mp;
}

/* This retrieves the number associated with another.
*/

static int     getnew(int num)
{
    unsigned hashval;
    struct mapentry *mp;

    hashval = bignumhash(num);
    mp = mapentrylist[hashval];

    while (mp != NULL)
    {
        if (mp->oldnum == num)
            return mp->newnum;
        mp = mp->next;
    }
    return num;
}

#ifdef PROPFILE
static int     getold(int num)
{
    unsigned hashval;
    struct mapentry *mp;

    hashval = bignumhash(num);
    mp = mapentrylist2[hashval];

    while (mp != NULL)
    {
        if (mp->newnum == num)
            return mp->oldnum;
        mp = mp->next;
    }
    return num;
}
#endif




/* TAB */
/* gprof suggests that this is a major CPU eater  :-(, that's
   because it gets called a _lot_ rather than because it is inefficient...

   ... and it's probably an indicator that free() is a major CPU hog :-(

   you'd think that putting the NULL assignment into the if() condition
   would be fastest, but either gcc is really stupid, or really smart,
   because gprof showed that unconditionally setting it after reading it
   saved about 10% over setting it unconditionally at the end of the loop
   (that could be sampling error though), and setting it inside the loop
   definitely increased it by 15-20% ... go figure?   - TAB oct/99

   For reference:
   Reading specs from /usr/lib/gcc-lib/i386-redhat-linux/2.7.2.3/specs
   gcc version 2.7.2.3

   hhmm... I wonder if we should consider making it a macro? No, there are
   routines using 1/4 the CPU, getting called 20 times as often (compress)
   so obviously subrouting overhead isn't the issue...

*/


/* Initialize the main file list.
*/

void    initindexfilehashlist()
{
    int     i;

    for (i = 0; i < BIGHASHSIZE; i++)
    {
        indexfilehashlist[i] = NULL;
    }
}

/* Initialize the mapentrylist 
*/

static void    initmapentrylist()
{
    int     i;

    for (i = 0; i < BIGHASHSIZE; i++)
    {
        mapentrylist[i] = NULL;
    }
}

/* Reads the meta names from the index. Needs to be different from
** readMetaNames because needs to zero out the counter.
*/
static struct metaMergeEntry *readMergeMeta(SWISH * sw, int metaCounter, struct metaEntry **metaEntryArray)
{
    struct metaMergeEntry *mme = NULL,
           *tmp = NULL,
           *tmp2 = NULL;
    int     i;

    for (i = 0; i < metaCounter; i++)
    {
        tmp2 = tmp;
        tmp = (struct metaMergeEntry *) emalloc(sizeof(struct metaMergeEntry));

        tmp->metaName = (char *) estrdup(metaEntryArray[i]->metaName);
        tmp->oldMetaID = metaEntryArray[i]->metaID;
        tmp->metaType = metaEntryArray[i]->metaType;
        tmp->next = NULL;
        if (!mme)
            mme = tmp;
        else
        {
            tmp2->next = tmp;
            tmp2 = tmp;
        }
    }
    return mme;
}


/* Adds an entry to the merged meta names list and changes the
 ** new index in the idividual file entry
 **
 ** 2001-10 jmruiz Fix to store metanames and properties with
 **                different ID
 */



static void addMetaMergeArray(IndexFILE *indexf, struct metaMergeEntry *metaFileEntry)
{
    int     newMetaID;
    struct metaEntry *tmpEntry;
    char   *metaWord;
    int     metaType = 0;
    int     i;
    int     count = indexf->header.metaCounter;
    struct metaEntry **metaEntryArray = indexf->header.metaEntryArray;

    newMetaID = 0;

    metaWord = metaFileEntry->metaName;
    metaType = metaFileEntry->metaType;

    if (metaEntryArray)
    {
        for (i = 0; i < count; i++)
        {
            tmpEntry = metaEntryArray[i];

            if (strcmp(tmpEntry->metaName, metaWord) == 0)
            {
                if(is_meta_property(metaFileEntry))
                {
                    if(is_meta_property(tmpEntry))
                    {
                        /* Both are META_PROP - Check for types */
                        if( match_meta_type(metaType,tmpEntry->metaType))
                        {
                            newMetaID = tmpEntry->metaID;
                            break;
                        }
                        else
                        {
                            progerr("Couldn't merge: metaname \"%s\" : types do not match.", metaWord);
                        }
                    }
                }
                else 
                {
                    if(!is_meta_property(tmpEntry))
                    {
                        /* Both are META_INDEX */
                        newMetaID = tmpEntry->metaID;
                        break;
                    }
                }    
            }
        }



        if (i < count)       /* metaname exists */
        {
            metaFileEntry->newMetaID = newMetaID;
        }
        else
        {
/***************** FIX ******************/       
            metaFileEntry->newMetaID = count + 1; 
            addNewMetaEntry( &indexf->header, metaWord, metaType, 0);
        }
    }
    else //   if (!metaEntryArray)
    {
        count = 0;
/***************** FIX ******************/       
        metaFileEntry->newMetaID = 1;
        addNewMetaEntry( &indexf->header, metaWord, metaType, 0);
    }
}




static void    addentryMerge(SWISH * sw, ENTRY * ip)
{
    int     hashval;
    IndexFILE *indexf = sw->indexlist;

    if (!sw->Index->entryArray)
    {
        sw->Index->entryArray = (ENTRYARRAY *) emalloc(sizeof(ENTRYARRAY));
        sw->Index->entryArray->numWords = 0;
        sw->Index->entryArray->elist = NULL;
    }
    /* Compute hash value of word */
    hashval = searchhash(ip->word);
    /* Add to the array of hashes */
    ip->next = sw->Index->hashentries[hashval];
    sw->Index->hashentries[hashval] = ip;

    sw->Index->entryArray->numWords++;
    indexf->header.totalwords++;

    CompressCurrentLocEntry(sw, indexf, ip);
    coalesce_word_locations(sw, indexf, ip);
}

/* Creates a list of all the meta names in the indexes
*/
static void createMetaMerge(IndexFILE *indexf, struct metaMergeEntry *metaFile1, struct metaMergeEntry *metaFile2)
{
    struct metaMergeEntry *tmpEntry;


    for (tmpEntry = metaFile1; tmpEntry; tmpEntry = tmpEntry->next)
        addMetaMergeArray(indexf, tmpEntry);

    for (tmpEntry = metaFile2; tmpEntry; tmpEntry = tmpEntry->next)
        addMetaMergeArray(indexf, tmpEntry);

}


/***********************************************************************
*  Merge the index headers into one
*
*
************************************************************************/

static void merge_index_headers( IndexFILE *indexf, IndexFILE *indexf1, IndexFILE *indexf2 )
{


    /* Merge values of both files */
    if (strcmp(indexf1->header.wordchars, indexf2->header.wordchars))
    {
        printf("Warning: WordCharacters do not match. Merging them\n");
        indexf->header.wordchars = mergestrings(indexf1->header.wordchars, indexf2->header.wordchars);
        indexf->header.lenwordchars = strlen(indexf->header.wordchars);
    }
    else
    {
        indexf->header.wordchars = SafeStrCopy(indexf->header.wordchars, indexf1->header.wordchars, &indexf->header.lenwordchars);
    }
    makelookuptable(indexf->header.wordchars, indexf->header.wordcharslookuptable);

    if (strcmp(indexf1->header.beginchars, indexf2->header.beginchars))
    {
        printf("Warning: BeginCharacters do not match. Merging them\n");
        indexf->header.beginchars = mergestrings(indexf1->header.beginchars, indexf2->header.beginchars);
        indexf->header.lenbeginchars = strlen(indexf->header.beginchars);
    }
    else
    {
        indexf->header.beginchars = SafeStrCopy(indexf->header.beginchars, indexf1->header.beginchars, &indexf->header.lenbeginchars);
    }
    makelookuptable(indexf->header.beginchars, indexf->header.begincharslookuptable);

    if (strcmp(indexf1->header.endchars, indexf2->header.endchars))
    {
        printf("Warning: EndCharacters do not match. Merging them\n");
        indexf->header.endchars = mergestrings(indexf1->header.endchars, indexf2->header.endchars);
        indexf->header.lenendchars = strlen(indexf->header.endchars);
    }
    else
    {
        indexf->header.endchars = SafeStrCopy(indexf->header.endchars, indexf1->header.endchars, &indexf->header.lenendchars);
    }
    makelookuptable(indexf->header.endchars, indexf->header.endcharslookuptable);

    if (strcmp(indexf1->header.ignorefirstchar, indexf2->header.ignorefirstchar))
    {
        printf("Warning: IgnoreFirstChar do not match. Merging them\n");
        indexf->header.ignorefirstchar = mergestrings(indexf1->header.ignorefirstchar, indexf2->header.ignorefirstchar);
        indexf->header.lenignorefirstchar = strlen(indexf->header.ignorefirstchar);
    }
    else
    {
        indexf->header.ignorefirstchar =
            SafeStrCopy(indexf->header.ignorefirstchar, indexf1->header.ignorefirstchar, &indexf->header.lenignorefirstchar);
    }
    makelookuptable(indexf->header.ignorefirstchar, indexf->header.ignorefirstcharlookuptable);

    if (strcmp(indexf1->header.ignorelastchar, indexf2->header.ignorelastchar))
    {
        printf("Warning: IgnoreLastChar do not match. Merging them\n");
        indexf->header.ignorelastchar = mergestrings(indexf1->header.ignorelastchar, indexf2->header.ignorelastchar);
        indexf->header.lenignorelastchar = strlen(indexf->header.ignorelastchar);
    }
    else
    {
        indexf->header.ignorelastchar = SafeStrCopy(indexf->header.ignorelastchar, indexf1->header.ignorelastchar, &indexf->header.lenignorelastchar);
    }
    makelookuptable(indexf->header.ignorelastchar, indexf->header.ignorelastcharlookuptable);

    indexf->header.applyStemmingRules = indexf1->header.applyStemmingRules && indexf2->header.applyStemmingRules;
    indexf->header.applySoundexRules = indexf1->header.applySoundexRules && indexf2->header.applySoundexRules;
    indexf->header.ignoreTotalWordCountWhenRanking = indexf1->header.ignoreTotalWordCountWhenRanking
        && indexf2->header.ignoreTotalWordCountWhenRanking;

    if (indexf1->header.minwordlimit < indexf2->header.minwordlimit)
        indexf->header.minwordlimit = indexf1->header.minwordlimit;
    else
        indexf->header.minwordlimit = indexf2->header.minwordlimit;

    if (indexf1->header.maxwordlimit < indexf2->header.maxwordlimit)
        indexf->header.maxwordlimit = indexf1->header.maxwordlimit;
    else
        indexf->header.maxwordlimit = indexf2->header.maxwordlimit;

    /* removed - Patents ...
       indexf->header.applyFileInfoCompression=indexf1->header.applyFileInfoCompression && indexf2->header.applyFileInfoCompression;
     */
}     

/***********************************************************************
*  Merge the word lists
*
*
************************************************************************/


static int merge_words(
    SWISH *sw, SWISH *sw1, SWISH *sw2,
    IndexFILE *indexf1, IndexFILE *indexf2,
    struct metaMergeEntry *metaFile1, struct metaMergeEntry *metaFile2,
    int indexfilenum1, int indexfilenum2 )
{
    int     j,
            result,
            skipwords;
    int     is_first1,
            is_first2;
    int     firstTime = 1;
    int     endip1,
            endip2;
    ENTRY  *ip1,
           *ip2,
           *ip3;
    ENTRY  *buffer1,
           *buffer2;



    DB_InitReadWords(sw1, indexf1->DB);
    DB_InitReadWords(sw2, indexf2->DB);

    /* Adjust file pointer to start of word info */
    skipwords = 0;
    /* Read on all chars */
    for (j = 0; j < 256; j++)
    {
        is_first1 = is_first2 = 1;
        ip1 = ip2 = ip3 = NULL;
        endip1 = endip2 = 0;
        buffer1 = buffer2 = NULL;

        while (1)
        {
            if (buffer1 == NULL)
            {
                if (endip1)
                    ip1 = NULL;
                else
                    ip1 = readindexline(sw1, indexf1, metaFile1, j, &is_first1,0);
                if (ip1 == NULL)
                {
                    endip1 = 1;
                    if (ip2 == NULL && !firstTime)
                    {
                        break;
                    }
                }
                buffer1 = ip1;
            }
            firstTime = 0;
            if (buffer2 == NULL)
            {
                if (endip2)
                    ip2 = NULL;
                else
                    ip2 = readindexline(sw2, indexf2, metaFile2, j, &is_first2, indexfilenum1);
                if (ip2 == NULL)
                {
                    endip2 = 1;
                    if (ip1 == NULL)
                    {
                        break;
                    }
                }

                buffer2 = ip2;
            }
            if (ip1 == NULL)
                result = 1;
            else if (ip2 == NULL)
                result = -1;
            else
                result = strcmp(ip1->word, ip2->word);
            if (!result)
            {
                ip3 = (ENTRY *) mergeindexentries(ip1, ip2, indexfilenum1);
                /* ip1 and ip2 are freeded in mergeindexentries */
                buffer1 = buffer2 = NULL;
                skipwords++;
            }
            else if (result < 0)
            {
                ip3 = ip1;
                buffer1 = NULL;
            }
            else
            {
                ip3 = ip2;
                buffer2 = NULL;
            }

            addentryMerge(sw, ip3);

            if(!result)
            {
                /* Make zone available for reuse */
                Mem_ZoneReset(sw->Index->perDocTmpZone);
            }
        }
    }
    DB_EndReadWords(sw1, indexf1->DB);
    DB_EndReadWords(sw2, indexf2->DB);

    return skipwords;
}

/***********************************************************************
*  Cache the meta name lookups, so only are done once per index
*
*
************************************************************************/

static void init_metas( METAS *metas, INDEXDATAHEADER *header )
{
    
    /* Get path from property list */
    metas->filename = getPropNameByName( header, AUTOPROPERTY_DOCPATH );
    metas->start    = getPropNameByName( header, AUTOPROPERTY_STARTPOS );
    metas->size     = getPropNameByName( header, AUTOPROPERTY_DOCSIZE );
    metas->mtime    = getPropNameByName( header, AUTOPROPERTY_LASTMODIFIED );
}



/***********************************************************************
*  adds a file and it's associated properties to the output index
*
*
************************************************************************/

static void addindexfilelist(SWISH * sw, int num, METAS * metas, struct docProperties **dProps, int *totalfiles, int ftotalwords,
                             struct metaMergeEntry *metaFile, IndexFILE *indexf)
{
    int     hashval;
    struct file *thisFileEntry = NULL;
    struct mergeindexfileinfo *ip;
    int     start,
            size;
    time_t  mtime;
    struct docProperties *docProperties = *dProps;
    char    *filename;
    


    /* Lookup the properties */
    filename = get_string_prop(docProperties, metas->filename );
    start    = get_numeric_prop(docProperties, metas->start );
    size     = get_numeric_prop(docProperties, metas->size );
    mtime    = get_numeric_prop(docProperties, metas->mtime );


    /* Do a hash lookup to find the file */
    ip = lookupindexfilepath(filename, start, size);

    if (ip)  /* If existing pathname */
    {

        /* Use mtime to map to the newest entry */
        *totalfiles = *totalfiles - 1;

        if(mtime > ip->mtime) /* Current one is newer */
        {
            addtofwordtotals(sw->indexlist, ip->filenum, ftotalwords);

            /* swap meta values for properties */
            swapDocPropertyMetaNames(&docProperties, metaFile);

#ifdef PROPFILE
            /* Save memory */
            freeDocProperties(docProperties);
            *dProps = NULL;

            sw->indexlist->filearray[ip->filenum - 1]->docProperties = NULL;
#else
            sw->indexlist->filearray[ip->filenum - 1]->docProperties = docProperties;
#endif

            /* Now save ip for later (so we can load the properties) */
            // Bill, here is the fix... ip->num and sw->Index->filenum are different
            sw->indexlist->filearray[sw->Index->filenum - 1]->currentSortProp = (struct  metaEntry *)ip;
            
            ip->mtime = mtime;
            ip->indexf = indexf;
            remap(ip->filenum, num);
        } 


        else  /* Flag that this one should not be merged */
        {
            remap(num, 0);   /* Remap to 0 - word's data of this filenum will be removed later */

#ifdef PROPFILE
            /* Save memory */
            freeDocProperties(docProperties);
            *dProps = NULL;
#endif
            if ( filename )
                efree( filename );
            return;
        }
    }


    /* New path name */

    else
    {
        sw->Index->filenum++;
        remap(num, sw->Index->filenum);
    
        ip = (struct mergeindexfileinfo *) emalloc(sizeof(struct mergeindexfileinfo) + strlen(filename) + 1);

        ip->filenum = num;
        ip->start = start;
        ip->size = size;
        ip->mtime = mtime;
        ip->indexf = indexf;
        strcpy(ip->path , filename);

        hashval = bighash(ip->path);
        ip->next = indexfilehashlist[hashval];
        indexfilehashlist[hashval] = ip;

        addtofilelist(sw, sw->indexlist, filename, &thisFileEntry);

        /* don't need to addCommonProperties since they will be copied with the "real" properties */
        // addCommonProperties( sw, indexf, fprop->mtime, fprop->real_filename, summary, start, size );
        addtofwordtotals(sw->indexlist, sw->Index->filenum, ftotalwords);

        /* swap meta values for properties */
        swapDocPropertyMetaNames(&docProperties, metaFile);

#ifdef PROPFILE
        /* Save memory */
        freeDocProperties(docProperties);
        *dProps =NULL;
        thisFileEntry->docProperties = NULL;
#else
        thisFileEntry->docProperties = docProperties;
#endif

        /* Now save ip for later (so we can load the properties) */
        // Bill, here is the fix... ip->num and sw->Index->filenum are different
        sw->indexlist->filearray[sw->Index->filenum - 1]->currentSortProp = (struct  metaEntry *)ip;


    }
    
    if (sw->Index->swap_filedata)
        SwapFileData(sw, sw->indexlist->filearray[sw->Index->filenum - 1]);


    if ( filename )
        efree( filename );
}



/***********************************************************************
*  Reads file1 and file2, and writes merged files to outfile
*
* 2001-09 jmruiz - rewritten
************************************************************************/


void    readmerge(char *file1, char *file2, char *outfile, int verbose)
{
    int     i,
            indexfilenum1,
            indexfilenum2,
            wordsfilenum1,
            wordsfilenum2,
            totalfiles,
            totalwords,
            skipwords,
            skipfiles;
    struct metaMergeEntry *metaFile1,
           *metaFile2;
    SWISH  *sw1,
           *sw2,
           *sw;
    IndexFILE *indexf1,
           *indexf2,
           *indexf;
    struct file *fi;
    METAS   metas;

    int     oldnum;

    if (verbose)
        printf("Opening and reading file 1 header...\n");

    if (!(sw1 = SwishOpen(file1)))
        progerr("Couldn't read the index file \"%s\".", file1);

    if (verbose)
        printf("Opening and reading file 2 header...\n");

    if (!(sw2 = SwishOpen(file2)))
        progerr("Couldn't read the index file \"%s\".", file2);



    indexf1 = sw1->indexlist;
    indexf2 = sw2->indexlist;

    /* Output data */
    sw = SwishNew();
    indexf = sw->indexlist = addindexfile(sw->indexlist, outfile);

    /* Create and empty outfile */
    /* and open it for read/write */
    sw->indexlist->DB = (void *) DB_Create(sw, sw->indexlist->line);

    /* Force the economic mode to save memory */
    sw->Index->swap_locdata = 1;
    sw->Index->swap_filedata = 0;

    initindexfilehashlist();

    merge_index_headers( indexf, indexf1, indexf2 );

    initmapentrylist();

    if (verbose)
        printf("Counting files... ");

    indexfilenum1 = indexf1->header.totalfiles;
    indexfilenum2 = indexf2->header.totalfiles;
    wordsfilenum1 = indexf1->header.totalwords;
    wordsfilenum2 = indexf2->header.totalwords;
    totalfiles = indexfilenum1 + indexfilenum2;
    totalwords = wordsfilenum1 + wordsfilenum2;

    if (verbose)
        printf("%d files.\n", indexfilenum1 + indexfilenum2);


    /* Prepare to read the two index files */
    
    DB_InitReadFiles(sw1, indexf1->DB);
    DB_InitReadFiles(sw2, indexf2->DB);



    metaFile1 = metaFile2 = NULL;
    metaFile1 = readMergeMeta(sw1, indexf1->header.metaCounter, indexf1->header.metaEntryArray);
    metaFile2 = readMergeMeta(sw2, indexf2->header.metaCounter, indexf2->header.metaEntryArray);



    /* Create the merged list and modify the
       individual ones with the new meta index
     */
    createMetaMerge(indexf, metaFile1, metaFile2);



    /** Read in the file entries, and save them to the new index **/

    if (verbose)
        printf("\nReading file 1 info ...");
    fflush(stdout);


    /* Cache metaname lookups */
    init_metas( &metas, &sw1->indexlist->header );


    for (i = 1; i <= indexfilenum1; i++)
    {
        fi = readFileEntry(sw1, indexf1, i);
        /* 2001-09 jmruiz  - Fix to allow merge and PROPFILE */
#ifdef PROPFILE
        fi->docProperties = ReadAllDocPropertiesFromDisk( sw1, indexf1, i );
#endif
        
        addindexfilelist(sw, i, &metas, &fi->docProperties, &totalfiles, indexf1->header.filetotalwordsarray[i - 1], metaFile1, indexf1);

        freefileinfo(fi);
        indexf1->filearray[i-1] = NULL;
    }


    if (verbose)
        printf("\nReading file 2 info ...");



    /* Cache metaname lookups for index2 */
    init_metas( &metas, &sw2->indexlist->header );


    for (i = 1; i <= indexfilenum2; i++)
    {
        fi = readFileEntry(sw2, indexf2, i);
        /* 2001-09 jmruiz  - Fix to allow merge and PROPFILE */
#ifdef PROPFILE
        fi->docProperties = ReadAllDocPropertiesFromDisk( sw2, indexf2, i );
#endif
        addindexfilelist(sw, i + indexfilenum1, &metas, &fi->docProperties, &totalfiles, indexf2->header.filetotalwordsarray[i - 1], metaFile2,indexf2);

        freefileinfo(fi);
        indexf2->filearray[i-1] = NULL;
    }




    /* If we are here, we have all the files, with dups removed from filelist */
    /* So, let's write them to disk if we have PROPFILE */

    for(i = 1; i <= sw->indexlist->header.totalfiles; i++)
    {
        struct mergeindexfileinfo *ip;

        ip = (struct mergeindexfileinfo *)indexf->filearray[i - 1]->currentSortProp;

        if(!ip)
            progerr("Internal merge error. File not found while merge");

// Hi Jose,
// you only had the #ifdef PROPFILE around the WritePropertiesToDisk( sw , i );
// The ReadAllDocProperetiesFromDisk is only defined with PROPFILE so I expanded the #ifdef section
// If not PROPFILE then docProperites are already attached to the file entry, right?


#ifdef PROPFILE
        /* Attach properties from the old index file, and write them back to the new index file */

        oldnum = getold(i);

        if(ip->indexf == indexf1)
            indexf->filearray[i - 1]->docProperties = ReadAllDocPropertiesFromDisk( sw1, indexf1, oldnum );
        else
            indexf->filearray[i - 1]->docProperties = ReadAllDocPropertiesFromDisk( sw2, indexf2, oldnum - indexfilenum1);

        /* write properties to disk, and release memory */
        WritePropertiesToDisk( sw , i );

        /* Now clean up the file entry */
        if(ip->indexf == indexf1)
        {
            indexf1->filearray[oldnum - 1]->docProperties = NULL;
            freefileinfo(indexf1->filearray[oldnum - 1]);
            indexf1->filearray[oldnum - 1] = NULL;
        }
        else 
        {
            indexf2->filearray[oldnum - indexfilenum1 - 1]->docProperties = NULL;
            freefileinfo(indexf2->filearray[oldnum -indexfilenum1 - 1]); 
            indexf2->filearray[oldnum - indexfilenum1 - 1] = NULL;
        }
#endif
    }

    /* Get rid of this data - We do not longer need it */
    freemergeindexfileinfo();

    if (verbose)
        printf("\nCreating output file ... ");



    if (verbose)
        printf("\nMerging words... ");

    skipwords = merge_words( sw, sw1, sw2, indexf1, indexf2, metaFile1, metaFile2, indexfilenum1, indexfilenum2 );


    if (verbose)
    {
        if (skipwords)
            printf("%d redundant word%s.\n", skipwords, (skipwords == 1) ? "" : "s");
        else
            printf("no redundant words.\n");
    }

    if (verbose)
        printf("\nSorting words ... ");

    /* Words are sorted in merge - So this should be useless but the routine also builds */
    /* an array with the words (elist) */
    sort_words(sw, indexf); 


    if (verbose)
        printf("\nPrinting header... ");

    write_header(sw, &indexf->header, indexf->DB, outfile, (totalwords - skipwords), totalfiles, 1);

    if (verbose)
        printf("\nPrinting words... \n");

    if (verbose)
        printf("Writing index entries ...\n");

    write_index(sw, indexf);

    if (verbose)
        printf("\nMerging file info... ");

    write_file_list(sw, indexf);

    write_sorted_index(sw, indexf);

    skipfiles = (indexfilenum1 + indexfilenum2) - totalfiles;

    if (verbose)
    {
        if (skipfiles)
            printf("%d redundant file%s.\n", skipfiles, (skipfiles == 1) ? "" : "s");
        else
            printf("no redundant files.\n");
    }

    SwishClose(sw1);
    SwishClose(sw2);

    SwishClose(sw);

    if (verbose)
        printf("\nDone.\n");
}


