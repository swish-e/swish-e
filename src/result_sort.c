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

#include "config.h"
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




/*****************************************************************************
* compare_results_single_index - qsort compare function
*
*  compares properties for a single index which means the results
*  share the same parent db_results structure and associated SortData.
*
*  Will lazy load properties as needed when primary key matches
*
*  Just to keep things confusing, +1 = desc sort, -1 = ascending
*  qsort's output is then reversed when rebuilding the linked list of results.
*
*
*****************************************************************************/
static int compare_results_single_index(const void *s1, const void *s2)
{
    RESULT    *r1 = *(RESULT * const *) s1;
    RESULT    *r2 = *(RESULT * const *) s2;
    int       i;
    int       rc;
    int       num_fields      = r1->db_results->num_sort_props;
    SortData  *sort_data;
    propEntry *prop_entry;
    int       *presorted;

    for (i = 0; i < num_fields; i++)
    {
        sort_data = &r1->db_results->sort_data[i];


        /* special case of sorting by (raw) rank */
        if ( sort_data->is_rank_sort )
        {
            if ( (rc = r1->rank - r2->rank) )
                return ( rc * sort_data->direction );
            else
                continue;
        }


        /* If haven't checked this property for a pre-sorted table then try to load the table */
        if ( !sort_data->checked_presorted )
        {
            sort_data->checked_presorted = 1;
            LoadSortedProps( r1->db_results->indexf, sort_data->property );
        }


        /* can we compare with pre-sorted numbers? (i.e. is the presorted data (sorted_data) loaded into the metaEntry?) */


        if ( (presorted = sort_data->property->sorted_data) )
        {
            if((rc = presorted[ r1->filenum - 1] - presorted[ r2->filenum - 1] ))
                return ( rc * sort_data->direction );  /* is the multiplication slow? */
        }


        else /* must compare properties directly */
        {
            /* First, does an array exist to hold the pointers to the properties? */
            if ( !sort_data->key )
            {
                sort_data->key = (propEntry **)emalloc( r1->db_results->result_count * sizeof( propEntry *) );
                memset( sort_data->key, -1, r1->db_results->result_count * sizeof( propEntry *) );
            }


            /* Now load the properties if they do not already exist (a -1 pointer indicates undefined) */
            /* tfrequency is used to store the index into the key (propery key) array */   
         
            if ( sort_data->key[ r1->tfrequency ] == (propEntry *)-1 )
                sort_data->key[ r1->tfrequency ] = getDocProperty( r1, &sort_data->property, 0, sort_data->property->sort_len );            

            if ( sort_data->key[ r2->tfrequency ] == (propEntry *)-1 )
                sort_data->key[ r2->tfrequency ] = getDocProperty( r2, &sort_data->property, 0, sort_data->property->sort_len );

            /* finally compare the properties */
            if ( (rc = Compare_Properties(  sort_data->property, sort_data->key[ r1->tfrequency ], sort_data->key[ r2->tfrequency ]) ) )
                return ( rc * sort_data->direction );
        }

    }
    return 0;
}
/*****************************************************************************
* compare_results
*
*  This code is used when tape-merging multiple indexes.  Almost the same
*  as above, but may be comparing results from different indexes must 
*  associate data with each result and can not use the presorted index tables.
*
*  Must make sure that the sort keys are the same for each index 
*  (e.g. same prop type, type of case compare)
*
*
*****************************************************************************/
int compare_results(const void *s1, const void *s2)
{
    RESULT    *r1 = *(RESULT * const *) s1;
    RESULT    *r2 = *(RESULT * const *) s2;
    int       i;
    int       rc;
    int       num_fields      = r1->db_results->num_sort_props;
    SortData  *sort_data1;
    SortData  *sort_data2;
    propEntry *prop_entry;
    int       *presorted;

    for (i = 0; i < num_fields; i++)
    {
        sort_data1 = &r1->db_results->sort_data[i];
        sort_data2 = &r2->db_results->sort_data[i];


        /* special case of sorting by (raw) rank */
        /* this is a bit more questionable because comparing raw ranks between two */
        /* indexes may not work well -- probably ok now with simple word-count based ranking */

        /* very unlikely that there is a property "swishrank" that was not the rank, so don't check both */
        if ( sort_data1 ->is_rank_sort )
        {
            if ( (rc = r1->rank - r2->rank) )
                return ( rc * sort_data1->direction );
            else
                continue;
        }



        /* First, does an array exist to hold the pointers to the properties for each result? */
        if ( !sort_data1->key )
        {
            sort_data1->key = (propEntry **)emalloc( r1->db_results->result_count * sizeof( propEntry *) );
            memset( sort_data1->key, -1, r1->db_results->result_count * sizeof( propEntry *) );
        }

        if ( !sort_data2->key )
        {
            sort_data2->key = (propEntry **)emalloc( r2->db_results->result_count * sizeof( propEntry *) );
            memset( sort_data2->key, -1, r2->db_results->result_count * sizeof( propEntry *) );
        }



        /* Now load the properties if they do not already exist (a -1 pointer indicates undefined) */
            
        if ( sort_data1->key[ r1->tfrequency ] == (propEntry *)-1 )
            sort_data1->key[ r1->tfrequency ] = getDocProperty( r1, &sort_data1->property, 0, sort_data1->property->sort_len );            

        if ( sort_data2->key[ r2->tfrequency ] == (propEntry *)-1 )
            sort_data2->key[ r2->tfrequency ] = getDocProperty( r2, &sort_data2->property, 0, sort_data2->property->sort_len );

        /* finally compare the properties */
        if ( (rc = Compare_Properties(  sort_data1->property, sort_data1->key[ r1->tfrequency ], sort_data2->key[ r2->tfrequency ]) ) )
           return ( rc * sort_data1->direction );

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
*       And it's name sucks.
*
********************************************************************/
int    *LoadSortedProps(IndexFILE * indexf, struct metaEntry *m)
{
    unsigned char *buffer;
    int     sz_buffer;
#ifndef USE_BTREE
    unsigned char *s;
    int     j;
#endif

    DB_InitReadSortedIndex(indexf->sw, indexf->DB);

    /* Get the sorted index of the property */

    /* Convert to a property index */
    DB_ReadSortedIndex(indexf->sw, m->metaID, &buffer, &sz_buffer, indexf->DB);

#ifndef USE_BTREE
    /* Table doesn't exist */
    if (!sz_buffer)
    {
        DB_EndReadSortedIndex(indexf->sw, indexf->DB);
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
    DB_EndReadSortedIndex(indexf->sw, indexf->DB);

    return m->sorted_data;
}


                                
/***************************************************************************************
* sort_single_index_results
*
*   Call with 
*       DB_RESULTS
*
*   Returns:
*       total results
*
*   This does all the work.  
*       - initializes an array of arrays to hold properties
*       - pre-load the array[0] elements with properties, if needed
*         (i.e. when the first sort key is non-presorted   
*
*   Todo:
*       This runs through the list results a number of times (plus qsort)
*
****************************************************************************************/
static int sort_single_index_results( DB_RESULTS *db_results )
{
    int lookup_props = 0;  /* flag if we need to load props initially */
    int results_in_index = 0;
    RESULT  *cur_result;
    RESULT **sort_array;
    propEntry *prop_entry;              
    SortData *sort_data;  

    /* Any results to process? */
    if( !db_results->resultlist )
        return 0;
        

    /* sanity checks */
    
    /* Should check for too big, too? */
    if ( db_results->num_sort_props < 1 )
        progerr("called sort_single_index_results with invalid number of sort keys");
    
    if ( ! db_results->sort_data )
        progerr("called sort_single_index_results without a vaild sort_data struct");
    


    /* Need to tally up the number of results in this set */
    /* $$$$ can search.c do this when creating results? It's an extra loop */
    /* perhaps it can't be done in search.c -- need to set an index number on the result */

    cur_result = db_results->resultlist->head;

    while ( cur_result )
    {
        /* Set an index number which can be used to point into the sort_data->key array. */
        /* tfrequency is not after rank calculations (or at all?) */

        cur_result->tfrequency = results_in_index++;

        cur_result = cur_result->next;
    }
    
    db_results->result_count = results_in_index;  /* needed so we know how big to create arrays */
    




    /* Do we need to lookup properties for the first key? */
    /* Not if sorting by rank (that's in the result) or if there's presorted data available */

    sort_data = &db_results->sort_data[0];



    if ( !sort_data->is_rank_sort && !sort_data->property->sorted_data )
        if ( !LoadSortedProps( db_results->indexf, sort_data->property ) )    /* can we load the array? */
        {
            /* otherwise, we must read all the properties off disk */

            lookup_props = 1;  


            /* Create the array the size of the number of results to hold *propEntry's */
            /* This array sticks around until freeing the results object */

            sort_data->key = (propEntry **)emalloc( db_results->result_count * sizeof( propEntry *) );
            memset( sort_data->key, -1, db_results->result_count * sizeof( propEntry *) );
        }

    sort_data->checked_presorted = 1; /* flag that this property has been checked for a pre-sorted index */
 


    /* Now build an array to hold the results for sorting */

    sort_array = (RESULT **) emalloc(db_results->result_count * sizeof(RESULT *));


    /* Fill the array for qsort -- and load the properties, if they are needed */
    /* This could be optimized for rank, since that's in the result structure */
    /* but would add complexity to the sort functions -- need to benchmark or profile */

    cur_result = db_results->resultlist->head;

    
    while ( cur_result )
    {
        sort_array[ cur_result->tfrequency ] = cur_result;

        /* If can't use the presorted numbers ("meta->sorted_data") then load the properties */
        if ( lookup_props )
            sort_data->key[ cur_result->tfrequency ] =
                getDocProperty( cur_result, &sort_data->property, 0, sort_data->property->sort_len );

        cur_result = cur_result->next;
    }
        
    /* Sort them */
    swish_qsort(sort_array, db_results->result_count, sizeof(RESULT *), compare_results_single_index);

        


    /* Build the list -- the list is in reverse order, so build the list backwards */
    {
        RESULT *head = NULL;
        int j;

        for (j = 0; j < db_results->result_count; j++)
        {
            RESULT *r = sort_array[j];


            /* Now's a good time to normalize the rank as we are processing each result */

             
            /* Find the largest rank for scaling */
            if (r->rank > db_results->results->bigrank)
                db_results->results->bigrank = r->rank;

                    
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

    
    return db_results->result_count;
}


/***************************************************************************************
* sortresults - sorts the results for one or more indexes searched
*
*   Call with 
*       A RESULTS_OBJECT which contains a list of DB_RESULTS
*
*   Returns:
*       total results
*
*   This simply loops through the indexex, and then scales the rank
*
*   rewritten Sept 2002 - moseley / again July 2003
*
****************************************************************************************/

int  sortresults(RESULTS_OBJECT *results)
{
    int         TotalResults = 0;
    DB_RESULTS *db_results = results->db_results;
    
    while ( db_results )
    {
        TotalResults += sort_single_index_results( db_results );
        db_results = db_results->next;
    }
    
    /* set rank scaling factor based on the largest rank found of all results */

    if (results->bigrank)
        results->rank_scale_factor = 10000000 / results->bigrank;
    else
        results->rank_scale_factor = 10000;


    return TotalResults;
}


