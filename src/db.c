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
#include "hash.h"
#include "mem.h"
#include "date_time.h"
#include "compress.h"
#include "error.h"
#include "string.h"
#include "metanames.h"
#include "db.h"
#include "db_native.h"


/*
  -- init structures for this module
*/

void initModule_DB (SWISH  *sw)
{
          /* Allocate structure */
  return;
}


/*
  -- release all wired memory for this module
*/

void freeModule_DB (SWISH *sw)
{
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
 // struct MOD_DBNative *md = sw->DBNative;
 // char *w0    = sl->word[0];
 int  retval = 1;


  retval = 0; // tmp due to empty routine

  return retval;
}


/* General write DB routines - Common to all DB */

/* Header routines */

#define write_header_int(id,num,DB) {long itmp = (num); PACKLONG(itmp); DB_WriteHeaderData((id), (unsigned char *)&itmp, sizeof(long), (DB));}
#define write_header_int2(id,num1,num2,DB) {long itmp[2]; itmp[0] = (num1); itmp[1] = (num2); PACKLONG(itmp[0]); PACKLONG(itmp[1]); DB_WriteHeaderData((id), (unsigned char *)itmp, sizeof(long) * 2, (DB));}


void    write_header(INDEXDATAHEADER * header, void * DB, char *filename, int totalwords, int totalfiles, int merged)
{
    char   *c,
           *tmp;

    c = (char *) strrchr(filename, '/');
    if (!c || (c && !*(c + 1)))
        c = filename;
    else
        c += 1;

    DB_InitWriteHeader(DB);

    DB_WriteHeaderData(INDEXHEADER_ID, INDEXHEADER, strlen(INDEXHEADER) +1, DB);
    DB_WriteHeaderData(INDEXVERSION_ID, INDEXVERSION, strlen(INDEXVERSION) + 1, DB);
    write_header_int(MERGED_ID, merged, DB);
    DB_WriteHeaderData(NAMEHEADER_ID, header->indexn, strlen(header->indexn) + 1, DB);
    DB_WriteHeaderData(SAVEDASHEADER_ID, c, strlen(c) + 1, DB);
    write_header_int2(COUNTSHEADER_ID, totalwords, totalfiles, DB);
    tmp = getTheDateISO(); DB_WriteHeaderData(INDEXEDONHEADER_ID, tmp, strlen(tmp) + 1,DB); efree(tmp);
    DB_WriteHeaderData(DESCRIPTIONHEADER_ID, header->indexd, strlen(header->indexd) + 1, DB);
    DB_WriteHeaderData(POINTERHEADER_ID, header->indexp, strlen(header->indexp) + 1, DB);
    DB_WriteHeaderData(MAINTAINEDBYHEADER_ID, header->indexa, strlen(header->indexa) + 1,DB);
    write_header_int(DOCPROPENHEADER_ID, 1, DB);
    write_header_int(STEMMINGHEADER_ID, header->applyStemmingRules, DB);
    write_header_int(SOUNDEXHEADER_ID, header->applySoundexRules, DB);
    write_header_int(IGNORETOTALWORDCOUNTWHENRANKING_ID, header->ignoreTotalWordCountWhenRanking, DB);
    DB_WriteHeaderData(WORDCHARSHEADER_ID, header->wordchars, strlen(header->wordchars) + 1, DB);
    write_header_int(MINWORDLIMHEADER_ID, header->minwordlimit, DB);
    write_header_int(MAXWORDLIMHEADER_ID, header->maxwordlimit, DB);
    DB_WriteHeaderData(BEGINCHARSHEADER_ID, header->beginchars, strlen(header->beginchars) + 1, DB);
    DB_WriteHeaderData(ENDCHARSHEADER_ID, header->endchars, strlen(header->endchars) + 1, DB);
    DB_WriteHeaderData(IGNOREFIRSTCHARHEADER_ID, header->ignorefirstchar, strlen(header->ignorefirstchar) + 1, DB);
    DB_WriteHeaderData(IGNORELASTCHARHEADER_ID, header->ignorelastchar, strlen(header->ignorelastchar) + 1,DB);
	/* Removed - Patents 
    write_header_int(FILEINFOCOMPRESSION_ID, header->applyFileInfoCompression, DB);
	*/

    /* Jose Ruiz 06/00 Added this line to delimite the header */
    write_integer_table_to_header(TRANSLATECHARTABLE_ID, header->translatecharslookuptable, sizeof(header->translatecharslookuptable) / sizeof(int), DB);
	
	/* Other header stuff */
		/* StopWords */
    write_words_to_header(STOPWORDS_ID, header->hashstoplist, DB);
		/* Metanames */
    write_MetaNames(METANAMES_ID, header, DB);

		/* lookup tables */
    write_locationlookuptables_to_header(LOCATIONLOOKUPTABLE_ID, header, DB);
    if (!header->pathlookup)
    {
        DB_Remove(DB);   /* Remove file: It is useless */
        progerr("No valid documents have been found. Check your files, directories and/or urls. Index file removed");
    }
    write_pathlookuptable_to_header(PATHLOOKUPTABLE_ID, header, DB);

		/* BuzzWords */
    write_words_to_header(BUZZWORDS_ID, header->hashbuzzwordlist, DB);	
		/* Write array containing the number of words per doc */
    write_integer_table_to_header(WORDSPERDOC_ID, header->filetotalwordsarray , header->totalfiles , DB);

    DB_EndWriteHeader(DB);
}

