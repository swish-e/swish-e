#include "acconfig.h"

#ifdef STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr(), *strrchr();
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#ifdef HAVE_UNISTD_H
# include <sys/types.h>
# include <unistd.h>
#endif

#ifdef STAT_MACROS_BROKEN
# include "posixstat.h"
#endif

#if STDC_HEADERS
# include <stdlib.h>
#endif


#ifndef NULL
# ifdef __STDC__
#   define NULL ((void *)0)
# else
#   define NULL (0x0)
# endif
#endif

