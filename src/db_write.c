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

**
** 2001-05-07 jmruiz init coding
**
*/

#include "swish.h"
#include "mem.h"
#include "swstring.h"
#include "index.h"
#include "hash.h"
#include "date_time.h"
#include "compress.h"
#include "error.h"
#include "metanames.h"
#include "sw_db.h"
#include "db_native.h"
// #include "db_berkeley_db.h"



#ifndef min
#define min(a, b)    (a) < (b) ? a : b
#endif


/* General write DB routines - Common to all DB */



static int    write_hash_words_to_header(SWISH *sw, int header_ID, struct swline **hash, void *DB);




/* Header routines */

#define write_header_int(sw,id,num,DB) {unsigned long itmp = (num); itmp = PACKLONG(itmp); DB_WriteHeaderData((sw),(id), (unsigned char *)&itmp, sizeof(long), (DB));}
#define write_header_int2(sw,id,num1,num2,DB)\
{  \
        unsigned long itmp[2]; \
        itmp[0] = (num1); \
        itmp[1] = (num2); \
        itmp[0] = PACKLONG(itmp[0]); \
        itmp[1] = PACKLONG(itmp[1]); \
        DB_WriteHeaderData((sw),(id), (unsigned char *)itmp, sizeof(long) * 2, (DB)); \
}
#define write_header_int4(sw,id,num1,num2,num3,num4,DB)\
{  \
        unsigned long itmp[4]; \
        itmp[0] = (num1); \
        itmp[1] = (num2); \
        itmp[2] = (num3); \
        itmp[3] = (num4); \
        itmp[0] = PACKLONG(itmp[0]); \
        itmp[1] = PACKLONG(itmp[1]); \
        itmp[2] = PACKLONG(itmp[2]); \
        itmp[3] = PACKLONG(itmp[3]); \
        DB_WriteHeaderData((sw),(id), (unsigned char *)itmp, sizeof(long) * 4, (DB)); \
}

