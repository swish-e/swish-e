/*
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
**
**
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
#include "db.h"
#include "db_native.h"
#include "stemmer.h"
// #include "db_berkeley_db.h"

#ifndef min
#define min(a, b)    (a) < (b) ? a : b
#endif

/*
  -- init structures for this module
*/

void initModule_DB (SWISH  *sw)
{
          /* Allocate structure */
   initModule_DBNative(sw);
   // initModule_DB_db(sw);
   return;
}


/*
  -- release all wired memory for this module
*/

void freeModule_DB (SWISH *sw)
{
  freeModule_DBNative(sw);
  // freeModule_DB_db(sw);
  return;
}



/* ---------------------------------------------- */





static void load_word_hash_from_buffer(WORD_HASH_TABLE *table_ptr, char *buffer);


/* 04/2002 jmruiz
** Function to read all word's data from the index DB
*/





sw_off_t read_worddata(SWISH * sw, ENTRY * ep, IndexFILE * indexf, unsigned char **buffer, int *sz_buffer)
{
sw_off_t wordID;
char *word = ep->word;
int saved_bytes = 0;

    DB_InitReadWords(sw, indexf->DB);
    DB_ReadWordHash(sw, word, &wordID, indexf->DB);

    if(!wordID)
    {    
        DB_EndReadWords(sw, indexf->DB);
        sw->lasterror = WORD_NOT_FOUND;
        *buffer = NULL;
        *sz_buffer = 0;
        return (sw_off_t)0;
   } 
   DB_ReadWordData(sw, wordID, buffer, sz_buffer, &saved_bytes, indexf->DB);
   uncompress_worddata(buffer,sz_buffer,saved_bytes);
   DB_EndReadWords(sw, indexf->DB);
   return wordID;
}





/* General read DB routines - Common to all DB */

/* Reads the file offset table in the index file.
*/

/* Reads and prints the header of an index file.
** Also reads the information in the header (wordchars, beginchars, etc)
*/

// $$$ to be rewritten as function = smaller code (rasc)

#define parse_int_from_buffer(num,s) (num) = UNPACKLONG2((s))
#define parse_int2_from_buffer(num1,num2,s) (num1) = UNPACKLONG2((s));(num2) = UNPACKLONG2((s+sizeof(long)))
#define parse_int3_from_buffer(num1,num2,num3,s) (num1) = UNPACKLONG2((s));(num2) = UNPACKLONG2((s+sizeof(long))); (num3) = UNPACKLONG2((s+sizeof(long)+sizeof(long)))
#define parse_int4_from_buffer(num1,num2,num3,num4,s) \
{ \
	(num1) = UNPACKLONG2((s));\
	(num2) = UNPACKLONG2((s+sizeof(long)));\
	(num3) = UNPACKLONG2((s+sizeof(long)+sizeof(long))); \
	(num4) = UNPACKLONG2((s+sizeof(long)+sizeof(long)+sizeof(long))); \
}

