/* 10/00 Jose Ruiz.
** The file and properties information can be compressed
** using the deflate algorithm. The code uses Jean-loup Gailly's zlib
** deflate algorithm but in a simpler way (no Huffman trees, simpler
** code and hashes, etc).
** Also, direct access is implemented to extract the information from
** the file as faster as possible. Zlib is stream oriented, so I could
** not use it.
** Thus, you must not expect great compression ratios.
** If enabled, the index proccess should be slower, but the performance
** of the search will not be affected
*/
/* This code would not be possible without Jean-loup Gailly's ZLIB */



struct dict_st                   /* Dictionary entry */
{
        struct dict_st *next;    /* Pointer to next entry */
        int index;               /* Index to all_entries (see bellow) */
        char *val;               /* Dictionary entry contents */
	int sz_val;              /* Size of val */
};

struct dict_lookup_st              /* Dictonary struct */
{
        int n_entries;             /* Number of entries */
        struct dict_st *entries[256][256];   /* fast lookup table for entries */
        struct dict_st *all_entries[1];    /* All dictionary entries array */
};

struct buffer_st
{
	struct buffer_st *next;  /* Pointer to next buffer */
	unsigned char *str;      /* Buffer memory area containing the data */
	int sz_str;              /* Size of the data */
	long *pos;               /* Pointer to the long containing the */ 
                                 /* offset of the data in the file */
};

struct buffer_pool {             /* Linked list of buffers */
	struct dict_lookup_st *dict;     /* pointer to dictionary */
	struct int_lookup_st *lookup;    /* pointer to lookup table */
	int max_size;                    /* Max allowed total size of buffers */
	int total_size;                  /* Total current size of proccessed
                                         ** buffers */
	int min_match,max_match;         /* min and max size of patterns to 
                                         ** search for */
	struct buffer_st *head;          /* Head of buffer's list */
	struct buffer_st *tail;          /* tail/last buffer */
};

/* You can change the following three values to adjust the compression to your
needs but it you are not sure about them, do not change them */

#define DEFLATE_MAX_WINDOW_SIZE 2000
/* This is the the size of the window where matching patterns are searched */
/* If you increment this value the index proccess will become slower */

#define DEFLATE_MIN_PATTERN_SIZE 3
/* This value must be >=3 */

#define DEFLATE_MAX_PATTERN_SIZE 6
/* This value must be >DEFLATE_MIN_PATTERN_SIZE and <255 */


/* The prototypes */
struct buffer_pool *zfwrite (struct buffer_pool *, unsigned char *, int, long *, FILE *);
void zfflush (struct buffer_pool *, FILE *);
unsigned char *zfread (unsigned char **dict,int *,FILE *);
void printdeflatedictionary (struct buffer_pool *,IndexFILE *);
void readdeflatepatterns (IndexFILE *);
int configModule_Deflate  (SWISH *sw, StringList *sl);