/* Jose Ruiz 11/00
** Function to write a word to the index DB
*/
void    write_word(SWISH * sw, ENTRY * ep, IndexFILE * indexf)
{
    long    wordID;

    wordID = DB_GetWordID(indexf->DB);
    DB_WriteWord(ep->word,wordID,indexf->DB);
	    /* Store word offset for futher hash computing */
    ep->u1.fileoffset = wordID;

}

/* Jose Ruiz 11/00
** Function to write all word's data to the index DB
*/
void    write_worddata(SWISH * sw, ENTRY * ep, IndexFILE * indexf)
{
    int     i,
            index,
            curmetaID;
    long    tmp,
            curmetanamepos;
    int     metaID,
            filenum,
            frequency,
            position;
    int     index_structfreq;
    unsigned char *compressed_data,
           *p,*q;

    static unsigned char *buffer=NULL;
    static int lenbuffer=0;

    if(!lenbuffer)
        buffer=(unsigned char *) emalloc((lenbuffer=MAXSTRLEN));

    curmetaID=0;
    curmetanamepos=0L;
    q=buffer;
        /* Write tfrequency */
    compress3(ep->tfrequency,q);
        /* Write location list */
    for(i=0;i<ep->u1.max_locations;i++) 
    {
        p = compressed_data = (unsigned char *)ep->locationarray[i];
        uncompress2(index,p);
        metaID=indexf->header.locationlookup->all_entries[index-1]->val[0];
        if(curmetaID!=metaID) 
        {
            if(curmetaID) 
            {
                /* Write in previous meta (curmetaID)
                ** file offset to next meta */
                tmp=q-buffer;
                PACKLONG2(tmp,buffer+curmetanamepos);
            }
                /* Check for enough memory */
                /* 5 is the worst case for a compressed number
                ** (curmetaID) - Well, it is too much for
                ** a Meta ID which normally fits in just
                ** 1 byte !! Consider it as a "safe" factor
                **
                ** sizeof(long) is to leave four bytes to
                ** store the offset of the next metaname
                ** (it will be 0 if no more metanames).
                **
                ** 1 is for the trailing '\0'
                */

            tmp=q-buffer;
            if((long)(tmp + 5 + sizeof(long) + 1) >= (long)lenbuffer)
            {
                lenbuffer=lenbuffer*2+5+sizeof(long)+1;
                buffer=(unsigned char *) erealloc(buffer,lenbuffer);
                q=buffer+tmp;   /* reasign pointer inside buffer */
            }
                /* store metaID in buffer */
            curmetaID=metaID;
            compress3(curmetaID,q);
                /* preserve position for offset to next
                ** metaname. We do not know its size
                ** so store it as a packed long */ 
            curmetanamepos=q-buffer;
                /* Store 0 and increase pointer */
            tmp=0L;
            PACKLONG2(tmp,q);
            q+=sizeof(long);
        }
            /* Write filenum,structure and position information to index file */
        uncompress2(filenum,p);
        index_structfreq=indexf->header.locationlookup->all_entries[index-1]->val[1];
        frequency=indexf->header.structfreqlookup->all_entries[index_structfreq-1]->val[0];
                /* Check for enough memory */
                /* 5 is the worst case for a compressed number
                ** We have at least 2+frequency numbers
                ** So, in the worst case we need
                **     5 bytes for filenum
                **     5 bytes for index_structfreq
                **     5* frequency bytes for the positions
                **
                ** Once again, probably it is too much space
                ** Consider 5 as a safe factor
                **
                ** 1 is for the trailing '\0';
                */
        tmp=q-buffer;  
        if((tmp + 5 *(2+frequency) + 1) >= (long)lenbuffer)
        {
            lenbuffer=lenbuffer*2+5*(2+frequency)+1;
            buffer=(unsigned char *) erealloc(buffer,lenbuffer);
            q=buffer+tmp;   /* reasign pointer inside buffer */
        }
        compress3(filenum,q);
        compress3(index_structfreq,q);
        for(;frequency;frequency--)
        {
            uncompress2(position,p);
            compress3(position,q);
        }
        efree(compressed_data);
    }
        /* Write in previous meta (curmetaID)
        ** file offset to end of metas */
    tmp=q-buffer;

    PACKLONG2(tmp,buffer+curmetanamepos);

        /* Write trailing '\0' */
    *q++ = '\0';

    DB_WriteWordData(ep->u1.fileoffset,buffer,q-buffer,indexf->DB);
}


