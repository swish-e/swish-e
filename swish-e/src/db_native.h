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
#define __HasSeenModule_DBNative    1


#ifdef USE_BTREE
#include "btree.h"
#include "array.h"
#include "worddata.h"
#include "fhash.h"
#define MAXCHARS 8            /* Only 8 are needed when BTREE is used */

#else

#define MAXCHARS 266            /* 255 for chars plus ten more for other data */

#endif

#define FILELISTPOS (MAXCHARS - 1)
#define FILEOFFSETPOS (MAXCHARS - 2)
#define HEADERPOS (MAXCHARS - 3)
#define WORDPOS (MAXCHARS - 4)
#define SORTEDINDEX (MAXCHARS - 5)
#define ENDWORDPOS (MAXCHARS - 6)

#ifdef USE_BTREE
#define TOTALWORDSPERFILEPOS (MAXCHARS - 7)
#define FILEHASHPOS (MAXCHARS - 8)
#endif



struct Handle_DBNative
{
       /* values used by the index proccess */
       /* points to the start of offsets to words in the file */
   sw_off_t offsetstart;

   SWISH *sw;  /* for reporting errors back */

#ifndef USE_BTREE
       /* points to the start of hashoffsets to words in the file */
   sw_off_t hashstart;
#endif
       /* File Offsets to words */
   sw_off_t offsets[MAXCHARS];

#ifndef USE_BTREE
   sw_off_t hashoffsets[VERYBIGHASHSIZE];

   sw_off_t lasthashval[VERYBIGHASHSIZE];
   int wordhash_counter;
#endif

   sw_off_t nextwordoffset;
   sw_off_t last_sortedindex;
   sw_off_t next_sortedindex;
   
   int worddata_counter;

#ifndef USE_BTREE
   sw_off_t *wordhashdata;

      /* Hash array to improve wordhashdata performance */
   struct numhash
   {
      int index;
      struct numhash *next;
   } *hash[BIGHASHSIZE];
   MEM_ZONE *hashzone;
#endif

   int num_words;

   DB_OPEN_MODE mode; 

   char *dbname;

#ifndef USE_BTREE
       /* ramdisk to store words */
   struct ramdisk *rd;
#endif

       /* Index FILE handle as returned from fopen */

       /* Pointers to words write/read functions */ 
   sw_off_t    (*w_tell)(FILE *);
   size_t  (*w_write)(const void *, size_t, size_t, FILE *);
   int     (*w_seek)(FILE *, sw_off_t, int);
   size_t  (*w_read)(void *, size_t, size_t, FILE *);
   int     (*w_close)(FILE *);
   int     (*w_putc)(int , FILE *);
   int     (*w_getc)(FILE *);

   FILE *fp;
   FILE *prop;

   int      tmp_index;      /* These indicates the file is opened as a temporary file */
   int      tmp_prop;
   char    *cur_index_file;
   char    *cur_prop_file;

   long     unique_ID;          /* just because it's called that doesn't mean it is! */

#ifdef USE_BTREE
   BTREE   *bt;
   FILE    *fp_btree;
   int      tmp_btree;
   char    *cur_btree_file;

   WORDDATA    *worddata;
   FILE    *fp_worddata;
   int      tmp_worddata;
   char    *cur_worddata_file;

   FHASH   *hashfile;
   FILE    *fp_hashfile;
   int      tmp_hashfile;
   char    *cur_hashfile_file;

   FILE    *fp_array;
   int      tmp_array;
   char    *cur_array_file;

   int      n_presorted_array;
   unsigned long *presorted_root_node;
   unsigned long *presorted_propid;
   ARRAY  **presorted_array;
   FILE    *fp_presorted;
   int      tmp_presorted;
   char    *cur_presorted_file;

   unsigned long cur_presorted_propid;
   ARRAY   *cur_presorted_array;

   ARRAY   *totwords_array;

   ARRAY   *props_array;
#endif
};

void initModule_DBNative (SWISH *);
void freeModule_DBNative (SWISH *);

void   *DB_Create_Native (SWISH *sw, char *dbname);
void   *DB_Open_Native (SWISH *sw, char *dbname, int mode);
void    DB_Close_Native(void *db);
void    DB_Remove_Native(void *db);



