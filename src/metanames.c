#include "swish.h"
#include "mem.h"
#include "metanames.h"

typedef struct 
{
char *metaName;
int metaType;   /* see metanames.h for values. All values must be "ored" */
} defaultMetaNames;

/* List of the internal metanames */
/********************************************************/
/* IMPORTANT: DO NOT REMOVE ANY OF THE DEFAULTMATANAMES */
/* Change them if you like but do not remove them       */
/********************************************************/
static defaultMetaNames SwishDefaultMetaNames[]={
    {META_FILENAME, META_PROP}, /* Filename No index (META_INDEX is not set) */
				/* If you want filename indexed and searchable*/
				/* just add META_INDEX to it */
    {META_TITLE, META_PROP}, /* Title No index (META_INDEX is not set) */
				/* If you want title indexed and searchable */
				/* just add META_INDEX to it */
    {META_FILEDATE, META_PROP | META_DATE},     /* File date */
    {META_START,META_PROP | META_NUMBER},     /* File start */
    {META_SIZE,META_PROP | META_NUMBER},     /* File size */
    {META_SUMMARY, META_PROP}, /* Summary No index (META_INDEX is not set) */
				/* If you want summary indexed and searchable */
				/* just add META_INDEX to it */
    {NULL,0}                    /* End mark */
};

/* Add the Internal swish metanames to the index file structure */
void add_default_metanames(IndexFILE *indexf)
{
int i,dummy,metaType;
char *metaName;
	for(i=0;SwishDefaultMetaNames[i].metaName;i++)
	{
		metaName=estrdup(SwishDefaultMetaNames[i].metaName);
		metaType=SwishDefaultMetaNames[i].metaType;
		addMetaEntry(indexf,metaName,metaType,&dummy);
		efree(metaName);
	}
}


/* Returns the structure associated with the metaName if it exists
*/

struct metaEntry * getMetaNameData(IndexFILE *indexf, char *word)
{
struct metaEntry* temp;

	for (temp = indexf->metaEntryList; temp != NULL; temp = temp->next) 
		if (!strcmp(temp->metaName, word))
			return temp;
	return NULL;
}

/* Returns the structure associated with the metaName ID if it exists
*/

struct metaEntry * getMetaIDData(IndexFILE *indexf, int number)
{
struct metaEntry* temp;

	for (temp = indexf->metaEntryList; temp != NULL; temp = temp->next) 
		if (temp->index==number)
			return temp;
	return NULL;
}



/* Add an entry to the metaEntryList with the given value and the
** appropriate index
*/
/* #### Changed the name isDocProp by metaType */
void addMetaEntry(IndexFILE *indexf, char *metaWord, int metaType, int *applyautomaticmetanames)
{
int i;
struct metaEntry* tmpEntry;
struct metaEntry* newEntry;
	
	if(metaWord == NULL || metaWord[0]=='\0') return;
	for( i=0; metaWord[i]; i++)
		metaWord[i] =  tolower(metaWord[i]);
	
	/* 06/00 Jose Ruiz - Check for automatic metanames
	*/
	if(((int)strlen(metaWord)==9) && memcmp(metaWord,"automatic",9)==0) {
		*applyautomaticmetanames =1;
		return;
	}
	if (indexf->Metacounter <2)
		indexf->Metacounter = 2;

/* #### Jose Ruiz - New Stuff. Use metaType */
	/* See if there is a previous metaname */
	for (tmpEntry=indexf->metaEntryList;tmpEntry;tmpEntry=tmpEntry->next)   
		if (strcmp(tmpEntry->metaName,metaWord)==0) break; /* found */

	if(!tmpEntry)      /* metaName not found - Create a new one */
	{
		newEntry=(struct metaEntry*) emalloc(sizeof(struct metaEntry));
		newEntry->metaType = 0;
		newEntry->metaName = (char*)estrdup(metaWord);
		newEntry->index = indexf->Metacounter++;
		newEntry->next = NULL;
			/* Add at the end of the list of metanames */
		if (indexf->metaEntryList)
		{
			for(tmpEntry=indexf->metaEntryList;tmpEntry->next!=NULL;tmpEntry=tmpEntry->next)
			;
			tmpEntry->next = newEntry;
		}
		else
			indexf->metaEntryList = newEntry;
		tmpEntry = newEntry;
	}
	/* Add metaType info */
	tmpEntry->metaType |= metaType;

	/* Asign internal metanames if found */
	if(strcmp(metaWord,META_FILENAME)==0) indexf->filenameProp=tmpEntry;
	else if(strcmp(metaWord,META_TITLE)==0) indexf->titleProp=tmpEntry;
	else if(strcmp(metaWord,META_FILEDATE)==0)indexf->filedateProp=tmpEntry;
	else if(strcmp(metaWord,META_START)==0) indexf->startProp=tmpEntry;
	else if(strcmp(metaWord,META_SIZE)==0) indexf->sizeProp=tmpEntry;
	else if(strcmp(metaWord,META_SUMMARY)==0) indexf->summaryProp=tmpEntry;

	/* #### */
}
