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
    along with Foobar; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    
    Swish-e includes a library for searching with a well-defined API. The library
    is named libswish-e.
    
    Linking libswish-e statically or dynamically with other modules is making a
    combined work based on Swish-e.  Thus, the terms and conditions of the GNU
    General Public License cover the whole combination.

    As a special exception, the copyright holders of Swish-e give you
    permission to link Swish-e with independent modules that communicate with
    Swish-e solely through the libswish-e API interface, regardless of the license
    terms of these independent modules, and to copy and distribute the
    resulting combined work under terms of your choice, provided that
    every copy of the combined work is accompanied by a complete copy of
    the source code of Swish-e (the version of Swish-e used to produce the
    combined work), being distributed under the terms of the GNU General
    Public License plus this exception.  An independent module is a module
    which is not derived from or based on Swish-e.

    Note that people who make modified versions of Swish-e are not obligated
    to grant this special exception for their modified versions; it is
    their choice whether to do so.  The GNU General Public License gives
    permission to release a modified version without this exception; this
    exception also makes it possible to release a modified version which
    carries forward this exception.
    
** Mon May  9 14:53:51 CDT 2005
** added GPL
**
** jmruiz - 02/2001 - Sorting results module
**
** 2001-05-04 jmruiz added new string comparison routines for proper sorting
**                   sw_strcasecmp and sw_strcmp
**                   also added the skeleton to initModule_ResultSort
**                   and freeModule_ResultSort
**
** 2001-05-05 rasc   just rearranged functions, to make modules look similar
**                   (makes code better to read and understand)
**
** 09/2002 - separated from result sorting code
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

// #define DEBUGSORT 1

/******************************************************
* Here's some static data to make the sort arrays smaller
* I don't think we need to worry about multi-threaded
* indexing at this time!
*******************************************************/

typedef struct
{
    PROP_INDEX  *prop_index;  /* cache of index pointers for this file */
    propEntry   *SortProp;    /* current property for this file */

#ifdef DEBUGSORT
    char *file_name;
#endif
} PROP_LOOKUP;

static struct metaEntry *CurrentPreSortMetaEntry;
static PROP_LOOKUP *PropLookup = NULL;




/*
** ----------------------------------------------
**
**  Module management code starts here
**
** ----------------------------------------------
*/





/*
  -- init structures for this module
*/

void    initModule_ResultSort(SWISH * sw)
{
    struct MOD_ResultSort *md;

    /* Allocate structure */
    md = (struct MOD_ResultSort *) emalloc(sizeof(struct MOD_ResultSort));

    sw->ResultSort = md;

    /* Init translation sortorder tables */
    initStrCmpTranslationTable(md->iSortTranslationTable);
    initStrCaseCmpTranslationTable(md->iSortCaseTranslationTable);

    /* Init data for -s command option */
    md->isPreSorted = 1;        /* Use presorted Index by default */
    md->presortedindexlist = NULL;

}


/*
  -- release all wired memory for this module
*/


/* Frees memory of vars used by ResultSortt properties configuration */
void    freeModule_ResultSort(SWISH * sw)
{
    struct MOD_ResultSort *md = sw->ResultSort;

    if (md->presortedindexlist)
        freeswline(md->presortedindexlist);

    efree(md);
    sw->ResultSort = NULL;
}


/*
** ----------------------------------------------
**
**  Module config code starts here
**
** ----------------------------------------------
*/


/*
 -- Config Directives
 -- Configuration directives for this Module
 -- return: 0/1 = none/config applied
*/

