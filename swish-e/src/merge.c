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
#include "string.h"
#include "merge.h"
#include "error.h"
#include "search.h"
#include "index.h"
#include "hash.h"
#include "mem.h"
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
    int     oldnum;
    int     newnum;
};
struct mapentry *mapentrylist[BIGHASHSIZE];


struct mergeindexfileinfo
{
    struct mergeindexfileinfo *next;
    int     filenum;
    char   *path;
    int     start;
    int     size;
};
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

    if ((meta_entry->metaID < docProperties->n) && (p = docProperties->propEntry[meta_entry->metaID]))
    {
        i = *(unsigned long *) p->propValue;
        return (int) UNPACKLONG(i);
    }

    return 0;
}






/* This reads the next line in the index file and puts the results
** in a result structure.
*/

static ENTRY  *readindexline(SWISH * sw, IndexFILE * indexf, struct metaMergeEntry *metaFile, int c, int *is_first)
{
    int     j,
            x,
            tfrequency,
            filenum,
            structure,
            metaID,
            metaID2,
            frequency,
           *position,
            index_structure,
            index_structfreq;
    LOCATION *loc;
    ENTRY  *ip;
    struct metaMergeEntry *tmp = NULL;
    unsigned long nextposmetaname;
    unsigned char word[2];
    char   *resultword;
    long    wordID;
    unsigned char *buffer,
           *s;
    int     sz_buffer;

    word[0] = (unsigned char) c;
    word[1] = '\0';

    j = tfrequency = filenum = structure = metaID = frequency = 0;
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

    ip = (ENTRY *) emalloc(sizeof(ENTRY) + strlen(resultword) + 1);
    strcpy(ip->word, resultword);
    ip->locationarray = (LOCATION **) emalloc(sizeof(LOCATION *));
    ip->u1.max_locations = 0;
    ip->currentlocation = 0;

    /* read Word data */
    DB_ReadWordData(sw, wordID, &buffer, &sz_buffer, indexf->DB);
    s = buffer;

    /* parse word data to add it to the structure */
    tfrequency = uncompress2(&s);
    ip->tfrequency = tfrequency;

    metaID = uncompress2(&s);
    while (metaID)
    {
        nextposmetaname = UNPACKLONG2(s);
        s += sizeof(long);

        do
        {
            filenum = uncompress2(&s);
            index_structfreq = uncompress2(&s);
            frequency = indexf->header.structfreqlookup->all_entries[index_structfreq - 1]->val[0];
            index_structure = indexf->header.structfreqlookup->all_entries[index_structfreq - 1]->val[1];
            structure = indexf->header.structurelookup->all_entries[index_structure - 1]->val[0];

            loc = (LOCATION *) emalloc(sizeof(LOCATION) + (frequency - 1) * sizeof(int));

            loc->filenum = filenum;
            loc->structure = structure;
            loc->frequency = frequency;
            for (j = 0; j < frequency; j++)
            {
                x = uncompress2(&s);
                loc->position[j] = x;
            }
            /*Need to modify metaID with new list */
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

            if (!ip->u1.max_locations)
                ip->locationarray = (LOCATION **) emalloc((++ip->u1.max_locations) * sizeof(LOCATION *));
            else
                ip->locationarray = (LOCATION **) erealloc(ip->locationarray, (++ip->u1.max_locations) * sizeof(LOCATION *));
            ip->locationarray[ip->u1.max_locations - 1] = loc;
        }
        while ((unsigned long) (s - buffer) != nextposmetaname);
        metaID = uncompress2(&s);
    }
    return ip;
}

/* This puts all the file info into a hash table so that it can
** be looked up by its pathname and filenumber. This is how
** we find redundant file information.
*/



/* This returns the file number corresponding to a pathname.
*/

static int     lookupindexfilepath(char *path, int start, int size)
{
    unsigned hashval;
    struct mergeindexfileinfo *ip;

    hashval = bighash(path);
    ip = indexfilehashlist[hashval];

    while (ip != NULL)
    {
        if ((strcmp(ip->path, path) == 0) && ip->start == start && ip->size == size)
            return ip->filenum;
        ip = ip->next;
    }
    return -1;
}

/* This simply concatenates two information lists that correspond
** to a word found in both index files.
*/

static ENTRY  *mergeindexentries(ENTRY * ip1, ENTRY * ip2, int num)
{
    ENTRY  *ep;
    LOCATION **lap;
    int     i,
            j;

    lap = (LOCATION **) emalloc((ip1->u1.max_locations + ip2->u1.max_locations) * sizeof(LOCATION *));
    for (j = 0; j < ip1->u1.max_locations; j++)
        lap[j] = ip1->locationarray[j];
    for (i = 0; i < ip2->u1.max_locations; i++)
    {
        if (ip2->locationarray[i]->filenum > num)
        {
            lap[j++] = ip2->locationarray[i];
        }
    }

    ep = (ENTRY *) emalloc(sizeof(ENTRY) + strlen(ip1->word) + 1);
    strcpy(ep->word, ip1->word);
    ep->locationarray = lap;
    ep->u1.max_locations = j;
    ep->currentlocation = 0;
    ep->tfrequency = ip1->tfrequency + ip2->tfrequency;

    efree(ip1->locationarray);
    efree(ip1);
    efree(ip2->locationarray);
    efree(ip2);

    return ep;
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
}