void    write_header(SWISH *sw, int merged_flag )
{
    IndexFILE       *indexf     = sw->indexlist;       /* first element in the list */
    INDEXDATAHEADER *header     = &indexf->header;
    char            *filename   = indexf->line;
    void            *DB         = indexf->DB;
    char            *c;
    char            *tmp;


    /* $$$ this isn't portable */
    c = (char *) strrchr(filename, '/');
    if (!c || (c && !*(c + 1)))
        c = filename;
    else
        c += 1;

    DB_InitWriteHeader(sw, DB);

    DB_WriteHeaderData(sw, INDEXHEADER_ID, (unsigned char *)INDEXHEADER, strlen(INDEXHEADER) +1, DB);
    DB_WriteHeaderData(sw, INDEXVERSION_ID, (unsigned char *)INDEXVERSION, strlen(INDEXVERSION) + 1, DB);
    write_header_int(sw, MERGED_ID, merged_flag, DB);
    DB_WriteHeaderData(sw, NAMEHEADER_ID, (unsigned char *)header->indexn, strlen(header->indexn) + 1, DB);
    DB_WriteHeaderData(sw, SAVEDASHEADER_ID, (unsigned char *)c, strlen(c) + 1, DB);

    write_header_int4(sw, COUNTSHEADER_ID,
            header->totalwords,
            header->totalfiles,
            header->total_word_positions + indexf->total_word_positions_cur_run, /* Total this run's total words (not unique) with any previous */
            header->removedfiles, DB);

    tmp = getTheDateISO();
    DB_WriteHeaderData(sw, INDEXEDONHEADER_ID, (unsigned char *)tmp, strlen(tmp) + 1,DB);
    efree(tmp);
    DB_WriteHeaderData(sw, DESCRIPTIONHEADER_ID, (unsigned char *)header->indexd, strlen(header->indexd) + 1, DB);
    DB_WriteHeaderData(sw, POINTERHEADER_ID, (unsigned char *)header->indexp, strlen(header->indexp) + 1, DB);
    DB_WriteHeaderData(sw, MAINTAINEDBYHEADER_ID, (unsigned char *)header->indexa, strlen(header->indexa) + 1,DB);
    write_header_int(sw, DOCPROPENHEADER_ID, 1, DB);

    write_header_int(sw, FUZZYMODEHEADER_ID, (int)fuzzy_mode_value(header->fuzzy_data), DB);

    write_header_int(sw, IGNORETOTALWORDCOUNTWHENRANKING_ID, header->ignoreTotalWordCountWhenRanking, DB);
    DB_WriteHeaderData(sw, WORDCHARSHEADER_ID, (unsigned char *)header->wordchars, strlen(header->wordchars) + 1, DB);
    write_header_int(sw, MINWORDLIMHEADER_ID, header->minwordlimit, DB);
    write_header_int(sw, MAXWORDLIMHEADER_ID, header->maxwordlimit, DB);
    DB_WriteHeaderData(sw, BEGINCHARSHEADER_ID, (unsigned char *)header->beginchars, strlen(header->beginchars) + 1, DB);
    DB_WriteHeaderData(sw, ENDCHARSHEADER_ID, (unsigned char *)header->endchars, strlen(header->endchars) + 1, DB);
    DB_WriteHeaderData(sw, IGNOREFIRSTCHARHEADER_ID, (unsigned char *)header->ignorefirstchar, strlen(header->ignorefirstchar) + 1, DB);
    DB_WriteHeaderData(sw, IGNORELASTCHARHEADER_ID, (unsigned char *)header->ignorelastchar, strlen(header->ignorelastchar) + 1,DB);


    /* Jose Ruiz 06/00 Added this line to delimite the header */
    write_integer_table_to_header(sw, TRANSLATECHARTABLE_ID, header->translatecharslookuptable, sizeof(header->translatecharslookuptable) / sizeof(int), DB);

    /* Other header stuff */

    /* StopWords */
    write_hash_words_to_header(sw, STOPWORDS_ID, header->hashstoplist.hash_array, DB);


    /* Metanames */
    write_MetaNames(sw, METANAMES_ID, header, DB);

    /* BuzzWords */
    write_hash_words_to_header(sw, BUZZWORDS_ID, header->hashbuzzwordlist.hash_array, DB);

    write_header_int(sw, TOTALWORDS_REMOVED_ID, header->removed_word_positions, DB);

    DB_EndWriteHeader(sw, DB);
}


/* Jose Ruiz 11/00
** Function to write a word to the index DB
*/
void    write_word(SWISH * sw, ENTRY * ep, IndexFILE * indexf)
{
    SW_INT32    wordID;

    wordID = DB_GetWordID(sw, indexf->DB);

    DB_WriteWord(sw, ep->word,wordID,indexf->DB);
        /* Store word offset for futher hash computing */
    ep->u1.wordID = wordID;

}

/* Jose Ruiz 11/00
** Function to write all word's data to the index DB
*/