void    read_header(SWISH *sw, INDEXDATAHEADER *header, void *DB)
{
    int     id,
            len;
    unsigned long    tmp, tmp1, tmp2, tmp3, tmp4;
    unsigned char   *buffer;

    DB_InitReadHeader(sw, DB);

    DB_ReadHeaderData(sw, &id,&buffer,&len,DB);

    while (id)
    {
        switch (id)
        {
        case INDEXHEADER_ID:
        case INDEXVERSION_ID:
        case MERGED_ID:
        case DOCPROPENHEADER_ID:
            break;
        case WORDCHARSHEADER_ID:
            header->wordchars = SafeStrCopy(header->wordchars, (char *)buffer, &header->lenwordchars);
            sortstring(header->wordchars);
            makelookuptable(header->wordchars, header->wordcharslookuptable);
            break;
        case BEGINCHARSHEADER_ID:
            header->beginchars = SafeStrCopy(header->beginchars, (char *)buffer, &header->lenbeginchars);
            sortstring(header->beginchars);
            makelookuptable(header->beginchars, header->begincharslookuptable);
            break;
        case ENDCHARSHEADER_ID:
            header->endchars = SafeStrCopy(header->endchars, (char *)buffer, &header->lenendchars);
            sortstring(header->endchars);
            makelookuptable(header->endchars, header->endcharslookuptable);
            break;
        case IGNOREFIRSTCHARHEADER_ID:
            header->ignorefirstchar = SafeStrCopy(header->ignorefirstchar, (char *)buffer, &header->lenignorefirstchar);
            sortstring(header->ignorefirstchar);
            makelookuptable(header->ignorefirstchar, header->ignorefirstcharlookuptable);
            break;
        case IGNORELASTCHARHEADER_ID:
            header->ignorelastchar = SafeStrCopy(header->ignorelastchar, (char *)buffer, &header->lenignorelastchar);
            sortstring(header->ignorelastchar);
            makelookuptable(header->ignorelastchar, header->ignorelastcharlookuptable);
            break;

        /* replaced by fuzzy_mode Aug 20, 2002     
        case STEMMINGHEADER_ID:
            parse_int_from_buffer(tmp,buffer);
            header-> = tmp;
            break;
        case SOUNDEXHEADER_ID:
            parse_int_from_buffer(tmp,buffer);
            header->applySoundexRules = tmp;
            break;
        */

        case FUZZYMODEHEADER_ID:
            parse_int_from_buffer(tmp,buffer);
            header->fuzzy_data = get_fuzzy_mode(header->fuzzy_data, tmp);
            break;
            
        case IGNORETOTALWORDCOUNTWHENRANKING_ID:
            parse_int_from_buffer(tmp,buffer);
            header->ignoreTotalWordCountWhenRanking = tmp;
            break;
        case MINWORDLIMHEADER_ID:
            parse_int_from_buffer(tmp,buffer);
            header->minwordlimit = tmp;
            break;
        case MAXWORDLIMHEADER_ID:
            parse_int_from_buffer(tmp,buffer);
            header->maxwordlimit = tmp;
            break;
        case SAVEDASHEADER_ID:
            header->savedasheader = SafeStrCopy(header->savedasheader, (char *)buffer, &header->lensavedasheader);
            break;
        case NAMEHEADER_ID:
            header->indexn = SafeStrCopy(header->indexn, (char *)buffer, &header->lenindexn);
            break;
        case DESCRIPTIONHEADER_ID:
            header->indexd = SafeStrCopy(header->indexd, (char *)buffer, &header->lenindexd);
            break;
        case POINTERHEADER_ID:
            header->indexp = SafeStrCopy(header->indexp, (char *)buffer, &header->lenindexp);
            break;
        case MAINTAINEDBYHEADER_ID:
            header->indexa = SafeStrCopy(header->indexa, (char *)buffer, &header->lenindexa);
            break;
        case INDEXEDONHEADER_ID:
            header->indexedon = SafeStrCopy(header->indexedon, (char *)buffer, &header->lenindexedon);
            break;
        case COUNTSHEADER_ID:
            parse_int4_from_buffer(tmp1,tmp2,tmp3,tmp4,buffer);
            header->totalwords = tmp1;
            header->totalfiles = tmp2;
            header->total_word_positions = tmp3;
	    header->removedfiles = tmp4;
            break;

        case TOTALWORDS_REMOVED_ID:  /* Added here instead of above to keep index compatible */
            parse_int_from_buffer(tmp, buffer);
            header->removedwords = tmp;
            break;

/* removed due to patents problems
        case FILEINFOCOMPRESSION_ID:
            ReadHeaderInt(itmp, fp);
            header->applyFileInfoCompression = itmp;
            break;
*/
        case TRANSLATECHARTABLE_ID:
            parse_integer_table_from_buffer(header->translatecharslookuptable, sizeof(header->translatecharslookuptable) / sizeof(int), (char *)buffer);
            break;

        case STOPWORDS_ID:
            load_word_hash_from_buffer(&header->hashstoplist, (char *)buffer);
            break;

        case METANAMES_ID:
            parse_MetaNames_from_buffer(header, (char *)buffer);
            break;

        case BUZZWORDS_ID:
            load_word_hash_from_buffer(&header->hashbuzzwordlist, (char *)buffer);
            break;

#ifndef USE_BTREE
        case TOTALWORDSPERFILE_ID:
            if ( !header->ignoreTotalWordCountWhenRanking )
            {
                header->TotalWordsPerFile = emalloc( header->totalfiles * sizeof(int) );
                parse_integer_table_from_buffer(header->TotalWordsPerFile, header->totalfiles, (char *)buffer);
            }
            break;
#endif

        default:
            progerr("Severe index error in header");
            break;
        }
        efree(buffer);
        DB_ReadHeaderData(sw, &id,&buffer,&len,DB);
    }
    DB_EndReadHeader(sw, DB);
}

/* Reads the metaNames from the index
*/

