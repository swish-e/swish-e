/* */

#define FHASH_SIZE 10001

typedef struct FHASH
{
    sw_off_t hash_offsets[FHASH_SIZE];  /* Hash table */
    sw_off_t start;  /* Pointer to start of hash table in file */
    FILE *fp;
} FHASH;

FHASH *FHASH_Create(FILE *fp);
FHASH *FHASH_Open(FILE *fp, sw_off_t start);
sw_off_t FHASH_Close(FHASH *f);
int FHASH_Insert(FHASH *f, unsigned char *key, int key_len, unsigned char *data, int data_len);
int FHASH_Search(FHASH *f, unsigned char *key, int key_len, unsigned char *data, int data_len);
int FHASH_Update(FHASH *f, unsigned char *key, int key_len, unsigned char *data, int data_len);
int FHASH_Delete(FHASH *f, unsigned char *key, int key_len);





