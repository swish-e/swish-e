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
** 2001-01  jose   initial coding
**
*/

#ifndef __HasSeenModule_DB
#define __HasSeenModule_DB    1

/* Possible Open File modes */
typedef enum {
    DB_CREATE,
	DB_READ,
	DB_READWRITE
}
DB_OPEN_MODE;

void initModule_DB (SWISH *);
void freeModule_DB (SWISH *);

void    write_header(SWISH *ws, SWINT_T merged_flag );
void    update_header(SWISH *, void *, SWINT_T, SWINT_T );
void    write_index(SWISH *, IndexFILE *);
void    write_word(SWISH *, ENTRY *, IndexFILE *);
#ifdef USE_BTREE
void    update_wordID(SWISH *, ENTRY *, IndexFILE *);
void    delete_worddata(SWISH *, sw_off_t, IndexFILE *);
#endif
void    build_worddata(SWISH *, ENTRY *);
void    write_worddata(SWISH *, ENTRY *, IndexFILE *);
sw_off_t    read_worddata(SWISH * sw, ENTRY * ep, IndexFILE * indexf, unsigned char **bufer, SWINT_T *sz_buffer);
void    add_worddata(SWISH *sw, unsigned char *buffer, SWINT_T sz_buffer);
void    write_pathlookuptable_to_header(SWISH *, SWINT_T id, INDEXDATAHEADER *header, void *DB);
void    write_MetaNames (SWISH *, SWINT_T id, INDEXDATAHEADER *header, void *DB);
SWINT_T     write_integer_table_to_header(SWISH *, SWINT_T id, SWINT_T table[], SWINT_T table_size, void *DB);

void    read_header(SWISH *, INDEXDATAHEADER *header, void *DB);

void    parse_MetaNames_from_buffer(INDEXDATAHEADER *header, char *buffer);
void    parse_pathlookuptable_from_buffer(INDEXDATAHEADER *header, char *buffer);
void    parse_integer_table_from_buffer(SWINT_T table[], SWINT_T table_size, char *buffer);
char    *getfilewords(SWISH *sw, SWINT_T, IndexFILE *);
void    setTotalWordsPerFile(IndexFILE *,SWINT_T ,SWINT_T );

SWINT_T     getTotalWordsInFile(IndexFILE *indexf, SWINT_T filenum);


/* Common DB api */
void   *DB_Create (SWISH *sw, char *dbname);
void   *DB_Open (SWISH *sw, char *dbname, SWINT_T mode);
void    DB_Close(SWISH *sw, void *DB);
void    DB_Remove(SWISH *sw, void *DB);

SWINT_T     DB_InitWriteHeader(SWISH *sw, void *DB);
SWINT_T     DB_EndWriteHeader(SWISH *sw, void *DB);
SWINT_T     DB_WriteHeaderData(SWISH *sw, SWINT_T id, unsigned char *s, SWINT_T len, void *DB);

SWINT_T     DB_InitReadHeader(SWISH *sw, void *DB);
SWINT_T     DB_ReadHeaderData(SWISH *sw, SWINT_T *id, unsigned char **s, SWINT_T *len, void *DB);
SWINT_T     DB_EndReadHeader(SWISH *sw, void *DB);

SWINT_T     DB_InitWriteWords(SWISH *sw, void *DB);
sw_off_t    DB_GetWordID(SWISH *sw, void *DB);
SWINT_T     DB_WriteWord(SWISH *sw, char *word, sw_off_t wordID, void *DB);
#ifdef USE_BTREE
SWINT_T     DB_UpdateWordID(SWISH *sw, char *word, sw_off_t wordID, void *DB);
SWINT_T     DB_DeleteWordData(SWISH *sw,sw_off_t wordID, void *DB);
#endif
SWINT_T     DB_WriteWordHash(SWISH *sw, char *word, sw_off_t wordID, void *DB);
SWINT_T    DB_WriteWordData(SWISH *sw, sw_off_t wordID, unsigned char *worddata, SWINT_T data_size, SWINT_T saved_bytes, void *DB);
SWINT_T     DB_EndWriteWords(SWISH *sw, void *DB);

