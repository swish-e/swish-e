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
** 2001-01  R. Scherg  (rasc)   initial coding
**
*/


#ifndef __HasSeenModule_RESULT_OUTPUT
#define __HasSeenModule_RESULT_OUTPUT	1


void initModule_ResultOutput (SWISH *sw);
void freeModule_ResultOutput (SWISH *sw);
int configModule_ResultOutput (SWISH *sw, StringList *sl);


void initPrintExtResult (SWISH *sw, char *fmt);
void printResultOutput (SWISH *sw);
void printSortedResults (SWISH *sw);


char *hasResultExtFmtStr (SWISH *sw, char *name);


int resultHeaderOut (SWISH *sw, int min_verbose, char *prtfmt, ...);
void resultPrintHeader (SWISH *sw, int min_verbose, INDEXDATAHEADER *h, 
				char *pathname, int merged);



#endif