/* Writes the list of metaNames into the DB index
*/

void    write_MetaNames(int id, INDEXDATAHEADER * header, void *DB)
{
    struct metaEntry *entry = NULL;
    int     i,
            sz_buffer,
            len;
    unsigned char *buffer,*s;

/* #### Use new metaType schema - see metanames.h */
    /* Format of metaname is
       <len><metaName><metaType>
       len and metaType are compressed numbers
       metaName is the ascii name of the metaname

       The list of metanames is delimited by a 0
     */
        /* Compute buffer size */
    for (sz_buffer = 0 , i = 0; i < header->metaCounter; i++)
    {
        entry = header->metaEntryArray[i];
        len = strlen(entry->metaName);
        sz_buffer += len + 5 *3; 
    }
    
    sz_buffer += 5;  /* Add extra 5 for the number of metanames */

    s = buffer = (unsigned char *) emalloc(sz_buffer);

    compress3(header->metaCounter,s);   /* store the number of metanames */

    for (i = 0; i < header->metaCounter; i++)
    {
        entry = header->metaEntryArray[i];
        len = strlen(entry->metaName);
        compress3(len, s);
        memcpy(s,entry->metaName,len);
        s += len;
        compress3(entry->metaID, s);
        compress3(entry->metaType, s);
    }
    DB_WriteHeaderData(id,buffer,s-buffer,DB);
    efree(buffer);
}



/* Write a the hashlist of words into the index header file (used by stopwords and buzzwords
*/

int    write_words_to_header(int header_ID, struct swline **hash, void *DB)
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

        compress3(num_words,s);

        for (hashval = 0; hashval < HASHSIZE; hashval++)
        {
            sp = hash[hashval];
            while (sp != NULL)
            {
                len = strlen(sp->line);
                compress3(len,s);
                memcpy(s,sp->line,len);
                s +=len;
                sp = sp->next;
            }
        }
        DB_WriteHeaderData(header_ID, buffer, s - buffer, DB);
        efree(buffer);
}
return 0;
}


/* Print the info lookuptable of structures and frequency */
/* These lookuptables make the file index small and decreases I/O op */
void    write_locationlookuptables_to_header(int id, INDEXDATAHEADER *header, void *DB)
{
    int     i,
            n,
            sz_buffer,
            tmp;
    char   *s,
           *buffer;
    char    empty_buffer[2];

    /* Let us begin with structure lookuptable */
    if (!header->structurelookup)
    {
            /* No words in DB */
        empty_buffer[0]='\0';    /* Write 2 empty numbers */
        empty_buffer[1]='\0';
        DB_WriteHeaderData(id,empty_buffer,2,DB);
        return;
    }
	
        /* First of all ->  Compute required size */

    sz_buffer = 5 + 5 * header->structurelookup->n_entries + 5 + 5 * 2 * header->structfreqlookup->n_entries;

    s = buffer = emalloc(sz_buffer);

    n = header->structurelookup->n_entries;

    compress3(n, s);
    for (i = 0; i < n; i++)
    {
        tmp = header->structurelookup->all_entries[i]->val[0] + 1;
        compress3(tmp, s);
    }
    /* Let us continue with structure_lookup,frequency lookuptable */

    n = header->structfreqlookup->n_entries;
    compress3(n, s);
    for (i = 0; i < n; i++)
    {
        /* frequency */
        tmp = header->structfreqlookup->all_entries[i]->val[0] + 1;
        compress3(tmp, s);
        /* structure lookup value */
        tmp = header->structfreqlookup->all_entries[i]->val[1] + 1;
        compress3(tmp, s);
    }

    DB_WriteHeaderData(id,buffer,s-buffer,DB);

    efree(buffer);
}

