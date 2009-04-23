/*
$Id$
**


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
    SWINT_T     n;                  /* Number of entries per file */
    SWINT_T    *pos;                /* Store metaID1,position1, metaID2,position2 ..... */
};

/* This is used to build a list of the metaIDs that are currently in scope when indexing words */

typedef struct
{
    SWINT_T    *array;              /* list of metaIDs that need to be indexed */
    SWINT_T     max;                /* max size of table */
    SWINT_T     num;                /* number in list */
    SWINT_T     defaultID;          /* default metaID (should always be one, I suppose) */
}
METAIDTABLE;


/*
   -- module data
*/


struct MOD_Index
{
    /* entry vars */
    METAIDTABLE metaIDtable;
    ENTRYARRAY *entryArray;
    ENTRY  *hashentries[VERYBIGHASHSIZE];
    char    hashentriesdirty[VERYBIGHASHSIZE]; /* just a 0/1 flag */

    /* Compression Work buffer while compression locations in index ** proccess */
    unsigned char *compression_buffer;
    SWINT_T     len_compression_buffer;

    unsigned char *worddata_buffer;  /* Buffer to store worddata */
    SWINT_T    len_worddata_buffer;     /* Max size of the buffer */
    SWINT_T    sz_worddata_buffer;      /* Space being used in worddata_buffer */

    /* File counter */
    SWINT_T     filenum;

    /* index tmp (both FS and HTTP methods) */
    char   *tmpdir;

    /* Filenames of the swap files */
    char   *swap_location_name[MAX_LOC_SWAP_FILES]; /* Location info file */

    /* handlers for both files */
    FILE   *fp_loc_write[MAX_LOC_SWAP_FILES];       /* Location (writing) */
    FILE   *fp_loc_read[MAX_LOC_SWAP_FILES];        /* Location (reading) */

    struct dev_ino *inode_hash[BIGHASHSIZE];

    /* Buffers used by indexstring */
    SWINT_T     lenswishword;
    char   *swishword;
    SWINT_T     lenword;
    char   *word;

    /* Economic mode (-e) */
    SWINT_T     swap_locdata;       /* swap location data */

    /* Pointer to swap functions */
    sw_off_t    (*swap_tell) (FILE *);
            size_t(*swap_write) (const void *, size_t, size_t, FILE *);
    int     (*swap_seek) (FILE *, sw_off_t, int);       // no rw64
            size_t(*swap_read) (void *, size_t, size_t, FILE *);
    int     (*swap_close) (FILE *);                     // no rw64
    int     (*swap_putc) (int, FILE *);                 // no rw64
    int     (*swap_getc) (FILE *);                      // no rw64

    /* IgnoreLimit option values */
    SWINT_T     plimit;
    SWINT_T     flimit;
    /* Number of words from IgnoreLimit */
    SWINT_T     nIgnoreLimitWords;
    struct swline *IgnoreLimitWords;

    /* Positions from stopwords from IgnoreLimit */
    struct IgnoreLimitPositions **IgnoreLimitPositionsArray;

    /* Index in blocks of chunk_size files */
    SWINT_T     chunk_size;

    /* Variable to control the size of the zone used for store locations during chunk proccesing */
    SWINT_T     optimalChunkLocZoneSize;

    /* variable to handle free memory space for locations inside currentChunkLocZone */

    LOCATION *freeLocMemChain;

    MEM_ZONE *perDocTmpZone;
    MEM_ZONE *currentChunkLocZone;
    MEM_ZONE *totalLocZone;
    MEM_ZONE *entryZone;

    SWINT_T     update_mode;    /* Set to 1 when in update mode */
                            /* Set to 2 when in remove mode */
};

void    initModule_Index(SWISH *);
void    freeModule_Index(SWISH *);
SWINT_T     configModule_Index(SWISH *, StringList *);


void    do_index_file(SWISH * sw, FileProp * fprop);

ENTRY  *getentry(SWISH * , char *);
void    addentry(SWISH *, ENTRY *, SWINT_T, SWINT_T, SWINT_T, SWINT_T);

void    addCommonProperties(SWISH * sw, FileProp * fprop, FileRec * fi, char *title, char *summary, SWINT_T start);


SWINT_T     getfilecount(IndexFILE *);

SWINT_T     getNumberOfIgnoreLimitWords(SWISH *);
void    getPositionsFromIgnoreLimitWords(SWISH * sw);

char   *ruleparse(SWISH *, char *);

#define isIgnoreFirstChar(header,c) (header)->ignorefirstcharlookuptable[(SWINT_T)((unsigned char)c)]
#define isIgnoreLastChar(header,c) (header)->ignorelastcharlookuptable[(SWINT_T)((unsigned char)c)]
#define isBumpPositionCounterChar(header,c) (header)->bumpposcharslookuptable[(SWINT_T)((unsigned char)c)]


void    computehashentry(ENTRY **, ENTRY *);

void    sort_words(SWISH *);

SWINT_T     indexstring(SWISH * sw, char *s, SWINT_T filenum, SWINT_T structure, SWINT_T numMetaNames, SWINT_T *metaID, SWINT_T *position);

void    addsummarytofile(IndexFILE *, SWINT_T, char *);

void    BuildSortedArrayOfWords(SWISH *, IndexFILE *);



void    PrintHeaderLookupTable(SWINT_T ID, SWINT_T table[], SWINT_T table_size, FILE * fp);
void    coalesce_all_word_locations(SWISH * sw, IndexFILE * indexf);
void    coalesce_word_locations(SWISH * sw, ENTRY * e);

void    adjustWordPositions(unsigned char *worddata, SWINT_T *sz_worddata, SWINT_T n_files, struct IgnoreLimitPositions **ilp);

#endif