int     configModule_ResultSort(SWISH * sw, StringList * sl)
{
    struct MOD_ResultSort *md = sw->ResultSort;
    char   *w0 = sl->word[0];
    unsigned char *w1,
           *w2,
           *w3;
    int     retval = 1;
    int     incr = 0;
    int     i,
            j;
    struct swline *tmplist = NULL;
    struct metaEntry *m = NULL;


    if (strcasecmp(w0, "PreSortedIndex") == 0)
    {
        md->isPreSorted = sl->n - 1; /* If n is 1 (No properties specified) - Do not create presorted indexes */
        if (sl->n > 1)
        {
            grabCmdOptions(sl, 1, &md->presortedindexlist);
            /* Go lowercase  and check with properties */
            for (tmplist = md->presortedindexlist; tmplist; tmplist = tmplist->next)
            {
                (void)strtolower(tmplist->line);

                /* Check if it is in metanames list */
                if (!(m = getPropNameByName(&sw->indexlist->header, tmplist->line)))
                    progerr("%s: parameter is not a property", tmplist->line);
            }
        }
    }
    else if (strcasecmp(w0, "ResultSortOrder") == 0)
    {
        if (sl->n == 4)
        {
            w1 = (unsigned char *) sl->word[1];
            w2 = (unsigned char *) sl->word[2];
            w3 = (unsigned char *) sl->word[3];

            if (strlen( (char *)w1) != 1)
            {
                progerr("%s: parameter 1 must be one char length", w0);
            }
            if (strlen( (char *)w2 ) != 1)
            {
                progerr("%s: parameter 2 must be one char length", w0);
            }
            switch (w1[0])
            {
            case '=':
                incr = 0;
                break;
            case '>':
                incr = 1;
                break;
            default:
                progerr("%s: parameter 1 must be = or >", w0);
                break;
            }
            for (i = 0; w3[i]; i++)
            {
                j = (int) w2[0];
                md->iSortTranslationTable[(int) w3[i]] = md->iSortTranslationTable[j] + incr * (i + 1);

                md->iSortCaseTranslationTable[(int) w3[i]] = md->iSortCaseTranslationTable[j] + incr * (i + 1);
            }
        }
        else
            progerr("%s: requires 3 parameters (Eg: [=|>] a áàä)", w0);
    }
    else
    {
        retval = 0;             /* not a module directive */
    }
    return retval;
}


/*
** ----------------------------------------------
**
**  Module code starts here
**
** ----------------------------------------------
*/




/* 01/2001 Jose Ruiz */
/* function for comparing data in order to
get sorted results with qsort (including combinations of asc and descending
fields */
/***********************************************************************
* qsort compare function used for presorting the properties
*
************************************************************************/


static int     compFileProps(const void *s1, const void *s2)
{
    int         a = *(int *)s1;
    int         b = *(int *)s2;

#ifdef DEBUGSORT
    int  ret = Compare_Properties(CurrentPreSortMetaEntry, PropLookup[a].SortProp, PropLookup[b].SortProp );
    printf(" results: file %d [%s] (len %d) vs. %d [%s] (len %d).  Lower file = %s\n",
            a, PropLookup[a].file_name, PropLookup[a].SortProp ? PropLookup[a].SortProp->propLen : -1,
            b, PropLookup[b].file_name, PropLookup[b].SortProp ? PropLookup[b].SortProp->propLen : -1,

            !ret ? "*same*" : ret < 0 ? PropLookup[a].file_name : PropLookup[b].file_name );

    return ret;

#else
    return Compare_Properties(CurrentPreSortMetaEntry, PropLookup[a].SortProp, PropLookup[b].SortProp );
#endif
}


/***********************************************************************
* Checks if the property is set to be presorted
*
************************************************************************/

int     is_presorted_prop(SWISH * sw, char *name)
{
    struct MOD_ResultSort *md = sw->ResultSort;
    struct swline *tmplist = NULL;

    if (!md->isPreSorted)
        return 0;               /* Do not sort any property */
    else
    {
        if (!md->presortedindexlist)
            return 1;           /* All properties must be indexed */
        else
        {
            for (tmplist = md->presortedindexlist; tmplist; tmplist = tmplist->next)
                if (strcmp(name, tmplist->line) == 0)
                    return 1;
            return 0;
        }
    }
    return 0;
}


