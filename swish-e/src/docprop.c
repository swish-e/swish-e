/*
** DocProperties.c, DocProperties.h
**
** Functions to manage the index's Document Properties 
**
** File Created.
** Mark Gaulin 11/24/98
**
** change sprintf to snprintf to avoid corruption, 
** and use MAXSTRLEN from swish.h instead of literal "200"
** SRE 11/17/99
**
** 04/00 Jose ruiz
** storeDocProperties and readNextDocPropEntry modified to store
** the int numbers compressed. This makes integers "portable"
**
** 04/00 Jose Ruiz
** Added sorting results by property
**
** 07/00 and 08/00 - Jose Ruiz
** Many modifications to make all functions thread safe
**
** 08/00 - Added ascending and descending capabilities in results sorting
*/

#include "swish.h"
#include "file.h"
#include "hash.h"
#include "mem.h"
#include "merge.h"
#include "error.h"
#include "search.h"
#include "index.h"
#include "string.h"
#include "docprop.h"
#include "error.h"
#include "compress.h"


/* 08/00 Jose Ruiz */
/* function for comparing data in order to
get sorted results with qsort (including combinations of asc and descending
fields */
/* The data in each pointer is as follows
<num_fileds><len_filed1><data_1><len_field_2><data_2> ....
the length of the fields  can be positive or negative depending
on ascending or descending sort */
int scompasc(const void *s1,const void *s2)
{
int i,rc,num_fields,len_field,sortmode;
char *ps1,*ps2;
	ps1=(char *) s1;
	ps2=(char *) s2;
	memcpy((char *)&num_fields,ps1,sizeof(int));
	ps1+=sizeof(int);
	ps2+=sizeof(int);
	for(i=0;i<num_fields;i++){
		memcpy((char *)&len_field,ps1,sizeof(int));
		ps1+=sizeof(int);	
		ps2+=sizeof(int);	
		if(len_field<0){
			len_field=-len_field;
			sortmode=-1;
		} else sortmode=1;
		if((rc=sortmode*memcmp(ps1,ps2,len_field)))return rc;
		ps1+=len_field;
		ps2+=len_field;
	}
        return 0;
}

void freeDocProperties(docProperties)
     struct docPropertyEntry **docProperties;
{
	/* delete the linked list of doc properties */
	struct docPropertyEntry *prop = NULL;

	prop = *docProperties;
	while (prop != NULL)
	{
		struct docPropertyEntry *nextOne = prop->next;
		efree(prop->propValue);
		efree(prop);

		prop = nextOne;
	}

	/* replace the ptr to the head of the list with a NULL */
	*docProperties = NULL;
}

void addDocProperty(docProperties, metaName, propValue)
struct docPropertyEntry **docProperties;
int metaName;
char* propValue;
{
	/* Add the given file/metaName/propValue data to the File object */
struct docPropertyEntry *docProp;
	/* new prop object */
	docProp = (struct docPropertyEntry *) emalloc(sizeof(struct docPropertyEntry));
	docProp->metaName = metaName;
	docProp->propValue = (char *)estrdup(propValue);

	/* insert at head of file objects list of properties */
	docProp->next = *docProperties;	/* "*docProperties" is the ptr to the head of the list */
	*docProperties = docProp;	/* update head-of-list ptr */
}

/*
* Dump the document properties into the index file 
* The format is:
*	<PropID:int><PropValueLen:int><PropValue:not-null-terminated>
*	  ...
*	<PropID:int><PropValueLen:int><PropValue:not-null-terminated>
*	<EndofList:null char>
* All ints are compressed to save space. Jose Ruiz 04/00
*
* The list is terminated with a PropID with a value of zero
*/
char *storeDocProperties(docProperties, datalen)
struct docPropertyEntry *docProperties;
int *datalen;
{
int propID;
int len;
char *buffer,*p,*q;
int lenbuffer;
	buffer=emalloc((lenbuffer=MAXSTRLEN));
	*datalen=0;
	while (docProperties != NULL)
	{
		/* the length of the property value */
		len = strlen(docProperties->propValue);
		/* Realloc buffer size if needed */
		if(lenbuffer<(*datalen+len+8*2))
		{
			lenbuffer +=(*datalen)+len+8*2+500;
			buffer=erealloc(buffer,lenbuffer);
		}
		if (len > 0)
		{
			p= q = buffer + *datalen;
			/* the ID of the property */
			propID = docProperties->metaName;
			/* Do not store 0!! - compress does not like it */
			propID++;
			compress3(propID,p);
			/* including the length will make retrieval faster */
			compress3(len,p);
			memcpy(p,docProperties->propValue, len);
			*datalen += (p-q)+len;
		}
		docProperties = docProperties->next;
	}
	return buffer;
}

