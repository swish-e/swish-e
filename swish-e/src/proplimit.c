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
**
*/

#include <limits.h>     // for ULONG_MAX
#include "swish.h"
#include "string.h"
#include "mem.h"
#include "merge.h"      // why is this needed for docprop.h???
#include "docprop.h"
#include "index.h"
#include "metanames.h"
#include "compress.h"
#include "error.h"
#include "db.h"
#include "result_sort.h"
#include "proplimit.h"

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

static docPropertyEntry *GetPropertyByFile( SWISH *sw, IndexFILE *indexf, int filenum, int metaID )
{
    struct file *fi;
    docPropertyEntry *d;

    /* this will read all doc data from the index file or from cache */

    if ( !(fi = readFileEntry(sw, indexf, filenum)) )
        progerr("Failed to read file entry for file '%d'", filenum );
    
    for( d = fi->docProperties; d && d->metaID != metaID; d = d->next);

    return d;
}

#ifdef DEBUGLIMIT
static void printdocprop( docPropertyEntry *d )
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
    docPropertyEntry *d;

    d = GetPropertyByFile( sw, indexf, filenum,metaID );
    printdocprop( d );
}
#endif
    


/* This should be a routine in result_sort.c */

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
********************************************************************/
static int *LoadSortedProps( SWISH *sw, IndexFILE *indexf, struct metaEntry *m )
{
    unsigned char *buffer, *s;
    int     sz_buffer;
    int     j;
    int     tmp;

    DB_InitReadSortedIndex(sw, indexf->DB);
    
    /* Get the sorted index of the property */
    DB_ReadSortedIndex(sw, m->metaID, &buffer, &sz_buffer, indexf->DB);

    /* Table doesn't exist */
    if ( !sz_buffer )
        return NULL;


    s = buffer;
    m->sorted_data = (int *)emalloc( indexf->header.totalfiles * sizeof(int) );

    /* Unpack / decompress the numbers */
    for( j = 0; j < indexf->header.totalfiles; j++)
    {
        tmp = uncompress2(&s);
        m->sorted_data[j] = tmp;
    }
    efree(buffer);

    return m->sorted_data;
}


/* This should be a routine in result_sort.c, maybe? */

/*******************************************************************
*   Compares two properties for sorting
*
*   Call with:
*       *metaEntry
*       *docPropertyEntry1
*       *docPropertyEntry2
*
*   Returns:
*       0 - two properties are the same
*      -1 - docPropertyEntry1 < docPropertyEntry2
*       1 - docPropertyEntry1 ? docPropertyEntry2
*
*   QUESTION: ??? what if it's not string, date, or number?
*
********************************************************************/
static int Compare_Properties( struct metaEntry *meta_entry, char *p1, int len1, char *p2, int len2 )
{

    if (is_meta_number( meta_entry ) || is_meta_date( meta_entry ))
    {
        /* The PACKEDLONG is a string */
        return memcmp( (const void *)p1, (const void *)p2, sizeof( unsigned long ) );
    }


    if ( is_meta_string(meta_entry) )
    {
        int rc;
        int len = Min( len1, len2 );

        rc = strncasecmp(p1, p2, len);
        if ( rc != 0 )
            return rc;
            
        return len1 - len2;
    }

    return 0;

}


/* This probably should be in docprop.c */

/*******************************************************************
*   Converts a string into a string for saving as a property
*   Which means will either return a duplicated string,
*   or a packed unsigned long.
*
*   Call with:
*       *metaEntry
*       *string
*
*   Returns:
*       pointer to a new string -- caller needs to free the memory
*       errors return NULL (and a warning was displayed)
*
*   QUESTION: ???
*       should this return a *docproperty instead?
*       numbers are unsigned longs.  What if someone
*       wanted to store signed numbers?
*
*   ToDO:
*       need to add a directive so users can mark some properties
*       as numeric, so they will sort and limit correctly.
*
********************************************************************/
static char * EncodeProperty( struct metaEntry *meta_entry, char *string )
{
    unsigned long int num;
    char     *newstr;
    char     *badchar;
    char     *tmpnum;
    
    if ( !string || !*string )
    {
        progwarn("Null string passed to EncodeProperty");
        return NULL;
    }

    if (is_meta_number( meta_entry ) || is_meta_date( meta_entry ))
    {
        int j;

        newstr = emalloc( sizeof( num ) + 1 );
        num = strtoul( string, &badchar, 10 ); // would base zero be more flexible?
        if ( num == ULONG_MAX )
        {
            progwarnno("Attempted to convert '%s' to a number", string );
            return NULL;
        }

        if ( *badchar ) // I think this is how it works...
        {
            progwarn("Invalid char '%c' found in string '%s'", badchar[0], string);
            return NULL;
        }

        /* I'll bet there's an easier way */
        num = PACKLONG(num);
        tmpnum = (unsigned char *)&num;

        for ( j=0; j <= sizeof(num)-1; j++ )
            newstr[j] = (unsigned char)tmpnum[j];
        
        newstr[ sizeof(num) ] = '\0';

        return newstr;
    }
        

    if ( is_meta_string(meta_entry) )
        return estrdup( string );


    progwarn("EncodeProperty called but doesn't know the property type :(");
    return NULL;
}
            
            


    
    

