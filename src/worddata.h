/* */


typedef struct WORDDATA_Page
{
    unsigned long      page_number;
    int                used_blocks;
    int                n;
    int                modified;
    int                in_use;

    struct WORDDATA_Page  *next_cache;

    unsigned char data[0];        /* Page data */
} WORDDATA_Page;

#define WORDDATA_CACHE_SIZE 97

typedef struct WORDDATA
{
    WORDDATA_Page      *last_page;
    struct WORDDATA_Page *cache[WORDDATA_CACHE_SIZE];
    int      page_counter;
    unsigned long lastid;
    FILE *fp;
} WORDDATA;

WORDDATA *WORDDATA_Open(FILE *fp);
void WORDDATA_Close(WORDDATA *bt);
unsigned long WORDDATA_Put(WORDDATA *b, unsigned int len, unsigned char *data);
unsigned char * WORDDATA_Get(WORDDATA *b, unsigned long global_id, unsigned int *len);
