#include "swish.h"
#include "mem.h"
#include "metanames.h"

typedef struct 
{
char *metaName;
int metaType;   /* see metanames.h for values. All values must be "ored" */
} defaultMetaNames;

/* List of the internal metanames */
static defaultMetaNames SwishDefaultMetaNames[]={
    {META_FILEDATE,META_INDEX | META_PROP | META_DATE},     /* File date */
    {META_SIZE,META_INDEX | META_PROP | META_NUMBER},     /* File date */
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
