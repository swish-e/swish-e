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

#ifndef __HasSeenModule_DB
#define __HasSeenModule_DB    1

/* Possible Open File modes */
typedef enum {
    DB_CREATE,
	DB_READ,
	DB_READWRITE,
}
DB_OPEN_MODE;

void initModule_DB (SWISH *);
void freeModule_DB (SWISH *);
int configModule_DB (SWISH *sw, StringList *sl);

void    write_header(SWISH *, INDEXDATAHEADER *, void *, char *, int, int, int);
void    update_header(SWISH *, void *, int, int );
void    write_index(SWISH *, IndexFILE *);
void    write_word(SWISH *, ENTRY *, IndexFILE *);
void    write_worddata(SWISH *, ENTRY *, IndexFILE *);
int     write_words_to_header(SWISH *, int header_ID, struct swline **hash, void *DB);
void    write_pathlookuptable_to_header(SWISH *, int id, INDEXDATAHEADER *header, void *DB);
void    write_MetaNames (SWISH *, int id, INDEXDATAHEADER *header, void *DB);
int     write_integer_table_to_header(SWISH *, int id, int table[], int table_size, void *DB);

void    read_header(SWISH *, INDEXDATAHEADER *header, void *DB);

void    parse_MetaNames_from_buffer(INDEXDATAHEADER *header, char *buffer);
void    parse_stopwords_from_buffer(INDEXDATAHEADER *header, char *buffer);
void    parse_buzzwords_from_buffer(INDEXDATAHEADER *header, char *buffer);
void    parse_pathlookuptable_from_buffer(INDEXDATAHEADER *header, char *buffer);
void    parse_integer_table_from_buffer(int table[], int table_size, char *buffer);
char *getfilewords(SWISH *sw, int, IndexFILE *);



/* Common DB api */
void   *DB_Create (SWISH *sw, char *dbname);
void   *DB_Open (SWISH *sw, char *dbname, int mode);
void    DB_Close(SWISH *sw, void *DB);
void    DB_Remove(SWISH *sw, void *DB);

int     DB_InitWriteHeader(SWISH *sw, void *DB);
int     DB_EndWriteHeader(SWISH *sw, void *DB);
int     DB_WriteHeaderData(SWISH *sw, int id, unsigned char *s, int len, void *DB);

int     DB_InitReadHeader(SWISH *sw, void *DB);
int     DB_ReadHeaderData(SWISH *sw, int *id, unsigned char **s, int *len, void *DB);
int     DB_EndReadHeader(SWISH *sw, void *DB);

int     DB_InitWriteWords(SWISH *sw, void *DB);
long    DB_GetWordID(SWISH *sw, void *DB);
int     DB_WriteWord(SWISH *sw, char *word, long wordID, void *DB);
int     DB_WriteWordHash(SWISH *sw, char *word, long wordID, void *DB);
long    DB_WriteWordData(SWISH *sw, long wordID, unsigned char *worddata, int lendata, void *DB);
int     DB_EndWriteWords(SWISH *sw, void *DB);

int     DB_InitReadWords(SWISH *sw, void *DB);
int     DB_ReadWordHash(SWISH *sw, char *word, long *wordID, void *DB);
int     DB_ReadFirstWordInvertedIndex(SWISH *sw, char *word, char **resultword, long *wordID, void *DB);
int     DB_ReadNextWordInvertedIndex(SWISH *sw, char *word, char **resultword, long *wordID, void *DB);
long    DB_ReadWordData(SWISH *sw, long wordID, unsigned char **worddata, int *lendata, void *DB);
int     DB_EndReadWords(SWISH *sw, void *DB);


#ifdef USE_BTREE
int     DB_InitWriteSortedIndex(SWISH *sw, void *DB, int n_props );
#else
int     DB_InitWriteSortedIndex(SWISH *sw, void *DB );
#endif
int     DB_WriteSortedIndex(SWISH *sw, int propID, unsigned char *data, int sz_data,void *DB);
int     DB_EndWriteSortedIndex(SWISH *sw, void *DB);
 
int     DB_InitReadSortedIndex(SWISH *sw, void *DB);
int     DB_ReadSortedIndex(SWISH *sw, int propID, unsigned char **data, int *sz_data,void *DB);
int     DB_ReadSortedData(SWISH *sw, int *data,int index, int *value, void *DB);
int     DB_EndReadSortedIndex(SWISH *sw, void *DB);


int     DB_InitWriteFiles(SWISH *sw, void *DB);
int     DB_WriteFile(SWISH *sw, int filenum, unsigned char *filedata,int sz_filedata, void *DB);
int     DB_EndWriteFiles(SWISH *sw, void *DB);

int     DB_InitReadFiles(SWISH *sw, void *DB);
int     DB_ReadFile(SWISH *sw, int filenum, unsigned char **filedata,int *sz_filedata, void *DB);
int     DB_EndReadFiles(SWISH *sw, void *DB);

void    DB_WriteProperty( SWISH *sw, IndexFILE *indexf, FileRec *fi, int propID, char *buffer, int buf_len, int uncompressed_len, void *db);
void    DB_WritePropPositions(SWISH *sw, IndexFILE *indexf, FileRec *fi, void *db);
void    DB_ReadPropPositions(SWISH *sw, IndexFILE *indexf, FileRec *fi, void *db);
char   *DB_ReadProperty(SWISH *sw, IndexFILE *indexf, FileRec *fi, int propID, int *buf_len, int *uncompressed_len, void *db);
void    DB_Reopen_PropertiesForRead(SWISH *sw, void *DB);

