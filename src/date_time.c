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
    along  with Swish-e; if not, write to the Free Software
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
    
** Mon May  9 15:51:39 CDT 2005
** added GPL


**
** 2001-03-20 rasc   own module for this routine  (from swish.c)
**
*/


#include <time.h>
#include "swish.h"
#include "mem.h"
#include "date_time.h"
#include "getruntime.c"



/*
  -- TimeHiRes returns a ClockTick value (double)
  -- in seconds.fractions
*/

#ifdef HAVE_BSDGETTIMEOFDAY
#define gettimeofday BSDgettimeofday
#endif

#ifdef NO_GETTOD

double TimeElapsed(void)
{
#ifdef HAVE_SYS_TIMEB_H
#include <sys/timeb.h>

	struct timeb ftimebuf;
	
	ftime(&ftimebuf);
	return (double)ftimebuf.time + (double)ftimebuf.millitm/1000.0;

#else

    return  ((double) clock()) / CLOCKS_PER_SEC;

#endif
}
    
#else

#include <sys/time.h>

double TimeElapsed(void)
{
 struct timeval t;
 int i;

    i = gettimeofday( &t, NULL );
    if ( i ) return 0;

    return (double)( t.tv_sec + t.tv_usec / 1000000.0 );
}
#endif

/* return CPU time used */
double TimeCPU(void)
{
    return (double) get_cpu_secs();
}





/*
  Returns the nicely formatted date.
  Returns ISO like char
*/

char *getTheDateISO()
{
char *date;
time_t now;

	date=emalloc(MAXSTRLEN);
	
	now = time(NULL);
	/* 2/22/00 - switched to 4-digit year (%Y vs. %y) */
	strftime(date, MAXSTRLEN, "%Y-%m-%d %H:%M:%S %Z", (struct tm *) localtime(&now)); 
	
	return date;
}
