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
** module to limit within a range of properties
** Created June 10, 2001 - moseley
**
*/

#include "swish.h"
#include "swstring.h"
#include "mem.h"
#include "merge.h"      // why is this needed for docprop.h???
#include "search.h"
#include "docprop.h"
#include "index.h"
#include "metanames.h"
#include "compress.h"
#include "error.h"
#include "db.h"
#include "result_sort.h"
#include "swish_qsort.h"
#include "proplimit.h"


// #define DEBUGLIMIT

/*==================== These should be in other modules ================*/

/* Should be in docprop.c */

/*******************************************************************
*   Fetch a doc's properties by file number and metaID
*
*   Call with:
*       *sw
*       *indexf
*       filenum
*       metaID
*
*   Returns:
*       pointer to a docPropertyEntry or NULL if not found
*
********************************************************************/

static propEntry *GetPropertyByFile( IndexFILE *indexf, int filenum, int metaID )
{
    propEntry *d;
    FileRec fi;
    memset(&fi, 0, sizeof( FileRec ));
    fi.filenum = filenum;
    

    d = ReadSingleDocPropertiesFromDisk(indexf, &fi, metaID, MAX_SORT_STRING_LEN );
    freefileinfo(&fi);

    return d;
}

#ifdef DEBUGLIMIT
static void printdocprop( propEntry *d )
{
    char str[1000];
    int  j;

    for (j=0; j < d->propLen; j++)
        str[j] = (d->propValue)[j];

    str[ d->propLen ] = '\0';

    printf("%s (%d)", str, d->propLen );
}

static void printfileprop( SWISH *sw, IndexFILE *indexf, int filenum, int metaID )
{
    propEntry *d;

    if ( (d = GetPropertyByFile( indexf, filenum,metaID )))
        printdocprop( d );
    else
        printf("File %d does not have a property for metaID %d", filenum, metaID );

    freeProperty( d );
}
#endif
    



/*==============================================================*/
/*                 typedefs and structures                      */
/*==============================================================*/

/* This is used to for inverting the metaEntry->sorted_data array */
typedef struct LOOKUP_TABLE
{
    int filenum;
    unsigned long   sort;
} LOOKUP_TABLE;




/*==============================================================*/
/*                  Code                                        */
/*==============================================================*/


/*******************************************************************
* SwishResetSearchLimit  -- clears memory used by -L 
*
*   Call with:
*       SEARCH_OBJECT
*
*   Returns:
*       void
*
*   This clears up the input params, and the stored data created
*   by Prepare_PropLookup(), but does not clear the tables create
*   in the search object to hold the data.
*
********************************************************************/


void SwishResetSearchLimit( SEARCH_OBJECT *srch )
{
    IndexFILE  *indexf = srch->sw->indexlist;
    int         index_count = 0;
    int         metaID;
    

    if ( !srch->limits_prepared )
        return;


    /* Free up the input parameters */
    ClearLimitParams( srch->limit_params );
    srch->limit_params = NULL;

            

    /* Free up the stored limits for each meta entry */
    while ( indexf )
    {
        PROP_LIMITS *index_limits = srch->prop_limits[index_count++];

        for ( metaID = 0; metaID <= indexf->header.metaCounter; metaID++ )
        {
            if ( index_limits[metaID].inPropRange )
            {
                efree ( index_limits[metaID].inPropRange );
                index_limits[metaID].inPropRange = NULL;
            }

            if ( index_limits[metaID].loPropRange )
            {
                efree ( index_limits[metaID].loPropRange );
                index_limits[metaID].loPropRange = NULL;
            }

            if ( index_limits[metaID].hiPropRange )
            {
                efree ( index_limits[metaID].hiPropRange );
                index_limits[metaID].hiPropRange = NULL;
            }
        }
        

        indexf = indexf->next;
    }

    srch->limits_prepared = 0;

}

/*  ClearLimitParams -- used internally */


