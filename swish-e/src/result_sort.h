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
** 2001-01  jose   initial coding
**
*/



#ifndef __HasSeenModule_ResultSort
#define __HasSeenModule_ResultSort	1

#ifdef __cplusplus
extern "C" {
#endif

/*
   -- global module data structure
*/

struct MOD_ResultSort
{

	    /* sorted index flag */
	    /* TRUE - Use sorted index */
	int isPreSorted;
	    /* structure for presorted properties - used by index proccess */
    struct swline *presortedindexlist;

        /* Sortorder Translation table arrays */
              /* case sensitive translation table */
    int iSortTranslationTable[256];
              /* Ignore Case translarion table */
    int iSortCaseTranslationTable[256];
    
};





void initModule_ResultSort (SWISH *);
void freeModule_ResultSort (SWISH *);
int configModule_ResultSort (SWISH *sw, StringList *sl);


int compare_results(const void *s1, const void *s2);


int     sortresults(RESULTS_OBJECT *results);



int *CreatePropSortArray(IndexFILE *indexf, struct metaEntry *m, FileRec *fi, int free_cache );
void sortFileProperties(SWISH *sw, IndexFILE *indexf);


void initStrCmpTranslationTable(int *);
void initStrCaseCmpTranslationTable(int *);

int sw_strcasecmp(unsigned char *,unsigned char *, int *);
int sw_strcmp(unsigned char *,unsigned char *, int *);

int *LoadSortedProps( IndexFILE *indexf, struct metaEntry *m );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __HasSeenModule_ResultSort  */
