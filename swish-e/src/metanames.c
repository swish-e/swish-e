
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
char *metaName;
int metaType;   /* see metanames.h for values. All values must be "ored" */
int metaID;     /* see metanames.h for values */
} defaultMetaNames;

/* List of the internal metanames */
/********************************************************/
/* IMPORTANT: DO NOT REMOVE ANY OF THE DEFAULTMATANAMES */
/* Change them if you like but do not remove them       */
/********************************************************/
static defaultMetaNames SwishDefaultMetaNames[]={
    {AUTOPROPERTY_DOCPATH, META_PROP , AUTOPROP_ID__DOCPATH}, /* DocPath not indexed by default (META_INDEX is not set) */
				/* If you want filename indexed and searchable*/
				/* just add META_INDEX to it */
    {AUTOPROPERTY_TITLE, META_PROP, AUTOPROP_ID__TITLE}, /* Title No index (META_INDEX is not set) */
				/* If you want title indexed and searchable */
				/* just add META_INDEX to it */
	{AUTOPROPERTY_DOCSIZE, META_PROP | META_NUMBER, AUTOPROP_ID__DOCSIZE},     /* File size */
    {AUTOPROPERTY_LASTMODIFIED, META_PROP | META_DATE, AUTOPROP_ID__LASTMODIFIED},     /* File date */
    {AUTOPROPERTY_SUMMARY, META_PROP , AUTOPROP_ID__SUMMARY}, /* Summary not indexed by default (META_INDEX is not set) */
				/* If you want summary indexed and searchable */
				/* just add META_INDEX to it */
    {AUTOPROPERTY_STARTPOS, META_PROP | META_NUMBER, AUTOPROP_ID__STARTPOS},     /* File start */
	{AUTOPROPERTY_INDEXFILE, META_PROP | META_NUMBER, AUTOPROP_ID__INDEXFILE},     /* It is here just for using a ID */
    {NULL,0}                    /* End mark */
};

/* Add the Internal swish metanames to the index file structure */
void add_default_metanames(IndexFILE *indexf)
{
int i,dummy,metaType,metaID;
char *metaName;
	for(i=0;SwishDefaultMetaNames[i].metaName;i++)
	{
		metaName=estrdup(SwishDefaultMetaNames[i].metaName);
		metaType=SwishDefaultMetaNames[i].metaType;
		metaID=SwishDefaultMetaNames[i].metaID;
		addMetaEntry(indexf,metaName,metaType,metaID,0,&dummy);
		efree(metaName);
	}
}


/* Returns the structure associated with the metaName if it exists
*/

struct metaEntry * getMetaNameData(IndexFILE *indexf, char *word)
{
int i;
	for (i=0;i<indexf->metaCounter; i++) 
		if (!strcmp(indexf->metaEntryArray[i]->metaName, word))
			return indexf->metaEntryArray[i];
	return NULL;
}

/* Returns the structure associated with the metaName ID if it exists
*/

struct metaEntry * getMetaIDData(IndexFILE *indexf, int number)
{
int i;
	for(i=0;i<indexf->metaCounter;i++)
	{
		if (number == indexf->metaEntryArray[i]->metaID)
			return indexf->metaEntryArray[i];
	}
	return NULL;
}



/* Add an entry to the metaEntryArray with the given value and the
** appropriate index
*/
/* #### Changed the name isDocProp by metaType */
void addMetaEntry(IndexFILE *indexf, char *metaWord, int metaType, int metaID, long sort_offset, int *applyautomaticmetanames)
{
struct metaEntry* tmpEntry;
struct metaEntry* newEntry;
	
	if(metaWord == NULL || metaWord[0]=='\0') return;
	strtolower(metaWord);

	
	/* 06/00 Jose Ruiz - Check for automatic metanames
	*/
	if(((int)strlen(metaWord)==9) && memcmp(metaWord,"automatic",9)==0) {
		*applyautomaticmetanames =1;
		return;
	}

/* #### Jose Ruiz - New Stuff. Use metaType */
	/* See if there is a previous metaname with the same name */
	tmpEntry = getMetaNameData(indexf, metaWord);

	if(!tmpEntry)      /* metaName not found - Create a new one */
	{
		newEntry=(struct metaEntry*) emalloc(sizeof(struct metaEntry));
		newEntry->metaType = 0;
		newEntry->sorted_data = NULL;
		newEntry->metaName = (char*)estrdup(metaWord);
		if(metaID)
			newEntry->metaID = metaID;
		else
			newEntry->metaID = indexf->metaCounter + AUTOPROP_ID__DOCPATH;   /* DOCPATH is the first metaname */

			/* Add at the end of the list of metanames */
		if (indexf->metaCounter)
			indexf->metaEntryArray = (struct metaEntry **)erealloc(indexf->metaEntryArray,(indexf->metaCounter+1)*sizeof(struct metaEntry *));
		else
			indexf->metaEntryArray = (struct metaEntry **)emalloc(sizeof(struct metaEntry *));

		indexf->metaEntryArray[indexf->metaCounter++] = newEntry;
		tmpEntry = newEntry;
	} 
	/* Offset to sorted table of filenums - Default vaule 0L means no yet sorted */
	tmpEntry->sort_offset = sort_offset;   
	/* Add metaType info */
	tmpEntry->metaType |= metaType;


	/* Asign internal metanames if found */
	if(strcmp(metaWord,AUTOPROPERTY_DOCPATH)==0) indexf->filenameProp=tmpEntry;
	else if(strcmp(metaWord,AUTOPROPERTY_TITLE)==0) indexf->titleProp=tmpEntry;
	else if(strcmp(metaWord,AUTOPROPERTY_LASTMODIFIED)==0)indexf->filedateProp=tmpEntry;
	else if(strcmp(metaWord,AUTOPROPERTY_STARTPOS)==0) indexf->startProp=tmpEntry;
	else if(strcmp(metaWord,AUTOPROPERTY_DOCSIZE)==0) indexf->sizeProp=tmpEntry;
	else if(strcmp(metaWord,AUTOPROPERTY_SUMMARY)==0) indexf->summaryProp=tmpEntry;

	/* #### */
}


/* Returns the ID associated with the metaName if it exists
*/

int getMetaNameID(indexf, word)
IndexFILE *indexf;
char * word;
{
int i;

	for (i=0;i<indexf->metaCounter;i++) 
		if (strcmp(indexf->metaEntryArray[i]->metaName, word)==0)
			return (indexf->metaEntryArray[i]->metaID);
	return 1;
}