#ifdef USE_BTREE
int	   DB_InitWriteTotalWordsPerFileArray(SWISH *sw, void *DB);
int    DB_WriteTotalWordsPerFileArray(SWISH *sw, int *totalWordsPerFile, int totalfiles, void *DB);
int    DB_EndWriteTotalWordsPerFileArray(SWISH *sw, void *DB);
int	   DB_InitReadTotalWordsPerFileArray(SWISH *sw, void *DB);
int    DB_ReadTotalWordsPerFileArray(SWISH *sw, int **totalWordsPerFile, void *DB);
int    DB_EndReadTotalWordsPerFileArray(SWISH *sw, void *DB);
#endif

int    DB_ReadTotalWordsPerFile(SWISH *sw, int *data,int index, int *value, void *DB);


struct MOD_DB
{
    char *DB_name; /* short name for data source */

    void * (*DB_Create) (char *dbname);
    void * (*DB_Open) (char *dbname, int mode);
    void   (*DB_Close) (void *DB);
    void   (*DB_Remove) (void *DB);
    
    int    (*DB_InitWriteHeader) (void *DB);
    int    (*DB_WriteHeaderData) (int id, unsigned char *s, int len, void *DB);
    int    (*DB_EndWriteHeader) (void *DB);
    
    int    (*DB_InitReadHeader) (void *DB);
    int    (*DB_ReadHeaderData) (int *id, unsigned char **s, int *len, void *DB);
    int    (*DB_EndReadHeader) (void *DB);
    
    int    (*DB_InitWriteWords) (void *DB);
    long   (*DB_GetWordID) (void *DB);
    int    (*DB_WriteWord) (char *word, long wordID, void *DB);
    int    (*DB_WriteWordHash) (char *word, long wordID, void *DB);
    long   (*DB_WriteWordData) (long wordID, unsigned char *worddata, int lendata, void *DB);
    int    (*DB_EndWriteWords) (void *DB);
    
    int    (*DB_InitReadWords) (void *DB);
    int    (*DB_ReadWordHash) (char *word, long *wordID, void *DB);
    int    (*DB_ReadFirstWordInvertedIndex) (char *word, char **resultword, long *wordID, void *DB);
    int    (*DB_ReadNextWordInvertedIndex) (char *word, char **resultword, long *wordID, void *DB);
    long   (*DB_ReadWordData) (long wordID, unsigned char **worddata, int *lendata, void *DB);
    int    (*DB_EndReadWords) (void *DB);
    
    
    int    (*DB_InitWriteFiles) (void *DB);
    int    (*DB_WriteFile) (int filenum, unsigned char *filedata,int sz_filedata, void *DB);
    int    (*DB_EndWriteFiles) (void *DB);

    int    (*DB_InitReadFiles) (void *DB);
    int    (*DB_ReadFile) (int filenum, unsigned char **filedata,int *sz_filedata, void *DB);
    int    (*DB_EndReadFiles) (void *DB);

#ifdef USE_BTREE
    int    (*DB_InitWriteSortedIndex) (void *DB, int n_props);
#else
    int    (*DB_InitWriteSortedIndex) (void *DB);
#endif
    int    (*DB_WriteSortedIndex) (int propID, unsigned char *data, int sz_data,void *DB);
    int    (*DB_EndWriteSortedIndex) (void *DB);
     
    int    (*DB_InitReadSortedIndex) (void *DB);
    int    (*DB_ReadSortedIndex) (int propID, unsigned char **data, int *sz_data,void *DB);
    int    (*DB_ReadSortedData) (int *data,int index, int *value, void *DB);
    int    (*DB_EndReadSortedIndex) (void *DB);

    void   (*DB_WriteProperty)( IndexFILE *indexf, FileRec *fi, int propID, char *buffer, int buf_len, int uncompressed_len, void *db);
    void   (*DB_WritePropPositions)(IndexFILE *indexf, FileRec *fi, void *db);
    void   (*DB_ReadPropPositions)(IndexFILE *indexf, FileRec *fi, void *db);
    char  *(*DB_ReadProperty)(IndexFILE *indexf, FileRec *fi, int propID, int *buf_len, int *uncompressed_len, void *db);
    void   (*DB_Reopen_PropertiesForRead)(void *DB);
#ifdef USE_BTREE
    int    (*DB_InitWriteTotalWordsPerFileArray)(SWISH *sw, void *DB);
    int    (*DB_WriteTotalWordsPerFileArray)(SWISH *sw, int *totalWordsPerFile, int totalfiles, void *DB);
    int    (*DB_EndWriteTotalWordsPerFileArray)(SWISH *sw, void *DB);
    int	   (*DB_InitReadTotalWordsPerFileArray)(SWISH *sw, void *DB);
    int    (*DB_ReadTotalWordsPerFileArray)(SWISH *sw, int **totalWordsPerFile, void *DB);
    int    (*DB_EndReadTotalWordsPerFileArray)(SWISH *sw, void *DB);
#endif
    int    (*DB_ReadTotalWordsPerFile)(SWISH *sw, int *data,int index, int *value, void *DB);
};


#endif
