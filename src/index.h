/*
$Id$
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
*/

#ifndef __HasSeenModule_Index
#define __HasSeenModule_Index       1

struct dev_ino
{
    dev_t   dev;
    ino_t   ino;
    struct dev_ino *next;
};

struct IgnoreLimitPositions
{
        int     n;              /* Number of entries per file */
        int    *pos;            /* Store metaID1,position1, metaID2,position2 ..... */
};


/*
   -- module data
*/

struct MOD_Index
{
	    /* entry vars */
    ENTRYARRAY *entryArray;
    ENTRY  *hashentries[SEARCHHASHSIZE];
	char	hashentriesdirty[SEARCHHASHSIZE];	/* just a 0/1 flag */

    /* Compression Work buffer while compression locations in index
       ** proccess */
    unsigned char *compression_buffer;
    int     len_compression_buffer;

    unsigned char *worddata_buffer;
    long  len_worddata_buffer;

	    /* File counter */
    int     filenum;

    /* index tmp (both FS and HTTP methods)*/
    int     lentmpdir;
    char   *tmpdir;

    /* Filenames of the swap files */
    unsigned char   *swap_file_name;     /* File and properties file */
    unsigned char   *swap_location_name; /* Location info file */
    /* handlers for both files */
    FILE   *fp_loc_write;       /* Location (writing) */
    FILE   *fp_loc_read;        /* Location (writing) */
    FILE   *fp_file_write;      /* File (writing) */
    FILE   *fp_file_read;       /* File (read) */

    struct dev_ino *inode_hash[BIGHASHSIZE];

    /* Buffers used by indexstring */
    int		lenswishword;
    char	*swishword;
    int		lenword;
    char	*word;

    /* Economic mode (-e) */
    int         swap_filedata;		/* swap file & property data */
    int         swap_locdata;		/* swap location data */

	/* Pointer to swap functions */ 
    long    (*swap_tell)(FILE *);
    size_t  (*swap_write)(const void *, size_t, size_t, FILE *);
    int  (*swap_seek)(FILE *, long, int);
    size_t  (*swap_read)(void *, size_t, size_t, FILE *);
    int     (*swap_close)(FILE *);
    int     (*swap_putc)(int , FILE *);
    int     (*swap_getc)(FILE *);

    /* IgnoreLimit option values */
    int		plimit;
    int		flimit;
    /* Number of words from IgnoreLimit */
    int     nIgnoreLimitWords;
    /* Positions from stopwords from IgnoreLimit */
    struct  IgnoreLimitPositions **IgnoreLimitPositionsArray;

    /* Index in blocks of chunk_size files */
    int chunk_size;

	/* Variable to control the size of the zone used for store locations during chunk proccesing */
	int optimalChunkLocZoneSize;

    /* variable to handle free memory space for locations inside currentChunkLocZone */

    LOCATION    *freeLocMemChain;

    MEM_ZONE	*perDocTmpZone;
    MEM_ZONE	*currentChunkLocZone;
    MEM_ZONE    *totalLocZone;
    MEM_ZONE	*entryZone;
};

void initModule_Index (SWISH *);
void freeModule_Index (SWISH *);
int  configModule_Index (SWISH *, StringList *);


void do_index_file (SWISH *sw, FileProp *fprop);

void addentry (SWISH *, char*, int, int, int, int );

void addCommonProperties( SWISH *sw, IndexFILE *indexf, time_t mtime, char *title, char *summary, int start, int size );
void addtofilelist(SWISH * sw, IndexFILE * indexf, char *filename,  struct file **newFileEntry);


int getfilecount (IndexFILE *);
int countwordstr (SWISH *, char *, int);

int getNumberOfIgnoreLimitWords (SWISH *);
void getPositionsFromIgnoreLimitWords(SWISH * sw);

int getrank(SWISH *, int, int, int, int, int);
void write_file_list(SWISH *, IndexFILE *);
void write_sorted_index(SWISH *, IndexFILE *);
void decompress(SWISH *, IndexFILE *);
char *ruleparse(SWISH *, char *);
void stripIgnoreFirstChars(INDEXDATAHEADER *, char *);
void stripIgnoreLastChars(INDEXDATAHEADER *, char *);

#define isIgnoreFirstChar(header,c) (header)->ignorefirstcharlookuptable[(int)((unsigned char)c)]
#define isIgnoreLastChar(header,c) (header)->ignorelastcharlookuptable[(int)((unsigned char)c)]
#define isBumpPositionCounterChar(header,c) (header)->bumpposcharslookuptable[(int)((unsigned char)c)]

unsigned char *buildFileEntry( struct file *, int *);
struct file *readFileEntry(SWISH *, IndexFILE *,int);

void computehashentry(ENTRY **,ENTRY *);

void sort_words(SWISH *, IndexFILE *);
void sortChunkLocations(SWISH *, IndexFILE *, ENTRY *);

int indexstring(SWISH*, char *, int, int, int, int *, int *);

void addtofwordtotals(IndexFILE *, int, int);
void addsummarytofile(IndexFILE *, int, char *);

void BuildSortedArrayOfWords(SWISH *,IndexFILE *);



void PrintHeaderLookupTable (int ID, int table[], int table_size, FILE *fp);
void coalesce_word_locations(SWISH * sw, IndexFILE * indexf, ENTRY *e);

void adjustWordPositions(unsigned char *worddata, int *sz_worddata, int n_files, struct IgnoreLimitPositions **ilp);

#endif