/*==============================================================*/
/*                 typedefs and structures                      */
/*==============================================================*/

/* This is used to for inverting the metaEntry->sorted_data array */
typedef struct LOOKUP_TABLE
{
    int filenum;
    unsigned long   sort;
} LOOKUP_TABLE;


/* This is the list of parameters supplied with the query */
typedef struct
{
    struct PARAMS   *next;
    char    *propname;
    char    *lowrange;
    char    *highrange;
} PARAMS;




struct MOD_PropLimit
{
    PARAMS   *params;  /* parameter */
};



/*==============================================================*/
/*                  Code                                        */
/*==============================================================*/

void initModule_PropLimit (SWISH  *sw)
{
    /* local data */
    struct MOD_PropLimit *self;
    self =(struct MOD_PropLimit *) emalloc(sizeof(struct MOD_PropLimit));
    sw->PropLimit = self;


    self->params = NULL;
}


void freeModule_PropLimit (SWISH *sw)
{
    struct MOD_PropLimit *self = sw->PropLimit;
    PARAMS  *tmp;
    PARAMS  *tmp2;



    tmp = self->params;
    while ( tmp ) {
        efree( tmp->propname );
        efree( tmp->lowrange );
        efree( tmp->highrange );
        tmp2 = (PARAMS *)tmp->next;
        efree( tmp );
        tmp = tmp2;
    }

    efree( sw->PropLimit );
    sw->PropLimit = NULL;
    
}


/*******************************************************************
*   Stores strings away for later processing
*   called from someplace?
*
*   Call with:
*       Three strings, first must be metaname.
*
*   Returns:
*       ** no, it now returns void **
*       pointer to a PARAMS
*       errors do not return (doesn't do many checks)
*
*   ToDo:
*       Error checking, and maybe pass in a StringList
*
********************************************************************/
void SetLimitParameter(SWISH *sw, char *propertyname, char *low, char *hi)
{
    PARAMS *newparam;
    PARAMS *params;
    struct MOD_PropLimit *self = sw->PropLimit;


    for ( params = self->params; params && (strcmp( params->propname, propertyname ) != 0); params = (PARAMS *)params->next);
    if ( params )
        progerr("Only one limit per property '%s'", propertyname );
        


    newparam = emalloc( sizeof( PARAMS ) );
    
    newparam->propname = estrdup( propertyname );
    newparam->lowrange = estrdup( low );
    newparam->highrange = estrdup( hi );

    params = self->params;

    /* put at head of list */
    self->params = newparam;
    newparam->next = (struct PARAMS *)params;

}



/* here we lookup a property and compare it with the key */

static int test_prop( SWISH *sw, IndexFILE *indexf, struct metaEntry *meta_entry, char *key, LOOKUP_TABLE *sort_array)
{
    docPropertyEntry *fileprop;

    if ( !(fileprop = GetPropertyByFile( sw, indexf, sort_array->filenum, meta_entry->metaID )) )
        /* No property found, assume it's very, very, small */
        return -1;


/***** display the compare
    {
        int i = Compare_Properties( meta_entry, &testprop, fileprop );
        printdocprop(&testprop);
        printf(" cmp ");
        printdocprop(fileprop);
        printf(" = %d\n", i);
    }
***********/    
        

    return Compare_Properties( meta_entry, key, strlen(key), fileprop->propValue, fileprop->propLen  );
}

    


