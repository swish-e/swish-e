/* */


typedef struct BTREE_Page
{
    unsigned long      next;    /* Next Page */
    unsigned long      prev;    /* Previous Page */
    unsigned int       size;    /* Size of page */
    unsigned int       n;       /* Number of keys in page */
    unsigned int       flags;
    unsigned int       data_end;

    unsigned long      page_number;
    int                modified;
    int                in_use;

    struct BTREE_Page  *next_cache;

    unsigned char data[0];        /* Page data */
} BTREE_Page;

#define BTREE_CACHE_SIZE 97

typedef struct BTREE
{
    unsigned long root_page;
    int page_size;
    struct BTREE_Page *cache[BTREE_CACHE_SIZE];
    int levels;
    unsigned long tree[1024];
          /* Values for sequential reading */
    unsigned long current_page;
    unsigned long current_position;

    FILE *fp;
} BTREE;

BTREE *BTREE_Create(FILE *fp, unsigned int size);
BTREE *BTREE_Open(FILE *fp, int size, unsigned long root_page);
unsigned long BTREE_Close(BTREE *bt);
int BTREE_Insert(BTREE *b, unsigned char *key, int key_len, unsigned long data_pointer);
long BTREE_Search(BTREE *b, unsigned char *key, int key_len, unsigned char **found, int *found_len, int exact_match);
long BTREE_Next(BTREE *b, unsigned char **found, int *found_len);





