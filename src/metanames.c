
/*
   -- This module does metaname handling for swish-e
   -- 
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


   -- 2001-02-12 rasc    minor changes, concering the tolower problem 
                 (unsigned char) problem!!!

*/




#include "swish.h"
#include "mem.h"
#include "merge.h"
#include "string.h"
#include "docprop.h"
#include "metanames.h"
#include "dump.h"
#include "error.h"

typedef struct
{
    char   *metaName;
    int     metaType;           /* see metanames.h for values. All values must be "ored" */
}
defaultMetaNames;


/**************************************************************************
*   List of *internal* meta names
*
*   Note:
*       Removing any of these will prevent access for result output
*       Removing any of the "real" meta names will also prevent storing
*       of the data in the property file.
*       That is, they may be commented out and then selected in the
*       configuration file as needed.
*       Hard to imagine not wanting the doc path!
*
***************************************************************************/


static defaultMetaNames SwishDefaultMetaNames[] = {

    /* This is the default meta ID ( number 1 ) that plain text is stored as */
    { AUTOPROPERTY_DEFAULT,      META_INDEX },  /* REQUIRED */


    /* These are the "internal" meta names generated at search time they are all required */
    { AUTOPROPERTY_REC_COUNT,    META_PROP | META_INTERNAL | META_NUMBER },
    { AUTOPROPERTY_RESULT_RANK,  META_PROP | META_INTERNAL | META_NUMBER },
    { AUTOPROPERTY_FILENUM,      META_PROP | META_INTERNAL | META_NUMBER },
    { AUTOPROPERTY_INDEXFILE,    META_PROP | META_INTERNAL | META_STRING },

    /* These meta names "real" meta names that are available by default */
    /* These can be commented out (e.g. to save disk space) and added back in with PropertyNames */
    { AUTOPROPERTY_DOCPATH,      META_PROP | META_STRING },
    { AUTOPROPERTY_TITLE,        META_PROP | META_STRING | META_IGNORE_CASE },
    { AUTOPROPERTY_DOCSIZE,      META_PROP | META_NUMBER},
    { AUTOPROPERTY_LASTMODIFIED, META_PROP | META_DATE},
 // { AUTOPROPERTY_SUMMARY,      META_PROP | META_STRING},
 // { AUTOPROPERTY_STARTPOS,     META_PROP | META_NUMBER},  // should be added only if LST is selected
};

/* Add the Internal swish metanames to the index file structure */
void    add_default_metanames(IndexFILE * indexf)
{
    int     i;

    for (i = 0; i < sizeof(SwishDefaultMetaNames) / sizeof(SwishDefaultMetaNames[0]); i++)
        addMetaEntry(&indexf->header, SwishDefaultMetaNames[i].metaName, SwishDefaultMetaNames[i].metaType, 0);
}



/**************************************************************************
*   These next routines add a new property/metaname to the list
*
*
***************************************************************************/



/* Add an entry to the metaEntryArray if one doesn't already exist */


struct metaEntry *addMetaEntry(INDEXDATAHEADER *header, char *metaname, int metaType, int metaID)
{
    struct metaEntry *tmpEntry = NULL;
    char *metaWord;

    if (metaname == NULL || metaname[0] == '\0')
        progerr("internal error - called addMetaEntry without a name");


    metaWord = estrdup( metaname );
    strtolower(metaWord);


    /* See if there is a previous metaname with the same name */
//    tmpEntry = metaType & META_PROP
//               ? getPropNameByName(header, metaWord)
//               : getMetaNameByName(header, metaWord);
               

    if (!tmpEntry)              /* metaName not found - Create a new one */
        tmpEntry = addNewMetaEntry( header, metaWord, metaType, metaID);

    else
        /* This allows adding Numeric or Date onto an existing property. */
        /* Probably not needed */
        tmpEntry->metaType |= metaType;


    efree( metaWord );

    return tmpEntry;
        
}

static struct metaEntry *create_meta_entry( char *name )
{
    struct metaEntry *newEntry = (struct metaEntry *) emalloc(sizeof(struct metaEntry));

    memset(newEntry, 0, sizeof(struct metaEntry));
    newEntry->metaName = (char *) estrdup( name );
    return newEntry;
}
    


