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
#include "file.h"
#include "error.h"
#include "string.h"
#include "compress.h"
#include "hash.h"
#include "db.h"
#include "db_native.h"


/*
  -- init structures for this module
*/

void initModule_DBNative (SWISH  *sw)
{
    struct MOD_DB *Db;

    Db = (struct MOD_DB *)emalloc(sizeof(struct MOD_DB));

    Db->DB_name = (char *) estrdup("native");

    Db->DB_Create = DB_Create_Native;
    Db->DB_Open = DB_Open_Native;
    Db->DB_Close = DB_Close_Native;
    Db->DB_Remove = DB_Remove_Native;
    
    Db->DB_InitWriteHeader = DB_InitWriteHeader_Native;
    Db->DB_WriteHeaderData = DB_WriteHeaderData_Native;
    Db->DB_EndWriteHeader = DB_EndWriteHeader_Native;
    
    Db->DB_InitReadHeader = DB_InitReadHeader_Native;
    Db->DB_ReadHeaderData = DB_ReadHeaderData_Native;
    Db->DB_EndReadHeader = DB_EndReadHeader_Native;
    
    Db->DB_InitWriteWords = DB_InitWriteWords_Native;
    Db->DB_GetWordID = DB_GetWordID_Native;
    Db->DB_WriteWord = DB_WriteWord_Native;
    Db->DB_WriteWordHash = DB_WriteWordHash_Native;
    Db->DB_WriteWordData = DB_WriteWordData_Native;
    Db->DB_EndWriteWords = DB_EndWriteWords_Native;
    
    Db->DB_InitReadWords = DB_InitReadWords_Native;
    Db->DB_ReadWordHash = DB_ReadWordHash_Native;
    Db->DB_ReadFirstWordInvertedIndex = DB_ReadFirstWordInvertedIndex_Native;
    Db->DB_ReadNextWordInvertedIndex = DB_ReadNextWordInvertedIndex_Native;
    Db->DB_ReadWordData = DB_ReadWordData_Native;
    Db->DB_EndReadWords = DB_EndReadWords_Native;
    
    Db->DB_InitWriteFiles = DB_InitWriteFiles_Native;
    Db->DB_WriteFile = DB_WriteFile_Native;
    Db->DB_EndWriteFiles = DB_EndWriteFiles_Native;

    Db->DB_InitReadFiles = DB_InitReadFiles_Native;
    Db->DB_ReadFile = DB_ReadFile_Native;
    Db->DB_EndReadFiles = DB_EndReadFiles_Native;
    
    Db->DB_InitWriteSortedIndex = DB_InitWriteSortedIndex_Native;
    Db->DB_WriteSortedIndex = DB_WriteSortedIndex_Native;
    Db->DB_EndWriteSortedIndex = DB_EndWriteSortedIndex_Native;
     
    Db->DB_InitReadSortedIndex = DB_InitReadSortedIndex_Native;
    Db->DB_ReadSortedIndex = DB_ReadSortedIndex_Native;
    Db->DB_EndReadSortedIndex = DB_EndReadSortedIndex_Native;

    sw->Db = Db;

    return;
}


/*
  -- release all wired memory for this module
*/

void freeModule_DBNative (SWISH *sw)
{
   efree(sw->Db->DB_name);
   efree(sw->Db);
   sw->Db = NULL;
   return;
}



/* ---------------------------------------------- */



/*
 -- Config Directives
 -- Configuration directives for this Module
 -- return: 0/1 = none/config applied
*/

int configModule_DBNative  (SWISH *sw, StringList *sl)
{
 // struct MOD_DBNative *md = sw->DBNative;
 // char *w0    = sl->word[0];
 int  retval = 1;


  retval = 0; // tmp due to empty routine

  return retval;
}






/* Does an index file have a readable format?
*/

int DB_OkHeader(FILE * fp)
{
    long    swish_magic;

    fseek(fp, 0, 0);
    swish_magic = readlong(fp);
    if (swish_magic != SWISH_MAGIC)
    {
        fseek(fp, 0, 0);
        return 0;
    }
    return 1;
}


