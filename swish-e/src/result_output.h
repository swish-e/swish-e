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


#ifndef __HasSeenModule_ResultOutput
#define __HasSeenModule_ResultOutput	1


/*
   -- module data
*/

struct ResultExtFmtStrList
{                               /* -x extended format by defined names */
    char   *name;
    char   *fmtstr;
    struct ResultExtFmtStrList *next;
    struct ResultExtFmtStrList *nodep;
};



/*
   -- global module data structure
*/

struct MOD_ResultOutput {
   /* public:  */
    int     headerOutVerbose;   /* -H <n> print extended header info */
                                /* should be private, of proper method is defined */

   /* private: don't use outside this module! */
                                /* -x extended format by defined names */
    char   *extendedformat;     /* -x "fmt", holds fmt or NULL */
    char   *stdResultFieldDelimiter; /* -d <c> delimiter , (def: config.h) v1.x output style */

    /* ResultExtendedFormat predefined List see: -x */
    struct ResultExtFmtStrList *resultextfmtlist;
};




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