SWINT_T     DB_InitReadWords(SWISH *sw, void *DB);
SWINT_T     DB_ReadWordHash(SWISH *sw, char *word, sw_off_t *wordID, void *DB);
SWINT_T     DB_ReadFirstWordInvertedIndex(SWISH *sw, char *word, char **resultword, sw_off_t *wordID, void *DB);
SWINT_T     DB_ReadNextWordInvertedIndex(SWISH *sw, char *word, char **resultword, sw_off_t *wordID, void *DB);
SWINT_T    DB_ReadWordData(SWISH *sw, sw_off_t wordID, unsigned char **worddata, SWINT_T *data_size, SWINT_T *saved_bytes, void *DB);
SWINT_T     DB_EndReadWords(SWISH *sw, void *DB);


#ifdef USE_PRESORT_ARRAY
SWINT_T     DB_InitWriteSortedIndex(SWISH *sw, void *DB, SWINT_T n_props );
SWINT_T     DB_WriteSortedIndex(SWISH *sw, SWINT_T propID, SWINT_T *data, SWINT_T sz_data,void *DB);
#else
SWINT_T     DB_InitWriteSortedIndex(SWISH *sw, void *DB );
SWINT_T     DB_WriteSortedIndex(SWISH *sw, SWINT_T propID, unsigned char *data, SWINT_T sz_data,void *DB);
#endif

SWINT_T     DB_EndWriteSortedIndex(SWISH *sw, void *DB);
 
SWINT_T     DB_InitReadSortedIndex(SWISH *sw, void *DB);
SWINT_T     DB_ReadSortedIndex(SWISH *sw, SWINT_T propID, unsigned char **data, SWINT_T *sz_data,void *DB);

/* this is defined in db_native.h now 
SWINT_T     DB_ReadSortedData(SWISH *sw, SWINT_T *data,SWINT_T index, SWINT_T *value, void *DB);
*/
#ifdef USE_PRESORT_ARRAY
#define  DB_ReadSortedData(data, index) (ARRAY_Get((ARRAY *)data,index))
#else
#define  DB_ReadSortedData(data, index) (data[index])
#endif




SWINT_T     DB_EndReadSortedIndex(SWISH *sw, void *DB);


SWINT_T     DB_WriteFileNum(SWISH *sw, SWINT_T filenum, unsigned char *filedata,SWINT_T sz_filedata, void *DB);
SWINT_T     DB_ReadFileNum(SWISH *sw, unsigned char *filedata, void *DB);
SWINT_T     DB_CheckFileNum(SWISH *sw, SWINT_T filenum, void *DB);
SWINT_T     DB_RemoveFileNum(SWISH *sw, SWINT_T filenum, void *DB);

SWINT_T     DB_InitWriteProperties(SWISH *sw, void *DB);
void    DB_WriteProperty( SWISH *sw, IndexFILE *indexf, FileRec *fi, SWINT_T propID, char *buffer, SWINT_T buf_len, SWINT_T uncompressed_len, void *db);
void    DB_WritePropPositions(SWISH *sw, IndexFILE *indexf, FileRec *fi, void *db);
void    DB_ReadPropPositions(SWISH *sw, IndexFILE *indexf, FileRec *fi, void *db);
char   *DB_ReadProperty(SWISH *sw, IndexFILE *indexf, FileRec *fi, SWINT_T propID, SWINT_T *buf_len, SWINT_T *uncompressed_len, void *db);
void    DB_Reopen_PropertiesForRead(SWISH *sw, void *DB);

#ifdef USE_BTREE
SWINT_T    DB_WriteTotalWordsPerFile(SWISH *sw, SWINT_T idx, SWINT_T wordcount, void *DB);
SWINT_T    DB_ReadTotalWordsPerFile(SWISH *sw, SWINT_T idx, SWINT_T *wordcount, void *DB);
#endif


struct MOD_DB
{
    char *DB_name; /* short name for data source */

    void * (*DB_Create) (SWISH *sw, char *dbname);
    void * (*DB_Open) (SWISH *sw, char *dbname, SWINT_T mode);
    void   (*DB_Close) (void *DB);
    void   (*DB_Remove) (void *DB);
    
    SWINT_T    (*DB_InitWriteHeader) (void *DB);
    SWINT_T    (*DB_WriteHeaderData) (SWINT_T id, unsigned char *s, SWINT_T len, void *DB);
    SWINT_T    (*DB_EndWriteHeader) (void *DB);
    