void *DB_Create_Native (char *dbname)
{
   int i;
   long    swish_magic;
   FILE *fp;

   struct Handle_DBNative *DB;


         /* Allocate structure */
   DB= (struct Handle_DBNative  *) emalloc(sizeof(struct Handle_DBNative));

          /* Create index File */
   CreateEmptyFile(dbname);
   if(!(DB->fp = openIndexFILEForReadAndWrite(dbname)))
	   progerrno("Couldn't create the index file \"%s\".", dbname);

   fp = DB->fp;

#ifdef PROPFILE
    {
        char *s = emalloc( strlen(dbname) + strlen(".prop") + 1 );
        strcpy( s, dbname );
        strcat( s, ".prop" );

        CreateEmptyFile(s);
        if( !(DB->prop = openIndexFILEForReadAndWrite(s)) )
            progerrno("Couldn't create the property file \"%s\".", s);

        efree(s);         
    }
#endif

   

   DB->offsetstart = 0;
   DB->hashstart = 0;
   DB->fileoffsetarray = NULL;
   DB->fileoffsetarray_maxsize = 0;
   DB->nextwordoffset = 0;
   DB->num_docs = 0;
   DB->num_words = 0;
   DB->worddata_counter = 0;
   DB->worddata_wordID = NULL;
   DB->worddata_offset = NULL;

   DB->mode = 1;
   DB->lastsortedindex = 0;

   DB->dbname = estrdup(dbname);

   for(i=0 ; i<MAXCHARS; i++) DB->offsets[i] = 0L;
   for(i=0 ; i<SEARCHHASHSIZE; i++) DB->hashoffsets[i] = 0L;
   for(i=0 ; i<SEARCHHASHSIZE; i++) DB->lasthashval[i] = 0L;

   swish_magic = SWISH_MAGIC;
   printlong(DB->fp,swish_magic);

         /* Reserve space for offset pointers */
   DB->offsetstart = ftell(fp);
   for (i = 0; i < MAXCHARS; i++)
      printlong(fp,(long)0);

   DB->hashstart = ftell(fp);
   for (i = 0; i < SEARCHHASHSIZE; i++)
      printlong(fp,(long)0);

   return (void *)DB;
}

void *DB_Open_Native (char *dbname)
{
   struct Handle_DBNative *DB;
   FILE *fp;
   int i;
   long pos;

          /* Allocate structure */
   DB= (struct Handle_DBNative  *) emalloc(sizeof(struct Handle_DBNative));
          /* Create index File */
   if(!(DB->fp = openIndexFILEForRead(dbname)))
      progerrno("Couldn't open the index file \"%s\".", dbname);


#ifdef PROPFILE
    {
        char *s = emalloc( strlen(dbname) + strlen(".prop") + 1 );
        strcpy( s, dbname );
        strcat( s, ".prop" );

        if( !(DB->prop = openIndexFILEForRead(s)) )
            progerrno("Couldn't open the property file \"%s\".", s);

        efree(s);         
    }
#endif


   if (!DB_OkHeader(DB->fp)) {
      progerr("File \"%s\" has an unknown format.", dbname);
   }

   fp = DB->fp;
   DB->offsetstart = 0;
   DB->hashstart = 0;
   DB->fileoffsetarray = NULL;
   DB->fileoffsetarray_maxsize = 0;
   DB->nextwordoffset = 0;
   DB->num_docs = 0;
   DB->num_words = 0;
   DB->worddata_counter = 0;
   DB->worddata_wordID = NULL;
   DB->worddata_offset = NULL;
   DB->mode = 0;
   DB->lastsortedindex = 0;


   DB->dbname = estrdup(dbname);

   		/* Read offsets lookuptable */
   DB->offsetstart = ftell(fp);
   for (i = 0; i < MAXCHARS; i++)
       DB->offsets[i] = readlong(fp);

		/* Read hashoffsets lookuptable */
   DB->hashstart = ftell(fp);
   for (i = 0; i < SEARCHHASHSIZE; i++)
       DB->hashoffsets[i] = readlong(fp);

        /* Now, read fileoffsets */
   DB->fileoffsetarray = emalloc((DB->fileoffsetarray_maxsize = BIGHASHSIZE) * sizeof(long));
   fseek(fp,DB->offsets[FILEOFFSETPOS],0);
   for ( i = 0; (pos=readlong(fp)) ; i++)
   {
       if (i == DB->fileoffsetarray_maxsize)
          DB->fileoffsetarray = erealloc(DB->fileoffsetarray,(DB->fileoffsetarray_maxsize += 10000) * sizeof(long));
       DB->fileoffsetarray[i] = pos;
   }
   DB->num_docs = i;

   return (void *)DB;
}

