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
#include "file.h"
#include "error.h"
#include "string.h"
#include "mem.h"
#include "compress.h"
#include "hash.h"
#include "mem.h"
#include "db.h"
#include "db_gdbm.h"


/*
  -- init structures for this module
*/

void initModule_DBgdbm (SWISH  *sw)
{
    struct MOD_DB *Db;

    Db = (struct MOD_DB *)emalloc(sizeof(struct MOD_DB));
    
    Db->DB_name = (char *) estrdup("gdbm");
 
    Db->DB_Create = DB_Create_gdbm;
    Db->DB_Open = DB_Open_gdbm;
    Db->DB_Close = DB_Close_gdbm;
    Db->DB_Remove = DB_Remove_gdbm;
    
    Db->DB_InitWriteHeader = DB_InitWriteHeader_gdbm;
    Db->DB_WriteHeaderData = DB_WriteHeaderData_gdbm;
    Db->DB_EndWriteHeader = DB_EndWriteHeader_gdbm;
    
    Db->DB_InitReadHeader = DB_InitReadHeader_gdbm;
    Db->DB_ReadHeaderData = DB_ReadHeaderData_gdbm;
    Db->DB_EndReadHeader = DB_EndReadHeader_gdbm;
    
    Db->DB_InitWriteWords = DB_InitWriteWords_gdbm;
    Db->DB_GetWordID = DB_GetWordID_gdbm;
    Db->DB_WriteWord = DB_WriteWord_gdbm;
    Db->DB_WriteWordHash = DB_WriteWordHash_gdbm;
    Db->DB_WriteWordData = DB_WriteWordData_gdbm;
    Db->DB_EndWriteWords = DB_EndWriteWords_gdbm;
    
    Db->DB_InitReadWords = DB_InitReadWords_gdbm;
    Db->DB_ReadWordHash = DB_ReadWordHash_gdbm;
    Db->DB_ReadFirstWordInvertedIndex = DB_ReadFirstWordInvertedIndex_gdbm;
    Db->DB_ReadNextWordInvertedIndex = DB_ReadNextWordInvertedIndex_gdbm;
    Db->DB_ReadWordData = DB_ReadWordData_gdbm;
    Db->DB_EndReadWords = DB_EndReadWords_gdbm;
    
    Db->DB_InitWriteFiles = DB_InitWriteFiles_gdbm;
    Db->DB_WriteFile = DB_WriteFile_gdbm;
    Db->DB_EndWriteFiles = DB_EndWriteFiles_gdbm;

    Db->DB_InitReadFiles = DB_InitReadFiles_gdbm;
    Db->DB_ReadFile = DB_ReadFile_gdbm;
    Db->DB_EndReadFiles = DB_EndReadFiles_gdbm;
    
    Db->DB_InitWriteSortedIndex = DB_InitWriteSortedIndex_gdbm;
    Db->DB_WriteSortedIndex = DB_WriteSortedIndex_gdbm;
    Db->DB_EndWriteSortedIndex = DB_EndWriteSortedIndex_gdbm;
     
    Db->DB_InitReadSortedIndex = DB_InitReadSortedIndex_gdbm;
    Db->DB_ReadSortedIndex = DB_ReadSortedIndex_gdbm;
    Db->DB_EndReadSortedIndex = DB_EndReadSortedIndex_gdbm;

    sw->Db = Db;

  return;
}


/*
  -- release all wired memory for this module
*/

void freeModule_DBgdbm (SWISH *sw)
{
   efree(sw->Db->DB_name);
   efree(sw->Db);
   return;
}



/* ---------------------------------------------- */



/*
 -- Config Directives
 -- Configuration directives for this Module
 -- return: 0/1 = none/config applied
*/

int configModule_DBgdbm  (SWISH *sw, StringList *sl)
{
 // struct MOD_DBgdbm *md = sw->DBgdbm;
 // char *w0    = sl->word[0];
 int  retval = 1;


  retval = 0; // tmp due to empty routine

  return retval;
}