void ClearLimitParams( LIMIT_PARAMS *params )
{
    LIMIT_PARAMS  *tmp;


    while ( params ) {
        efree( params->propname );
        efree( params->lowrange );
        efree( params->highrange );
        tmp = (LIMIT_PARAMS *)params->next;
        efree( params );
        params = tmp;
    }
}





/*******************************************************************
*  SwishSetSearchLimit - add a limit parameter
*
*   Stores strings away for later processing
*   called from someplace?
*
*   Call with:
*       Three strings, first must be metaname.
*
*   Returns:
*       returns false (0) on failure
*       pointer to a LIMIT_PARAMS
*       errors do not return (doesn't do many checks)
*
*   ToDo:
*       Error checking, and maybe pass in a StringList
*
********************************************************************/
int SwishSetSearchLimit(SEARCH_OBJECT *srch, char *propertyname, char *low, char *hi)
{
    reset_lasterror( srch->sw );
    
    if ( srch->limits_prepared )
    {
        set_progerr( PROP_LIMIT_ERROR, srch->sw, "Limits have been prepared (and executed) -- call SwishResetSearchLimit() first" );
        return 0;
    }

    srch->limit_params = setlimit_params( srch->sw, srch->limit_params, propertyname, low, hi );
    return ( srch->sw->lasterror == 0 );

}

/* This just sets the LIMIT_PARAMS struct -- useful when don't have a SEARCH_OBJECT */

LIMIT_PARAMS *setlimit_params( SWISH *sw, LIMIT_PARAMS *params, char *propertyname, char *low, char *hi )
{
    LIMIT_PARAMS *newparam;
    LIMIT_PARAMS *head = params;


    /* Currently, can only limit by one property -- so check that one hasn't already been used */
    while ( params )
    {
        if (strcmp( (char *)params->propname, propertyname ) == 0)
            break;
        params = (LIMIT_PARAMS *)params->next;
    }


    if ( params )
    {
        set_progerr( PROP_LIMIT_ERROR, sw, "Property '%s' is already limited", propertyname );
        return 0;
    }
        


    newparam = emalloc( sizeof( LIMIT_PARAMS ) );
    
    newparam->propname = (unsigned char *)estrdup( propertyname );
    newparam->lowrange = (unsigned char *)estrdup( low );
    newparam->highrange = (unsigned char *)estrdup( hi );

    /* put at head of list */
    newparam->next = head;

    return newparam;
}



/*******************************************************************
*   This compares the user supplied value with a file's property
*   The file's property is looked up and then Compare_Properties is called
*
*   Call with:
*       *SWISH
*       *indexf
*       *propEntry key - compare key
*       *LOOKUP_TABLE - element containing file number
*
*   Returns:
*
********************************************************************/
static int test_prop( IndexFILE *indexf, struct metaEntry *meta_entry, propEntry *key, LOOKUP_TABLE *sort_array)
{
    propEntry *fileprop;
    int        cmp_value;

#ifdef DEBUGLIMIT
    {
        char *p = DecodeDocProperty( meta_entry, key );
        printf("test_prop comparing '%s' cmp '%s' with ", meta_entry->metaName, p);
        efree( p );
    }
#endif    
        
        

    if ( !(fileprop = GetPropertyByFile( indexf, sort_array->filenum, meta_entry->metaID )) )
    {
#ifdef DEBUGLIMIT
        printf("(no prop found for filenum %d) - return +1\n", sort_array->filenum );
#endif        

        /* No property found, assume it's very, very, small */
        return +1;
    }

#ifdef DEBUGLIMIT
    {
        char *p = DecodeDocProperty( meta_entry, fileprop );
        int i = Compare_Properties( meta_entry, key, fileprop  );
        printf("'%s' returning %d\n", p, i );
        efree( p );
    }
#endif    


    cmp_value = Compare_Properties( meta_entry, key, fileprop  );
    freeProperty( fileprop );
    return cmp_value;
    
}

    


/************************************************************************
* Adapted from: msdn, I believe...
*
*    Call with:
*       See below
*
*   Returns:
*       Exact match, true (but could be more than one match location
*       Between two, returns false and the lower position
*       Below list, returns false and -1
*       Above list, return false and numelements (one past end of array)
*
*   ToDo:
*       Check for out of bounds on entry as that may be reasonably common
*
***************************************************************************/