/***********************************************************************
* Pre sort a single property
*
************************************************************************/
int *CreatePropSortArray(IndexFILE *indexf, struct metaEntry *m, FileRec *fi, int free_cache )
{
    int             *sort_array = NULL;     /* array that gets sorted */
    int             *out_array = NULL;     /* array that gets sorted */
    int             total_files = indexf->header.totalfiles;
    int             i,
                    k;


    sort_array = emalloc( total_files * sizeof( long ) );
    out_array  = emalloc( total_files * sizeof( long ) );

    /* First time called, create place to cache property positions */
    if ( !PropLookup )
    {
        PropLookup = emalloc( total_files * sizeof( PROP_LOOKUP ));
        memset( PropLookup, 0, total_files * sizeof( PROP_LOOKUP ) );
    }


    /* This is need to know how to compare the properties */
    CurrentPreSortMetaEntry = m;


#ifdef DEBUGSORT
    {
        propEntry *d;
        FileRec fi;
        struct metaEntry *me = getPropNameByName( &indexf->header, "swishdocpath" );
        char *s;

        for (i = 0; i < total_files; i++)
        {
            memset(&fi, 0, sizeof( FileRec ));
            fi.filenum = i+1;

            d = ReadSingleDocPropertiesFromDisk(indexf, &fi, me->metaID, 0 );

            s = emalloc( d->propLen + 1 );
            memcpy( s, d->propValue, d->propLen );
            s[d->propLen] = '\0';

            PropLookup[i].file_name = s;
        }
    }
#endif



    /* Populate the arrays */

    for (i = 0; i < total_files; i++)
    {
        /* Here's a FileRec where the property index will get loaded */
        fi->filenum = i + 1;

        /* Used cached seek pointers for this file, if not the first time */
        if ( PropLookup[i].prop_index )
            fi->prop_index = PropLookup[i].prop_index;
        else
            fi->prop_index = NULL;


        PropLookup[i].SortProp = ReadSingleDocPropertiesFromDisk(indexf, fi, m->metaID, m->sort_len);
        PropLookup[i].prop_index = fi->prop_index;  // save it for next time
        sort_array[i] = i;
    }


    /* Sort them using qsort. The main work is done by compFileProps */
    swish_qsort( sort_array, total_files, sizeof( int ), &compFileProps);


    /* Build the sorted table */

    for (i = 0, k = 1; i < total_files; i++)
    {
        /* 02/2001 We can have duplicated values - So all them may have the same number asigned  - qsort justs sorts */
        if (i)
        {
            /* If consecutive elements are different increase the number */
            if ((compFileProps( &sort_array[i - 1], &sort_array[i])))
                k++;
        }

        out_array[ sort_array[i] ] = k;
    }

    efree( sort_array );


    if ( free_cache )
    {
        for (i = 0; i < total_files; i++)
            if ( PropLookup[i].prop_index )
                efree( PropLookup[i].prop_index );
        efree( PropLookup );
        PropLookup = NULL;
    }


    return out_array;
}


/***********************************************************************
* Pre sort all the properties
*
*
*
************************************************************************/


