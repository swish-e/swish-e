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
#include "string.h"
#include "index.h"
#include "hash.h"
#include "date_time.h"
#include "compress.h"
#include "error.h"
#include "metanames.h"
#include "db.h"
#include "db_native.h"
// #include "db_berkeley_db.h"


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



/*
 -- Config Directives
 -- Configuration directives for this Module
 -- return: 0/1 = none/config applied
*/

int configModule_DB  (SWISH *sw, StringList *sl)
{
 //struct MOD_DB *DB = sw->Db;
 // char *w0    = sl->word[0];
 int  retval = 1;


  retval = 0; // tmp due to empty routine

  return retval;
}


/* General write DB routines - Common to all DB */

/* Header routines */

#define write_header_int(sw,id,num,DB) {unsigned long itmp = (num); itmp = PACKLONG(itmp); DB_WriteHeaderData((sw),(id), (unsigned char *)&itmp, sizeof(long), (DB));}
#define write_header_int2(sw,id,num1,num2,DB) {unsigned long itmp[2]; itmp[0] = (num1); itmp[1] = (num2); itmp[0]=  PACKLONG(itmp[0]); itmp[1] = PACKLONG(itmp[1]); DB_WriteHeaderData((sw),(id), (unsigned char *)itmp, sizeof(long) * 2, (DB));}


void    write_header(SWISH *sw, INDEXDATAHEADER * header, void * DB, char *filename, int totalwords, int totalfiles, int merged)
{
    char   *c,
           *tmp;

    c = (char *) strrchr(filename, DIRDELIMITER);
    if (!c || (c && !*(c + 1)))
        c = filename;
    else
        c += 1;

    DB_InitWriteHeader(sw, DB);

    DB_WriteHeaderData(sw, INDEXHEADER_ID, INDEXHEADER, strlen(INDEXHEADER) +1, DB);
    DB_WriteHeaderData(sw, INDEXVERSION_ID, INDEXVERSION, strlen(INDEXVERSION) + 1, DB);
    write_header_int(sw, MERGED_ID, merged, DB);
    DB_WriteHeaderData(sw, NAMEHEADER_ID, header->indexn, strlen(header->indexn) + 1, DB);
    DB_WriteHeaderData(sw, SAVEDASHEADER_ID, c, strlen(c) + 1, DB);
    write_header_int2(sw, COUNTSHEADER_ID, totalwords, totalfiles, DB);
    tmp = getTheDateISO(); 
	DB_WriteHeaderData(sw, INDEXEDONHEADER_ID, tmp, strlen(tmp) + 1,DB); 
	efree(tmp);
    DB_WriteHeaderData(sw, DESCRIPTIONHEADER_ID, header->indexd, strlen(header->indexd) + 1, DB);
    DB_WriteHeaderData(sw, POINTERHEADER_ID, header->indexp, strlen(header->indexp) + 1, DB);
    DB_WriteHeaderData(sw, MAINTAINEDBYHEADER_ID, header->indexa, strlen(header->indexa) + 1,DB);
    write_header_int(sw, DOCPROPENHEADER_ID, 1, DB);
    write_header_int(sw, STEMMINGHEADER_ID, header->applyStemmingRules, DB);
    write_header_int(sw, SOUNDEXHEADER_ID, header->applySoundexRules, DB);
    write_header_int(sw, IGNORETOTALWORDCOUNTWHENRANKING_ID, header->ignoreTotalWordCountWhenRanking, DB);
    DB_WriteHeaderData(sw, WORDCHARSHEADER_ID, header->wordchars, strlen(header->wordchars) + 1, DB);
    write_header_int(sw, MINWORDLIMHEADER_ID, header->minwordlimit, DB);
    write_header_int(sw, MAXWORDLIMHEADER_ID, header->maxwordlimit, DB);
    DB_WriteHeaderData(sw, BEGINCHARSHEADER_ID, header->beginchars, strlen(header->beginchars) + 1, DB);
    DB_WriteHeaderData(sw, ENDCHARSHEADER_ID, header->endchars, strlen(header->endchars) + 1, DB);
    DB_WriteHeaderData(sw, IGNOREFIRSTCHARHEADER_ID, header->ignorefirstchar, strlen(header->ignorefirstchar) + 1, DB);
    DB_WriteHeaderData(sw, IGNORELASTCHARHEADER_ID, header->ignorelastchar, strlen(header->ignorelastchar) + 1,DB);
	/* Removed - Patents 
    write_header_int(FILEINFOCOMPRESSION_ID, header->applyFileInfoCompression, DB);
	*/

    /* Jose Ruiz 06/00 Added this line to delimite the header */
    write_integer_table_to_header(sw, TRANSLATECHARTABLE_ID, header->translatecharslookuptable, sizeof(header->translatecharslookuptable) / sizeof(int), DB);
	
	/* Other header stuff */
		/* StopWords */
    write_words_to_header(sw, STOPWORDS_ID, header->hashstoplist, DB);
		/* Metanames */
    write_MetaNames(sw, METANAMES_ID, header, DB);


		/* BuzzWords */
    write_words_to_header(sw, BUZZWORDS_ID, header->hashbuzzwordlist, DB);	

    DB_EndWriteHeader(sw, DB);
}