struct metaEntry *addNewMetaEntry(INDEXDATAHEADER *header, char *metaWord, int metaType, int metaID)
{
    int    metaCounter = header->metaCounter;
    struct metaEntry *newEntry;
    struct metaEntry **metaEntryArray = header->metaEntryArray;
    newEntry = create_meta_entry( metaWord );
    
    newEntry->metaType = metaType;

    /* If metaID is 0 asign a value using metaCounter */
    /* Loaded stored metanames from index specifically sets the metaID */
    
    newEntry->metaID = metaID ? metaID : metaCounter + 1;


    /* Create or enlarge the array, as needed */
    if(! metaEntryArray)
    {
        metaEntryArray = (struct metaEntry **) emalloc(sizeof(struct metaEntry *));
        metaCounter = 0;
    }
    else
        metaEntryArray = (struct metaEntry **) erealloc(metaEntryArray,(metaCounter + 1) * sizeof(struct metaEntry *));


    /* And save it in the array */
    metaEntryArray[metaCounter++] = newEntry;

    /* Now update the header */
    header->metaCounter = metaCounter;
    header->metaEntryArray = metaEntryArray;

    return newEntry;
}

/**************************************************************************
*   Clear in_tag flags on all metanames
*   The flags are used for indexing
*
***************************************************************************/


/** Lookup META_INDEX -- these only return meta names, not properties **/

void ClearInMetaFlags(INDEXDATAHEADER * header)
{
    int     i;

    for (i = 0; i < header->metaCounter; i++)
        header->metaEntryArray[i]->in_tag = 0;
}




/**************************************************************************
*   Initialize the property mapping array
*   Used to get the property seek pointers from the index file
*
*   THIS IS TEMPORARY until I break up the metanames and properties
*
***************************************************************************/

void init_property_list(INDEXDATAHEADER *header)
{
    int i;

    /* only needs to be called one time */
    if ( header->property_count )
        return;
 
    if ( header->propIDX_to_metaID )
        progerr("Called init_property_list with non-null header->propIDX_to_metaID");

    if ( !header->metaCounter )
    {
        header->property_count = -1;  
        return;
    }


    header->propIDX_to_metaID = emalloc( (1 + header->metaCounter) * sizeof( int ) );
    header->metaID_to_PropIDX = emalloc( (1 + header->metaCounter) * sizeof( int ) );

    for (i = 0; i < header->metaCounter; i++)
    {
        if (is_meta_property(header->metaEntryArray[i]) && !header->metaEntryArray[i]->alias && !is_meta_internal(header->metaEntryArray[i]) )
        {
            header->metaID_to_PropIDX[header->metaEntryArray[i]->metaID] = header->property_count;
            header->propIDX_to_metaID[header->property_count++] = header->metaEntryArray[i]->metaID;
        }
        else
            header->metaID_to_PropIDX[header->metaEntryArray[i]->metaID] = -1;
    }

    if ( !header->property_count )
        header->property_count = -1;
}

    


/**************************************************************************
*   These routines lookup either a property or a metaname
*   by its ID or name
*
*   The routines only look at either properites or metanames
*
*   Note: probably could save a bit by just saying that if not META_PROP then
*   it's a a meta index entry.  In otherwords, the type flag of zero could mean
*   META_INDEX, otherwise it's a PROPERTY.  $$$ todo...
*
*
***************************************************************************/


/** Lookup META_INDEX -- these only return meta names, not properties **/

struct metaEntry *getMetaNameByNameNoAlias(INDEXDATAHEADER * header, char *word)
{
    int     i;

    for (i = 0; i < header->metaCounter; i++)
        if (is_meta_index(header->metaEntryArray[i]) && !strcmp(header->metaEntryArray[i]->metaName, word))
            return header->metaEntryArray[i];

    return NULL;
}


/* Returns the structure associated with the metaName if it exists
*  Requests for Aliased names returns the base meta entry, not the alias meta entry.
*  Note that on a alias it checks the *alias*'s type, so it must match.
*/

struct metaEntry *getMetaNameByName(INDEXDATAHEADER * header, char *word)
{
    int     i;

