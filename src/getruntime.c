/* Return time used so far, in microseconds.
   Copyright (C) 1994, 1999 Free Software Foundation, Inc.

This file is part of the libiberty library.
Libiberty is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

Libiberty is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with libiberty; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include "acconfig.h"

/* For testing */
// #undef HAVE_GETRUSAGE
// #undef HAVE_SYS_RESOURCE_H
// #undef HAVE_TIMES

/* There are several ways to get elapsed execution time; unfortunately no
   single way is available for all host systems, nor are there reliable
   ways to find out which way is correct for a given host. */

#include "getruntime.h"
#include <time.h>

#if defined (HAVE_GETRUSAGE) && defined (HAVE_SYS_RESOURCE_H)
#include <sys/time.h>
#include <sys/resource.h>
#endif

#ifdef HAVE_TIMES
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <sys/times.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* This is a fallback; if wrong, it will likely make obviously wrong
   results. */

#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC 1
#endif

#ifdef _SC_CLK_TCK
#define GNU_HZ  sysconf(_SC_CLK_TCK)
#else
#ifdef HZ
#define GNU_HZ  HZ
#else
#ifdef CLOCKS_PER_SEC
#define GNU_HZ  CLOCKS_PER_SEC
#endif
#endif
#endif

cpu_seconds 
get_cpu_secs ()
{
#if defined (HAVE_GETRUSAGE) && defined (HAVE_SYS_RESOURCE_H)
  struct rusage rusage;
  cpu_seconds secs;

  getrusage (0, &rusage);
  secs = (cpu_seconds)( rusage.ru_utime.tv_sec + rusage.ru_stime.tv_sec );

  if (  rusage.ru_utime.tv_usec > 500000 )
     secs++;
  if (  rusage.ru_stime.tv_usec > 500000 )
     secs++;

  return secs;


#else /* ! HAVE_GETRUSAGE */
#ifdef HAVE_TIMES

  /* This returns number of clock "ticks" since: */
  /* In linux since boot, in BSD since 1/1/1970 */
  /* Again, these are clock_t, which may overflow, but under linux it's 1/100 second so about 6000 hours */

  struct tms tms;

  times (&tms);

  return  (cpu_seconds)( (tms.tms_utime + tms.tms_stime) / GNU_HZ);


#else /* ! HAVE_TIMES */
  /* Fall back on clock and hope it's correctly implemented. */
  /* clock() returns clock_t, which seems to be a long.  On Linux CLOCKS_PER_SEC is 10^6 */
  /* so expect an overflow at about 35 minutes. */

  clock_t t = clock();
  if ( t < 0 )
      t = 0;

  return (cpu_seconds) (t / CLOCKS_PER_SEC );

#endif  /* HAVE_TIMES */
#endif  /* HAVE_GETRUSAGE */
}