static int binary_search(
    IndexFILE *indexf,              // 
    LOOKUP_TABLE *sort_array,       // table to search through
    int numelements,                // size of table
    propEntry *key,                 // property to compare against
    struct metaEntry *meta_entry,   // associated meta entry (for metaType)
    int *result,                    // result is stored here
    int direction,                  // looking up (positive) looking down (negative)
    int *exact_match)               // last exact match found
{
    int low = 0;
    int high = numelements - 1;
    int num  = numelements;
    int mid;
    int cmp;
    unsigned int half;

    *exact_match = -1;

#ifdef DEBUGLIMIT
    printf("\nbinary_search looking for %s entry\n", ( direction > 0 ? "high" : "low" ) );
#endif    

    while ( low <= high )
    {
        if ( (half = num / 2) )
        {
            mid = low + (num & 1 ? half : half - 1);


            if ( (cmp = test_prop( indexf, meta_entry, key, &sort_array[mid] )) == 0 )
            {
                *exact_match = mid;  // exact match
                cmp = direction;     // but still look for the lowest/highest exact match.
            }

            if ( cmp < 0 )
            {
                high = mid - 1;
                 num = (num & 1 ? half : half - 1);
            }

            else // cmp > 0
            {
                low = mid + 1;
                num = half;
            }
         }
         else if (num)
         {
            if( (cmp = test_prop( indexf, meta_entry, key, &sort_array[low] )) ==0)
            {
                *result = low;
                return 1;
            }
            if ( cmp < 0 ) // this breaks need another compare
            {
                /* less than current, but is is greater */
                if ( low > 0 && (test_prop( indexf, meta_entry, key, &sort_array[low-1] ) < 0))
                    *result = low - 1;
                else
                    *result = low;
                return 0;
            }
            else
            {
                *result = low + 1;
                return 0;
            }
         }
         else // if !num
         {
            /* I can't think of a case for this to match?? */
            progwarn("Binary Sort issue - please report to swish-e list");
            *result = -1;
            return 0;
         }
     }
     *result = low;  // was high, but wasn't returning expected results
     return 0;
}
         

/*******************************************************************
*   This takes a *sort_array and the low/hi range of limits and marks
*   which files are in that range
*
*   Call with:
*       pointer to SWISH
*       pointer to the IndexFile
*       pointer to the LOOKUP_TABLE
*       *metaEntry
*       LIMIT_PARAMS (low/hi range)
*
*   Returns:
*       true if any in range, otherwise false
*
********************************************************************/
static int find_prop(IndexFILE *indexf,  LOOKUP_TABLE *sort_array, int num, PROP_LIMITS *prop_limits, struct metaEntry *meta_entry )
{
    int low, high, j;
    int foundLo, foundHi;
    int some_selected = 0;
    int exact_match;
    

    if ( !prop_limits->loPropRange )
    {
        foundLo = 1;    /* signal exact match */
        low = 0;        /* and start at beginning */
    }
    else
    {
        foundLo = binary_search(indexf, sort_array, num, prop_limits->loPropRange, meta_entry, &low, -1, &exact_match);

        if ( !foundLo && exact_match >= 0 )
        {
            low = exact_match;
            foundLo = 1;  /* mark as an exact match */
        }
    }



    if ( !prop_limits->hiPropRange )
    {
        foundHi = 1;    /* signal exact match */
        high = num -1;  /* and end very end */
    }
    else
    {
        foundHi = binary_search(indexf, sort_array, num, prop_limits->hiPropRange, meta_entry, &high, +1, &exact_match);

        if ( !foundHi && exact_match >= 0 )
        {
            high = exact_match;
            foundHi = 1;
        }
    }

#ifdef DEBUGLIMIT
    printf("Returned range %d - %d (exact: %d %d) cnt: %u\n", low, high, foundLo, foundHi, num );
#endif    

    /* both inbetween two adjacent entries */
    if ( !foundLo && !foundHi && low == high )
    {
        for ( j = 0; j < num; j++ )
            sort_array[j].sort = 0;

        return 0;
    }


    /* now, if not an exact match for the high range, decrease high by one
     * because high is pointing to the *next* higher element, which is TOO high
     */
     
    if ( !foundHi && low < high )
        high--;


    /* Now mark by file number if it's within the range or not */
    for ( j = 0; j < num; j++ )
    {
         if ( j >= low && j <= high )
         {
            sort_array[j].sort = 1;
            some_selected++;
         }
         else
            sort_array[j].sort = 0;
    }

    return some_selected;        

}

