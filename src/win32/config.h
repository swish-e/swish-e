#ifdef _WIN32
#include "win32/dirent.h"
#include "win32/regex.h"

#include <stdlib.h>			/* _sleep() */
#include <process.h>			/* _getpid() */

/* ifdef logic  */
#define NO_GETTOD			/* Win32 has no Get Time Of Day  */
#define NO_SYMBOLIC_FILE_LINKS		/* Win32 has no symbolic links */
#undef INDEXPERMS			/* Win32 has no chmod() */

/* Macros which rewrite values  */
#define SWISH_VERSION "2.1-dev-20"	/* Should we find a better way to handle this */
#define TMPDIR "c:\\windows\\temp"	/* Should this be taken from the environment at runtime */
#define DIRDELIMITER '\\'		/* Does this work right?  Might explain dir problems which have reported */

#define FILEMODE_READ		"rb"	/* Read only */
#define FILEMODE_WRITE		"wb"	/* Write only */
#define FILEMODE_READWRITE	"rb+"	/* Read Write */

/* Type definitions */
typedef int pid_t;			/* process ID */

/* Rewrite ANSI functions to Win32 equivalents */
#define popen		_popen
#define pclose		_pclose
#define strcasecmp	stricmp
#define strncasecmp	strnicmp
#define sleep		_sleep
#define getpid		_getpid

#endif