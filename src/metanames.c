
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

typedef struct
{
    char   *metaName;
    int     metaType;           /* see metanames.h for values. All values must be "ored" */
    int     metaID;             /* see metanames.h for values */
}
defaultMetaNames;

/* List of the internal metanames */
/********************************************************/
/* IMPORTANT: DO NOT REMOVE ANY OF THE DEFAULTMATANAMES */
/* Change them if you like but do not remove them       */
/********************************************************/
static defaultMetaNames SwishDefaultMetaNames[] = {
    {AUTOPROPERTY_DOCPATH, META_PROP, AUTOPROP_ID__DOCPATH}, /* DocPath not indexed by default (META_INDEX is not set) */
    /* If you want filename indexed and searchable */
    /* just add META_INDEX to it */
    {AUTOPROPERTY_TITLE, META_PROP, AUTOPROP_ID__TITLE}, /* Title No index (META_INDEX is not set) */
    /* If you want title indexed and searchable */
    /* just add META_INDEX to it */
    {AUTOPROPERTY_DOCSIZE, META_PROP | META_NUMBER, AUTOPROP_ID__DOCSIZE}, /* File size */
    {AUTOPROPERTY_LASTMODIFIED, META_PROP | META_DATE, AUTOPROP_ID__LASTMODIFIED}, /* File date */
    {AUTOPROPERTY_SUMMARY, META_PROP, AUTOPROP_ID__SUMMARY}, /* Summary not indexed by default (META_INDEX is not set) */
    /* If you want summary indexed and searchable */
    /* just add META_INDEX to it */
    {AUTOPROPERTY_STARTPOS, META_PROP | META_NUMBER, AUTOPROP_ID__STARTPOS}, /* File start */
    {AUTOPROPERTY_INDEXFILE, META_PROP | META_NUMBER, AUTOPROP_ID__INDEXFILE}, /* It is here just for using a ID */
    {NULL, 0}                   /* End mark */
};

/* Add the Internal swish metanames to the index file structure */
void    add_default_metanames(IndexFILE * indexf)
{
    int     i,
            dummy,
            metaType,
            metaID;
    char   *metaName;

    for (i = 0; SwishDefaultMetaNames[i].metaName; i++)
    {
        metaName = estrdup(SwishDefaultMetaNames[i].metaName);
        metaType = SwishDefaultMetaNames[i].metaType;
        metaID = SwishDefaultMetaNames[i].metaID;
        addMetaEntry(&indexf->header, metaName, metaType, metaID, NULL, &dummy);
        efree(metaName);
    }
}


/* Returns the structure associated with the metaName if it exists
*/

struct metaEntry *getMetaNameData(INDEXDATAHEADER * header, char *word)
{
    int     i;

    for (i = 0; i < header->metaCounter; i++)
        if (!strcmp(header->metaEntryArray[i]->metaName, word))
            return header->metaEntryArray[i];
    return NULL;
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





/* Add an entry to the metaEntryArray with the given value and the
** appropriate index
*/
/* #### Changed the name isDocProp by metaType */

void    addMetaEntry(INDEXDATAHEADER *header, char *metaWord, int metaType, int metaID, int *sort_array, int *applyautomaticmetanames)
{
    struct metaEntry *tmpEntry;
    struct metaEntry *newEntry;

    if (metaWord == NULL || metaWord[0] == '\0')
        return;

    strtolower(metaWord);


    /* 06/00 Jose Ruiz - Check for automatic metanames
     */
    if (((int) strlen(metaWord) == 9) && memcmp(metaWord, "automatic", 9) == 0)
    {
        *applyautomaticmetanames = 1;
        return;
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
        newEntry = (struct metaEntry *) emalloc(sizeof(struct metaEntry));

        newEntry->metaType = 0;
        newEntry->sorted_data = NULL;
        newEntry->inPropRange = NULL;  /* for limiting by this property - moseley */
        newEntry->metaName = (char *) estrdup(metaWord);

        if (metaID)
            newEntry->metaID = metaID;
        else
            newEntry->metaID = header->metaCounter + AUTOPROP_ID__DOCPATH; /* DOCPATH is the first metaname */

         /* what, no sw->verbose available?
         printf("Added '%s' into metaEntryArray[%d] with metaID = %d\n", metaWord,  header->metaCounter, newEntry->metaID );
         */

        /* Add at the end of the list of metanames */
        if (header->metaCounter)
            header->metaEntryArray = (struct metaEntry **) erealloc(header->metaEntryArray, (header->metaCounter + 1) * sizeof(struct metaEntry *));

        else
            header->metaEntryArray = (struct metaEntry **) emalloc(sizeof(struct metaEntry *));

        header->metaEntryArray[header->metaCounter++] = newEntry;
        tmpEntry = newEntry;
    }


    /* Array table of filenums - Default value NULL means no yet sorted */
    tmpEntry->sorted_data = sort_array;
    /* Add metaType info */
    tmpEntry->metaType |= metaType;


    /* Assign internal metanames if found */
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

    /* #### */
}

/* Free meta entries for an index file */

void   freeMetaEntries( INDEXDATAHEADER *header )
{
    int i;

    /* Make sure there are meta names assigned */
    if ( !header->metaCounter )
        return; 


    for( i = 0; i < header->metaCounter; i++ )
    {
        struct metaEntry *meta = header->metaEntryArray[i];

        efree( meta->metaName );

        if( meta->sorted_data)
            efree( meta->sorted_data );

        if( meta->inPropRange )
            efree( meta->inPropRange);

        efree( meta );
    }

    /* And free the pointer to the list */
    efree( header->metaEntryArray);
}


/* Returns the ID associated with the metaName if it exists
*/

int     getMetaNameID(indexf, word)
     IndexFILE *indexf;
     char   *word;
{
    int     i;

    for (i = 0; i < indexf->header.metaCounter; i++)
        if (strcmp(indexf->header.metaEntryArray[i]->metaName, word) == 0)
            return (indexf->header.metaEntryArray[i]->metaID);
    return 1;
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