/* Read one entry and return it; also set the metaName.
 * if targetMetaName is zero then return values for all entries.
 * if targetMetaName is non-zero then only return the value
 * for the property matching that value.
 * In all cases, metaName will be zero when the end of the
 * set is reached.
 */
char* readNextDocPropEntry(buf, metaName, targetMetaName)
      char **buf;
      int *metaName;
      int targetMetaName;
{
char* propValueBuf=NULL;
int tempPropID;
int len;
char *p=*buf;
	uncompress3(tempPropID,p);
	if(tempPropID) tempPropID--;

	*metaName = tempPropID;

	if (tempPropID == 0)
		return NULL;		/* end of list */

	/* grab the string length */
	uncompress3(len,p);

	if ((targetMetaName != 0) && (tempPropID != (short int) targetMetaName))
	{
		/* we were looking for something specific, and this is not it */
		/* move to the next property */
		p+=len;
		propValueBuf=(char *)estrdup("");;
	}
	else
	{
		/* allocate buffer for prop value */
		propValueBuf=(char *)emalloc(len+1);
		memcpy(propValueBuf, p, len);
		p+=len;
		propValueBuf[len]='\0';
	}
	*buf=p;
	return propValueBuf;
}

/*
 * Read the docProperties section that the buffer pointer is
 * currently pointing to.
 */
struct docPropertyEntry *fetchDocProperties(buf)
     char *buf;
{
struct docPropertyEntry *docProperties=NULL;
char* tempPropValue=NULL;
int tempMetaName=0;
int targetMetaName = 0;	/* 0 = no target; return all data */

	/* read all of the properties */
	while((tempPropValue = readNextDocPropEntry(&buf, &tempMetaName, targetMetaName)) && tempMetaName > 0)
	{
			/* add the entry to the list of properties */
		addDocProperty(&docProperties, tempMetaName, tempPropValue);
		efree(tempPropValue);
	}
	return docProperties;
}


char* lookupDocPropertyValue(metaName, buf)
     int metaName;
     char *buf;
{
	/*
	 * Returns the string containing the document's
	 * property value, or an empty string if it was not found.
	 */
	char* tempPropValue=NULL;
	int tempMetaName=0;

	while((tempPropValue = readNextDocPropEntry(&buf, &tempMetaName, metaName)) && tempMetaName > 0)
	{
		/* a match? */
		if (tempMetaName == metaName)
			return tempPropValue;
		efree(tempPropValue);
	}
	return estrdup("");
}

int getnumPropertiesToDisplay(SWISH *sw)
{
	if(sw)
		return sw->numPropertiesToDisplay;
	return 0;
}

void addSearchResultDisplayProperty(SWISH *sw, char *propName)
{
	/* add a property to the list of properties that will be displayed */
	if (sw->numPropertiesToDisplay >= sw->currentMaxPropertiesToDisplay)
	{
		if(sw->currentMaxPropertiesToDisplay) {
			sw->currentMaxPropertiesToDisplay+=2;
			sw->propNameToDisplay=(char **)erealloc(sw->propNameToDisplay,sw->currentMaxPropertiesToDisplay*sizeof(char *));
			sw->propIDToDisplay=(int *)erealloc(sw->propIDToDisplay,sw->currentMaxPropertiesToDisplay*sizeof(int));
		} else {
			sw->currentMaxPropertiesToDisplay=5;
			sw->propNameToDisplay=(char **)emalloc(sw->currentMaxPropertiesToDisplay*sizeof(char *));
			sw->propIDToDisplay=(int *)emalloc(sw->currentMaxPropertiesToDisplay*sizeof(int));
		}
	}
	sw->propNameToDisplay[sw->numPropertiesToDisplay++] = propName;
}

void addSearchResultSortProperty(SWISH *sw, char *propName,int mode)
{
	/* add a property to the list of properties that will be displayed */
	if (sw->numPropertiesToSort >= sw->currentMaxPropertiesToSort)
	{
		if(sw->currentMaxPropertiesToSort) {
			sw->currentMaxPropertiesToSort+=2;
			sw->propNameToSort=(char **)erealloc(sw->propNameToSort,sw->currentMaxPropertiesToSort*sizeof(char *));
			sw->propIDToSort=(int *)erealloc(sw->propIDToSort,sw->currentMaxPropertiesToSort*sizeof(int));
			sw->propModeToSort=(int *)erealloc(sw->propModeToSort,sw->currentMaxPropertiesToSort*sizeof(int));
		} else {
			sw->currentMaxPropertiesToSort=5;
			sw->propNameToSort=(char **)emalloc(sw->currentMaxPropertiesToSort*sizeof(char *));
			sw->propIDToSort=(int *)emalloc(sw->currentMaxPropertiesToSort*sizeof(int));
			sw->propModeToSort=(int *)emalloc(sw->currentMaxPropertiesToSort*sizeof(int));
		}
	}
	sw->propNameToSort[sw->numPropertiesToSort] = propName;
	sw->propModeToSort[sw->numPropertiesToSort++] = mode;
}


