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
#define __HasSeenModule_DB	1

void initModule_DB (SWISH *);
void freeModule_DB (SWISH *);
int configModule_DB (SWISH *sw, StringList *sl);

void    write_header(SWISH *, INDEXDATAHEADER *, void *, char *, int, int, int);
void    write_index(SWISH *, IndexFILE *);
void    write_word(SWISH *, ENTRY *, IndexFILE *);
void    write_worddata(SWISH *, ENTRY *, IndexFILE *);
int     write_words_to_header(SWISH *, int header_ID, struct swline **hash, void *DB);
void    write_pathlookuptable_to_header(SWISH *, int id, INDEXDATAHEADER *header, void *DB);
void    write_locationlookuptables_to_header(SWISH *, int id, INDEXDATAHEADER *header, void *DB);
void    write_MetaNames (SWISH *, int id, INDEXDATAHEADER *header, void *DB);
int     write_integer_table_to_header(SWISH *, int id, int table[], int table_size, void *DB);

void    read_header(SWISH *, INDEXDATAHEADER *header, void *DB);

void    parse_MetaNames_from_buffer(INDEXDATAHEADER *header, char *buffer);
void    parse_stopwords_from_buffer(INDEXDATAHEADER *header, char *buffer);
void    parse_buzzwords_from_buffer(INDEXDATAHEADER *header, char *buffer);
void    parse_locationlookuptables_from_buffer(INDEXDATAHEADER *header, char *buffer);
void    parse_pathlookuptable_from_buffer(INDEXDATAHEADER *header, char *buffer);
void    parse_integer_table_from_buffer(int table[], int table_size, char *buffer);
char *getfilewords(SWISH *sw, int, IndexFILE *);



/* Common DB api */
void   *DB_Create (SWISH *sw, char *dbname);
void   *DB_Open (SWISH *sw, char *dbname);
void    DB_Close(SWISH *sw, void *DB);
void    DB_Remove(SWISH *sw, void *DB);

int     DB_InitWriteHeader(SWISH *sw, void *DB);
int     DB_EndWriteHeader(SWISH *sw, void *DB);
int     DB_WriteHeaderData(SWISH *sw, int id, char *s, int len, void *DB);

int     DB_InitReadHeader(SWISH *sw, void *DB);
int     DB_ReadHeaderData(SWISH *sw, int *id, char **s, int *len, void *DB);
int     DB_EndReadHeader(SWISH *sw, void *DB);

int     DB_InitWriteWords(SWISH *sw, void *DB);
long    DB_GetWordID(SWISH *sw, void *DB);
int     DB_WriteWord(SWISH *sw, char *word, long wordID, void *DB);
int     DB_WriteWordHash(SWISH *sw, char *word, long wordID, void *DB);
long    DB_WriteWordData(SWISH *sw, long wordID, char *worddata, int lendata, void *DB);
int     DB_EndWriteWords(SWISH *sw, void *DB);

int     DB_InitReadWords(SWISH *sw, void *DB);
int     DB_ReadWordHash(SWISH *sw, char *word, long *wordID, void *DB);
int     DB_ReadFirstWordInvertedIndex(SWISH *sw, char *word, char **resultword, long *wordID, void *DB);
int     DB_ReadNextWordInvertedIndex(SWISH *sw, char *word, char **resultword, long *wordID, void *DB);
long    DB_ReadWordData(SWISH *sw, long wordID, char **worddata, int *lendata, void *DB);
int     DB_EndReadWords(SWISH *sw, void *DB);


int     DB_InitWriteFiles(SWISH *sw, void *DB);
int     DB_WriteFile(SWISH *sw, int filenum, char *filedata,int sz_filedata, void *DB);
int     DB_EndWriteFiles(SWISH *sw, void *DB);

int     DB_InitReadFiles(SWISH *sw, void *DB);
int     DB_ReadFile(SWISH *sw, int filenum, char **filedata,int *sz_filedata, void *DB);
int     DB_EndReadFiles(SWISH *sw, void *DB);


int     DB_InitWriteSortedIndex(SWISH *sw, void *DB);
int     DB_WriteSortedIndex(SWISH *sw, int propID, char *data, int sz_data,void *DB);
int     DB_EndWriteSortedIndex(SWISH *sw, void *DB);
 
int     DB_InitReadSortedIndex(SWISH *sw, void *DB);
int     DB_ReadSortedIndex(SWISH *sw, int propID, char **data, int *sz_data,void *DB);
int     DB_EndReadSortedIndex(SWISH *sw, void *DB);

struct MOD_DB
{
    char *DB_name; /* short name for data source */

	void * (*DB_Create) (char *dbname);
    void * (*DB_Open) (char *dbname);
    void   (*DB_Close) (void *DB);
    void   (*DB_Remove) (void *DB);
    
    int    (*DB_InitWriteHeader) (void *DB);
    int    (*DB_WriteHeaderData) (int id, char *s, int len, void *DB);
    int    (*DB_EndWriteHeader) (void *DB);
    
    int    (*DB_InitReadHeader) (void *DB);
    int    (*DB_ReadHeaderData) (int *id, char **s, int *len, void *DB);
    int    (*DB_EndReadHeader) (void *DB);
    
    int    (*DB_InitWriteWords) (void *DB);
    long   (*DB_GetWordID) (void *DB);
    int    (*DB_WriteWord) (char *word, long wordID, void *DB);
    int    (*DB_WriteWordHash) (char *word, long wordID, void *DB);
    long   (*DB_WriteWordData) (long wordID, char *worddata, int lendata, void *DB);
    int    (*DB_EndWriteWords) (void *DB);
    
    int    (*DB_InitReadWords) (void *DB);
    int    (*DB_ReadWordHash) (char *word, long *wordID, void *DB);
    int    (*DB_ReadFirstWordInvertedIndex) (char *word, char **resultword, long *wordID, void *DB);
    int    (*DB_ReadNextWordInvertedIndex) (char *word, char **resultword, long *wordID, void *DB);
    long   (*DB_ReadWordData) (long wordID, char **worddata, int *lendata, void *DB);
    int    (*DB_EndReadWords) (void *DB);
    
    
    int    (*DB_InitWriteFiles) (void *DB);
    int    (*DB_WriteFile) (int filenum, char *filedata,int sz_filedata, void *DB);
    int    (*DB_EndWriteFiles) (void *DB);

    int    (*DB_InitReadFiles) (void *DB);
    int    (*DB_ReadFile) (int filenum, char **filedata,int *sz_filedata, void *DB);
    int    (*DB_EndReadFiles) (void *DB);
    
    int    (*DB_InitWriteSortedIndex) (void *DB);
    int    (*DB_WriteSortedIndex) (int propID, char *data, int sz_data,void *DB);
    int    (*DB_EndWriteSortedIndex) (void *DB);
     
    int    (*DB_InitReadSortedIndex) (void *DB);
    int    (*DB_ReadSortedIndex) (int propID, char **data, int *sz_data,void *DB);
    int    (*DB_EndReadSortedIndex) (void *DB);
};


#endif