/* This retrieves the number associated with another.
*/

static int     getmap(int num)
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


/* This adds an offset to the file numbers in a particular
** result list. For instance, file 1 has file numbers going from
** 1 to 10, but so does file 2, so I have to add 10 to all the
** file numbers in file 2 before merging.
*/

static void    addfilenums(ENTRY * ip, int num)
{
    int     i;

    for (i = 0; i < ip->u1.max_locations; i++)
        ip->locationarray[i]->filenum = getmap(ip->locationarray[i]->filenum + num);
}



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

/********** NOT USED (moseley Jun 16, 01) 
static void    initmarkentrylistMerge()
{
    int     i;
    struct markentryMerge *mp;

    for (i = 0; i < BIGHASHSIZE; i++)
    {
        mp = markentrylistMerge[i];
        markentrylistMerge[i] = NULL; 
        if (mp != NULL)
            efree(mp);
    }
}
********************/

/* Initialize the main file list.
*/

void    initindexfilehashlist()
{
    int     i;
    struct mergeindexfileinfo *ip;

    for (i = 0; i < BIGHASHSIZE; i++)
    {
        ip = indexfilehashlist[i];
        if (ip != NULL)
            efree(ip);
        indexfilehashlist[i] = NULL;
    }
}

/* Initialize the mapentrylist 
*/

static void    initmapentrylist()
{
    int     i;
    struct mapentry *ip;

    for (i = 0; i < BIGHASHSIZE; i++)
    {
        ip = mapentrylist[i];
        if (ip != NULL)
            efree(ip);
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
 */

static struct metaEntry **addMetaMergeArray(struct metaEntry **metaEntryArray, struct metaMergeEntry *metaFileEntry, int *count)
{
    int     newMetaID;
    struct metaEntry *newEntry;
    struct metaEntry *tmpEntry;
    char   *metaWord;
    int     metaType = 0;
    int     i;

    newMetaID = 0;

    metaWord = metaFileEntry->metaName;
    metaType = metaFileEntry->metaType;

    if (metaEntryArray)
    {
        for (i = 0; i < (*count); i++)
        {
            tmpEntry = metaEntryArray[i];

            if (strcmp(tmpEntry->metaName, metaWord) == 0)
            {
                newMetaID = tmpEntry->metaID;

                /*
                 * Keep the docProperties fields in synch.
                 * The semantics we want for the metaEntry are:
                 * set META_PROP if either index is using as PropertyName
                 * set META if either index is using as MetaName
                 */

                if (metaType & META_PROP) /* new entry is docProp, so assert it */
                    tmpEntry->metaType |= META_PROP;

                if (metaType & META_INDEX) /* new entry is not *only* docProp, so unassert that */
                    tmpEntry->metaType |= META_INDEX;


                /* Finally check that the rest of the */
                /* metaType does match */

                if (!match_meta_type(metaType, tmpEntry->metaType))
                {
                    fprintf(stderr, "Couldn't merge: metaname \"%s\" :", metaWord);
                    progerr("types do not match.");
                }
                break;
            }
        }



        if (i < (*count))       /* metaname exists */
        {
            metaFileEntry->newMetaID = newMetaID;
        }
        else
        {
            metaEntryArray = (struct metaEntry **) erealloc(metaEntryArray, ((*count) + 1) * sizeof(struct metaEntry *));
            newEntry = (struct metaEntry *) emalloc(sizeof(struct metaEntry));

            newEntry->metaName = (char *) estrdup(metaWord);
            newEntry->metaID = (*count) + AUTOPROP_ID__DOCPATH;
            newEntry->metaType = metaType;
            newEntry->sorted_data = NULL;
			newEntry->inPropRange = NULL;
            metaFileEntry->newMetaID = (*count) + AUTOPROP_ID__DOCPATH;;
            metaEntryArray[(*count)++] = newEntry;
        }
    }
    else //   if (!metaEntryArray)
    {
        metaEntryArray = (struct metaEntry **) emalloc(sizeof(struct metaEntry *));
        newEntry = (struct metaEntry *) emalloc(sizeof(struct metaEntry));

        newEntry->metaName = (char *) estrdup(metaWord);
        newEntry->metaID = AUTOPROP_ID__DOCPATH;
        newEntry->metaType = metaType;
        newEntry->sorted_data = NULL;
		newEntry->inPropRange = NULL;
        metaFileEntry->newMetaID = AUTOPROP_ID__DOCPATH;;
        *count = 1;
        metaEntryArray[0] = newEntry;
    }


    return metaEntryArray;
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

    /* In merge there is no dup !!! */
    CompressCurrentLocEntry(sw, indexf, ip);

}

/* Creates a list of all the meta names in the indexes
*/
struct metaEntry **createMetaMerge(struct metaMergeEntry *metaFile1, struct metaMergeEntry *metaFile2, int *metaCounter)
{
    struct metaMergeEntry *tmpEntry;
    int     counter;
    struct metaEntry **metaEntryArray = NULL;

    counter = 0;

    for (tmpEntry = metaFile1; tmpEntry; tmpEntry = tmpEntry->next)
        metaEntryArray = addMetaMergeArray(metaEntryArray, tmpEntry, &counter);

    for (tmpEntry = metaFile2; tmpEntry; tmpEntry = tmpEntry->next)
        metaEntryArray = addMetaMergeArray(metaEntryArray, tmpEntry, &counter);
    *metaCounter = counter;
    return metaEntryArray;
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
                    ip1 = readindexline(sw1, indexf1, metaFile1, j, &is_first1);
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
                    ip2 = readindexline(sw2, indexf2, metaFile2, j, &is_first2);
                if (ip2 == NULL)
                {
                    endip2 = 1;
                    if (ip1 == NULL)
                    {
                        break;
                    }
                }
                else
                    addfilenums(ip2, indexfilenum1);
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
        }
    }
    DB_EndReadWords(sw1, indexf1->DB);
    DB_EndReadWords(sw2, indexf2->DB);

    return skipwords;
}


