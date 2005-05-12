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
    
** Mon May  9 18:19:34 CDT 2005
** added GPL



**
**
** 2001-07  jose ruiz  initial coding
**
*/



#ifndef __HasSeenModule_DB_db
#define __HasSeenModule_DB_db	1


//#include <db.h>
#include "/usr/local/BerkeleyDB.3.2/include/db.h"

struct Handle_DB_db
{
      
   int wordcounter;         /* word counter - Used to asign WordID's */

   /* How about an array of *DB ? */    
   DB *dbf_header;          /* db file to store header */
   DB *dbf_words;           /* db file to store words */
   DB *dbf_worddata;        /* db file to store word's data */
   DB *dbf_invertedindex;   /* db file to store word's inverted index */
   DB *dbf_docs;            /* db file to store docs's data */
   DB *dbf_sorted_indexes;  /* db file to store sorted indexes */
   DB *dbf_properties;      /* db file to store properties */

   DBC *cursor_header;      /* cursor to read header data */
   DBC *cursor_inverted;    /* cursor to read the inverted word data */
};


void initModule_DB_db (SWISH *);
void freeModule_DB_db (SWISH *);
int configModule_DB_db (SWISH *sw, StringList *sl);

void   *DB_Create_db (SWISH *sw, char *dbname);
void   *DB_Open_db (SWISH *sw, char *dbname);
void    DB_Close_db(void *db);
void    DB_Remove_db(void *db);

int     DB_InitWriteHeader_db(void *db);
int     DB_EndWriteHeader_db(void *db);
int     DB_WriteHeaderData_db(int id, unsigned char *s, int len, void *db);

int     DB_InitReadHeader_db(void *db);
int     DB_ReadHeaderData_db(int *id, unsigned char **s, int *len, void *db);
int     DB_EndReadHeader_db(void *db);

int     DB_InitWriteWords_db(void *db);
long    DB_GetWordID_db(void *db);
int     DB_WriteWord_db(char *word, long wordID, void *db);
int     DB_WriteWordHash_db(char *word, long wordID, void *db);
long    DB_WriteWordData_db(long wordID, unsigned char *worddata, int lendata, void *db);
int     DB_EndWriteWords_db(void *db);

int     DB_InitReadWords_db(void *db);
int     DB_ReadWordHash_db(char *word, long *wordID, void *db);
int     DB_ReadFirstWordInvertedIndex_db(char *word, char **resultword, long *wordID, void *db);
int     DB_ReadNextWordInvertedIndex_db(char *word, char **resultword, long *wordID, void *db);
long    DB_ReadWordData_db(long wordID, unsigned char **worddata, int *lendata, void *db);
int     DB_EndReadWords_db(void *db);


int     DB_InitWriteFiles_db(void *db);
int     DB_EndWriteFiles_db(void *db);
int     DB_WriteFile_db(int filenum, unsigned char *filedata,int sz_filedata, void *db);
int     DB_InitReadFiles_db(void *db);
int     DB_ReadFile_db(int filenum, unsigned char **filedata,int *sz_filedata, void *db);
int     DB_EndReadFiles_db(void *db);

int     DB_InitWriteSortedIndex_db(void *db);
int     DB_WriteSortedIndex_db(int propID, unsigned char *data, int sz_data,void *db);
int     DB_EndWriteSortedIndex_db(void *db);
 
int     DB_InitReadSortedIndex_db(void *db);
int     DB_ReadSortedIndex_db(int propID, unsigned char **data, int *sz_data,void *db);
int     DB_EndReadSortedIndex_db(void *db);


#ifdef PROPFILE
void    DB_WriteProperty_db( FileRec *fi, int propID, char *buffer, int datalen, void *db );
char  * DB_ReadProperty_db( FileRec *fi, int propID, void *db );
void    DB_Reopen_PropertiesForRead_db(void *db);
#endif


#endif