/* Jose Ruiz 11/00
** Function to write a word to the index DB
*/
void    write_word(SWISH * sw, ENTRY * ep, IndexFILE * indexf)
{
    long    wordID;

    wordID = DB_GetWordID(sw, indexf->DB);

    DB_WriteWord(sw, ep->word,wordID,indexf->DB);
	    /* Store word offset for futher hash computing */
    ep->u1.wordID = wordID;

}

/* Jose Ruiz 11/00
** Function to write all word's data to the index DB
*/


void write_worddata(SWISH * sw, ENTRY * ep, IndexFILE * indexf)
{
    int     i, j,
            curmetaID,
            sz_worddata;
    unsigned long    tmp,
            curmetanamepos;
    int     metaID;
	int     bytes_size,
			chunk_size;
    unsigned char *compressed_data,
           *p,*q;
    LOCATION *l, *next;


    if(sw->Index->swap_locdata)
        unSwapLocDataEntry(sw,ep);

    curmetaID=0;
    curmetanamepos=0L;
    q=sw->Index->worddata_buffer;

		/* Compute bytes required for chunk location size. Eg: 4096 -> 2 bytes, 65535 -> 2 bytes */
    for(bytes_size = 0, i = COALESCE_BUFFER_MAX_SIZE; i; i >>= 8)
        bytes_size++;

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

        for(chunk_size = 0, i = 0, j = bytes_size - 1; i < bytes_size; i++, j--)
            chunk_size |= p[i] << (j * 8);
        p += bytes_size;

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
				** 5 is for the worst case metaID
				**
                ** sizeof(long) is to leave four bytes to
                ** store the offset of the next metaname
                ** (it will be 0 if no more metanames).
                **
                ** 1 is for the trailing '\0'
                */

            tmp=q - sw->Index->worddata_buffer;

            if((long)(tmp + 5 + sizeof(long) + 1) >= (long)sw->Index->len_worddata_buffer)
            {
                sw->Index->len_worddata_buffer=sw->Index->len_worddata_buffer*2+5+sizeof(long)+1;
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
    ** if ignorelimit was set and some new stopwords weer found, positions
    ** are recalculated
    ** Also call it even if we have not set IgnoreLimit to calesce word chunks
    ** and remove trailing 0 from chunks to save some bytes
    */
    adjustWordPositions(sw->Index->worddata_buffer, &sz_worddata, sw->indexlist->header.totalfiles, sw->Index->IgnoreLimitPositionsArray);

    DB_WriteWordData(sw, ep->u1.wordID,sw->Index->worddata_buffer,sz_worddata,indexf->DB);

    if(sw->Index->swap_locdata)
        Mem_ZoneReset(sw->Index->totalLocZone);
}

/* Writes the list of metaNames into the DB index
*/

void    write_MetaNames(SWISH *sw, int id, INDEXDATAHEADER * header, void *DB)
{
    struct metaEntry *entry = NULL;
    int     i,
            sz_buffer,
            len;
    unsigned char *buffer,*s;

/* #### Use new metaType schema - see metanames.h */
    /* Format of metaname is
       <len><metaName><metaType><Alias>
       len, metaType, and alias are compressed numbers
       metaName is the ascii name of the metaname

       The list of metanames is delimited by a 0
     */
        /* Compute buffer size */
    for (sz_buffer = 0 , i = 0; i < header->metaCounter; i++)
    {
        entry = header->metaEntryArray[i];
        len = strlen(entry->metaName);
        sz_buffer += len + 5 *4; /* how is this calculated? */
    }
    
    sz_buffer += 5;  /* Add extra 5 for the number of metanames */

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
    }
    DB_WriteHeaderData(sw, id,buffer,s-buffer,DB);
    efree(buffer);
}