void DB_Close_Native(void *db)
{
   int i;
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
   FILE *fp = DB->fp;

   if(DB->mode)  /* If we are indexing update offsets to words and files */
   {
	       /* Update internal pointers */
       fseek(fp,DB->offsetstart,0);
       for (i = 0; i < MAXCHARS; i++)
           printlong(fp,DB->offsets[i]);
       fseek(fp,DB->hashstart,0);
       for (i = 0; i < SEARCHHASHSIZE; i++)
           printlong(fp,DB->hashoffsets[i]);
   }
   fclose(DB->fp);
   efree(DB->dbname);
   if(DB->fileoffsetarray) efree(DB->fileoffsetarray);
   efree(DB);
}

void DB_Remove_Native(void *db)
{
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

   fclose(DB->fp);
   remove(DB->dbname);
   efree(DB->dbname);
   efree(DB);
}


/*--------------------------------------------*/
/*--------------------------------------------*/
/*              Header stuff                  */
/*--------------------------------------------*/
/*--------------------------------------------*/

int DB_InitWriteHeader_Native(void *db)
{
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

   DB->offsets[HEADERPOS] = ftell(DB->fp);
   return 0;
}


int DB_EndWriteHeader_Native(void *db)
{
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
   FILE *fp=DB->fp;

      /* End of header delimiter */
   fputc(0, fp);

   return 0;
}

int DB_WriteHeaderData_Native(int id, unsigned char *s, int len, void *db)
{
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

   FILE *fp = DB->fp;

   compress1(id, fp, fputc);
   compress1(len, fp, fputc);
   fwrite(s, len, sizeof(char), fp);

   return 0;
}


int DB_InitReadHeader_Native(void *db)
{
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

   fseek(DB->fp,DB->offsets[HEADERPOS],0);
   return 0;
}

int DB_ReadHeaderData_Native(int *id, unsigned char **s, int *len, void *db)
{
   int tmp;
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
   FILE *fp = DB->fp;

   tmp = uncompress1(fp,fgetc);
   *id = tmp;
   if(tmp)
   {
      tmp = uncompress1(fp,fgetc);
      *s = (char *) emalloc( tmp +1);
      *len = tmp;
      fread(*s, *len, sizeof(char), fp);
   }
   else
   {
      len = 0;
      *s = NULL;
   }
   return 0;
}

int DB_EndReadHeader_Native(void *db)
{
    return 0;
}

/*--------------------------------------------*/
/*--------------------------------------------*/
/*                 Word Stuff                 */
/*--------------------------------------------*/
/*--------------------------------------------*/

int DB_InitWriteWords_Native(void *db)
{
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

   DB->offsets[WORDPOS] = ftell(DB->fp);
   return 0;
}


int DB_EndWriteWords_Native(void *db)
{
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
   FILE *fp = (FILE *) DB->fp;
   int i, wordlen;
   long wordID,f_offset;

   fputc(0, fp);   /* End of words mark */
   

         /* Now update word's data offset into the list of words */
         /* Simple check  words and worddata must match */

   if(DB->num_words != DB->worddata_counter)
       progerrno("Internal DB_native error - DB->num_words != DB->worddata_counter");

   for(i=0;i<DB->num_words;i++)
   {
       wordID = DB->worddata_wordID[i];
       f_offset = DB->worddata_offset[i];
        /* Position file pointer in word */
       fseek(fp,wordID,SEEK_SET);
        /* Jump over word length and word */
       wordlen = uncompress1(fp,fgetc);   /* Get Word length */
       fseek(fp,(long)wordlen,SEEK_CUR);  /* Jump Word */
        /* Jump also hash pointer */
       fseek(fp,(long)sizeof(long),SEEK_CUR);  /* Jump Hash pointer */
        /* Write offset to word data */
       printlong(fp,f_offset);
   } 
   efree(DB->worddata_wordID);
   DB->worddata_wordID = NULL;
   efree(DB->worddata_offset);
   DB->worddata_offset = NULL;
   DB->worddata_counter = 0;
       /* Restore file pointer at the end of file */
   fseek(fp,0,SEEK_END);
   return 0;
}

long DB_GetWordID_Native(void *db)
{
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

   return ftell(DB->fp);   /* Native database uses position as a Word ID */
}

