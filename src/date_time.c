/*
$Id$
**
** This program and library is free software; you can redistribute it and/or
** modify it under the terms of the GNU (Library) General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU (Library) General Public License for more details.
**
** You should have received a copy of the GNU (Library) General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
**
**
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
    long micro_sec = get_run_time();
    return ((double) micro_sec ) / 1000000.0;
    //return  ((double) clock()) / CLOCKS_PER_SEC;
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