/* Write a the hashlist of words into the index header file (used by stopwords and buzzwords
*/

int    write_words_to_header(SWISH *sw, int header_ID, struct swline **hash, void *DB)
{
    int     hashval,
            len,
            num_words,
            sz_buffer;
    char   *buffer, *s;
    struct swline *sp = NULL;
		
		/* Let's count the words */

    for (sz_buffer = 0, num_words = 0 , hashval = 0; hashval < HASHSIZE; hashval++)
    {
        sp = hash[hashval];
        while (sp != NULL)
        {
            num_words++;
            sz_buffer += 5 + strlen(sp->line);
            sp = sp->next;
        }
    }

    if(num_words)
    {
        sz_buffer += 5;  /* Add 5 for the number of words */

        s = buffer = (char *)emalloc(sz_buffer);

        s = compress3(num_words,s);

        for (hashval = 0; hashval < HASHSIZE; hashval++)
        {
            sp = hash[hashval];
            while (sp != NULL)
            {
                len = strlen(sp->line);
                s = compress3(len,s);
                memcpy(s,sp->line,len);
                s +=len;
                sp = sp->next;
            }
        }
        DB_WriteHeaderData(sw, header_ID, buffer, s - buffer, DB);
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
	
    s = buffer = (char *) emalloc((table_size + 1) * 5);

    s = compress3(table_size,s);   /* Put the number of elements */
    for (i = 0; i < table_size; i++)
    {
        tmp = table[i] + 1;
        s = compress3(tmp, s); /* Put all the elements */
    }

    DB_WriteHeaderData(sw, id, buffer, s-buffer, DB);

    efree(buffer);
    return 0;
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


void    read_header(SWISH *sw, INDEXDATAHEADER *header, void *DB)
{
    int     id,
            len;
    unsigned long    tmp, tmp1, tmp2;
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
            header->wordchars = SafeStrCopy(header->wordchars, buffer, &header->lenwordchars);
            sortstring(header->wordchars);
            makelookuptable(header->wordchars, header->wordcharslookuptable);
            break;
        case BEGINCHARSHEADER_ID:
            header->beginchars = SafeStrCopy(header->beginchars, buffer, &header->lenbeginchars);
            sortstring(header->beginchars);
            makelookuptable(header->beginchars, header->begincharslookuptable);
            break;
        case ENDCHARSHEADER_ID:
            header->endchars = SafeStrCopy(header->endchars, buffer, &header->lenendchars);
            sortstring(header->endchars);
            makelookuptable(header->endchars, header->endcharslookuptable);
            break;
        case IGNOREFIRSTCHARHEADER_ID:
            header->ignorefirstchar = SafeStrCopy(header->ignorefirstchar, buffer, &header->lenignorefirstchar);
            sortstring(header->ignorefirstchar);
            makelookuptable(header->ignorefirstchar, header->ignorefirstcharlookuptable);
            break;
        case IGNORELASTCHARHEADER_ID:
            header->ignorelastchar = SafeStrCopy(header->ignorelastchar, buffer, &header->lenignorelastchar);
            sortstring(header->ignorelastchar);
            makelookuptable(header->ignorelastchar, header->ignorelastcharlookuptable);
            break;
        case STEMMINGHEADER_ID:
            parse_int_from_buffer(tmp,buffer);
            header->applyStemmingRules = tmp;
            break;
        case SOUNDEXHEADER_ID:
            parse_int_from_buffer(tmp,buffer);
            header->applySoundexRules = tmp;
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
            header->savedasheader = SafeStrCopy(header->savedasheader, buffer, &header->lensavedasheader);
            break;
        case NAMEHEADER_ID:
            header->indexn = SafeStrCopy(header->indexn, buffer, &header->lenindexn);
            break;
        case DESCRIPTIONHEADER_ID:
            header->indexd = SafeStrCopy(header->indexd, buffer, &header->lenindexd);
            break;
        case POINTERHEADER_ID:
            header->indexp = SafeStrCopy(header->indexp, buffer, &header->lenindexp);
            break;
        case MAINTAINEDBYHEADER_ID:
            header->indexa = SafeStrCopy(header->indexa, buffer, &header->lenindexa);
            break;
        case INDEXEDONHEADER_ID:
            header->indexedon = SafeStrCopy(header->indexedon, buffer, &header->lenindexedon);
            break;
        case COUNTSHEADER_ID:
            parse_int2_from_buffer(tmp1,tmp2,buffer);
            header->totalwords = tmp1;
            header->totalfiles = tmp2;
            break;
/* removed due to patents problems
        case FILEINFOCOMPRESSION_ID:
            ReadHeaderInt(itmp, fp);
            header->applyFileInfoCompression = itmp;
            break;
*/
        case TRANSLATECHARTABLE_ID:
            parse_integer_table_from_buffer(header->translatecharslookuptable, sizeof(header->translatecharslookuptable) / sizeof(int), buffer);
            break;
        case STOPWORDS_ID:
            parse_stopwords_from_buffer(header, buffer);
            break;
        case METANAMES_ID:
            parse_MetaNames_from_buffer(header, buffer);
            break;
        case BUZZWORDS_ID:
            parse_buzzwords_from_buffer(header, buffer);
            break;
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
        

        /* add the meta tag */
        if ( !(m = addNewMetaEntry(header, word, metaType, metaID)))
            progerr("failed to add new meta entry '%s:%d'", word, metaID );

        m->alias = alias;

        efree(word);
    }
}

/* Reads the stopwords in the index file.
*/

void    parse_stopwords_from_buffer(INDEXDATAHEADER *header, char *buffer)
{
    int     len;
    int	    num_words;
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
        addStopList(header, word);
        addstophash(header, word);
        efree(word);
    }
}