    SWINT_T    (*DB_InitReadHeader) (void *DB);
    SWINT_T    (*DB_ReadHeaderData) (SWINT_T *id, unsigned char **s, SWINT_T *len, void *DB);
    SWINT_T    (*DB_EndReadHeader) (void *DB);
    
    SWINT_T    (*DB_InitWriteWords) (void *DB);
    sw_off_t   (*DB_GetWordID) (void *DB);
    SWINT_T    (*DB_WriteWord) (char *word, sw_off_t wordID, void *DB);
#ifdef USE_BTREE
    SWINT_T    (*DB_UpdateWordID)(char *word, sw_off_t new_wordID, void *DB);
    SWINT_T    (*DB_DeleteWordData)(sw_off_t wordID, void *DB);
#endif
    SWINT_T    (*DB_WriteWordHash) (char *word, sw_off_t wordID, void *DB);
    SWINT_T   (*DB_WriteWordData) (sw_off_t wordID, unsigned char *worddata, SWINT_T data_size, SWINT_T saved_bytes, void *DB);
    SWINT_T    (*DB_EndWriteWords) (void *DB);
    
    SWINT_T    (*DB_InitReadWords) (void *DB);
    SWINT_T    (*DB_ReadWordHash) (char *word, sw_off_t *wordID, void *DB);
    SWINT_T    (*DB_ReadFirstWordInvertedIndex) (char *word, char **resultword, sw_off_t *wordID, void *DB);
    SWINT_T    (*DB_ReadNextWordInvertedIndex) (char *word, char **resultword, sw_off_t *wordID, void *DB);
    SWINT_T   (*DB_ReadWordData) (sw_off_t wordID, unsigned char **worddata, SWINT_T *data_size, SWINT_T *saved_bytes, void *DB);
    SWINT_T    (*DB_EndReadWords) (void *DB);
    
    
    SWINT_T    (*DB_WriteFileNum) (SWINT_T filenum, unsigned char *filedata,SWINT_T sz_filedata, void *DB);
    SWINT_T    (*DB_ReadFileNum) ( unsigned char *filedata, void *DB);
    SWINT_T    (*DB_CheckFileNum) (SWINT_T filenum, void *DB);
    SWINT_T    (*DB_RemoveFileNum) (SWINT_T filenum, void *DB);

#ifdef USE_PRESORT_ARRAY
    SWINT_T    (*DB_InitWriteSortedIndex) (void *DB, SWINT_T n_props);
    SWINT_T    (*DB_WriteSortedIndex) (SWINT_T propID, SWINT_T *data, SWINT_T sz_data,void *DB);
#else
    SWINT_T    (*DB_InitWriteSortedIndex) (void *DB);
    SWINT_T    (*DB_WriteSortedIndex) (SWINT_T propID, unsigned char *data, SWINT_T sz_data,void *DB);
#endif
    SWINT_T    (*DB_EndWriteSortedIndex) (void *DB);
     
    SWINT_T    (*DB_InitReadSortedIndex) (void *DB);
    SWINT_T    (*DB_ReadSortedIndex) (SWINT_T propID, unsigned char **data, SWINT_T *sz_data,void *DB);
    SWINT_T    (*DB_ReadSortedData) (SWINT_T *data,SWINT_T index, SWINT_T *value, void *DB);
    SWINT_T    (*DB_EndReadSortedIndex) (void *DB);

    SWINT_T    (*DB_InitWriteProperties) (void *DB);
    void   (*DB_WriteProperty)( IndexFILE *indexf, FileRec *fi, SWINT_T propID, char *buffer, SWINT_T buf_len, SWINT_T uncompressed_len, void *db);
    void   (*DB_WritePropPositions)(IndexFILE *indexf, FileRec *fi, void *db);
    void   (*DB_ReadPropPositions)(IndexFILE *indexf, FileRec *fi, void *db);
    char  *(*DB_ReadProperty)(IndexFILE *indexf, FileRec *fi, SWINT_T propID, SWINT_T *buf_len, SWINT_T *uncompressed_len, void *db);
    void   (*DB_Reopen_PropertiesForRead)(void *DB);
#ifdef USE_BTREE
    SWINT_T    (*DB_WriteTotalWordsPerFile)(SWISH *sw, SWINT_T idx, SWINT_T wordcount, void *DB);
    SWINT_T    (*DB_ReadTotalWordsPerFile)(SWISH *sw, SWINT_T idx, SWINT_T *wordcount, void *DB);
#endif
};


#endif