void    sortFileProperties(SWISH * sw, IndexFILE * indexf)
{
    int             i;
    int             *out_array = NULL;     /* array that gets sorted */
#ifndef USE_PRESORT_ARRAY
    unsigned char   *out_buffer  = NULL;
    unsigned char   *cur;
#endif
    struct metaEntry *m;
    int             props_sorted = 0;
    int             total_files = indexf->header.totalfiles;
    FileRec         fi;
    INDEXDATAHEADER *header = &indexf->header;
    int             propIDX;

    memset( &fi, 0, sizeof( FileRec ) );

#ifdef USE_PRESORT_ARRAY
    DB_InitWriteSortedIndex(sw, indexf->DB ,header->property_count);
#else
    DB_InitWriteSortedIndex(sw, indexf->DB );
#endif

    /* Any properties to check? */
    if ( header->property_count <= 0 )
    {
        DB_EndWriteSortedIndex(sw, indexf->DB);
        return;
    }


    /* Execute for each property */
    for (propIDX = 0; propIDX < header->property_count; propIDX++)
    {
        /* convert the count to a propID (metaID) */
        int metaID = header->propIDX_to_metaID[propIDX];

        if ( !(m = getPropNameByID(&indexf->header, metaID )))
            progerr("Failed to lookup propIDX %d (metaID %d)", propIDX, metaID );


        /* Check if this property must be in a presorted index */
        if (!is_presorted_prop(sw, m->metaName))
            continue;


        /* "internal" properties are sorted at runtime */
        if (is_meta_internal(m))
            continue;



        if (sw->verbose)
        {
#ifdef DEBUGSORT
            printf("\n-------------------\nSorting property: %s\n", m->metaName);
#else
            printf("Sorting property: %-40.40s\r", m->metaName);
#endif
            fflush(stdout);
        }

        out_array = CreatePropSortArray( indexf, m, &fi, 0 );


#ifdef USE_PRESORT_ARRAY
        DB_WriteSortedIndex(sw, metaID, out_array, total_files, indexf->DB);

        for (i = 0; i < total_files; i++)
            if ( PropLookup[i].SortProp )
                freeProperty( PropLookup[i].SortProp );
#else
        out_buffer = emalloc( total_files * MAXINTCOMPSIZE );


        /* Now compress */
        /* $$$ this should be in db_natvie.c */
        cur = out_buffer;

        for (i = 0; i < total_files; i++)
        {
            cur = compress3( out_array[i], cur );

            /* Free the property */
            if ( PropLookup[i].SortProp )
                freeProperty( PropLookup[i].SortProp );
        }


        DB_WriteSortedIndex(sw, metaID, out_buffer, cur - out_buffer, indexf->DB);

        efree( out_buffer );

#endif
        efree( out_array );

        props_sorted++;
    }

    DB_EndWriteSortedIndex(sw, indexf->DB);



    if ( props_sorted )
    {
        for (i = 0; i < total_files; i++)
            if ( PropLookup[i].prop_index )
                efree( PropLookup[i].prop_index );
        efree( PropLookup );
        PropLookup = NULL;
    }

    if (sw->verbose)
    {
        if ( !props_sorted )
            printf("No properties sorted.      %-40s\n", " ");
        else if ( props_sorted == 1 )
            printf("One property sorted.       %-40s\n", " ");
        else
            printf("%d properties sorted.      %-40s\n", props_sorted, " ");
    }
}


/* Routines to get the proper sortorder of chars to be called when sorting */
/* sw_strcasecmp sw_strcmp */

/*** $$$!!!$$$ I do not think any of these routines are used any more
 *  Better to use locale settings, IMO. - moseley 2/2005
 */


/* Exceptions to the standard translation table for sorting strings */
/* See initStrCaseCmpTranslationTable to see how it works */
/* The table shows the equivalences in the following way: */
/*     val(from) = val(order) + offset */
/* where val is asciivalue * 256 */