/* read the buzzwords from the index file */

void    parse_buzzwords_from_buffer(INDEXDATAHEADER *header, char *buffer)
{
    int     len;
    int     num_words;
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
        addbuzzwordhash(header, word);
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
    long    wordID;

	

    if (!c)
        return "";
    /* Check if already read */
    j = (int) ((unsigned char) c);
    if (indexf->keywords[j])
        return (indexf->keywords[j]);

    DB_InitReadWords(sw, indexf->DB);

    word[0]=(unsigned char)c;
    word[1]='\0';

    DB_ReadFirstWordInvertedIndex(sw, word, &resultword, &wordID, indexf->DB);
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
    DB_ReadNextWordInvertedIndex(sw, word, &resultword, &wordID, indexf->DB);
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
        DB_ReadNextWordInvertedIndex(sw, word, &resultword, &wordID, indexf->DB);
    }
    buffer[bufferpos] = '\0';
    indexf->keywords[j] = buffer;
    return (indexf->keywords[j]);
}



/*------------------------------------------------------*/
/*---------- General entry point of DB module ----------*/

void   *DB_Create (SWISH *sw, char *dbname)
{
   return sw->Db->DB_Create(dbname);
}

void   *DB_Open (SWISH *sw, char *dbname)
{
   return sw->Db->DB_Open(dbname);
}

void    DB_Close(SWISH *sw, void *DB)
{
   sw->Db->DB_Close(DB);
}


void    DB_Remove(SWISH *sw, void *DB)
{
   sw->Db->DB_Remove(DB);
}

int     DB_InitWriteHeader(SWISH *sw, void *DB)
{
   return sw->Db->DB_InitWriteHeader(DB);
}

int     DB_WriteHeaderData(SWISH *sw, int id, unsigned char *s, int len, void *DB)
{
   return sw->Db->DB_WriteHeaderData(id, s,len,DB);
}

