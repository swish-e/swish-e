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
** Move index presorting code to pre_sort.c  - Sept 2002 - moseley
**
*/

#include "swish.h"
#include "swstring.h"
#include "mem.h"
#include "merge.h"
#include "list.h"
#include "search.h"
#include "docprop.h"
#include "metanames.h"
#include "compress.h"
#include "error.h"
#include "db.h"
#include "parse_conffile.h"
#include "swish_qsort.h"
#include "result_sort.h"

#include "rank.h"

// #define DEBUGSORT 1



/* 02/2001 Jose Ruiz */
/*****************************************************************************
* compResultsByNonSortedProps - qsort compare function
*
*   function for comparing data in order to
*   get sorted results with qsort
*   (including combinations of asc and descending fields)
*
*   $$$ This does not follow the case setting specified in the config file.
*   Should convert to sorting Prop structures (using Compare_Properties)
*   Would still need a function like this for doing the merge sort in search.c. - moseley 9/2002
*   
*
*****************************************************************************/

int     compResultsByNonSortedProps(const void *s1, const void *s2)
{
    RESULT *r1 = *(RESULT * const *) s1;
    RESULT *r2 = *(RESULT * const *) s2;
    int     i;
    int     rc;
    int     num_fields      = r1->db_results->num_sort_props;
    int    *sort_direction  = r1->db_results->sort_directions;
    /* struct MOD_ResultSort *md = r1->db_results->results->sw->ResultSort; */

    for (i = 0; i < num_fields; i++)
    {
        /* $$$ this feature is still pending (and needs to be extended to work with Compare_Properties() */
        /* if ((rc = sw_strcasecmp( (unsigned char*)r1->PropSort[i], (unsigned char*)r2->PropSort[i], md->iSortCaseTranslationTable))) */

        if ((rc = strcasecmp( (unsigned char*)r1->PropSort[i], (unsigned char*)r2->PropSort[i])))
            return ( rc * sort_direction[i] );
    }
    return 0;
}

/*****************************************************************************
* compResultsBySortedProps - qsort compare function when the index is pre-sorted
*
*
*****************************************************************************/

int     compResultsBySortedProps(const void *s1, const void *s2)
{
    RESULT *r1 = *(RESULT * const *) s1;
    RESULT *r2 = *(RESULT * const *) s2;
    int     i;
    int     rc;
    int     num_fields      = r1->db_results->num_sort_props;
    int    *sort_direction  = r1->db_results->sort_directions;

    for (i = 0; i < num_fields; i++)
    {
        if((rc = r1->iPropSort[i] - r2->iPropSort[i]))
            return ( rc * sort_direction[i] );
    }
    return 0;
}



/*******************************************************************
*   Loads metaentry->sorted_data with sorted array for the given metaEntry
*
*   Call with:
*       *sw
*       *indexf
*       *metaEntry - meta entry in question
*
*   Returns:
*       pointer to an array of int (metaentry->sorted_data)
*
*   Notes:
*       This is also called by proplimit and merge code
*
********************************************************************/
int    *LoadSortedProps(SWISH * sw, IndexFILE * indexf, struct metaEntry *m)
{
    unsigned char *buffer;
    int     sz_buffer;
#ifndef USE_BTREE
    unsigned char *s;
    int     j;
#endif

    DB_InitReadSortedIndex(sw, indexf->DB);

    /* Get the sorted index of the property */

    /* Convert to a property index */
    DB_ReadSortedIndex(sw, m->metaID, &buffer, &sz_buffer, indexf->DB);

#ifndef USE_BTREE
    /* Table doesn't exist */
    if (!sz_buffer)
    {
        DB_EndReadSortedIndex(sw, indexf->DB);
        return NULL;
    }


    s = buffer;
    m->sorted_data = (int *) emalloc(indexf->header.totalfiles * sizeof(int));

    /* Unpack / decompress the numbers */
    for (j = 0; j < indexf->header.totalfiles; j++)
        m->sorted_data[j] = uncompress2(&s);

    efree(buffer);
#else
    m->sorted_data = (int *)buffer;
#endif
    DB_EndReadSortedIndex(sw, indexf->DB);

    return m->sorted_data;
}



/***************************************************************************************
* getLookupResultSortedProperties
*   create an array to hold the sort key when all sorts are pre-sorted
*
*   Creates array in a mem-zone.
*
*   Returns:
*       pointer to array of integers for the current result.
*
****************************************************************************************/

static int    *getLookupResultSortedProperties(RESULT * r)
{
    int             i;
    int            *props = NULL;       /* Array to Store properties Lookups */
    struct metaEntry *m = NULL;
    DB_RESULTS     *db_results = r->db_results;
    RESULTS_OBJECT *results = db_results->results;
    IndexFILE      *indexf = db_results->indexf;
    SWISH          *sw = indexf->sw;
    int            num_props = db_results->num_sort_props;


    props = (int *) Mem_ZoneAlloc(results->resultSortZone, num_props * sizeof(int));

    for (i = 0; i < num_props; i++)
    {
        /* This shouldn't happen -- the meta names should be checked before this */
        if (!(m = getPropNameByID(&indexf->header, db_results->propIDToSort[i])))
        {
            props[i] = 0;
            continue;
        }

        /* Deal with internal meta names */
        if (is_meta_internal(m))
        {
            if (is_meta_entry(m, AUTOPROPERTY_RESULT_RANK))
            {
                props[i] = r->rank;
                continue;
            }

            if (is_meta_entry(m, AUTOPROPERTY_REC_COUNT))
            {
                props[i] = 0;   /* rec count is set in result_output.c */
                continue;
            }

            if (is_meta_entry(m, AUTOPROPERTY_FILENUM))
            {
                props[i] = r->filenum;
                continue;
            }

            if (is_meta_entry(m, AUTOPROPERTY_INDEXFILE))
            {
                props[i] = 0;
                continue;
            }
        }


        /* Load the sorted data from disk, if first time */
        /* If no sorted data available, return NULL, which will fall back to old sort method */

        if (!m->sorted_data && !LoadSortedProps(sw, indexf, m))
        {
            /* FIX: Removed!!! efree(props); props is allocated using Zone. Do not call efree here */
            return NULL;
        }


        /* Now store the sort value in the array */
        DB_ReadSortedData(sw, m->sorted_data,r->filenum - 1, &props[i], sw->Db);

    }

    return props;
}