int initSortResultProperties(SWISH *sw)
{
int i;
        if (sw->numPropertiesToSort == 0)
                return RC_OK;
        for (i = 0; i<sw->numPropertiesToSort; i++)
        {
                makeItLow(sw->propNameToSort[i]);
		/* Use ID from file 1 */
                sw->propIDToSort[i] = getMetaName(sw->indexlist, sw->propNameToSort[i]);
                if (sw->propIDToSort[i] == 1)
                {
                        sw->errorstr=BuildErrorString(sw->errorstr, &sw->lenerrorstr, "err: Unknown Sort property name \"%s\"\n.\n", sw->propNameToSort[i]);
                        return (sw->lasterror=UNKNOWN_PROPERTY_NAME_IN_SEARCH_SORT);
                }
        }
	return RC_OK;
}

int isSortProp(SWISH *sw)
{
	return sw->numPropertiesToSort;
}

int initSearchResultProperties(SWISH *sw)
{
	/* lookup selected property names */
	int i;

	if (sw->numPropertiesToDisplay == 0)
		return RC_OK;
	for (i = 0; i<sw->numPropertiesToDisplay; i++)
	{
		makeItLow(sw->propNameToDisplay[i]);
		/* use ID from file 1 */
		sw->propIDToDisplay[i] = getMetaName(sw->indexlist, sw->propNameToDisplay[i]);
		if (sw->propIDToDisplay[i] == 1)
		{
                        sw->errorstr=BuildErrorString(sw->errorstr, &sw->lenerrorstr, "err: Unknown Display property name \"%s\"\n.\n", sw->propNameToDisplay[i]);
			return (sw->lasterror=UNKNOWN_PROPERTY_NAME_IN_SEARCH_DISPLAY);
		}
	}
	return RC_OK;
}

void printSearchResultProperties(SWISH *sw, char **prop)
{
int i;
	if (sw->numPropertiesToDisplay == 0)
		return;

	for (i = 0; i<sw->numPropertiesToDisplay; i++)
	{
		char* propValue;
		propValue = prop[i];
		
		if (sw->useCustomOutputDelimiter)
			printf("%s", sw->customOutputDelimiter);
		else
			printf(" \"");	/* default is to quote the string, with leading space */

		/* print value, handling newlines and quotes */
		while (*propValue)

		{
			if (*propValue == '\n')
				printf(" ");
			else if (*propValue == '\"')	/* should not happen */
				printf("&quot;");
			else
				printf("%c", *propValue);
			propValue++;
		}
		printf("%s", propValue);

		if (!sw->useCustomOutputDelimiter)
			printf("\"");	/* default is to quote the string */
	}
}

char **getResultProperties(sw, docProperties)
SWISH *sw;
struct docPropertyEntry *docProperties;
{
int i;
char **props;      /* Array to Store properties */
struct docPropertyEntry *p;

	if (sw->numPropertiesToDisplay == 0)
		return NULL;

	props=(char **)emalloc(sw->numPropertiesToDisplay * sizeof(char *));
	for (i = 0; i<sw->numPropertiesToDisplay; i++)
	{
		for(p=docProperties;p;p=p->next)
		{
			if(sw->propIDToDisplay[i]==p->metaName) break;
		}
		if(!p)
			props[i] = estrdup("");
		else
                	props[i] = estrdup(p->propValue);
	}
	return props;
}


char **getResultSortProperties(sw, docProperties)
SWISH *sw;
struct docPropertyEntry *docProperties;
{
int i;
char **props=NULL;      /* Array to Store properties */
struct docPropertyEntry *p;

        if (sw->numPropertiesToSort == 0)
                return NULL;
	
	props=(char **) emalloc(sw->numPropertiesToSort*sizeof(char *));
        for (i = 0; i<sw->numPropertiesToSort; i++)
	{
		for(p=docProperties;p;p=p->next)
		{
			if(sw->propIDToSort[i]==p->metaName) break;
		}
		if(!p)
			props[i] = estrdup("");
		else
                	props[i] = estrdup(p->propValue);
	}
	return props;
}