/************************************************************************
* Adapted from: msdn, I believe...
*
*    Call with:
*       sort_array = pointer to first element
*       numelements= number of elements in array
*       key        = string key to get passed to compare function
*       meta_entry = meta entry in question
*       *result    = place to store index of result
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

static int binary_sort(SWISH *sw, IndexFILE *indexf, LOOKUP_TABLE *sort_array, int numelements, char *key, struct metaEntry *meta_entry, int *result)
{
    int low = 0;
    int high = numelements - 1;
    int num  = numelements;
    int mid;
    int cmp;
    unsigned int half;

    /* Not sure if this will be any faster */
    if ( test_prop( sw, indexf, meta_entry, key, &sort_array[0] ) < 0 )
    {
        *result = 0;
        return 0;
    }
        
    if ( test_prop( sw, indexf, meta_entry, key, &sort_array[high] ) > 0 )
    {
        *result = num;
        return 0;
    }


    while ( low <= high )
    {
        if ( (half = num / 2) )
        {
            mid = low + (num & 1 ? half : half - 1);


            if ( (cmp = test_prop( sw, indexf, meta_entry, key, &sort_array[mid] )) == 0 )
            {
                *result = mid;  // exact match
                return 1;
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
            if( (cmp = test_prop( sw, indexf, meta_entry, key, &sort_array[low] )) ==0)
            {
                *result = low;
                return 1;
            }
            if ( cmp < 0 ) // this breaks need another compare
            {
                /* less than current, but is is greater */
                if ( low > 0 && (test_prop( sw, indexf, meta_entry, key, &sort_array[low-1] ) < 0))
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
*       PARAMS (low/hi range)
*
*   Returns:
*       true if any in range, otherwise false
*
********************************************************************/
static int find_prop(SWISH *sw, IndexFILE *indexf,  LOOKUP_TABLE *sort_array, int num, struct metaEntry *meta_entry, PARAMS *param  )
{
    int low, high, j;
    int foundLo, foundHi;
    char    *paramlo;
    char    *paramhi;
    int some_selected = 0;
    
#ifdef DEBUGLIMIT
    printf("Looking for %s in range %s - %s\n", meta_entry->metaName, param->lowrange, param->highrange );
#endif    

    /* First convert the input parameters into their packed form */
    paramlo = EncodeProperty( meta_entry, param->lowrange );
    paramhi = EncodeProperty( meta_entry, param->highrange );


    /* return false indicating that no match can be made */
    if ( !paramlo || !paramhi )
    {
        for ( j = 0; j < num; j++ )
            sort_array[j].sort = 0;

        efree( paramlo );
        efree( paramhi );
        return 0;
    }


    if ( Compare_Properties( meta_entry, paramlo, strlen(paramlo), paramhi, strlen(paramhi) ) > 0 )
        progerr("Property '%s' value '%s' must be <= '%s'", meta_entry->metaName, param->lowrange, param->highrange );

    foundLo = binary_sort(sw, indexf, sort_array, num, paramlo, meta_entry, &low);
    if ( foundLo ) // exact match
        while ( low > 0 && (test_prop( sw, indexf, meta_entry, paramlo, &sort_array[low-1] ) == 0))
            low--;



    foundHi = binary_sort(sw, indexf, sort_array, num, paramhi, meta_entry, &high);
    if ( foundHi )
        while ( high < num-1 && (test_prop( sw, indexf, meta_entry, paramhi, &sort_array[high+1] ) == 0))
            high++;

#ifdef DEBUGLIMIT
    printf("Returned range %d - %d (exact: %d %d)\n", low, high, foundLo, foundHi );
#endif    

    /* both inbetween */
    if ( !foundLo && !foundHi && low == high )
    {
        for ( j = 0; j < num; j++ )
            sort_array[j].sort = 0;

        efree( paramlo );
        efree( paramhi );
        return 0;
    }


    if ( !foundHi && low < high )
        high--;




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

    efree( paramlo );
    efree( paramhi );

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
*       PARAMS (low/hi range)
*
*   Returns:
*       true if any were marked as found
*       false means no match
*
********************************************************************/

static int create_lookup_array( SWISH *sw, IndexFILE*indexf, struct metaEntry *meta_entry, PARAMS *curp )
{
    LOOKUP_TABLE *sort_array;
    int      i;
    int     size = indexf->filearray_cursize;
    int     some_found;

    /* Now do the work of creating the lookup table */

    /* Create memory  -- probably could do this once and use it over and over */
    sort_array = (LOOKUP_TABLE *) emalloc( indexf->filearray_cursize * sizeof(LOOKUP_TABLE) );

    /* copy in the data to the sort array */
    for (i = 0; i < size; i++)
    {
        sort_array[i].filenum = i+1;
        sort_array[i].sort = meta_entry->sorted_data[i];
    }


    /* now sort by it's sort value */
    qsort(sort_array, size, sizeof(LOOKUP_TABLE), &sortbysort);



    /* This marks the array which ones are in range */
    some_found = find_prop( sw, indexf, sort_array, size, meta_entry, curp );

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
    qsort(sort_array, size, sizeof(LOOKUP_TABLE), &sortbyfile);


    /* allocate a place to save the lookup table */
    meta_entry->inPropRange = (int *) emalloc( size * sizeof(int) );

    /* populate the array in the metaEntry */
    for (i = 0; i < size; i++)
        meta_entry->inPropRange[i] = sort_array[i].sort;

    efree( sort_array );

    return some_found;        

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
*
*   ToDo:
*       This ONLY works if the limits are absolute -- that is
*       that you can OR limits.  Will need fixing at some point
*
********************************************************************/
static int load_index( SWISH *sw, IndexFILE *indexf, PARAMS *params )
{
    int j;
    struct metaEntry *meta_entry;
    PARAMS  *curp;
    int found;
    

    curp = params;

    /* Look at each parameter */
    while ( curp )
    {
        found = 0;


        /* look for a matching metaname */
        
        for ( j = 0; j < indexf->header.metaCounter; j++)
        {
            meta_entry =
                getMetaIDData( &indexf->header, indexf->header.metaEntryArray[j]->metaID );

            /* see if array has already been allocated (cached) */
            if ( meta_entry->inPropRange )
            {
                found++;
                break;
            }

            /* Is this our metaEntry? */
            if ( strcasecmp( curp->propname, meta_entry->metaName) != 0 )
                continue;
            if ( !is_meta_property( meta_entry ) )
                progerr("Name '%s' is not a PropertyName", curp->propname );


            found++;

            /* load the sorted_data array, if not already done*/
            if ( !meta_entry->sorted_data )
                if( !LoadSortedProps( sw, indexf, meta_entry ) )
                {
                    progwarn("Cannot limit by '%s'.  No pre-sorted data", meta_entry->metaName );
                    break;
                }


            /* Now create the lookup table in the metaEntry */
            /* A false return means that an array was built but it was all zero */
            /* No need to check anything else at this time, since can only AND -L options */
            if ( !create_lookup_array( sw, indexf, meta_entry, curp ) )
                return 0;

            break;  // out to next parameter
        }

        if ( !found )
            progerr("Specified limit name '%s' is not a PropertyName", curp->propname );

        (struct PARAMS *)curp = curp->next;
    }

    return 1;  // ** flag that it's ok to continue the search.
        
}

/*******************************************************************
*   Prepares the lookup tables for every index
*
*   Call with:
*       pointer to SWISH
*
*   Returns:
*       true if ok to continue search
*       false indicates that a lookup array was created, but it is all zero
*       indicating there will never be a match
*       ( this falls apart if allow OR limits )
*
*   ToDo:
*       How to deal with non-presorted properties?
*
********************************************************************/

int Prepare_PropLookup(SWISH *sw )
{
    IndexFILE *indexf;
    struct MOD_PropLimit *self = sw->PropLimit;
    int total_indexes = 0;
    int total_no_docs = 0;



    /* nothing to limit by */
    if ( !self->params )
        return 1;




    /* process each index file */
    for( indexf = sw->indexlist; indexf; indexf = indexf->next)
    {
        total_indexes++;
        
        if ( !load_index( sw, indexf, self->params ) )
            total_no_docs++;
    }

    /* if all indexes are all no docs within limits, then return false */
    return total_indexes != total_no_docs;            

}

/*******************************************************************
*   Removes results that don't fit within the limit
*
*   Call with:
*       IndexFILE = current index file
*       File number
*
*   Returns
*       true if file should NOT be included in results
*
*
********************************************************************/
int LimitByProperty( IndexFILE *indexf, int filenum )
{
    int j;
    struct metaEntry  *meta_entry;

    for ( j = 0; j < indexf->header.metaCounter; j++)
    {
        meta_entry =
            getMetaIDData( &indexf->header, indexf->header.metaEntryArray[j]->metaID );

        /* see if array has already been allocated (cached) */
        if ( meta_entry->inPropRange &&  !meta_entry->inPropRange[filenum-1] )
            return 1;
    }

    return 0;
}    