int DB_WriteWord_Native(char *word, long wordID, void *db)
{
    int     i,
            wordlen;
    struct  Handle_DBNative *DB = (struct Handle_DBNative *) db;

    FILE   *fp = DB->fp;


    i = (int) ((unsigned char) word[0]);
    /* if(isindexchar(indexf->header,i) && !indexf->offsets[i]) indexchars stuff removed */
    if (!DB->offsets[i])
        DB->offsets[i] = wordID;


    /* Write word length, word and a NULL offset */
    wordlen = strlen(word);
    compress1(wordlen, fp, fputc);
    fwrite(word, wordlen, sizeof(char), fp);

    printlong(fp, (long) 0);    /* hash offset */
    printlong(fp, (long) 0);    /* word's data pointer */

    DB->num_words++;

    return 0;
}


long DB_WriteWordData_Native(long wordID, unsigned char *worddata, int lendata, void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

    FILE *fp = DB->fp;

        /* We must be at the end of the file */

    if(!DB->worddata_counter)
    {
        DB->worddata_wordID = emalloc(DB->num_words * sizeof(long));
        DB->worddata_offset = emalloc(DB->num_words * sizeof(long));
    }
        /* Preserve word's data offset */
    DB->worddata_wordID[DB->worddata_counter] = wordID;
    DB->worddata_offset[DB->worddata_counter++] = ftell(fp);

        /* Write the worddata to disk */
    compress1(lendata,fp, fputc);
    fwrite(worddata,lendata,1,fp);

        /* A NULL byte to indicate end of word data */
    fputc(0,fp);

    return 0;
}



int DB_WriteWordHash_Native(char *word, long wordID, void *db)
{
    int     wordlen,
            hashval;
    struct  Handle_DBNative *DB = (struct Handle_DBNative *) db;
    FILE   *fp = DB->fp;

    hashval = searchhash(word);
    if(!DB->hashoffsets[hashval])
    {
        DB->hashoffsets[hashval] = wordID;
    }
    fseek(fp, wordID, SEEK_SET);
        /* Get wordlen */
    wordlen = uncompress1(fp,fgetc);
            /* Jump word */
    fseek(fp, (long) wordlen, SEEK_CUR);
            /* Write Null terminator */
    printlong(fp, (long) 0);
            /* Update previous word in hashlist */
    if(DB->lasthashval[hashval])
    {
        fseek(fp, DB->lasthashval[hashval], SEEK_SET);
            /* Get wordlen */
        wordlen = uncompress1(fp,fgetc);
            /* Jump word */
        fseek(fp, (long) wordlen, SEEK_CUR);
                /* Overwrite previous NULL pointer */
        printlong(fp, wordID);

    }
	DB->lasthashval[hashval] = wordID;

    return 0;
}

int     DB_InitReadWords_Native(void *db)
{
    return 0;
}

int     DB_EndReadWords_Native(void *db)
{
    return 0;
}


int DB_ReadWordHash_Native(char *word, long *wordID, void *db)
{
    int     wordlen,
            res,
            hashval;
    long    offset, dataoffset;
    char   *fileword = NULL;
    struct  Handle_DBNative *DB = (struct Handle_DBNative *) db;
    FILE   *fp = DB->fp;


        /* If there is not a star use the hash approach ... */
    res = 1;
        
        /* Get hash file offset */
    hashval = searchhash(word);
    if (!(offset = DB->hashoffsets[hashval]))
    {
        *wordID = 0;
        return 0;
    }
        /* Search for word */
    while (res)
    {
        /* Position in file */
        fseek(fp, offset, SEEK_SET);
        /* Get word */
        wordlen = uncompress1(fp,fgetc);
        fileword = emalloc(wordlen + 1);
        fread(fileword, 1, wordlen, fp);
        fileword[wordlen] = '\0';
        offset = readlong(fp); /* Next hash */
        dataoffset = readlong(fp); /* Offset to Word data */

        res = strcmp(word, fileword);
        efree(fileword);

        if(!res)
            break;          /* Found !! */
        else if (!offset)
        {
            dataoffset = 0;
            break;
        }
    }
    *wordID = dataoffset;
    return 0;
}