/* Print the info lookuptable of paths/urls */
/* This lookuptable make the file index small and decreases I/O op */
void    write_pathlookuptable_to_header(int id, INDEXDATAHEADER *header, void *DB)
{
    int     n,
            i,
            sz_buffer,
            len;
    char   *tmp;
	char *s,*buffer;

    n = header->pathlookup->n_entries;
    for (sz_buffer = 0, i = 0; i < n; i++)
    {
        tmp = header->pathlookup->all_entries[i]->val;
        len = strlen(tmp) + 1;
        sz_buffer += 5 + len;
    }

    sz_buffer += 5;  /* For the number of elements */

    s = buffer = emalloc(sz_buffer);

    n = header->pathlookup->n_entries;
    compress3(n, s);
    for (i = 0; i < n; i++)
    {
        tmp = header->pathlookup->all_entries[i]->val;
        len = strlen(tmp) + 1;
        compress3(len, s);
        memcpy(s,tmp,len); s += len;
    }

    DB_WriteHeaderData(id,buffer,s-buffer,DB);

    efree(buffer);
}


int write_integer_table_to_header(int id, int table[], int table_size, void *DB)
{
    int     i,
            tmp;
    char   *s;
    char   *buffer;
	
    s = buffer = (char *) emalloc((table_size + 1) * 5);

    compress3(table_size,s);   /* Put the number of elements */
    for (i = 0; i < table_size; i++)
    {
        tmp = table[i] + 1;
        compress3(tmp, s); /* Put all the elements */
    }

    DB_WriteHeaderData(id, buffer, s-buffer, DB);

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

#define parse_int_from_buffer(num,s) UNPACKLONG2((num),(s))
#define parse_int2_from_buffer(num1,num2,s) UNPACKLONG2((num1),(s));UNPACKLONG2((num2),(s+sizeof(long)))


void    read_header(INDEXDATAHEADER *header, void *DB)
{
    int     id,
            len,
            tmp, tmp1, tmp2;
    char   *buffer;

    DB_InitReadHeader(DB);

    DB_ReadHeaderData(&id,&buffer,&len,DB);

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
        case LOCATIONLOOKUPTABLE_ID:
            parse_locationlookuptables_from_buffer(header, buffer);
            break;
        case PATHLOOKUPTABLE_ID:
            parse_pathlookuptable_from_buffer(header, buffer);
            break;
        case BUZZWORDS_ID:
            parse_stopwords_from_buffer(header, buffer);
            break;
        case WORDSPERDOC_ID:
            header->filetotalwordsarray = emalloc(sizeof(int) * header->totalfiles);
            parse_integer_table_from_buffer(header->filetotalwordsarray, header->totalfiles, buffer);
            break;
        default:
            progerr("Severe index error in header");
            break;
        }
        efree(buffer);
        DB_ReadHeaderData(&id,&buffer,&len,DB);
    }
    DB_EndReadHeader(DB);
}

/* Reads the metaNames from the index
*/

