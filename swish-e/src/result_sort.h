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
** 2001-01  jose   initial coding
**
*/



#ifndef __HasSeenModule_ResultSort
#define __HasSeenModule_ResultSort	1


/*
   -- global module data structure
*/

struct MOD_ResultSort
{
        /* Sortorder Translation table arrays */
              /* case sensitive translation table */
    int iSortTranslationTable[256];
              /* Ignore Case translarion table */
    int iSortCaseTranslationTable[256];

        /* In search, the following variables hold the parameters specified */
        /* in -s command line option */
              /* Number of properties */
    int     numPropertiesToSort;    
              /* Internal variable - holds the max size of the arrays */
              /* propNameToSort and propModeToSort. It is modified */
              /* dinamically */
    int     currentMaxPropertiesToSort;
              /* Array to hold the names of the props specified in -s */
    char  **propNameToSort;
              /* Array to hold the sort mode of the props specified in -s */
              /* -1 for asc and 1 for desc */
    int    *propModeToSort;
};





void initModule_ResultSort (SWISH *);
void freeModule_ResultSort (SWISH *);
int configModule_ResultSort (SWISH *sw, StringList *sl);


int compResultsByNonSortedProps(const void *,const void *);
int compResultsBySortedProps(const void *,const void *);

char **getResultSortProperties(RESULT *);

int initSortResultProperties (SWISH *);

void addSearchResultSortProperty (SWISH *, char*, int );

void sortFileProperties(IndexFILE *indexf);

RESULT *addsortresult(SWISH *, RESULT *sp, RESULT *);
int sortresults (SWISH *, int );

void initStrCmpTranslationTable(int *);
void initStrCaseCmpTranslationTable(int *);

int sw_strcasecmp(unsigned char *,unsigned char *, int *);
int sw_strcmp(unsigned char *,unsigned char *, int *);


#endif
