
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

typedef struct
{
    char   *metaName;
    int     metaType;           /* see metanames.h for values. All values must be "ored" */
    
}
defaultMetaNames;


/**************************************************************************
*   List of internal meta names
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
    { AUTOPROPERTY_STARTPOS,     META_PROP | META_NUMBER},
};

/* Add the Internal swish metanames to the index file structure */
void    add_default_metanames(IndexFILE * indexf)
{
    int     i,
            dummy;

    for (i = 0; i < sizeof(SwishDefaultMetaNames) / sizeof(SwishDefaultMetaNames[0]); i++)
        addMetaEntry(&indexf->header, SwishDefaultMetaNames[i].metaName, SwishDefaultMetaNames[i].metaType, 0, NULL, &dummy);

}


/* Returns the structure associated with the metaName if it exists
*  Requests for Aliased names returns the base meta entry, not the alias meta entry.
*/

struct metaEntry *getMetaNameData(INDEXDATAHEADER * header, char *word)
{
    int     i;

    for (i = 0; i < header->metaCounter; i++)
        if (!strcmp(header->metaEntryArray[i]->metaName, word))
            return header->metaEntryArray[i]->alias
                   ? getMetaIDData( header, header->metaEntryArray[i]->alias )
                   : header->metaEntryArray[i];

    return NULL;
}


struct metaEntry *getMetaNameDataNoAlias(INDEXDATAHEADER * header, char *word)
{
    int     i;

    for (i = 0; i < header->metaCounter; i++)
        if (!strcmp(header->metaEntryArray[i]->metaName, word))
            return header->metaEntryArray[i];

    return NULL;
}

/* Returns the ID associated with the metaName if it exists
   $$$ this can probably be replace with getMetaNameData
*/

int     getMetaNameID(IndexFILE *indexf, char * word)
{
    struct metaEntry *m;

    if ( (m = getMetaNameData( &indexf->header, word )) )
        return m->metaID;

    return 1;
}



/* Returns the structure associated with the metaName ID if it exists
*/

struct metaEntry *getMetaIDData(INDEXDATAHEADER *header, int number)
{
    int     i;

    for (i = 0; i < header->metaCounter; i++)
    {
        if (number == header->metaEntryArray[i]->metaID)
            return header->metaEntryArray[i];
    }
    return NULL;
}

int is_meta_entry( struct metaEntry *meta_entry, char *name )
{
    return strcasecmp( meta_entry->metaName, name ) == 0;
}





/* Add an entry to the metaEntryArray with the given value and the
** appropriate index
*/
/* #### Changed the name isDocProp by metaType */

struct metaEntry *addMetaEntry(INDEXDATAHEADER *header, char *metaname, int metaType, int metaID, int *sort_array, int *applyautomaticmetanames)
{
    struct metaEntry *tmpEntry;
    char *metaWord;

    if (metaname == NULL || metaname[0] == '\0')
        return NULL;

    metaWord = estrdup( metaname );
    strtolower(metaWord);


    /* 06/00 Jose Ruiz - Check for automatic metanames
     */
    if (((int) strlen(metaWord) == 9) && memcmp(metaWord, "automatic", 9) == 0)
    {
        *applyautomaticmetanames = 1;
        efree( metaWord );
        return NULL;
    }

    /* Hack to allow synonym in meta names */
    if ( !metaID && metaType == META_INDEX )
    {
        /* !metaname says this is not a meta name (useful for nested xml tags) */
        if ( metaWord[0] == '!' )
        {
            metaWord++;
            metaID = 1;
        }

        /* Allow synonyms in meta names with =metaname */
        if ( metaWord[0] == '=' )
        {
            metaWord++;
            metaID = header->metaEntryArray[header->metaCounter - 1]->metaID;
        }
    }
            
        

    /* #### Jose Ruiz - New Stuff. Use metaType */
    /* See if there is a previous metaname with the same name */
    tmpEntry = getMetaNameData(header, metaWord);

    if (!tmpEntry)              /* metaName not found - Create a new one */
    {
        header->metaEntryArray = addNewMetaEntry(header->metaEntryArray, &header->metaCounter, metaID, metaWord, metaType, sort_array);
        tmpEntry = getMetaNameData(header, metaWord);
    }
    else
    {
        /* Default value NULL means no yet sorted */
        tmpEntry->sorted_data = sort_array;
        /* Add metaType info */
        tmpEntry->metaType |= metaType;
	}


    /* Assign internal metanames if found */
    /* This just makes adding common properties a bit faster -- probably should just lookup the metaID */
    
    if (strcmp(metaWord, AUTOPROPERTY_DOCPATH) == 0)
        header->filenameProp = tmpEntry;
    else if (strcmp(metaWord, AUTOPROPERTY_TITLE) == 0)
        header->titleProp = tmpEntry;
    else if (strcmp(metaWord, AUTOPROPERTY_LASTMODIFIED) == 0)
        header->filedateProp = tmpEntry;
    else if (strcmp(metaWord, AUTOPROPERTY_STARTPOS) == 0)
        header->startProp = tmpEntry;
    else if (strcmp(metaWord, AUTOPROPERTY_DOCSIZE) == 0)
        header->sizeProp = tmpEntry;
    else if (strcmp(metaWord, AUTOPROPERTY_SUMMARY) == 0)
        header->summaryProp = tmpEntry;

    efree( metaWord );

    return tmpEntry;
        
}

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
    


struct metaEntry **addNewMetaEntry(struct metaEntry **metaEntryArray, int *metaCounter, int metaID, char *metaWord, int metaType, int *sort_array)
{
    struct metaEntry *newEntry;

    newEntry = create_meta_entry( metaWord );
    newEntry->metaType = metaType;
    newEntry->sorted_data = sort_array;
    

    /* If metaID is 0 asign a value using metaCounter */
    if (metaID)
        newEntry->metaID = metaID;
    else
        newEntry->metaID = (*metaCounter) + 1;

    if(! metaEntryArray)
    {
        metaEntryArray = (struct metaEntry **) emalloc(sizeof(struct metaEntry *));
        *metaCounter = 0;
    }
    else
        metaEntryArray = (struct metaEntry **) erealloc(metaEntryArray,(*metaCounter + 1) * sizeof(struct metaEntry *));

    metaEntryArray[(*metaCounter)++] = newEntry;

    return metaEntryArray;
}

