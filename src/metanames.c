
/*
   -- This module does metaname handling for swish-e
   -- 
   -- License: see swish licence file


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


    /* These are the "internal" meta names generated at search time */
    { AUTOPROPERTY_REC_COUNT,    META_PROP | META_INTERNAL | META_NUMBER },
    { AUTOPROPERTY_RESULT_RANK,  META_PROP | META_INTERNAL | META_NUMBER },
    { AUTOPROPERTY_FILENUM,      META_PROP | META_INTERNAL | META_NUMBER },
    { AUTOPROPERTY_INDEXFILE,    META_PROP | META_INTERNAL },

    /* These meta names "real" meta names that are available by default */
    { AUTOPROPERTY_DOCPATH,      META_PROP},
    { AUTOPROPERTY_TITLE,        META_PROP},
    { AUTOPROPERTY_DOCSIZE,      META_PROP | META_NUMBER},
    { AUTOPROPERTY_LASTMODIFIED, META_PROP | META_DATE},
    { AUTOPROPERTY_SUMMARY,      META_PROP},
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

    newEntry->metaName = (char *) estrdup( name );
    newEntry->metaType = 0;
    newEntry->sorted_data = 0;
    newEntry->inPropRange = NULL;
    newEntry->loPropRange = NULL;
    newEntry->hiPropRange = NULL;
    newEntry->alias = 0;

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

        if ( meta->sorted_data)
            efree( meta->sorted_data );

        if ( meta->inPropRange )
            efree( meta->inPropRange);

        if ( meta->loPropRange )
            freeProperty( meta->loPropRange );

        if ( meta->hiPropRange )
            freeProperty( meta->hiPropRange );

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


int isDontBumpMetaName(SWISH *sw,char *tag)
{
struct swline *tmplist=sw->dontbumptagslist;
char *tmptag;
	if(!tmplist) return 0;
	tmptag=estrdup(tag);
	tmptag=strtolower(tmptag);
	while(tmplist)
	{
		if(strcmp(tmptag,tmplist->line)==0)
		{
			efree(tmptag);
			return 1;
		}
		tmplist=tmplist->next;
	}
	efree(tmptag);
	return 0;

}