int     DB_EndWriteHeader(SWISH *sw, void *DB)
{
   return sw->Db->DB_EndWriteHeader(DB);
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


int     DB_InitWriteWords(SWISH *sw, void *DB)
{
   return sw->Db->DB_InitWriteWords(DB);
}

long    DB_GetWordID(SWISH *sw, void *DB)
{
   return sw->Db->DB_GetWordID(DB);
}

int     DB_WriteWord(SWISH *sw, char *word, long wordID, void *DB)
{
   return sw->Db->DB_WriteWord(word, wordID, DB);
}

int     DB_WriteWordHash(SWISH *sw, char *word, long wordID, void *DB)
{
   return sw->Db->DB_WriteWordHash(word, wordID, DB);
}

long    DB_WriteWordData(SWISH *sw, long wordID, unsigned char *worddata, int lendata, void *DB)
{
   return sw->Db->DB_WriteWordData(wordID, worddata, lendata, DB);
}

int     DB_EndWriteWords(SWISH *sw, void *DB)
{
   return sw->Db->DB_EndWriteWords(DB);
}


int     DB_InitReadWords(SWISH *sw, void *DB)
{
   return sw->Db->DB_InitReadWords(DB);
}

int     DB_ReadWordHash(SWISH *sw, char *word, long *wordID, void *DB)
{
   return sw->Db->DB_ReadWordHash(word, wordID, DB);
}

int     DB_ReadFirstWordInvertedIndex(SWISH *sw, char *word, char **resultword, long *wordID, void *DB)
{
   return sw->Db->DB_ReadFirstWordInvertedIndex(word, resultword, wordID, DB);
}

int     DB_ReadNextWordInvertedIndex(SWISH *sw, char *word, char **resultword, long *wordID, void *DB)
{
   return sw->Db->DB_ReadNextWordInvertedIndex(word, resultword, wordID, DB);
}

long    DB_ReadWordData(SWISH *sw, long wordID, unsigned char **worddata, int *lendata, void *DB)
{
   return sw->Db->DB_ReadWordData(wordID, worddata, lendata, DB);
}

int     DB_EndReadWords(SWISH *sw, void *DB)
{
   return sw->Db->DB_EndReadWords(DB);
}



int     DB_InitWriteFiles(SWISH *sw, void *DB)
{
   return sw->Db->DB_InitWriteFiles(DB);
}

int     DB_WriteFile(SWISH *sw, int filenum, unsigned char *filedata,int sz_filedata, void *DB)
{
   return sw->Db->DB_WriteFile(filenum, filedata, sz_filedata, DB);
}

int     DB_EndWriteFiles(SWISH *sw, void *DB)
{
   return sw->Db->DB_EndWriteFiles(DB);
}


int     DB_InitReadFiles(SWISH *sw, void *DB)
{
   return sw->Db->DB_InitReadFiles(DB);
}

int     DB_ReadFile(SWISH *sw, int filenum, unsigned char **filedata,int *sz_filedata, void *DB)
{
   return sw->Db->DB_ReadFile(filenum, filedata,sz_filedata, DB);
}

int     DB_EndReadFiles(SWISH *sw, void *DB)
{
   return sw->Db->DB_EndReadFiles(DB);
}


int     DB_InitWriteSortedIndex(SWISH *sw, void *DB)
{
   return sw->Db->DB_InitWriteSortedIndex(DB);
}

int     DB_WriteSortedIndex(SWISH *sw, int propID, unsigned char *data, int sz_data,void *DB)
{
   return sw->Db->DB_WriteSortedIndex(propID, data, sz_data,DB);
}

int     DB_EndWriteSortedIndex(SWISH *sw, void *DB)
{
   return sw->Db->DB_EndWriteSortedIndex(DB);
}

 
int     DB_InitReadSortedIndex(SWISH *sw, void *DB)
{
   return sw->Db->DB_InitReadSortedIndex(DB);
}

int     DB_ReadSortedIndex(SWISH *sw, int propID, unsigned char **data, int *sz_data,void *DB)
{
   return sw->Db->DB_ReadSortedIndex(propID, data, sz_data,DB);
}

int     DB_EndReadSortedIndex(SWISH *sw, void *DB)
{
   return sw->Db->DB_EndReadSortedIndex(DB);
}


void DB_WriteProperty( SWISH *sw, FileRec *fi, int propID, char *buffer, int buf_len, int uncompressed_len, void *db)
{
    sw->Db->DB_WriteProperty( sw, fi, propID, buffer, buf_len, uncompressed_len, db );
}

void    DB_WritePropPositions(SWISH *sw, FileRec *fi, void *db)
{
    sw->Db->DB_WritePropPositions( sw, fi, db);
}

void    DB_ReadPropPositions(SWISH *sw, FileRec *fi, void *db)
{
    sw->Db->DB_ReadPropPositions( sw, fi, db);
}


char *DB_ReadProperty(SWISH *sw, FileRec *fi, int propID, int *buf_len, int *uncompressed_len, void *db)
{
    return sw->Db->DB_ReadProperty( sw, fi, propID, buf_len, uncompressed_len, db );
}


void    DB_Reopen_PropertiesForRead(SWISH *sw, void *DB )
{
    sw->Db->DB_Reopen_PropertiesForRead(DB);
}