void swapDocPropertyMetaNames(docProperties, metaFile)
     struct docPropertyEntry *docProperties;
     struct metaMergeEntry* metaFile;
{
	/* swap metaName values for properties */
	while (docProperties)
	{
		struct metaMergeEntry* metaFileTemp;
		/* scan the metaFile list to get the new metaName value */
		metaFileTemp = metaFile;
		while (metaFileTemp)
		{
			if (docProperties->metaName == metaFileTemp->oldIndex)
			{
				docProperties->metaName = metaFileTemp->newIndex;
				break;
			}

			metaFileTemp = metaFileTemp->next;
		}
		docProperties = docProperties->next;
	}
}

/* Jose Ruiz 04/00
** Sort results by property
*/
RESULT *sortresultsbyproperty(sw, structure)
SWISH *sw;
int structure;
{ 
int i, j, k, l;
unsigned char *ptmp,*ptmp2;
char *ps;
int tmp;
RESULT *pv;
RESULT *rtmp;
RESULT *sortresultlist;
RESULT *rp;
int MaxWide,*maxWide=NULL,len;
	rp=sw->resultlist;
		/* Trivial case */
	if (!rp) return NULL;
	if(sw->numPropertiesToSort) maxWide=(int *)emalloc(sw->numPropertiesToSort * sizeof(int));
	for (i = 0; i<sw->numPropertiesToSort; i++) maxWide[i] = 0;
	sortresultlist = NULL;
		/* Compute results */
	for(i=0,rtmp=rp;rtmp;rtmp = rtmp->next) {
		if (rtmp->structure & structure) {
			i++;
			for (l = 0; l<sw->numPropertiesToSort; l++){
				len=strlen(rtmp->PropSort[l]);
				if(len > maxWide[l]) maxWide[l] = len;
			}
		}
	}
		/* Another trivial case */
	if (!i) {
		if(maxWide) efree(maxWide);
		return NULL;
	}

	for (k = 0,MaxWide=0; k<sw->numPropertiesToSort; k++) MaxWide+=maxWide[k];
		/* We need to compute array wide 
		** including field number and each field size */
	MaxWide+=((sw->numPropertiesToSort+1)*sizeof(int));
	j=MaxWide+sizeof(void *);
		/* Compute array size */
	ptmp=(void *)emalloc((j*i));
		/* Build an array with the elements to compare
			 and pointers to data */
	for(ptmp2=ptmp,rtmp=rp;rtmp;rtmp = rtmp->next) {
		if (rtmp->structure & structure) {
			ps=(char *)ptmp2;
			memcpy((char *)ps,(char *)&sw->numPropertiesToSort,sizeof(int));
			ps+=sizeof(int);
			for(l=0;l<sw->numPropertiesToSort; l++){
				tmp=maxWide[l]*sw->propModeToSort[l];
				memcpy((char *)ps,(char *)&tmp,sizeof(int));
				ps+=sizeof(int);
				memset(ps,0,maxWide[l]); /* padded with 0 */
				len=strlen(rtmp->PropSort[l]);
				if(len)memcpy(ps,rtmp->PropSort[l],len);
				ps+=maxWide[l];
			}
			ptmp2+=MaxWide;
			memcpy((char *)ptmp2,(char *)&rtmp,sizeof(RESULT *));
			ptmp2+=sizeof(void *);
		}
	}
		/* Sort them */
	qsort(ptmp,i,j,&scompasc);
		/* Build the list */
	for(j=0,ptmp2=ptmp;j<i;j++){
		ps=(char *)ptmp2;
		ptmp2+=MaxWide;
		memcpy((char *)&pv,(char *)ptmp2,sizeof(RESULT *));
		ptmp2+=sizeof(void *);
		sortresultlist = (RESULT *) addsortresult(sw, sortresultlist, pv);
	}
		/* Free the memory of the array */
	efree(ptmp);

	efree(maxWide);
	return sortresultlist;
}

/* Duplicates properties (used by merge) */
struct docPropertyEntry *DupProps(struct docPropertyEntry *dp)
{
struct docPropertyEntry *new=NULL,*tmp=NULL,*last=NULL;
	if(!dp) return NULL;
	while(dp)
	{
		tmp=emalloc(sizeof(struct docPropertyEntry));
		tmp->metaName=dp->metaName;
		tmp->propValue=estrdup(dp->propValue);
		tmp->next=NULL;
		if(!new) new=tmp;
		if(last) last->next=tmp;
		last=tmp;
		dp=dp->next;
	}
	return new;
}