/***********************************************************************
*  adds a file and it's associated properties to the output index
*
*
************************************************************************/

static void addindexfilelist(SWISH * sw, int num, char *filename, struct docProperties *docProperties, int *totalfiles, int ftotalwords,
                             struct metaMergeEntry *metaFile)
{
    int     i,
            hashval;
    struct file *thisFileEntry = NULL;
    struct mergeindexfileinfo *ip;
    int     start,
            size;

    //addtofilelist(sw, sw->indexlist, filename, &thisFileEntry);
    //return;

    start = get_numeric_prop(docProperties, sw->indexlist->header.startProp);
    size = get_numeric_prop(docProperties, sw->indexlist->header.sizeProp);


    i = lookupindexfilepath(filename, start, size);
    if (i != -1)
    {
        *totalfiles = *totalfiles - 1;
        remap(num, i);
        return;
    }

    sw->Index->filenum++;
    remap(num, sw->Index->filenum);

    ip = (struct mergeindexfileinfo *) emalloc(sizeof(struct mergeindexfileinfo));

    ip->filenum = num;
    ip->path = (char *) estrdup(filename);
    ip->start = start;
    ip->size = size;

    hashval = bighash(ip->path);
    ip->next = indexfilehashlist[hashval];
    indexfilehashlist[hashval] = ip;

    addtofilelist(sw, sw->indexlist, filename, &thisFileEntry);
    /* don't need to addCommonProperties since they will be copied with the "real" properties */
    // addCommonProperties( sw, indexf, fprop->mtime, fprop->real_filename, summary, start, size );


    addtofwordtotals(sw->indexlist, sw->Index->filenum, ftotalwords);


    /* swap meta values for properties */
    swapDocPropertyMetaNames(&docProperties, metaFile);

    thisFileEntry->docProperties = docProperties;


    if (sw->Index->economic_flag)
        SwapFileData(sw, sw->indexlist->filearray[sw->Index->filenum - 1]);

}



/***********************************************************************
*  Reads file1 and file2, and writes merged files to outfile
*
*
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

    if (verbose)
        printf("Opening and reading file 1 header...\n");

    if (!(sw1 = SwishOpen(file1)))
    {
        progerr("Couldn't read the index file \"%s\".", file1);
    }

    if (verbose)
        printf("Opening and reading file 2 header...\n");

    if (!(sw2 = SwishOpen(file2)))
    {
        progerr("Couldn't read the index file \"%s\".", file2);
    }

    indexf1 = sw1->indexlist;
    indexf2 = sw2->indexlist;

    /* Output data */
    sw = SwishNew();
    indexf = sw->indexlist = addindexfile(sw->indexlist, outfile);

    /* Create and empty outfile */
    /* and open it for read/write */
    sw->indexlist->DB = (void *) DB_Create(sw, sw->indexlist->line);

    /* Force the economic mode to save memory */
    sw->Index->economic_flag = 1;

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
    indexf->header.metaEntryArray = createMetaMerge(metaFile1, metaFile2,&indexf->header.metaCounter);



    /** Read in the file entries, and save them to the new index **/

    if (verbose)
        printf("\nReading file 1 info ...");
    fflush(stdout);


    for (i = 1; i <= indexfilenum1; i++)
    {
        fi = readFileEntry(sw1, indexf1, i);
        addindexfilelist(sw, i, fi->filename, fi->docProperties, &totalfiles, indexf1->header.filetotalwordsarray[i - 1], metaFile1);
    }




    if (verbose)
        printf("\nReading file 2 info ...");

    for (i = 1; i <= indexfilenum2; i++)
    {
        fi = readFileEntry(sw2, indexf2, i);
        addindexfilelist(sw, i + indexfilenum1, fi->filename, fi->docProperties, &totalfiles, indexf2->header.filetotalwordsarray[i - 1], metaFile2);
    }

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


