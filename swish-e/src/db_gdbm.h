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



#ifndef __HasSeenModule_DBgdbm
#define __HasSeenModule_DBgdbm	1


#include <gdbm.h>

struct Handle_DBgdbm
{
      /* word counter - Used to asign WordID's */
   int wordcounter;
      /* Flag that shows if there is a firstkey executed or not when reading */
      /* header sequentially */
   datum lastkey_header;
      /* gdbm file to store header */
   GDBM_FILE dbf_header;
      /* gdbm file to store words */
   GDBM_FILE dbf_words;
      /* gdbm file to store word's data */
   GDBM_FILE dbf_worddata;
      /* gdbm file to store word's inverted index */
   GDBM_FILE dbf_invertedindex;
      /* gdbm file to store docs's data */
   GDBM_FILE dbf_docs;
      /* gdbm file to store sorted indexes */
   GDBM_FILE dbf_sorted_indexes;
};


void initModule_DBgdbm (SWISH *);
void freeModule_DBgdbm (SWISH *);
int configModule_DBgdbm (SWISH *sw, StringList *sl);

void   *DB_Create_gdbm (char *dbname);
void   *DB_Open_gdbm (char *dbname);
void    DB_Close_gdbm(void *db);
void    DB_Remove_gdbm(void *db);

int     DB_InitWriteHeader_gdbm(void *db);
int     DB_EndWriteHeader_gdbm(void *db);
int     DB_WriteHeaderData_gdbm(int id, char *s, int len, void *db);

int     DB_InitReadHeader_gdbm(void *db);
int     DB_ReadHeaderData_gdbm(int *id, char **s, int *len, void *db);
int     DB_EndReadHeader_gdbm(void *db);

int     DB_InitWriteWords_gdbm(void *db);
long    DB_GetWordID_gdbm(void *db);
int     DB_WriteWord_gdbm(char *word, long wordID, void *db);
int     DB_WriteWordHash_gdbm(char *word, long wordID, void *db);
long    DB_WriteWordData_gdbm(long wordID, char *worddata, int lendata, void *db);
int     DB_EndWriteWords_gdbm(void *db);

int     DB_InitReadWords_gdbm(void *db);
int     DB_ReadWordHash_gdbm(char *word, long *wordID, void *db);
int     DB_ReadFirstWordInvertedIndex_gdbm(char *word, char **resultword, long *wordID, void *db);
int     DB_ReadNextWordInvertedIndex_gdbm(char *word, char **resultword, long *wordID, void *db);
long    DB_ReadWordData_gdbm(long wordID, char **worddata, int *lendata, void *db);
int     DB_EndReadWords_gdbm(void *db);


int     DB_InitWriteFiles_gdbm(void *db);
int     DB_EndWriteFiles_gdbm(void *db);
int     DB_WriteFile_gdbm(int filenum, char *filedata,int sz_filedata, void *db);
int     DB_InitReadFiles_gdbm(void *db);
int     DB_ReadFile_gdbm(int filenum, char **filedata,int *sz_filedata, void *db);
int     DB_EndReadFiles_gdbm(void *db);

int     DB_InitWriteSortedIndex_gdbm(void *db);
int     DB_WriteSortedIndex_gdbm(int propID, char *data, int sz_data,void *db);
int     DB_EndWriteSortedIndex_gdbm(void *db);
 
int     DB_InitReadSortedIndex_gdbm(void *db);
int     DB_ReadSortedIndex_gdbm(int propID, char **data, int *sz_data,void *db);
int     DB_EndReadSortedIndex_gdbm(void *db);


#endif
