/* */


typedef struct ARRAY_Page
{
    unsigned long      next;    /* Next Page */

    unsigned long      page_number;
    int                modified;
    int                in_use;

    struct ARRAY_Page  *next_cache;

    unsigned char data[0];        /* Page data */
} ARRAY_Page;

#define ARRAY_CACHE_SIZE 97

typedef struct ARRAY
{
    unsigned long root_page;
    int page_size;
    struct ARRAY_Page *cache[ARRAY_CACHE_SIZE];
    int levels;

    FILE *fp;
} ARRAY;

ARRAY *ARRAY_Create(FILE *fp);
ARRAY *ARRAY_Open(FILE *fp, unsigned long root_page);
unsigned long ARRAY_Close(ARRAY *bt);
int ARRAY_Put(ARRAY *b, int index, unsigned long value);
unsigned long ARRAY_Get(ARRAY *b, int index);
