/*
**
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
**
** 2001-01  jose   initial coding
**
*/



#ifndef __HasSeenModule_DBNative
#define __HasSeenModule_DBNative    1


#include <db.h>   /* Berkeley DB include file */


struct Handle_DBNative
{
   SWISH *sw;  /* for reporting errors back */

   sw_off_t last_sortedindex;
   sw_off_t next_sortedindex;
   
   SW_INT32 worddata_counter;

   int num_words;

   DB_OPEN_MODE mode; 

   char *dbname;

       /* Index FILE handle as returned from fopen */

       /* Pointers to words write/read functions */ 
   sw_off_t    (*w_tell)(FILE *);
   size_t  (*w_write)(const void *, size_t, size_t, FILE *);
   int     (*w_seek)(FILE *, sw_off_t, int);        // no rw64
   size_t  (*w_read)(void *, size_t, size_t, FILE *);
   int     (*w_close)(FILE *);                      // no rw64
   int     (*w_putc)(int , FILE *);                 // no rw64
   int     (*w_getc)(FILE *);                       // no rw64

   FILE *fp_prop;

   int      tmp_index;      /* These indicates the file is opened as a temporary file */
   int      tmp_prop;
   char    *cur_prop_file;

   long     unique_ID;          /* just because it's called that doesn't mean it is! */

   DB      *db_btree;    /* DB BTREE handle */
   DBC     *dbc_btree;    /* DB BTREE handle */
   int      tmp_btree;
   char    *cur_btree_file;

   DB      *db_worddata;
   int      tmp_worddata;
   char    *cur_worddata_file;

   DB      *db_hashfile;
   int      tmp_hashfile;
   char    *cur_hashfile_file;

   FILE    *fp_presorted;
   int      tmp_presorted;
   char    *cur_presorted_file;

   unsigned long cur_presorted_propid;

   FILE    *fp_totwords;
   int      tmp_totwords;
   char    *cur_totwords_file;

   FILE    *fp_propindex;
   int      tmp_propindex;
   char    *cur_propindex_file;

   FILE    *fp_header;
   int      tmp_header;
   char    *cur_header_file;

};

void initModule_DBNative (SWISH *);
void freeModule_DBNative (SWISH *);

void   *_DB_Create (SWISH *sw, char *dbname);
void   *_DB_Open (SWISH *sw, char *dbname, int mode);
void    _DB_Close (void *db);
void    _DB_Remove (void *db);


int     _DB_InitWriteHeader (void *db);
int     _DB_EndWriteHeader (void *db);
int     _DB_WriteHeaderData (int id, unsigned char *s, int len, void *db);

int     _DB_InitReadHeader (void *db);
int     _DB_ReadHeaderData (int *id, unsigned char **s, int *len, void *db);
int     _DB_EndReadHeader (void *db);
int     _DB_InitWriteWords (void *db);
SW_INT32    _DB_GetWordID (void *db);
int     _DB_WriteWord (char *word, SW_INT32 wordID, void *db);
long    _DB_WriteWordData (SW_INT32 wordID, unsigned char *worddata, int data_size, int saved_bytes, void *db);
int     _DB_EndWriteWords (void *db);
int     _DB_InitReadWords (void *db);
int     _DB_ReadWord (char *word, DB_WORDID **wordID, void *db);
int     _DB_ReadFirstWordInvertedIndex (char *word, char **resultword, DB_WORDID **wordID, void *db);
int     _DB_ReadNextWordInvertedIndex (char *word, char **resultword, DB_WORDID **wordID, void *db);
long    _DB_ReadWordData (SW_INT32 wordID, unsigned char **worddata, int *data_size, int *saved_bytes, void *db);
int     _DB_EndReadWords (void *db);



int     _DB_WriteFileNum (int filenum, unsigned char *filedata,int sz_filedata, void *db);
int     _DB_ReadFileNum ( unsigned char *filedata, void *db);
int     _DB_CheckFileNum (int filenum, void *db);
int     _DB_RemoveFileNum (int filenum, void *db);

/** Pre-sorted array access **/

#ifdef USE_PRESORT_ARRAY
int     _DB_InitWriteSortedIndex (void *db , int n_props);
int     _DB_WriteSortedIndex (int propID, int *data, int num_elements,void *db);
#else
int     _DB_InitWriteSortedIndex (void *db );
int     _DB_WriteSortedIndex (int propID, unsigned char *data, int sz_data,void *db);
#endif
int     _DB_EndWriteSortedIndex (void *db);

int     _DB_InitReadSortedIndex (void *db);
int     _DB_ReadSortedIndex (int propID, unsigned char **data, int *sz_data,void *db);
/* Note that a macro is not provided for reading the data */
int     _DB_ReadSortedData (int *data,int index, int *value, void *db);
int     _DB_EndReadSortedIndex (void *db);




int     _DB_InitWriteProperties (void *db);
void    _DB_WriteProperty ( IndexFILE *indexf, FileRec *fi, int propID, char *buffer, int buf_len, int uncompressed_len, void *db);
void    _DB_WritePropPositions (IndexFILE *indexf, FileRec *fi, void *db);
void    _DB_ReadPropPositions (IndexFILE *indexf, FileRec *fi, void *db);
char   *_DB_ReadProperty (IndexFILE *indexf, FileRec *fi, int propID, int *buf_len, int *uncompressed_len, void *db);
void    _DB_Reopen_PropertiesForRead (void *db);

int     _DB_InitWriteTotalWordsPerFile (SWISH *sw, void *DB);
int     _DB_WriteTotalWordsPerFile (SWISH *sw, int idx, int wordcount, void *DB);
int     _DB_EndWriteTotalWordsPerFile (SWISH *sw, void *DB);
int     _DB_InitReadTotalWordsPerFile (SWISH *sw, void *DB);
int     _DB_ReadTotalWordsPerFile (SWISH *sw, int idx, int *wordcount, void *DB);
int     _DB_EndReadTotalWordsPerFile (SWISH *sw, void *DB);

/* 04/00 Jose Ruiz
** Functions to read/write longs from a file
*/
void    printlong(FILE * fp, unsigned long num, size_t (*f_write)(const void *, size_t, size_t, FILE *));
unsigned long    readlong(FILE * fp, size_t (*f_read)(void *, size_t, size_t, FILE *));

/* 2003/10 Jose Ruiz
** Functions to read/write file offsets
*/
void    printfileoffset(FILE * fp, sw_off_t num, size_t (*f_write)(const void *, size_t, size_t, FILE *));
sw_off_t    readfileoffset(FILE * fp, size_t (*f_read)(void *, size_t, size_t, FILE *));


#endif
