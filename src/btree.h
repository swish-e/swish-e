/* */


typedef struct BTREE_Page
{
    sw_off_t           next;    /* Next Page */
    sw_off_t           prev;    /* Previous Page */
    unsigned int       size;    /* Size of page */
    unsigned int       n;       /* Number of keys in page */
    unsigned int       flags;
    unsigned int       data_end;

    sw_off_t           page_number;
    int                modified;
    int                in_use;

    struct BTREE_Page  *next_cache;

    unsigned char data[0];        /* Page data */
} BTREE_Page;

#define BTREE_CACHE_SIZE 97

typedef struct BTREE
{
    sw_off_t root_page;
    int page_size;
    struct BTREE_Page *cache[BTREE_CACHE_SIZE];
    int levels;
    sw_off_t tree[1024];
          /* Values for sequential reading */
    sw_off_t current_page;
    unsigned long current_position;

    FILE *fp;
} BTREE;

BTREE *BTREE_Create(FILE *fp, unsigned int size);
BTREE *BTREE_Open(FILE *fp, int size, sw_off_t root_page);
sw_off_t BTREE_Close(BTREE *bt);
int BTREE_Insert(BTREE *b, unsigned char *key, int key_len, sw_off_t data_pointer);
sw_off_t BTREE_Search(BTREE *b, unsigned char *key, int key_len, unsigned char **found, int *found_len, int exact_match);
sw_off_t BTREE_Next(BTREE *b, unsigned char **found, int *found_len);
int BTREE_Update(BTREE *b, unsigned char *key, int key_len, sw_off_t new_data_pointer);

