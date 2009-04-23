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
      
   SWINT_T wordcounter;         /* word counter - Used to asign WordID's */

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
SWINT_T configModule_DB_db (SWISH *sw, StringList *sl);

void   *DB_Create_db (SWISH *sw, char *dbname);
void   *DB_Open_db (SWISH *sw, char *dbname);
void    DB_Close_db(void *db);
void    DB_Remove_db(void *db);

SWINT_T     DB_InitWriteHeader_db(void *db);
SWINT_T     DB_EndWriteHeader_db(void *db);
SWINT_T     DB_WriteHeaderData_db(SWINT_T id, unsigned char *s, SWINT_T len, void *db);

SWINT_T     DB_InitReadHeader_db(void *db);
SWINT_T     DB_ReadHeaderData_db(SWINT_T *id, unsigned char **s, SWINT_T *len, void *db);
SWINT_T     DB_EndReadHeader_db(void *db);

SWINT_T     DB_InitWriteWords_db(void *db);
SWINT_T    DB_GetWordID_db(void *db);
SWINT_T     DB_WriteWord_db(char *word, SWINT_T wordID, void *db);
SWINT_T     DB_WriteWordHash_db(char *word, SWINT_T wordID, void *db);
SWINT_T    DB_WriteWordData_db(SWINT_T wordID, unsigned char *worddata, SWINT_T lendata, void *db);
SWINT_T     DB_EndWriteWords_db(void *db);

SWINT_T     DB_InitReadWords_db(void *db);
SWINT_T     DB_ReadWordHash_db(char *word, SWINT_T *wordID, void *db);
SWINT_T     DB_ReadFirstWordInvertedIndex_db(char *word, char **resultword, SWINT_T *wordID, void *db);
SWINT_T     DB_ReadNextWordInvertedIndex_db(char *word, char **resultword, SWINT_T *wordID, void *db);
SWINT_T    DB_ReadWordData_db(SWINT_T wordID, unsigned char **worddata, SWINT_T *lendata, void *db);
SWINT_T     DB_EndReadWords_db(void *db);


SWINT_T     DB_InitWriteFiles_db(void *db);
SWINT_T     DB_EndWriteFiles_db(void *db);
SWINT_T     DB_WriteFile_db(SWINT_T filenum, unsigned char *filedata,SWINT_T sz_filedata, void *db);
SWINT_T     DB_InitReadFiles_db(void *db);
SWINT_T     DB_ReadFile_db(SWINT_T filenum, unsigned char **filedata,SWINT_T *sz_filedata, void *db);
SWINT_T     DB_EndReadFiles_db(void *db);

SWINT_T     DB_InitWriteSortedIndex_db(void *db);
SWINT_T     DB_WriteSortedIndex_db(SWINT_T propID, unsigned char *data, SWINT_T sz_data,void *db);
SWINT_T     DB_EndWriteSortedIndex_db(void *db);
 
SWINT_T     DB_InitReadSortedIndex_db(void *db);
SWINT_T     DB_ReadSortedIndex_db(SWINT_T propID, unsigned char **data, SWINT_T *sz_data,void *db);
SWINT_T     DB_EndReadSortedIndex_db(void *db);


#ifdef PROPFILE
void    DB_WriteProperty_db( FileRec *fi, SWINT_T propID, char *buffer, SWINT_T datalen, void *db );
char  * DB_ReadProperty_db( FileRec *fi, SWINT_T propID, void *db );
void    DB_Reopen_PropertiesForRead_db(void *db);
#endif


#endif
