#ifdef _WIN32

/* Special Inclusions  */

#include <windows.h>    /* Win32 API */
#include <stdlib.h>		/* _sleep() */
#include <process.h>	/* _getpid() */
#include <io.h>			/* _umask(), _mktemp, etc */
#include <fcntl.h>		/* Most io.h functions want this */
#include <sys/types.h>	/* Most io.h functions want this */
#include <sys/stat.h>	/* Most io.h functions want this */


/* We define this in OS-specific code which include's config.h  */

#ifndef _SWISH_PORT
#  include "dirent.h"
#  include "pcreposix.h"
#  include "mkstemp.h"
#endif


/* ifdef logic  */
#define NO_GETTOD		/* Win32 has no Get Time Of Day  */
#undef HAVE_LSTAT		/* Win32 has no symbolic links */
#undef INDEXPERMS		/* Win32 has no chmod() - DLN 2001-11-05 Umm, yes it does... */
#define HAVE_SYS_TIMEB_H /* _ftime(), struct _timeb */ 
#define HAVE_STDLIB_H	/* We need stdlib.h instead of unistd.h  */
#define HAVE_PROCESS_H  /* _getpid is here  */
#define HAVE_VARARGS_H  /* va_list, vsnprintf, etc */
#define HAVE_LIBXML2 1  /* enable libxml2 XML parser */
#define HAVE_MEMCPY  1  /* sys.h explodes without this */
#define STDC_HEADERS 1  /* We have Standard C Headers, I think */

/* Environment Stuff */
#define HAVE_SETENV     /* Enable PATH setting */
#define PATH_SEPARATOR ";" /* PATH separator string */

#define HAVE_STRING_H   /* For mkstemp from libiberty  */

/* Macros which rewrite values  */
#define SWISH_VERSION "2.4.3"	/* Should we find a better way to handle this */
#define VERSION SWISH_VERSION   /* Some things want this  */

#define libexecdir "/usr/local/lib"  /* Microsoft CPP is brain damaged */


/* Internal SWISH-E File Access Modes */
#define FILEMODE_READ		"rb"	/* Read only */
#define FILEMODE_WRITE		"wb"	/* Write only */
#define FILEMODE_READWRITE	"rb+"	/* Read Write */

/* External POSIX File Access Modes */
#define O_RDWR _O_RDWR
#define O_CREAT _O_CREAT
#define O_EXCL _O_EXCL
#define O_BINARY _O_BINARY

/* Stat Stuff; borrowed from linux/stat.h */
#define S_ISLNK(m)      (((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)      (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)      (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)     (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)     (((m) & S_IFMT) == S_IFSOCK)


/* Win32 filename lengths  */
#define SW_MAXPATHNAME 4096
#define SW_MAXFILENAME 256

/* Type definitions */
typedef int pid_t;			/* process ID */
typedef int mode_t;         /* file permission mode ID */

/* Rewrite ANSI functions to Win32 equivalents */
#define popen		_popen
#define pclose		_pclose
#define strcasecmp	stricmp
#define strncasecmp	strnicmp
#define sleep		_sleep
#define getpid		_getpid
#define umask       _umask
#define vsnprintf   _vsnprintf
#define stat	    _stat

#endif