/* These sort the LOOKUP_TABLE */
int sortbysort(const void *s1, const void *s2)
{
    LOOKUP_TABLE *a = (LOOKUP_TABLE *)s1;
    LOOKUP_TABLE *b = (LOOKUP_TABLE *)s2;

    return a->sort - b->sort;
}

int sortbyfile(const void *s1, const void *s2)
{
    LOOKUP_TABLE *a = (LOOKUP_TABLE *)s1;
    LOOKUP_TABLE *b = (LOOKUP_TABLE *)s2;

    return a->filenum - b->filenum;
}


/*******************************************************************
*   This creates the lookup table for the range of values selected
*   and stores it in the MetaEntry
*
*   Call with:
*       pointer to SWISH
*       pointer to the IndexFile
*       *metaEntry
*       LIMIT_PARAMS (low/hi range)
*
*   Returns:
*       true if any were marked as found
*       false means no match
*
********************************************************************/

static int create_lookup_array( IndexFILE *indexf, PROP_LIMITS *prop_limits, struct metaEntry *meta_entry )
{
    LOOKUP_TABLE *sort_array;
    int      i;
    int     size = indexf->header.totalfiles;
    int     some_found;

    /* Now do the work of creating the lookup table */

    /* Create memory  -- probably could do this once and use it over and over */
    sort_array = (LOOKUP_TABLE *) emalloc( size * sizeof(LOOKUP_TABLE) );

    /* copy in the data to the sort array */
    for (i = 0; i < size; i++)
    {
        sort_array[i].filenum = i+1;
        sort_array[i].sort = meta_entry->sorted_data[i];
    }


    /* now sort by it's sort value */
    swish_qsort(sort_array, size, sizeof(LOOKUP_TABLE), &sortbysort);

    /* This marks in the new array which ones are in range */
    some_found = find_prop( indexf, sort_array, size, prop_limits, meta_entry );


#ifdef DEBUGLIMIT
    for (i = 0; i < size; i++)
    {
        printf("%d File: %d Sort: %lu : ", i, sort_array[i].filenum, sort_array[i].sort );
        printfileprop( sw, indexf, sort_array[i].filenum, meta_entry->metaID );
        printf("\n");
    }
#endif

    /* If everything in range, then don't even bother creating the lookup array */
    if ( some_found && sort_array[0].sort && sort_array[size-1].sort )
    {
        efree( sort_array );
        return 1;
    }


    /* sort back by file number */
    swish_qsort(sort_array, size, sizeof(LOOKUP_TABLE), &sortbyfile);


    /* allocate a place to save the lookup table */
    /* What size is best for speed?  bitvector for size would be good */

    prop_limits->inPropRange = (unsigned char *) emalloc( size * sizeof(char) );

    /* populate the array in the metaEntry */
    for (i = 0; i < size; i++)
        prop_limits->inPropRange[i] = (unsigned char)sort_array[i].sort;

    efree( sort_array );

    return some_found;        

}