/***************************************************************************************
* getResultSortProperties
*   create an array to hold the sort key of strings
*
*   Creates array in a mem-zone.
*
*   Returns:
*       pointer to array of points to string for the current result.
*
*   Note, this is also called by search.c
*
****************************************************************************************/
char  **getResultSortProperties(RESULT * r)
{
    int     i;
    char  **props = NULL;       /* Array to Store properties */
    DB_RESULTS     *db_results = r->db_results;
    RESULTS_OBJECT *results = db_results->results;
    int            num_props = db_results->num_sort_props;

    if ( !num_props )
        return NULL;

    props = (char **) Mem_ZoneAlloc(results->resultSortZone, num_props * sizeof(char *));

    /* $$$ -- need to pass in the max string length */
    for (i = 0; i < num_props; i++)
        props[i] = getResultPropAsString(r, db_results->propIDToSort[i]);

    return props;
}




/***************************************************************************************
* sortresults - sorts each index file's list of results
*
*   Returns:
*       total results
*
*   ToDo:
*       1) when not using pre-sorted index then should use Props, not strings
*          this would allow correct sorting (THIS IS A BUG)
*       2) there's no reason[*] can't mix-n-match pre-sorted and not pre-sorted
*          just need an array of pointers, and an array of flages to indicate
*          if the pointer is to an int or to a Prop.
*
*       [*] well, it would make the qsort compare function more complicated
*           which is not the best for performance
*
*   rewritten Sept 2002 - moseley
*
****************************************************************************************/

int  sortresults(RESULTS_OBJECT *results)
{
    int         TotalResults = 0;
    DB_RESULTS *db_results;
    int        (*compResults) (const void *, const void *) = NULL;


    /* Sort each index file's resultlist */

    for (db_results = results->db_results; db_results; db_results = db_results->next)
    {
        RESULT  *cur_result;
        RESULT **sort_array;
        int      presorted_data_not_available = 0;
        int      files_in_index = 0;


        /* any results for this index? */
        
        if( !db_results->resultlist )
            continue;

        compResults = compResultsBySortedProps;  /* assume that can use pre-sorted data */
            
        cur_result = db_results->resultlist->head;

        while ( cur_result )
        {
            files_in_index++;
            
            if ( !(cur_result->iPropSort = getLookupResultSortedProperties(cur_result) ) )
            {
                presorted_data_not_available = 1;
                break;
            }
            cur_result = cur_result->next;
        }



        /* If some property didn't have a presorted index, then fall back to comparing the strings */
        /* $$$ should compare Prop's instead of strings so can compare case as specified in config */

        if ( presorted_data_not_available )
        {
            files_in_index = 0;
            compResults = compResultsByNonSortedProps;

            cur_result = db_results->resultlist->head;

            while ( cur_result )
            {
                files_in_index++;
                cur_result->PropSort = getResultSortProperties(cur_result);

                cur_result = cur_result->next;
            }
        }

        TotalResults += files_in_index;
        

        /* Now build an array to hold the results for sorting */

        sort_array = (RESULT **) emalloc(files_in_index * sizeof(RESULT *));

        /* Build an array with the elements to compare and pointers to data */

        cur_result = db_results->resultlist->head;
        files_in_index = 0;

        while ( cur_result )
        {
            sort_array[files_in_index++] = cur_result;
            cur_result = cur_result->next;
        }
        
        /* Sort them */
        swish_qsort(sort_array, files_in_index, sizeof(RESULT *), compResults);

        


        /* Build the list -- the list is in reverse order, so build the list backwards */
        {
            RESULT *head = NULL;
            int j;

            for (j = 0; j < files_in_index; j++)
            {
                RESULT *r = sort_array[j];


                /* Now's a good time to normalize the rank as we are processing each result */

               
                /* Find the largest rank for scaling */
                if (r->rank > results->bigrank)
                    results->bigrank = r->rank;

                    
                if ( !head )             // first time
                {
                    head = r;
                    r->next = NULL;
                }
                else                    // otherwise, place this at the head of the list
                {
                    r->next = head;
                    head = r;
                }
                
            }
            db_results->sortresultlist = head;
            db_results->resultlist->head = head;
            db_results->currentresult = head;
        }


        /* Free the memory of the array */
        efree( sort_array );
    }



    /* set rank scaling factor based on the largest rank found of all results */

    if (results->bigrank)  
        results->rank_scale_factor = 10000000 / results->bigrank;
    else
        results->rank_scale_factor = 10000;
    

    
    return TotalResults;
}