void    parse_MetaNames_from_buffer(INDEXDATAHEADER *header, char *buffer)
{
    int     len;
    int     num_metanames;
    int     metaType,
            i,
            alias,
            sort_len,
            bias,
            metaID;
    char   *word;
    unsigned char   *s = (unsigned char *)buffer;
    struct metaEntry *m;


    /* First clear out the default metanames */
    freeMetaEntries( header );

    num_metanames = uncompress2(&s);

    for (i = 0; i < num_metanames; i++)
    {
        len = uncompress2(&s);
        word = emalloc(len +1);
        memcpy(word,s,len); s += len;
        word[len] = '\0';
        /* Read metaID */
        metaID = uncompress2(&s);
        /* metaType was saved as metaType+1 */
        metaType = uncompress2(&s);

        alias = uncompress2(&s) - 1;

        sort_len = uncompress2(&s);

        bias = uncompress2(&s) - RANK_BIAS_RANGE - 1;


        /* add the meta tag */
        if ( !(m = addNewMetaEntry(header, word, metaType, metaID)))
            progerr("failed to add new meta entry '%s:%d'", word, metaID );

        m->alias = alias;
        m->rank_bias = bias;
        m->sort_len = sort_len;

        efree(word);
    }
}


static void load_word_hash_from_buffer(WORD_HASH_TABLE *table_ptr, char *buffer)
{
    int     len;
    int        num_words;
    int     i;
    char   *word = NULL;

    unsigned char   *s = (unsigned char *)buffer;

    num_words = uncompress2(&s);
    
    for (i=0; i < num_words ; i++)   
    {
        len = uncompress2(&s);
        word = emalloc(len+1);
        memcpy(word,s,len); s += len;
        word[len] = '\0';

        add_word_to_hash_table( table_ptr, word , HASHSIZE);
        efree(word);
    }
}





void parse_integer_table_from_buffer(int table[], int table_size, char *buffer)
{
    int     tmp,i;
    unsigned char    *s = (unsigned char *)buffer;

    tmp = uncompress2(&s);   /* Jump the number of elements */
    for (i = 0; i < table_size; i++)
    {
        tmp = uncompress2(&s); /* Gut all the elements */
        table[i] = tmp - 1;
    }
}

/* Used by rank.c */


#ifdef USE_BTREE
void getTotalWordsPerFile(SWISH *sw, IndexFILE *indexf, int idx,int *wordcount)
{
        DB_ReadTotalWordsPerFile(sw, idx, wordcount, indexf->DB);
}

#else


void getTotalWordsPerFile(IndexFILE *indexf, int idx,int *wordcount)
{
    INDEXDATAHEADER *header = &indexf->header;

    *wordcount = header->TotalWordsPerFile[idx];
}

#endif

/*------------------------------------------------------*/
/*---------- General entry point of DB module ----------*/


void   *DB_Open (SWISH *sw, char *dbname, int mode)
{
   return sw->Db->DB_Open(sw, dbname,mode);
}

void    DB_Close(SWISH *sw, void *DB)
{
   sw->Db->DB_Close(DB);
}


int     DB_InitReadHeader(SWISH *sw, void *DB)
{
   return sw->Db->DB_InitReadHeader(DB);
}

int     DB_ReadHeaderData(SWISH *sw, int *id, unsigned char **s, int *len, void *DB)
{
   return sw->Db->DB_ReadHeaderData(id, s, len, DB);
}

int     DB_EndReadHeader(SWISH *sw, void *DB)
{
   return sw->Db->DB_EndReadHeader(DB);
}



int     DB_InitReadWords(SWISH *sw, void *DB)
{
   return sw->Db->DB_InitReadWords(DB);
}

int     DB_ReadWordHash(SWISH *sw, char *word, sw_off_t *wordID, void *DB)
{
   return sw->Db->DB_ReadWordHash(word, wordID, DB);
}

int     DB_ReadFirstWordInvertedIndex(SWISH *sw, char *word, char **resultword, sw_off_t *wordID, void *DB)
{
   return sw->Db->DB_ReadFirstWordInvertedIndex(word, resultword, wordID, DB);
}

int     DB_ReadNextWordInvertedIndex(SWISH *sw, char *word, char **resultword, sw_off_t *wordID, void *DB)
{
   return sw->Db->DB_ReadNextWordInvertedIndex(word, resultword, wordID, DB);
}

long    DB_ReadWordData(SWISH *sw, sw_off_t wordID, unsigned char **worddata, int *data_size, int *saved_bytes, void *DB)
{
   return sw->Db->DB_ReadWordData(wordID, worddata, data_size, saved_bytes, DB);
}