void *DB_Create_gdbm (char *dbname)
{
   char *tmp;
   struct Handle_DBgdbm *DB;

          /* Allocate structure */
   DB = (struct Handle_DBgdbm  *) emalloc(sizeof(struct Handle_DBgdbm));
   DB->wordcounter = 0;

   tmp = emalloc(strlen(dbname)+4+1);
          /* Create index DB */
   sprintf(tmp,"%s.hdr",dbname);
   DB->dbf_header = gdbm_open(tmp, 65536, GDBM_NEWDB | GDBM_FAST, 0666, NULL);
      /* gdbm file to store words */
   sprintf(tmp,"%s.wds",dbname);
   DB->dbf_words = gdbm_open(tmp, 65536, GDBM_NEWDB | GDBM_FAST, 0666, NULL);
      /* gdbm file to store word's data */
   sprintf(tmp,"%s.wdd",dbname);
   DB->dbf_worddata = gdbm_open(tmp, 65536, GDBM_NEWDB | GDBM_FAST, 0666, NULL);
      /* gdbm file to store word's inverted index */
   sprintf(tmp,"%s.wii",dbname);
   DB->dbf_invertedindex = gdbm_open(tmp, 65536, GDBM_NEWDB | GDBM_FAST, 0666, NULL);
      /* gdbm file to store docs's data */
   sprintf(tmp,"%s.doc",dbname);
   DB->dbf_docs = gdbm_open(tmp, 65536, GDBM_NEWDB | GDBM_FAST, 0666, NULL);
      /* gdbm file to store sorted indexes */
   sprintf(tmp,"%s.srt",dbname);
   DB->dbf_sorted_indexes = gdbm_open(tmp, 65536, GDBM_NEWDB | GDBM_FAST, 0666, NULL);

   efree(tmp);

   return (void *)DB;
}

void *DB_Open_gdbm (char *dbname)
{
   char *tmp;
   struct Handle_DBgdbm *DB;

          /* Allocate structure */
   DB = (struct Handle_DBgdbm  *) emalloc(sizeof(struct Handle_DBgdbm));
   DB->wordcounter = 0;
   DB->lastkey_header.dsize = 0;
   DB->lastkey_header.dptr = NULL;

   tmp = emalloc(strlen(dbname)+4+1);
          /* Create index DB */
   sprintf(tmp,"%s.hdr",dbname);
   DB->dbf_header = gdbm_open(tmp, 65536, GDBM_READER, 0666, NULL);
      /* gdbm file to store words */
   sprintf(tmp,"%s.wds",dbname);
   DB->dbf_words = gdbm_open(tmp, 65536, GDBM_READER, 0666, NULL);
      /* gdbm file to store word's data */
   sprintf(tmp,"%s.wdd",dbname);
   DB->dbf_worddata = gdbm_open(tmp, 65536, GDBM_READER, 0666, NULL);
      /* gdbm file to store word's inverted index */
   sprintf(tmp,"%s.wii",dbname);
   DB->dbf_invertedindex = gdbm_open(tmp, 65536, GDBM_READER, 0666, NULL);
      /* gdbm file to store docs's data */
   sprintf(tmp,"%s.doc",dbname);
   DB->dbf_docs = gdbm_open(tmp, 65536, GDBM_READER, 0666, NULL);
      /* gdbm file to store sorted indexes */
   sprintf(tmp,"%s.srt",dbname);
   DB->dbf_sorted_indexes = gdbm_open(tmp, 65536, GDBM_READER, 0666, NULL);

   efree(tmp);

   return (void *)DB;
}

void DB_Close_gdbm(void *db)
{
   struct Handle_DBgdbm *DB = (struct Handle_DBgdbm *)db;

      /* gdbm file to store header */
   gdbm_close( DB->dbf_header );
      /* gdbm file to store words */
   gdbm_close( DB->dbf_words );
      /* gdbm file to store word's data */
   gdbm_close( DB->dbf_worddata );
      /* gdbm file to store word's inverted index */
   gdbm_close( DB->dbf_invertedindex );
      /* gdbm file to store docs's data */
   gdbm_close( DB->dbf_docs );
      /* gdbm file to store sorted indexes */
   gdbm_close( DB->dbf_sorted_indexes );

   efree(DB);
}

void DB_Remove_gdbm(void *db)
{
   struct Handle_DBgdbm *DB = (struct Handle_DBgdbm *)db;

      /* gdbm file to store header */
   gdbm_close( DB->dbf_header );
      /* gdbm file to store words */
   gdbm_close( DB->dbf_words );
      /* gdbm file to store word's data */
   gdbm_close( DB->dbf_worddata );
      /* gdbm file to store word's inverted index */
   gdbm_close( DB->dbf_invertedindex );
      /* gdbm file to store docs's data */
   gdbm_close( DB->dbf_docs );
      /* gdbm file to store sorted indexes */
   gdbm_close( DB->dbf_sorted_indexes );

   efree(DB);
}