/*******************************************************************
*   Encode parameters specified on -L command line into two propEntry's
*   which can be used to compare with a file's property
*
*   Call with:
*       *metaEntry  -> current meta entry 
*       *LIMIT_PARAMS     -> associated parameters
*
*   Returns:
*       True if a range was found, otherwise false.
*       sets sw->lasterror on failure
*
*
********************************************************************/
static int params_to_props( IndexFILE *indexf, PROP_LIMITS *prop_limits, struct metaEntry *meta_entry, LIMIT_PARAMS *param )
{
    int error_flag;
    unsigned char *lowrange  = param->lowrange;
    unsigned char *highrange = param->highrange;
    SWISH *sw = indexf->sw;

    /* properties do not have leading white space */


    /* Allow <= and >= in limits.  A NULL property means very low/very high */

    if ( (strcmp( "<=", (char *)lowrange ) == 0)   )
    {
        prop_limits->loPropRange = NULL; /* indicates very small */
        prop_limits->hiPropRange = CreateProperty( meta_entry, highrange, strlen( (char *)highrange ), 0, &error_flag );
    }

    else if ( (strcmp( ">=", (char *)lowrange ) == 0)   )
    {
        prop_limits->loPropRange = CreateProperty( meta_entry, highrange, strlen( (char *)highrange ), 0, &error_flag );
        prop_limits->hiPropRange = NULL; /* indicates very big */
    }

    else
    {
        prop_limits->loPropRange = CreateProperty( meta_entry, lowrange, strlen( (char *)lowrange ), 0, &error_flag );
        prop_limits->hiPropRange = CreateProperty( meta_entry, highrange, strlen( (char *)highrange ), 0, &error_flag );


        if ( !(prop_limits->loPropRange && prop_limits->hiPropRange) )
        {
            set_progerr(PROP_LIMIT_ERROR, sw, "Failed to set range for property '%s' values '%s' and '%s'", meta_entry->metaName, lowrange, highrange );
            return 0;
        }

        /* Validate range */
    
        if ( Compare_Properties( meta_entry, prop_limits->loPropRange, prop_limits->hiPropRange ) > 0 )
        {
            set_progerr(PROP_LIMIT_ERROR, sw, "Property '%s' value '%s' must be <= '%s'", meta_entry->metaName, lowrange, highrange );
            return 0;
        }
    }


    return (  prop_limits->loPropRange || prop_limits->hiPropRange );
}        


/*******************************************************************
*   Scans all the meta entries to see if any are limited, and if so, creates the lookup array
*
*   Call with:
*       pointer to SWISH
*       poinger to an IndexFile
*       user supplied limit parameters
*
*   Returns:
*       false if any arrays are all zero
*       no point in even searching.
*       (meaning that no possible matches exist)
*       but also return false on errors, caller must check sw->lasterror
*
*   ToDo:
*       This ONLY works if the limits are absolute -- that is
*       that you can't OR limits.  Will need fixing at some point
*
********************************************************************/
static int load_index( IndexFILE *indexf, PROP_LIMITS *prop_limits, LIMIT_PARAMS *params )
{
    struct metaEntry *meta_entry;
    LIMIT_PARAMS     *curp;
    PROP_LIMITS       *cur_prop_limits;
    int               found;
    SWISH            *sw = indexf->sw;
    
    

    curp = params;

    /* Look at each parameter */
    for (curp = params; curp; curp = curp->next )
    {
        found = 0;

        if ( !(meta_entry = getPropNameByName( &indexf->header, (char *)curp->propname )))
        {
            set_progerr( PROP_LIMIT_ERROR, sw, "Specified limit name '%s' is not a PropertyName", curp->propname );
            return 0;
        }


        /* This, of course, is not the truth -- but the only slightly useful would be filenum  */
        /* indexfile can be specified on the command line, rank and reccount is not really known */
       
        if ( is_meta_internal( meta_entry ) )
        {
            set_progerr( PROP_LIMIT_ERROR, sw, "Cannot limit by swish result property '%s'", curp->propname );
            return 0;
        }

        cur_prop_limits = &prop_limits[meta_entry->metaID];


        /* see if array has already been allocated (cached) */
        /* probably should catch this earlier */

        if ( cur_prop_limits->inPropRange )
            continue;



        /* Encode the parameters into properties for comparing, and store */

        if ( !params_to_props( indexf, cur_prop_limits, meta_entry, curp ) )
        {
            if ( sw->lasterror )  // check for failure
                return 0;
                
            continue;  /* This means that it failed to set a range */
        }
            

        /* load the sorted_data array, if not already done */

        if ( !meta_entry->sorted_data )
            if( !LoadSortedProps( sw, indexf, meta_entry ) )
                continue;  /* thus it will sort manually without pre-sorted index */


        /* Now create the lookup table in the metaEntry */
        /* A false return means that an array was built but it was all zero */
        /* No need to check anything else at this time, since can only AND -L options */
        /* i.e. = return No Results right away */
        /* This allows search.c to bail out early */

        if ( !create_lookup_array( indexf, cur_prop_limits, meta_entry ) )
            return 0;

    }

    return 1;  // ** flag that it's ok to continue the search.
        
}