int     DB_EndReadWords(SWISH *sw, void *DB)
{
   return sw->Db->DB_EndReadWords(DB);
}


int     DB_CheckFileNum(SWISH *sw, int filenum, void *DB)
{
   return sw->Db->DB_CheckFileNum(filenum, DB);
}

int     DB_ReadFileNum(SWISH *sw, unsigned char *filedata, void *DB)
{
   return sw->Db->DB_ReadFileNum( filedata, DB);
}

 
int     DB_InitReadSortedIndex(SWISH *sw, void *DB)
{
   return sw->Db->DB_InitReadSortedIndex(DB);
}

int     DB_ReadSortedIndex(SWISH *sw, int propID, unsigned char **data, int *sz_data,void *DB)
{
   return sw->Db->DB_ReadSortedIndex(propID, data, sz_data,DB);
}

/* ******* This is now a macro and accessies the native data by default
int     DB_ReadSortedData(SWISH *sw, int *data,int index, int *value, void *DB)
{
   return sw->Db->DB_ReadSortedData(data,index,value,DB);
}
*******/

int     DB_EndReadSortedIndex(SWISH *sw, void *DB)
{
   return sw->Db->DB_EndReadSortedIndex(DB);
}


void    DB_ReadPropPositions(SWISH *sw, IndexFILE *indexf, FileRec *fi, void *db)
{
    sw->Db->DB_ReadPropPositions( indexf, fi, db);
}


char *DB_ReadProperty(SWISH *sw, IndexFILE *indexf, FileRec *fi, int propID, int *buf_len, int *uncompressed_len, void *db)
{
    return sw->Db->DB_ReadProperty( indexf, fi, propID, buf_len, uncompressed_len, db );
}



#ifdef USE_BTREE

int       DB_ReadTotalWordsPerFile(SWISH *sw, int index, int *value, void *DB)
{
    return sw->Db->DB_ReadTotalWordsPerFile(sw, index, value, DB);
}

#endif



/* 11/00 Function to read all words starting with a character */
char   *getfilewords(SWISH * sw, int c, IndexFILE * indexf)
{
    int     i,
            j;
    int     wordlen;
    char   *buffer, *resultword;
    int     bufferpos,
            bufferlen;
    unsigned char    word[2];
    sw_off_t    wordID;

    

    if (!c)
        return "";
    /* Check if already read */
    j = (int) ((unsigned char) c);
    if (indexf->keywords[j])
        return (indexf->keywords[j]);

    DB_InitReadWords(sw, indexf->DB);

    word[0]=(unsigned char)c;
    word[1]='\0';

    DB_ReadFirstWordInvertedIndex(sw, (char *)word, &resultword, &wordID, indexf->DB);
    i = (int) ((unsigned char) c);
    if (!wordID)
    {
        DB_EndReadWords(sw, indexf->DB);
        sw->lasterror = WORD_NOT_FOUND;
        return "";
    }

    wordlen = strlen(resultword);    
    bufferlen = wordlen + MAXSTRLEN * 10;
    bufferpos = 0;
    buffer = emalloc(bufferlen + 1);
    buffer[0] = '\0';


    memcpy(buffer, resultword, wordlen);
    efree(resultword);
    if (c != (int)((unsigned char) buffer[bufferpos]))
    {
        buffer[bufferpos] = '\0';
        indexf->keywords[j] = buffer;
        return (indexf->keywords[j]);
    }

    buffer[bufferpos + wordlen] = '\0';
    bufferpos += wordlen + 1;

    /* Look for occurrences */
    DB_ReadNextWordInvertedIndex(sw, (char *)word, &resultword, &wordID, indexf->DB);
    while (wordID)
    {
        wordlen = strlen(resultword);
        if ((bufferpos + wordlen + 1 + 1) > bufferlen)
        {
            bufferlen += MAXSTRLEN + wordlen + 1 + 1;
            buffer = (char *) erealloc(buffer, bufferlen + 1);
        }
        memcpy(buffer + bufferpos, resultword, wordlen);
        efree(resultword);
        if (c != (int)((unsigned char)buffer[bufferpos]))
        {
            buffer[bufferpos] = '\0';
            break;
        }
        
        buffer[bufferpos + wordlen] = '\0';
        bufferpos += wordlen + 1;
        DB_ReadNextWordInvertedIndex(sw, (char *)word, &resultword, &wordID, indexf->DB);
    }
    buffer[bufferpos] = '\0';
    indexf->keywords[j] = buffer;
    return (indexf->keywords[j]);
}


