/* */

struct ramdisk *ramdisk_create(char *, int);

int ramdisk_close(FILE *);

void add_buffer_ramdisk(struct ramdisk *);

long ramdisk_tell(FILE *);

size_t ramdisk_write(const void *,size_t, size_t, FILE *);

int ramdisk_seek(FILE *,long, int );

size_t ramdisk_read(void *, size_t, size_t, FILE *);

int ramdisk_getc(FILE *);

int ramdisk_putc(int , FILE *);
