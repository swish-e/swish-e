/* */


typedef struct ARRAY_Page
{
    sw_off_t           next;    /* Next Page */

    sw_off_t           page_number;
    int                modified;
    int                in_use;

    struct ARRAY_Page  *next_cache;

    unsigned char data[0];        /* Page data */
} ARRAY_Page;

#define ARRAY_CACHE_SIZE 97

typedef struct ARRAY
{
    sw_off_t root_page;
    int page_size;
    struct ARRAY_Page *cache[ARRAY_CACHE_SIZE];
    int levels;

    FILE *fp;
} ARRAY;

ARRAY *ARRAY_Create(FILE *fp);
ARRAY *ARRAY_Open(FILE *fp, sw_off_t root_page);
sw_off_t ARRAY_Close(ARRAY *bt);
int ARRAY_Put(ARRAY *b, int index, unsigned long value);
unsigned long ARRAY_Get(ARRAY *b, int index);
