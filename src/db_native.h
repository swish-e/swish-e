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
** 2001-01  jose   initial coding
**
*/



#ifndef __HasSeenModule_DBNative
#define __HasSeenModule_DBNative	1


#define MAXCHARS 266            /* 255 for chars plus ten more for other data */

#define FILELISTPOS (MAXCHARS - 1)
#define FILEOFFSETPOS (MAXCHARS - 2)
#define HEADERPOS (MAXCHARS - 3)
#define WORDPOS (MAXCHARS - 4)
#define SORTEDINDEX (MAXCHARS - 5)


struct Handle_DBNative
{
       /* values used by the index proccess */
       /* points to the start of offsets to words in the file */
   int offsetstart;
       /* points to the start of hashoffsets to words in the file */
   int hashstart;
       /* File Offsets to words */
   long offsets[MAXCHARS];
   long hashoffsets[SEARCHHASHSIZE];
   long   *fileoffsetarray;
   int     fileoffsetarray_maxsize;

   int lasthashval[SEARCHHASHSIZE];
   long nextwordoffset;
   long lastsortedindex;
   
   int worddata_counter;
   long *worddata_wordID;
   long *worddata_offset;

   int num_words;
   int num_docs;

   int mode;  /* 1 - Create  0 - Open */

   char *dbname;
       /* Index FILE handle as returned from fopen */
   FILE *fp;
   FILE *prop;
};


void initModule_DBNative (SWISH *);
void freeModule_DBNative (SWISH *);
int configModule_DBNative (SWISH *sw, StringList *sl);

void   *DB_Create_Native (char *dbname);
void   *DB_Open_Native (char *dbname);
void    DB_Close_Native(void *db);
void    DB_Remove_Native(void *db);



int     DB_InitWriteHeader_Native(void *db);
int     DB_EndWriteHeader_Native(void *db);
int     DB_WriteHeaderData_Native(int id, unsigned char *s, int len, void *db);

int     DB_InitReadHeader_Native(void *db);
int     DB_ReadHeaderData_Native(int *id, unsigned char **s, int *len, void *db);
int     DB_EndReadHeader_Native(void *db);



int     DB_InitWriteWords_Native(void *db);
long    DB_GetWordID_Native(void *db);
int     DB_WriteWord_Native(char *word, long wordID, void *db);
int     DB_WriteWordHash_Native(char *word, long wordID, void *db);
long    DB_WriteWordData_Native(long wordID, unsigned char *worddata, int lendata, void *db);
int     DB_EndWriteWords_Native(void *db);

int     DB_InitReadWords_Native(void *db);
int     DB_ReadWordHash_Native(char *word, long *wordID, void *db);
int     DB_ReadFirstWordInvertedIndex_Native(char *word, char **resultword, long *wordID, void *db);
int     DB_ReadNextWordInvertedIndex_Native(char *word, char **resultword, long *wordID, void *db);
long    DB_ReadWordData_Native(long wordID, unsigned char **worddata, int *lendata, void *db);
int     DB_EndReadWords_Native(void *db);



int     DB_InitWriteFiles_Native(void *db);
int     DB_WriteFile_Native(int filenum, unsigned char *filedata,int sz_filedata, void *db);
int     DB_EndWriteFiles_Native(void *db);

int     DB_InitReadFiles_Native(void *db);
int     DB_ReadFile_Native(int filenum, unsigned char **filedata,int *sz_filedata, void *db);
int     DB_EndReadFiles_Native(void *db);



int     DB_InitWriteSortedIndex_Native(void *db);
int     DB_WriteSortedIndex_Native(int propID, unsigned char *data, int sz_data,void *db);
int     DB_EndWriteSortedIndex_Native(void *db);
 
int     DB_InitReadSortedIndex_Native(void *db);
int     DB_ReadSortedIndex_Native(int propID, unsigned char **data, int *sz_data,void *db);
int     DB_EndReadSortedIndex_Native(void *db);

/* 04/00 Jose Ruiz
** Functions to read/write longs from a file
*/
void    printlong(FILE *, unsigned long);
unsigned long    readlong(FILE *);

#endif