/*--------------------------------------------*/
/*--------------------------------------------*/
/*              Header stuff                  */
/*--------------------------------------------*/
/*--------------------------------------------*/

int DB_InitWriteHeader_gdbm(void *db)
{
   return 0;
}


int DB_EndWriteHeader_gdbm(void *db)
{
   struct Handle_DBgdbm *DB = (struct Handle_DBgdbm *)db;

   DB->lastkey_header.dsize = 0;
   DB->lastkey_header.dptr = NULL;

   return 0;
}

int DB_WriteHeaderData_gdbm(int id, unsigned char *s, int len, void *db)
{
   struct Handle_DBgdbm *DB = (struct Handle_DBgdbm *)db;

   datum key, content;
   int ret;

   key.dsize = sizeof(int);
   key.dptr = (char *) &id;

   content.dsize = len;
   content.dptr = s;

   ret = gdbm_store(DB->dbf_header,key,content,GDBM_INSERT);

   if(ret)
      progerr("GDBM error writing header");

   return 0;
}


int DB_InitReadHeader_gdbm(void *db)
{
   struct Handle_DBgdbm *DB = (struct Handle_DBgdbm *)db;

   DB->lastkey_header.dsize = 0;
   DB->lastkey_header.dptr = NULL;

   return 0;
}

int DB_ReadHeaderData_gdbm(int *id, unsigned char **s, int *len, void *db)
{
   struct Handle_DBgdbm *DB = (struct Handle_DBgdbm *)db;
   datum key,content;

   if(DB->lastkey_header.dptr)
   {
      key = gdbm_nextkey(DB->dbf_header,DB->lastkey_header);
   }
   else
   {
      key = gdbm_firstkey(DB->dbf_header);
   }
   DB->lastkey_header = key;

   if(!key.dptr)
   {
      *id = 0;
      *len = 0;
      *s = NULL;
   }
   else 
   {
      *id = * (int*) key.dptr;
      content = gdbm_fetch(DB->dbf_header, key);
      *len = content.dsize;
      *s = content.dptr;
   }

   return 0;
}

int DB_EndReadHeader_gdbm(void *db)
{
    return 0;
}

/*--------------------------------------------*/
/*--------------------------------------------*/
/*                 Word Stuff                 */
/*--------------------------------------------*/
/*--------------------------------------------*/

int DB_InitWriteWords_gdbm(void *db)
{
   return 0;
}


int DB_EndWriteWords_gdbm(void *db)
{
   return 0;
}

long DB_GetWordID_gdbm(void *db)
{
   struct Handle_DBgdbm *DB = (struct Handle_DBgdbm *)db;

   return (long) ++(DB->wordcounter);
}

int DB_WriteWord_gdbm(char *word, long wordID, void *db)
{
   return 0;
}


long DB_WriteWordData_gdbm(long wordID, unsigned char *worddata, int lendata, void *db)
{
   struct Handle_DBgdbm *DB = (struct Handle_DBgdbm *)db;

   datum key, content;
   int ret;

   key.dsize = sizeof(long);
   key.dptr = (char *) &wordID;

   content.dsize = lendata;
   content.dptr = worddata;

   ret = gdbm_store(DB->dbf_worddata,key,content,GDBM_INSERT);

   if(ret)
      progerr("GDBM error writing word's data");

   return 0;
}



int DB_WriteWordHash_gdbm(char *word, long wordID, void *db)
{
   struct Handle_DBgdbm *DB = (struct Handle_DBgdbm *)db;

   datum key, content;
   int ret;

   key.dsize = strlen(word) + 1;
   key.dptr = (char *) word;

   content.dsize = sizeof(int);
   content.dptr = (char *) &wordID;

   ret = gdbm_store(DB->dbf_words,key,content,GDBM_INSERT);

   if(ret)
      progerr("GDBM error writing word");

   return 0;
}

int     DB_InitReadWords_gdbm(void *db)
{
    return 0;
}

int     DB_EndReadWords_gdbm(void *db)
{
    return 0;
}


int DB_ReadWordHash_gdbm(char *word, long *wordID, void *db)
{
   struct Handle_DBgdbm *DB = (struct Handle_DBgdbm *)db;

   datum key, content;

   key.dsize = strlen(word) + 1;
   key.dptr = (char *) word;

   content = gdbm_fetch(DB->dbf_words,key);

   if(!content.dptr)
   {
      *wordID = 0;
   }
   else
   {
      *wordID = (long) *(int *)content.dptr;
   }

   return 0;
}



