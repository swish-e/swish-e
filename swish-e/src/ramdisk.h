/* */

struct ramdisk *ramdisk_create(int);

int ramdisk_close(FILE *);

void add_buffer_ramdisk(struct ramdisk *);

long ramdisk_tell_write(FILE *);

long ramdisk_tell_read(FILE *);

size_t ramdisk_write(const void *,size_t, size_t, FILE *);


int ramdisk_seek_read(FILE *,long, int );

size_t ramdisk_read(void *, size_t, size_t, FILE *);

int ramdisk_getc(FILE *);

int ramdisk_putc(int , FILE *);
