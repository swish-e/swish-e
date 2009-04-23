/*
$Id$
**


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
    
    See the COPYING file that accompanies the Swish-e distribution for details
    of the GNU GPL and the special exception available for linking against
    the Swish-e library.
    
** Mon May  9 18:19:34 CDT 2005
** added GPL


**
**
** 2001-02-28 rasc    own module started for filters
** 2001-04-09 rasc    enhancing filters
*/


#ifndef __HasSeenModule_Filter
#define __HasSeenModule_Filter	1


/* Module data and structures */



typedef struct FilterList  /* 2002-03-16 moseley */
{
    struct FilterList   *next;
    char                *prog;      /* program name to run */
    regex_list          *regex;     /* list of regular expressions */
    char                *suffix;    /* or plain text suffix */
    StringList          *options;   /* list of parsed options */

} FilterList;




/* Global module data */

struct MOD_Filter {
   /* public:  */
   /* none */
   
   /* private: don't use outside this module! */
    char   *filterdir;              /* 1998-08-07 rasc */ /* depreciated */
    FilterList *filterlist;  /* 2002-03-16 moseley */

};





/* exported Prototypes */

void initModule_Filter   (SWISH *sw);
void freeModule_Filter   (SWISH *sw);
SWINT_T  configModule_Filter (SWISH *sw, StringList *sl);

struct FilterList *hasfilter (SWISH *sw, char *filename);
FILE *FilterOpen (FileProp *fprop);
SWINT_T FilterClose (FileProp *fprop);


#endif