int DB_ReadFirstWordInvertedIndex_Native(char *word, char **resultword, long *wordID, void *db)
{
    int     wordlen,
            i,
            res,
            len,
            found;
    long    dataoffset = 0;
    char   *fileword = NULL;
    struct  Handle_DBNative *DB = (struct Handle_DBNative *) db;
    FILE   *fp = DB->fp;
  

    len = strlen(word);

    i = (int) ((unsigned char) word[0]);

    if (!DB->offsets[i])
    {
        *resultword = NULL;
        *wordID = 0;
        return 0;
    }
    found = 1;
    fseek(fp, DB->offsets[i], 0);

        /* Look for first occurrence */
    wordlen = uncompress1(fp,fgetc);
    fileword = (char *) emalloc(wordlen + 1);

    while (wordlen)
    {
        fread(fileword, 1, wordlen, fp);
        fileword[wordlen] = '\0';
        readlong(fp);       /* jump hash offset */
        dataoffset = readlong(fp); /* Get offset to word's data */
        if (!(res = strncmp(word, fileword, len))) /*Found!! */
        {
            DB->nextwordoffset = ftell(fp); /* preserve next word pos */
            break;
        }
        if (res < 0)
        {
            dataoffset = 0;
            break;    
        }
            /* Go to next value */
        wordlen = uncompress1(fp,fgetc); /* Next word */
        if (!wordlen)
        {
            dataoffset = 0;
            break;    
        }
        efree(fileword);
        fileword = (char *) emalloc(wordlen + 1);
    }
    if(!dataoffset)
    {
        efree(fileword);
        *resultword = NULL;
    }
    else
        *resultword = fileword;
    *wordID = dataoffset;

    return 0;
}

int DB_ReadNextWordInvertedIndex_Native(char *word, char **resultword, long *wordID, void *db)
{
    int     len,
            wordlen;
    long    dataoffset;
    char   *fileword;
    struct  Handle_DBNative *DB = (struct Handle_DBNative *) db;
    FILE   *fp = DB->fp;

    if(!DB->nextwordoffset)
    {
    *resultword = NULL;
        *wordID = 0;
        return 0;
    }

    len = strlen(word);


    fseek(fp, DB->nextwordoffset, SEEK_SET);

    wordlen = uncompress1(fp,fgetc);
    fileword = (char *) emalloc(wordlen + 1);

    fread(fileword, 1, wordlen, fp);
    fileword[wordlen] = '\0';
    if(strncmp(word, fileword, len))
    {
        efree(fileword);
        fileword = NULL;
        dataoffset = 0; /* No more data */
        DB->nextwordoffset = 0;
    } 
    else
    {
        readlong(fp);       /* jump hash offset */
        dataoffset = readlong(fp); /* Get data offset */
        DB->nextwordoffset = ftell(fp);
    }
    *resultword = fileword;
    *wordID = dataoffset;

    return 0;

}


long DB_ReadWordData_Native(long wordID, unsigned char **worddata, int *lendata, void *db)
{
    int      len;
    char    *buffer;
    struct   Handle_DBNative *DB = (struct Handle_DBNative *) db;
    FILE    *fp = DB->fp;

    fseek(fp,wordID,0);
    len = uncompress1(fp,fgetc);
    buffer = emalloc(len);
    fread(buffer,len,1,fp);

    *worddata=buffer;
    *lendata=len;

    return 0;
}

/*--------------------------------------------*/
/*--------------------------------------------*/
/*              FileList  Stuff               */
/*--------------------------------------------*/
/*--------------------------------------------*/

int DB_InitWriteFiles_Native(void *db)
{
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

   DB->offsets[FILELISTPOS] = ftell(DB->fp);
   DB->fileoffsetarray = (long *) emalloc((DB->fileoffsetarray_maxsize = BIGHASHSIZE) * sizeof(long));

   return 0;
}


int DB_EndWriteFiles_Native(void *db)
{
   int     i;
   long    offset;
   struct  Handle_DBNative *DB = (struct Handle_DBNative *) db;
   FILE   *fp = DB->fp;

   fputc(0, fp);   /* End of filelist mark */

   DB->offsets[FILEOFFSETPOS] = ftell(fp);
   for (i = 0; i < DB->num_docs; i++)
   {
       offset = (long) DB->fileoffsetarray[i];
       printlong(fp, offset);
   }
   printlong(fp, (long) 0);
   return 0;
}