void build_worddata(SWISH * sw, ENTRY * ep)
{
    int     curmetaID,
            sz_worddata;
    unsigned long    tmp,
            curmetanamepos;
    int     metaID;
    int     chunk_size;
    unsigned char *compressed_data,
           *p,*q;
    LOCATION *l, *next;


    curmetaID=0;
    curmetanamepos=0L;
    q=sw->Index->worddata_buffer;

    /* Write tfrequency */
    q = compress3(ep->tfrequency,q);

    /* Write location list */
    for(l=ep->allLocationList;l;)
    {
        compressed_data = (unsigned char *) l;

        /* Get next element */
        next = *(LOCATION **)compressed_data;

        /* Jump pointer to next element */
        p = compressed_data + sizeof(LOCATION *);

        metaID = uncompress2(&p);

        memcpy((char *)&chunk_size,(char *)p,sizeof(chunk_size));
        p += sizeof(chunk_size);

        if(curmetaID!=metaID)
        {
            if(curmetaID)
            {
                /* Write in previous meta (curmetaID)
                ** file offset to next meta */
                tmp=q - sw->Index->worddata_buffer;
                PACKLONG2(tmp,sw->Index->worddata_buffer+curmetanamepos);
            }
                /* Check for enough memory */
                /*
                ** MAXINTCOMPSIZE is for the worst case metaID
                **
                ** sizeof(long) is to leave four bytes to
                ** store the offset of the next metaname
                ** (it will be 0 if no more metanames).
                **
                ** 1 is for the trailing '\0'
                */

            tmp=q - sw->Index->worddata_buffer;

            if((long)(tmp + MAXINTCOMPSIZE + sizeof(long) + 1) >= (long)sw->Index->len_worddata_buffer)
            {
                sw->Index->len_worddata_buffer=sw->Index->len_worddata_buffer*2+MAXINTCOMPSIZE+sizeof(long)+1;
                sw->Index->worddata_buffer=(unsigned char *) erealloc(sw->Index->worddata_buffer,sw->Index->len_worddata_buffer);
                q=sw->Index->worddata_buffer+tmp;   /* reasign pointer inside buffer */
            }

            /* store metaID in buffer */
            curmetaID=metaID;
            q = compress3(curmetaID,q);


            /* preserve position for offset to next
            ** metaname. We do not know its size
            ** so store it as a packed long */
            curmetanamepos=q - sw->Index->worddata_buffer;

            /* Store 0 and increase pointer */
            tmp=0L;
            PACKLONG2(tmp,q);
            q+=sizeof(unsigned long);

        }


        /* Store all data for this chunk */
        /* First check for enough space
        **
        ** 1 is for the trailing '\0'
        */

        tmp=q - sw->Index->worddata_buffer;

        if((long)(tmp + chunk_size + 1) >= (long)sw->Index->len_worddata_buffer)
        {
            sw->Index->len_worddata_buffer=sw->Index->len_worddata_buffer*2+chunk_size+1;
            sw->Index->worddata_buffer=(unsigned char *) erealloc(sw->Index->worddata_buffer,sw->Index->len_worddata_buffer);
            q=sw->Index->worddata_buffer+tmp;   /* reasign pointer inside buffer */
        }

        /* Copy it and advance pointer */
        memcpy(q,p,chunk_size);
        q += chunk_size;

        /* End of chunk mark -> Write trailing '\0' */
        *q++ = '\0';

        l = next;
    }

    /* Write in previous meta (curmetaID)
    ** file offset to end of metas */
    tmp=q - sw->Index->worddata_buffer;
    PACKLONG2(tmp,sw->Index->worddata_buffer+curmetanamepos);

    sz_worddata = q - sw->Index->worddata_buffer;

    /* Adjust word positions.
    ** if ignorelimit was set and some new stopwords weee found, positions
    ** are recalculated
    ** Also call it even if we have not set IgnoreLimit to calesce word chunks
    ** and remove trailing 0 from chunks to save some bytes
    */
    adjustWordPositions(sw->Index->worddata_buffer, &sz_worddata, sw->indexlist->header.totalfiles, sw->Index->IgnoreLimitPositionsArray);

    sw->Index->sz_worddata_buffer = sz_worddata;
}


/* 04/2002 jmruiz
** New simpler routine to write worddata
*/
void write_worddata(SWISH * sw, ENTRY * ep, IndexFILE * indexf )
{
    int zlib_size;

    if(sw->compressPositions)
        zlib_size = compress_worddata(sw->Index->worddata_buffer, sw->Index->sz_worddata_buffer,sw->Index->swap_locdata);
    else
        zlib_size = sw->Index->sz_worddata_buffer;

    /* Write worddata */
    DB_WriteWordData(sw, ep->u1.wordID,sw->Index->worddata_buffer,zlib_size, sw->Index->sz_worddata_buffer - zlib_size ,indexf->DB);

}