/* Some comments about äöü ...
** In french and spanish this chars are equivalent to
** ä -> a   (french)
** ö -> o   (french)
** ü -> u   (french + spanish)
** In the other hand, in german:
** ä -> a + 1  (german)
** ö -> o + 1  (german)
** ü -> u + 1  (german)
** I have put the german default. I think that in spanish we can live with that
** If you cannot modify them (change 1 by 0)
** Any comments about other languages are always welcome
*/
struct
{
    unsigned char from;
    unsigned char order;
    int     offset;
}
iTranslationTableExceptions[] =
{
    {'Ä', 'A', 1},                           /* >>> german sort order of umlauts */
    {'Ö', 'O', 1},                           /*     2001-05-04 rasc */
    {'Ü', 'U', 1},
    {'ä', 'a', 1},
    {'ö', 'o', 1},
    {'ü', 'u', 1},
    {'ß', 's', 1},                           /* <<< german */

    {'á', 'a', 0},                           /* >>> spanish sort order exceptions */
    {'Á', 'A', 0},                           /*     2001-05-04 jmruiz */
    {'é', 'e', 0},
    {'É', 'E', 0},
    {'í', 'i', 0},
    {'Í', 'I', 0},
    {'ó', 'o', 0},
    {'Ó', 'O', 0},
    {'ú', 'u', 0},
    {'Ú', 'U', 0},
    {'ñ', 'n', 1},
    {'Ñ', 'N', 1},                           /* <<< spanish */

    {'â', 'a', 0},                           /* >>> french sort order exceptions */
    {'Â', 'A', 0},                           /*     2001-05-04 jmruiz */
    {'à', 'a', 0},                           /*     Taken from the list - Please check */
    {'À', 'A', 0},                           /*     áéíóúÁÉÍÓÚ added in the spanish part */
    {'ç', 'c', 0},                           /*     äöüÄÖÜ added in the german part */
    {'Ç', 'C', 0},
    {'è', 'e', 0},
    {'È', 'E', 0},
    {'ê', 'e', 0},
    {'Ê', 'E', 0},
    {'î', 'i', 0},
    {'Î', 'I', 0},
    {'ï', 'i', 0},
    {'Ï', 'I', 0},
    {'ô', 'o', 0},
    {'Ô', 'O', 0},
    {'ù', 'u', 0},
    {'Ù', 'U', 0},                           /* <<< french */
    {0, 0, 0}
};

/* Initialization routine for the comparison table (ignoring case )*/
/* This routine should be called once  at the start of the module */
void    initStrCaseCmpTranslationTable(int *iCaseTranslationTable)
{
    int     i;

    /* Build default table using tolower(asciival) * 256 */
    /* The goal of multiply by 256 is having holes to put values inside
       eg: ñ is between n and o */
    for (i = 0; i < 256; i++)
        iCaseTranslationTable[i] = tolower(i) * 256;

    /* Exceptions */
    for (i = 0; iTranslationTableExceptions[i].from; i++)
        iCaseTranslationTable[iTranslationTableExceptions[i].from] =
            tolower(iTranslationTableExceptions[i].order) * 256 + iTranslationTableExceptions[i].offset;
}

/* Initialization routine for the comparison table (case sensitive) */
/* This routine should be called once at the start of the module */
void    initStrCmpTranslationTable(int *iCaseTranslationTable)
{
    int     i;

    /* Build default table using asciival * 256 */
    /* The goal of multiply by 256 is having holes to put values inside
       eg: ñ is between n and o */
    for (i = 0; i < 256; i++)
        iCaseTranslationTable[i] = i * 256;

    /* Exceptions */
    for (i = 0; iTranslationTableExceptions[i].from; i++)
        iCaseTranslationTable[iTranslationTableExceptions[i].from] =
            iTranslationTableExceptions[i].order * 256 + iTranslationTableExceptions[i].offset;
}

/* Comparison string routine function.
** Similar to strcasecmp but using our own translation table
*/
int     sw_strcasecmp(unsigned char *s1, unsigned char *s2, int *iTranslationTable)
{
    while (iTranslationTable[*s1] == iTranslationTable[*s2])
        if (!*s1++)
            return 0;
        else
            s2++;
    return iTranslationTable[*s1] - iTranslationTable[*s2];
}

/* Comparison string routine function.
** Similar to strcmp but using our own translation table
*/
int     sw_strcmp(unsigned char *s1, unsigned char *s2, int *iTranslationTable)
{
    while (iTranslationTable[*s1] == iTranslationTable[*s2])
        if (!*s1++)
            return 0;
        else
            s2++;
    return iTranslationTable[*s1] - iTranslationTable[*s2];
}