int DB_WriteFile_Native(int filenum, unsigned char *filedata,int sz_filedata, void *db)
{
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

   if (DB->fileoffsetarray_maxsize == filenum)
   {
       DB->fileoffsetarray = (long *) erealloc(DB->fileoffsetarray, (DB->fileoffsetarray_maxsize += 10000) * sizeof(long));
   }

   DB->fileoffsetarray[filenum] = ftell(DB->fp);
   compress1(sz_filedata, DB->fp, fputc); /* Write length */
   fwrite(filedata, sz_filedata, 1, DB->fp); /* Write data */

   DB->num_docs++;

   return 0;
}

int DB_InitReadFiles_Native(void *db)
{
   return 0;
}

int DB_ReadFile_Native(int filenum, unsigned char **filedata,int *sz_filedata, void *db)
{
    int     len;
    char   *buffer;
    struct  Handle_DBNative *DB = (struct Handle_DBNative *) db;

    if (filenum > DB->num_docs)
    {
        *filedata=NULL;
        *sz_filedata=0;
    }
    else
    {
        fseek(DB->fp, DB->fileoffsetarray[filenum-1], 0);

        len = uncompress1(DB->fp,fgetc);
        buffer = emalloc(len);
        fread(buffer, len, 1, DB->fp);

        *filedata=buffer;
        *sz_filedata=len;
    }

    return 0;
}


int DB_EndReadFiles_Native(void *db)
{

   return 0;
}


/*--------------------------------------------*/
/*--------------------------------------------*/
/*              Sorted data Stuff             */
/*--------------------------------------------*/
/*--------------------------------------------*/





int     DB_InitWriteSortedIndex_Native(void *db)
{
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

   DB->offsets[SORTEDINDEX] = ftell(DB->fp);

   return 0;
}

int     DB_WriteSortedIndex_Native(int propID, unsigned char *data, int sz_data,void *db)
{
   long tmp1,tmp2;
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
   FILE *fp = DB->fp;

   tmp1 = ftell(fp);
   printlong(fp,(long)0);  /* Pointer to next table if any */
	/* Write ID */
   compress1(propID,fp,fputc);
   /* Write len of data */
   compress1(sz_data,fp,putc);
   /* Write data */
   fwrite(data,sz_data,1,fp);

   tmp2 = ftell(fp);
   if(DB->lastsortedindex)
   {
       fseek(fp,DB->lastsortedindex,0);
       printlong(fp,tmp1);
       fseek(fp,tmp2,0);
   }
   DB->lastsortedindex = tmp1;
   return 0;
}

int     DB_EndWriteSortedIndex_Native(void *db)
{
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
   FILE *fp = DB->fp;

   printlong(fp,(long)0);  /* No next table mark - Useful if no presorted indexes */
         /* NULL meta id- Only useful if no presorted indexes  */
   fputc(0, fp);

   return 0;
}

 
int     DB_InitReadSortedIndex_Native(void *db)
{
   return 0;
}

int     DB_ReadSortedIndex_Native(int propID, unsigned char **data, int *sz_data,void *db)
{
   long next, id, tmp;
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
   FILE *fp = DB->fp;

   fseek(fp,DB->offsets[SORTEDINDEX],0);
   next = readlong(fp);
        /* read propID */
   id = uncompress1(fp,fgetc);
   while(1)
   {
       if(id == propID)
       {
           tmp = uncompress1(fp,fgetc);
           *sz_data = tmp;
           *data = emalloc(*sz_data);
           fread(*data,*sz_data,1,fp);
           return 0;
       }
       if(next)
       {
           fseek(fp,next,0);
           next = readlong(fp);
           id = uncompress1(fp,fgetc);
       }
       else
       {
           *sz_data = 0;
           *data = NULL;
           return 0;
       }
   }
   return 0;
}

int     DB_EndReadSortedIndex_Native(void *db)
{
   return 0;
}

/* 
** Jose Ruiz 04/00
** Store a portable long with just four bytes
*/
void    printlong(FILE * fp, unsigned long num)
{
    num = PACKLONG(num);              /* Make the number portable */
    fwrite(&num, MAXLONGLEN, 1, fp);
}

/* 
** Jose Ruiz 04/00
** Read a portable long (just four bytes)
*/
unsigned long    readlong(FILE * fp)
{
    unsigned long    num;

    fread(&num, MAXLONGLEN, 1, fp);
    return UNPACKLONG(num);            /* Make the number readable */
}