/* Writes the list of metaNames into the DB index
 *  (should maybe be in metanames.c)
*/

void    write_MetaNames(SWISH *sw, int id, INDEXDATAHEADER * header, void *DB)
{
    struct metaEntry *entry = NULL;
    int     i,
            sz_buffer,
            len;
    unsigned char *buffer,*s;
    int     fields;

    /* Use new metaType schema - see metanames.h */
    // Format of metaname is
    //   <len><metaName><metaType><Alias><rank_bias>
    //   len, metaType, alias, and rank_bias are compressed numbers
    //   metaName is the ascii name of the metaname
    //
    // The list of metanames is delimited by a 0

    fields = 5;  // len, metaID, metaType, alias, rank_bias


    /* Compute buffer size */
    for (sz_buffer = 0 , i = 0; i < header->metaCounter; i++)
    {
        entry = header->metaEntryArray[i];
        len = strlen(entry->metaName);
        sz_buffer += len + fields * MAXINTCOMPSIZE; /* compress can use MAXINTCOMPSIZE bytes in worse case,  */
    }

    sz_buffer += MAXINTCOMPSIZE;  /* Add extra MAXINTCOMPSIZE for the number of metanames */

    s = buffer = (unsigned char *) emalloc(sz_buffer);

    s = compress3(header->metaCounter,s);   /* store the number of metanames */

    for (i = 0; i < header->metaCounter; i++)
    {
        entry = header->metaEntryArray[i];
        len = strlen(entry->metaName);
        s = compress3(len, s);
        memcpy(s,entry->metaName,len);
        s += len;
        s = compress3(entry->metaID, s);
        s = compress3(entry->metaType, s);
        s = compress3(entry->alias+1, s);  /* keep zeros away from compress3, I believe */
        s = compress3(entry->sort_len, s);
        s = compress3(entry->rank_bias+RANK_BIAS_RANGE+1, s);
    }
    DB_WriteHeaderData(sw, id,buffer,s-buffer,DB);
    efree(buffer);
}



/* Write a the hashlist of words into the index header file (used by stopwords and buzzwords
*/

static int    write_hash_words_to_header(SWISH *sw, int header_ID, struct swline **hash, void *DB)
{
    int     hashval,
            len,
            num_words,
            sz_buffer;
    char   *buffer, *s;
    struct swline *sp = NULL;

    /* Let's count the words */

    if ( !hash )
        return 0;

    for (sz_buffer = 0, num_words = 0 , hashval = 0; hashval < HASHSIZE; hashval++)
    {
        sp = hash[hashval];
        while (sp != NULL)
        {
            num_words++;
            sz_buffer += MAXINTCOMPSIZE + strlen(sp->line);
            sp = sp->next;
        }
    }

    if(num_words)
    {
        sz_buffer += MAXINTCOMPSIZE;  /* Add MAXINTCOMPSIZE for the number of words */

        s = buffer = (char *)emalloc(sz_buffer);

        s = (char *)compress3(num_words, (unsigned char *)s);

        for (hashval = 0; hashval < HASHSIZE; hashval++)
        {
            sp = hash[hashval];
            while (sp != NULL)
            {
                len = strlen(sp->line);
                s = (char *)compress3(len,(unsigned char *)s);
                memcpy(s,sp->line,len);
                s +=len;
                sp = sp->next;
            }
        }
        DB_WriteHeaderData(sw, header_ID, (unsigned char *)buffer, s - buffer, DB);
        efree(buffer);
    }
    return 0;
}