void    parse_MetaNames_from_buffer(INDEXDATAHEADER *header, char *buffer)
{
    int     len;
    int     num_metanames;
    int     dummy;
    int     metaType,
            i,
            metaID;
    char   *word;
    char   *s = buffer;


    uncompress2(num_metanames, s);

    for (i = 0; i < num_metanames; i++)
    {
        uncompress2(len,s);
        word = emalloc(len +1);
        memcpy(word,s,len); s += len;
        word[len] = '\0';
        /* Read metaID */
        uncompress2(metaID, s);
        /* metaType was saved as metaType+1 */
        uncompress2(metaType, s);

        /* add the meta tag */
        addMetaEntry(header, word, metaType, metaID, NULL, &dummy);

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

    char   *s = buffer;

    uncompress2(num_words, s);
	
    for (i=0; i < num_words ; i++)   
    {
        uncompress2(len, s);
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

    char   *s = buffer;

    uncompress2(num_words, s);
    for (i=0; i < num_words ; i++)
    {
        uncompress2(len, s);
        word = emalloc(len+1);
	memcpy(word,s,len); s += len;
        word[len] = '\0';
        addbuzzwordhash(header, word);
	efree(word);
    }
}



/* Read the lookuptables for structure, frequency */
void    parse_locationlookuptables_from_buffer(INDEXDATAHEADER *header, char *buffer)
{

    int     i,
            n,
            tmp;

    char   *s =buffer;

    uncompress2(n, s);
    if (!n)                     /* No words in file !!! */
    {
        header->structurelookup = NULL;
        uncompress2(n, s);     /* just to maintain file pointer */
        header->structfreqlookup = NULL;
        return;
    }
    header->structurelookup = (struct int_lookup_st *) emalloc(sizeof(struct int_lookup_st) + sizeof(struct int_st *) * (n - 1));

    header->structurelookup->n_entries = n;
    for (i = 0; i < n; i++)
    {
        header->structurelookup->all_entries[i] = (struct int_st *) emalloc(sizeof(struct int_st));

        uncompress2(tmp, s);
        header->structurelookup->all_entries[i]->val[0] = tmp - 1;
    }
    uncompress2(n, s);
    header->structfreqlookup = (struct int_lookup_st *) emalloc(sizeof(struct int_lookup_st) + sizeof(struct int_st *) * (n - 1));

    header->structfreqlookup->n_entries = n;
    for (i = 0; i < n; i++)
    {
        header->structfreqlookup->all_entries[i] = (struct int_st *) emalloc(sizeof(struct int_st) + sizeof(int));

        uncompress2(tmp, s);
        header->structfreqlookup->all_entries[i]->val[0] = tmp - 1;
        uncompress2(tmp, s);
        header->structfreqlookup->all_entries[i]->val[1] = tmp - 1;
    }
}

/* Read the lookuptable for paths/urls */
void    parse_pathlookuptable_from_buffer(INDEXDATAHEADER *header, char *buffer)
{
    int     i,
            n,
            len;
    char   *tmp;

    char   *s = buffer;

    uncompress2(n, s);
    header->pathlookup = (struct char_lookup_st *) emalloc(sizeof(struct char_lookup_st) + sizeof(struct char_st *) * (n - 1));

    header->pathlookup->n_entries = n;
    for (i = 0; i < n; i++)
    {
        header->pathlookup->all_entries[i] = (struct char_st *) emalloc(sizeof(struct char_st));

        uncompress2(len, s);
        tmp = emalloc(len);
        memcpy(tmp, s, len); s += len;
        header->pathlookup->all_entries[i]->val = tmp;
    }
}


void parse_integer_table_from_buffer(int table[], int table_size, char *buffer)
{
    int     tmp,i;
    char    *s = buffer;

    uncompress2(tmp,s);   /* Jump the number of elements */
    for (i = 0; i < table_size; i++)
    {
        uncompress2(tmp, s); /* Gut all the elements */
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

    DB_InitReadWords(indexf->DB);

    word[0]=(unsigned char)c;
    word[1]='\0';

    DB_ReadFirstWordInvertedIndex(word, &resultword, &wordID, indexf->DB);
    i = (int) ((unsigned char) c);
    if (!wordID)
    {
        DB_EndReadWords(indexf->DB);
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
    DB_ReadNextWordInvertedIndex(word, &resultword, &wordID, indexf->DB);
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
        DB_ReadNextWordInvertedIndex(word, &resultword, &wordID, indexf->DB);
    }
    buffer[bufferpos] = '\0';
    indexf->keywords[j] = buffer;
    return (indexf->keywords[j]);
}



/*------------------------------------------------------*/
/*---------- General entry point of DB module ----------*/

void   *DB_Create (char *dbname)
{
   return DB_Create_Native(dbname);
}

void   *DB_Open (char *dbname)
{
   return DB_Open_Native(dbname);
}

void    DB_Close(void *DB)
{
   DB_Close_Native((struct Handle_DBNative  *)DB);
}


void    DB_Remove(void *DB)
{
   DB_Remove_Native((struct Handle_DBNative  *)DB);
}

int     DB_InitWriteHeader(void *DB)
{
   return DB_InitWriteHeader_Native((struct Handle_DBNative  *)DB);
}

int     DB_EndWriteHeader(void *DB)
{
   return DB_EndWriteHeader_Native((struct Handle_DBNative  *)DB);
}

int     DB_WriteHeaderData(int id, char *s, int len, void *DB)
{
   return DB_WriteHeaderData_Native(id, s,len,(struct Handle_DBNative  *)DB);
}


int     DB_InitReadHeader(void *DB)
{
   return DB_InitReadHeader_Native((struct Handle_DBNative  *)DB);
}

int     DB_ReadHeaderData(int *id, char **s, int *len, void *DB)
{
   return DB_ReadHeaderData_Native(id, s, len, (struct Handle_DBNative  *)DB);
}

int     DB_EndReadHeader(void *DB)
{
   return DB_EndReadHeader_Native((struct Handle_DBNative  *)DB);
}


int     DB_InitWriteWords(void *DB)
{
   return DB_InitWriteWords_Native((struct Handle_DBNative  *)DB);
}

long    DB_GetWordID(void *DB)
{
   return DB_GetWordID_Native((struct Handle_DBNative  *)DB);
}

int     DB_WriteWord(char *word, long wordID, void *DB)
{
   return DB_WriteWord_Native(word, wordID, (struct Handle_DBNative  *)DB);
}

int     DB_WriteWordHash(char *word, long wordID, void *DB)
{
   return DB_WriteWordHash_Native(word, wordID, (struct Handle_DBNative  *)DB);
}

long    DB_WriteWordData(long wordID, char *worddata, int lendata, void *DB)
{
   return DB_WriteWordData_Native(wordID, worddata, lendata, (struct Handle_DBNative  *)DB);
}

int     DB_EndWriteWords(void *DB)
{
   return DB_EndWriteWords_Native((struct Handle_DBNative  *)DB);
}


int     DB_InitReadWords(void *DB)
{
   return DB_InitReadWords_Native((struct Handle_DBNative  *)DB);
}

int     DB_ReadWordHash(char *word, long *wordID, void *DB)
{
   return DB_ReadWordHash_Native(word, wordID, (struct Handle_DBNative  *)DB);
}

int     DB_ReadFirstWordInvertedIndex(char *word, char **resultword, long *wordID, void *DB)
{
   return DB_ReadFirstWordInvertedIndex_Native(word, resultword, wordID, (struct Handle_DBNative  *)DB);
}

int     DB_ReadNextWordInvertedIndex(char *word, char **resultword, long *wordID, void *DB)
{
   return DB_ReadNextWordInvertedIndex_Native(word, resultword, wordID, (struct Handle_DBNative  *)DB);
}

long    DB_ReadWordData(long wordID, char **worddata, int *lendata, void *DB)
{
   return DB_ReadWordData_Native(wordID, worddata, lendata, (struct Handle_DBNative  *)DB);
}

int     DB_EndReadWords(void *DB)
{
   return DB_EndReadWords_Native((struct Handle_DBNative  *)DB);
}



int     DB_InitWriteFiles(void *DB)
{
   return DB_InitWriteFiles_Native((struct Handle_DBNative  *)DB);
}

int     DB_EndWriteFiles(void *DB)
{
   return DB_EndWriteFiles_Native((struct Handle_DBNative  *)DB);
}

int     DB_WriteFile(int filenum, char *filedata,int sz_filedata, void *DB)
{
   return DB_WriteFile_Native(filenum, filedata, sz_filedata, (struct Handle_DBNative  *)DB);
}

int     DB_InitReadFiles(void *DB)
{
   return DB_InitReadFiles_Native((struct Handle_DBNative  *)DB);
}

int     DB_ReadFile(int filenum, char **filedata,int *sz_filedata, void *DB)
{
   return DB_ReadFile_Native(filenum, filedata,sz_filedata, (struct Handle_DBNative  *)DB);
}

int     DB_EndReadFiles(void *DB)
{
   return DB_EndReadFiles_Native((struct Handle_DBNative  *)DB);
}


int     DB_InitWriteSortedIndex(void *DB)
{
   return DB_InitWriteSortedIndex_Native((struct Handle_DBNative  *)DB);
}

int     DB_WriteSortedIndex(int propID, char *data, int sz_data,void *DB)
{
   return DB_WriteSortedIndex_Native(propID, data, sz_data,(struct Handle_DBNative  *)DB);
}

int     DB_EndWriteSortedIndex(void *DB)
{
   return DB_EndWriteSortedIndex_Native((struct Handle_DBNative  *)DB);
}

 
int     DB_InitReadSortedIndex(void *DB)
{
   return DB_InitReadSortedIndex_Native((struct Handle_DBNative  *)DB);
}

int     DB_ReadSortedIndex(int propID, char **data, int *sz_data,void *DB)
{
   return DB_ReadSortedIndex_Native(propID, data, sz_data,(struct Handle_DBNative  *)DB);
}

int     DB_EndReadSortedIndex(void *DB)
{
   return DB_EndReadSortedIndex_Native((struct Handle_DBNative  *)DB);
}