int     DB_InitWriteHeader_Native(void *db);
int     DB_EndWriteHeader_Native(void *db);
int     DB_WriteHeaderData_Native(int id, unsigned char *s, int len, void *db);

int     DB_InitReadHeader_Native(void *db);
int     DB_ReadHeaderData_Native(int *id, unsigned char **s, int *len, void *db);
int     DB_EndReadHeader_Native(void *db);



int     DB_InitWriteWords_Native(void *db);
sw_off_t    DB_GetWordID_Native(void *db);
int     DB_WriteWord_Native(char *word, sw_off_t wordID, void *db);

#ifdef USE_BTREE
int     DB_UpdateWordID_Native(char *word, sw_off_t new_wordID, void *db);
int     DB_DeleteWordData_Native(sw_off_t wordID, void *db);
#endif

int     DB_WriteWordHash_Native(char *word, sw_off_t wordID, void *db);
long    DB_WriteWordData_Native(sw_off_t wordID, unsigned char *worddata, int data_size, int saved_bytes, void *db);
int     DB_EndWriteWords_Native(void *db);

int     DB_InitReadWords_Native(void *db);
int     DB_ReadWordHash_Native(char *word, sw_off_t *wordID, void *db);
int     DB_ReadFirstWordInvertedIndex_Native(char *word, char **resultword, sw_off_t *wordID, void *db);
int     DB_ReadNextWordInvertedIndex_Native(char *word, char **resultword, sw_off_t *wordID, void *db);
long    DB_ReadWordData_Native(sw_off_t wordID, unsigned char **worddata, int *data_size, int *saved_bytes, void *db);
int     DB_EndReadWords_Native(void *db);



int     DB_WriteFileNum_Native(int filenum, unsigned char *filedata,int sz_filedata, void *db);
int     DB_ReadFileNum_Native(int *filenum, unsigned char *filedata,int sz_filedata, void *db);
int     DB_CheckFileNum_Native(int filenum, void *db);
int     DB_RemoveFileNum_Native(int filenum, void *db);

/** Pre-sorted array access **/

#ifdef USE_PRESORT_ARRAY
int     DB_InitWriteSortedIndex_Native(void *db , int n_props);
int     DB_WriteSortedIndex_Native(int propID, int *data, int num_elements,void *db);
#else
int     DB_InitWriteSortedIndex_Native(void *db );
int     DB_WriteSortedIndex_Native(int propID, unsigned char *data, int sz_data,void *db);
#endif
int     DB_EndWriteSortedIndex_Native(void *db);

int     DB_InitReadSortedIndex_Native(void *db);
int     DB_ReadSortedIndex_Native(int propID, unsigned char **data, int *sz_data,void *db);
/* Note that a macro is not provided for reading the data */
int     DB_ReadSortedData_Native(int *data,int index, int *value, void *db);
int     DB_EndReadSortedIndex_Native(void *db);




int     DB_InitWriteProperties_Native(void *db);
void    DB_WriteProperty_Native( IndexFILE *indexf, FileRec *fi, int propID, char *buffer, int buf_len, int uncompressed_len, void *db);
void    DB_WritePropPositions_Native(IndexFILE *indexf, FileRec *fi, void *db);
void    DB_ReadPropPositions_Native(IndexFILE *indexf, FileRec *fi, void *db);
char   *DB_ReadProperty_Native(IndexFILE *indexf, FileRec *fi, int propID, int *buf_len, int *uncompressed_len, void *db);
void    DB_Reopen_PropertiesForRead_Native(void *db);

#ifdef USE_BTREE
int        DB_InitWriteTotalWordsPerFile_Native(SWISH *sw, void *DB);
int    DB_WriteTotalWordsPerFile_Native(SWISH *sw, int idx, int wordcount, void *DB);
int    DB_EndWriteTotalWordsPerFile_Native(SWISH *sw, void *DB);
int        DB_InitReadTotalWordsPerFile_Native(SWISH *sw, void *DB);
int    DB_ReadTotalWordsPerFile_Native(SWISH *sw, int idx, int *wordcount, void *DB);
int    DB_EndReadTotalWordsPerFile_Native(SWISH *sw, void *DB);
#endif





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