int write_integer_table_to_header(SWISH *sw, int id, int table[], int table_size, void *DB)
{
    int     i,
            tmp;
    char   *s;
    char   *buffer;

    s = buffer = (char *) emalloc((table_size + 1) * MAXINTCOMPSIZE);

    s = (char *)compress3(table_size,(unsigned char *)s);   /* Put the number of elements */
    for (i = 0; i < table_size; i++)
    {
        tmp = table[i] + 1;
        s = (char *)compress3(tmp, (unsigned char *)s); /* Put all the elements */
    }

    DB_WriteHeaderData(sw, id, (unsigned char *)buffer, s-buffer, DB);

    efree(buffer);
    return 0;
}



void setTotalWordsPerFile(IndexFILE *indexf, int idx,int wordcount)
{
        DB_WriteTotalWordsPerFile(indexf->sw, idx, wordcount, indexf->DB);
}



/*------------------------------------------------------*/
/*---------- General entry point of DB module ----------*/

void   *DB_Create (SWISH *sw, char *dbname)
{
   return _DB_Create(sw, dbname);
}

void    DB_Remove(SWISH *sw, void *DB)
{
   _DB_Remove(DB);
}

int     DB_InitWriteHeader(SWISH *sw, void *DB)
{
   return _DB_InitWriteHeader(DB);
}

int     DB_WriteHeaderData(SWISH *sw, int id, unsigned char *s, int len, void *DB)
{
   return _DB_WriteHeaderData(id, s,len,DB);
}

int     DB_EndWriteHeader(SWISH *sw, void *DB)
{
   return _DB_EndWriteHeader(DB);
}



int     DB_InitWriteWords(SWISH *sw, void *DB)
{
   return _DB_InitWriteWords(DB);
}

SW_INT32    DB_GetWordID(SWISH *sw, void *DB)
{
   return _DB_GetWordID(DB);
}

int     DB_WriteWord(SWISH *sw, char *word, sw_off_t wordID, void *DB)
{
   return _DB_WriteWord(word, wordID, DB);
}

long    DB_WriteWordData(SWISH *sw, sw_off_t wordID, unsigned char *worddata, int data_size, int saved_bytes, void *DB)
{
   return _DB_WriteWordData(wordID, worddata, data_size, saved_bytes, DB);
}

int     DB_EndWriteWords(SWISH *sw, void *DB)
{
   return _DB_EndWriteWords(DB);
}


int     DB_InitWriteProperties(SWISH *sw, void *DB)
{
   return _DB_InitWriteProperties(DB);
}


int     DB_WriteFileNum(SWISH *sw, int filenum, unsigned char *filedata,int sz_filedata, void *DB)
{
   return _DB_WriteFileNum(filenum, filedata, sz_filedata, DB);
}

int     DB_RemoveFileNum(SWISH *sw, int filenum, void *DB)
{
   return _DB_RemoveFileNum(filenum, DB);
}


int     DB_InitWriteSortedIndex(SWISH *sw, void *DB)
{
   return _DB_InitWriteSortedIndex(DB);
}

int     DB_WriteSortedIndex(SWISH *sw, int propID, unsigned char *data, int sz_data,void *DB)
{
   return _DB_WriteSortedIndex(propID, data, sz_data,DB);
}


int     DB_EndWriteSortedIndex(SWISH *sw, void *DB)
{
   return _DB_EndWriteSortedIndex(DB);
}


void DB_WriteProperty( SWISH *sw, IndexFILE *indexf, FileRec *fi, int propID, char *buffer, int buf_len, int uncompressed_len, void *db)
{
    _DB_WriteProperty( indexf, fi, propID, buffer, buf_len, uncompressed_len, db );
}

void    DB_WritePropPositions(SWISH *sw, IndexFILE *indexf, FileRec *fi, void *db)
{
    _DB_WritePropPositions( indexf, fi, db);
}


void    DB_Reopen_PropertiesForRead(SWISH *sw, void *DB )
{
    _DB_Reopen_PropertiesForRead(DB);
}


int    DB_WriteTotalWordsPerFile(SWISH *sw, int idx, int wordcount, void *DB)
{
    return _DB_WriteTotalWordsPerFile(sw, idx, wordcount, DB);
}



