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
** 2001-02-28 rasc    own module started for filters
** 2001-04-09 rasc    enhancing filters
*/


#ifndef __HasSeenModule_FILTERS
#define __HasSeenModule_FILTERS	1


/* Module data and structures */


struct FilterList {	/* Store filtersprogs and extension */
    char *suffix;
    char *prog;
    char *options;       /* 2001-04-09 rasc */
    struct FilterList *next;
    struct FilterList *nodep;
};



/* exported Prototypes */

void initModule_Filter   (SWISH *sw);
void freeModule_Filter   (SWISH *sw);
int  configModule_Filter (SWISH *sw, StringList *sl);

struct FilterList *hasfilter (char *filename, struct FilterList *filterlist);
FILE *FilterOpen (FileProp *fprop);
int FilterClose (FILE *fp);


#endif