int DB_ReadFirstWordInvertedIndex_gdbm(char *word, char **resultword, long *wordID, void *db)
{
    return 0;
}

int DB_ReadNextWordInvertedIndex_gdbm(char *word, char **resultword, long *wordID, void *db)
{
    return 0;
}


long DB_ReadWordData_gdbm(long wordID, unsigned char **worddata, int *lendata, void *db)
{
   struct Handle_DBgdbm *DB = (struct Handle_DBgdbm *)db;

   datum key, content;

   key.dsize = sizeof(long);
   key.dptr = (char *) &wordID;

   content = gdbm_fetch(DB->dbf_worddata,key);

   if(!content.dptr)
   {
      *lendata = 0;
      *worddata = NULL;
   }
   else
   {
      *lendata = content.dsize;
      *worddata = content.dptr;
   }

   return 0;
}

/*--------------------------------------------*/
/*--------------------------------------------*/
/*              FileList  Stuff               */
/*--------------------------------------------*/
/*--------------------------------------------*/

int DB_InitWriteFiles_gdbm(void *db)
{
   return 0;
}


int DB_EndWriteFiles_gdbm(void *db)
{
   return 0;
}

int DB_WriteFile_gdbm(int filenum, unsigned char *filedata,int sz_filedata, void *db)
{
   struct Handle_DBgdbm *DB = (struct Handle_DBgdbm *)db;

   datum key, content;
   int ret;

   key.dsize = sizeof(int);
   key.dptr = (char *) &filenum;

   content.dsize = sz_filedata;
   content.dptr = (char *) filedata;

   ret = gdbm_store(DB->dbf_docs,key,content,GDBM_INSERT);

   if(ret)
      progerr("GDBM error writing docs");

   return 0;
}

int DB_InitReadFiles_gdbm(void *db)
{
   return 0;
}

int DB_ReadFile_gdbm(int filenum, unsigned char **filedata,int *sz_filedata, void *db)
{
   struct Handle_DBgdbm *DB = (struct Handle_DBgdbm *)db;

   datum key, content;

   key.dsize = sizeof(int);
   key.dptr = (char *) &filenum;

   content = gdbm_fetch(DB->dbf_docs,key);

   if(!content.dptr)
   {
      *sz_filedata = 0;
      *filedata = NULL;
   }
   else
   {
      *sz_filedata = content.dsize;
      *filedata = content.dptr;
   }

   return 0;
}


int DB_EndReadFiles_gdbm(void *db)
{
   return 0;
}


/*--------------------------------------------*/
/*--------------------------------------------*/
/*              Sorted data Stuff             */
/*--------------------------------------------*/
/*--------------------------------------------*/





int     DB_InitWriteSortedIndex_gdbm(void *db)
{
   return 0;
}

int     DB_WriteSortedIndex_gdbm(int propID, unsigned char *data, int sz_data,void *db)
{
   struct Handle_DBgdbm *DB = (struct Handle_DBgdbm *)db;

   datum key, content;
   int ret;

   key.dsize = sizeof(int);
   key.dptr = (char *) &propID;

   content.dsize = sz_data;
   content.dptr = (char *) data;

   ret = gdbm_store(DB->dbf_sorted_indexes,key,content,GDBM_INSERT);

   if(ret)
      progerr("GDBM error writing sorted indexes");

   return 0;
}

int     DB_EndWriteSortedIndex_gdbm(void *db)
{
   return 0;
}

 
int     DB_InitReadSortedIndex_gdbm(void *db)
{
   return 0;
}

int     DB_ReadSortedIndex_gdbm(int propID, unsigned char **data, int *sz_data,void *db)
{
   struct Handle_DBgdbm *DB = (struct Handle_DBgdbm *)db;

   datum key, content;

   key.dsize = sizeof(int);
   key.dptr = (char *) &propID;

   content = gdbm_fetch(DB->dbf_sorted_indexes,key);

   if(!content.dptr)
   {
      *sz_data = 0;
      *data = NULL;
   }
   else
   {
      *sz_data = content.dsize;
      *data = content.dptr;
   }

   return 0;
}

int     DB_EndReadSortedIndex_gdbm(void *db)
{
   return 0;
}