/*******************************************************************
*   Prepares the lookup tables for every index
*
*   Call with:
*       pointer to SEARCH_OBJECT
*
*   Returns:
*       true if ok to continue search
*       false indicates that a lookup array was created, but it is all zero
*       indicating there will never be a match
*       ( this falls apart if allow OR limits )
*
*   Notes:
*       See note in search.c limit_result_list()
*       processed limits are stored in the search object
*
*
********************************************************************/

int Prepare_PropLookup(SEARCH_OBJECT *srch )
{
    int             total_indexes = 0;
    int             total_no_docs = 0;
    LIMIT_PARAMS   *params = srch->limit_params;
    SWISH          *sw = srch->sw;
    IndexFILE      *indexf = sw->indexlist;


    /* nothing to limit by */
    if ( !params )
        return 1;

    /* already prepared? */
    if ( srch->limits_prepared++ )
        return 1;


    /* process each index file */
    while ( indexf )
    {
        /* Prop limits is where to store the tables */
        PROP_LIMITS *prop_limits = srch->prop_limits[total_indexes++];

        if ( !load_index( indexf, prop_limits, params ) )
        {
            if ( sw->lasterror )    // check for error
                return 0;
                
            total_no_docs++;
        }
        indexf = indexf->next;
    }

    /* if all indexes are all no docs within limits, then return false */
    return total_indexes != total_no_docs;            

}

/*******************************************************************
*   Removes results that don't fit within the limit
*
*   Call with:
*       *SWISH - to read a file entry if pre-sorted data not available
*       IndexFILE = current index file
*       File number
*
*   Returns
*       true if file should NOT be included in results
*
*
********************************************************************/
int LimitByProperty( IndexFILE *indexf, PROP_LIMITS *prop_limits, int filenum )
{
    int j;
    struct metaEntry  *meta_entry;
    for ( j = 0; j < indexf->header.metaCounter; j++)
    {
         PROP_LIMITS *cur_limit;

        /* Look at all the properties */

        /* Should cache this in the index file, or is this fast enough? */
        if ( !(meta_entry = getPropNameByID( &indexf->header, indexf->header.metaEntryArray[j]->metaID )))
            continue;  /* continue if it's not a property */

        cur_limit = &prop_limits[meta_entry->metaID];            

        /* anything to check? */
        if ( !cur_limit->loPropRange && !cur_limit->hiPropRange )
            continue;
            



        /* If inPropRange is allocated then there is an array for limiting already created from the presorted data */

        if ( cur_limit->inPropRange )
        {
            if ( !cur_limit->inPropRange[filenum-1] )
                return 1;
            continue;
        }




        /* Otherwise, if either range is set, then use a manual lookup of the property */
        
        {
            int limit = 0;
            propEntry *prop = GetPropertyByFile( indexf, filenum, meta_entry->metaID );

            /* Return true (i.e. limit) if the file's prop is less than the low range */
            /* or if its property is greater than the high range */
            if (
                (Compare_Properties( meta_entry, prop, cur_limit->loPropRange ) < 0 ) ||
                (cur_limit->hiPropRange && (Compare_Properties( meta_entry, prop, cur_limit->hiPropRange ) > 0 ))
               )
                limit = 1;

            freeProperty( prop );

            /* If limit by this property, then return to limit right away */
            if ( limit )
                return 1;
        }
    }

    return 0;  /* don't limit by default */
}    


