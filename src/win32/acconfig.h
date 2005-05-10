/*
    $Id$

    This file is part of Swish-e.

    Swish-e is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Swish-e is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    
    Swish-e includes a library for searching with a well-defined API. The library
    is named libswish-e.
    
    Linking libswish-e statically or dynamically with other modules is making a
    combined work based on Swish-e.  Thus, the terms and conditions of the GNU
    General Public License cover the whole combination.

    As a special exception, the copyright holders of Swish-e give you
    permission to link Swish-e with independent modules that communicate with
    Swish-e solely through the libswish-e API interface, regardless of the license
    terms of these independent modules, and to copy and distribute the
    resulting combined work under terms of your choice, provided that
    every copy of the combined work is accompanied by a complete copy of
    the source code of Swish-e (the version of Swish-e used to produce the
    combined work), being distributed under the terms of the GNU General
    Public License plus this exception.  An independent module is a module
    which is not derived from or based on Swish-e.

    Note that people who make modified versions of Swish-e are not obligated
    to grant this special exception for their modified versions; it is
    their choice whether to do so.  The GNU General Public License gives
    permission to release a modified version without this exception; this
    exception also makes it possible to release a modified version which
    carries forward this exception.

* 
*   2005-05-09 - DLN - Added GPL with linking clause.
* 
*/

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
