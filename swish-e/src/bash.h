/* For swish */
#define xmalloc emalloc
#define xfree efree
#define xrealloc erealloc

#define FS_EXISTS		0x1
#ifdef _WIN32
#define FS_EXECABLE		0x1
#else
#define FS_EXECABLE		0x2
#endif

/* horrible Win32 hack */
#ifdef _WIN32
/* Fake group functions... */
#define GETGROUPS_T int
#define getegid() 0
#define geteuid() 0
#define getgid()  0
#endif

#define savestring(x) (char *)strcpy((char *)xmalloc(1 + strlen (x)), (x))

extern int file_status(const char *name);
extern int absolute_program(const char *string);
extern char *get_next_path_element(const char *path_list, int *path_index_pointer);
extern char *make_full_pathname(const char *path, const char *name, int name_len);