    for (i = 0; i < header->metaCounter; i++)
        if (is_meta_index(header->metaEntryArray[i]) && !strcmp(header->metaEntryArray[i]->metaName, word))
            return header->metaEntryArray[i]->alias
                   ? getMetaNameByID( header, header->metaEntryArray[i]->alias )
                   : header->metaEntryArray[i];

    return NULL;
}


/* Returns the structure associated with the metaName ID if it exists
*/

struct metaEntry *getMetaNameByID(INDEXDATAHEADER *header, int number)
{
    int     i;

    for (i = 0; i < header->metaCounter; i++)
    {
        if (is_meta_index(header->metaEntryArray[i]) && number == header->metaEntryArray[i]->metaID)
            return header->metaEntryArray[i];
    }
    return NULL;
}



/** Lookup META_PROP -- these only return properties **/

struct metaEntry *getPropNameByNameNoAlias(INDEXDATAHEADER * header, char *word)
{
    int     i;

    for (i = 0; i < header->metaCounter; i++)
        if (is_meta_property(header->metaEntryArray[i]) && !strcmp(header->metaEntryArray[i]->metaName, word))
            return header->metaEntryArray[i];

    return NULL;
}


/* Returns the structure associated with the metaName if it exists
*  Requests for Aliased names returns the base meta entry, not the alias meta entry.
*  Note that on a alias it checks the *alias*'s type, so it must match.
*/

struct metaEntry *getPropNameByName(INDEXDATAHEADER * header, char *word)
{
    int     i;


    for (i = 0; i < header->metaCounter; i++)
        if (is_meta_property(header->metaEntryArray[i]) && !strcmp(header->metaEntryArray[i]->metaName, word))
            return header->metaEntryArray[i]->alias
                   ? getPropNameByID( header, header->metaEntryArray[i]->alias )
                   : header->metaEntryArray[i];

    return NULL;
}


/* Returns the structure associated with the metaName ID if it exists
*/

struct metaEntry *getPropNameByID(INDEXDATAHEADER *header, int number)
{
    int     i;

    for (i = 0; i < header->metaCounter; i++)
    {
        if (is_meta_property(header->metaEntryArray[i]) && number == header->metaEntryArray[i]->metaID)
            return header->metaEntryArray[i];
    }

    return NULL;
}





/* This is really used to check for seeing which internal metaname is being requested */

int is_meta_entry( struct metaEntry *meta_entry, char *name )
{
    return strcasecmp( meta_entry->metaName, name ) == 0;
}


/**************************************************************************
*   Free list of MetaEntry's
*
***************************************************************************/



/* Free meta entries for an index file */

void   freeMetaEntries( INDEXDATAHEADER *header )
{
    int i;

    /* Make sure there are meta names assigned */
    if ( !header->metaCounter )
        return; 


    /* should the elements be set to NULL? */
    for( i = 0; i < header->metaCounter; i++ )
    {
        struct metaEntry *meta = header->metaEntryArray[i];

        efree( meta->metaName );

#ifndef USE_BTREE
        if ( meta->sorted_data)
            efree( meta->sorted_data );
#endif

        if ( meta->inPropRange )
            efree( meta->inPropRange);

        if ( meta->loPropRange )
            freeProperty( meta->loPropRange );

        if ( meta->hiPropRange )
            freeProperty( meta->hiPropRange );

        if ( meta->extractpath_default )
            efree( meta->extractpath_default );
            

        efree( meta );
    }

    /* And free the pointer to the list */
    efree( header->metaEntryArray);
    header->metaEntryArray = NULL;
    header->metaCounter = 0;
}


/**************************************************************************
*   Check if should bump word position on this meta name
*
***************************************************************************/


int isDontBumpMetaName( struct swline *tmplist, char *tag)
{
char *tmptag;

    if (!tmplist) return 0;
    if (strcmp(tmplist->line,"*")==0) return 1;
    
    tmptag=estrdup(tag);
    tmptag=strtolower(tmptag);
    while(tmplist)
    {
    
        if( strcmp(tmptag,tmplist->line)==0 )
        {
            efree(tmptag);
            return 1;
        }
        tmplist=tmplist->next;
    }
    efree(tmptag);
    return 0;

}



