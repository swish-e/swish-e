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
#include "metanames.h"


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
     docPropertyEntry **docProperties;
{
	/* delete the linked list of doc properties */
	docPropertyEntry *prop = NULL;

	prop = *docProperties;
	while (prop != NULL)
	{
		docPropertyEntry *nextOne = prop->next;
		efree(prop->propValue);
		efree(prop);

		prop = nextOne;
	}

	/* replace the ptr to the head of the list with a NULL */
	*docProperties = NULL;
}

/* #### Added propLen to allow binary data */
void addDocProperty(docPropertyEntry **docProperties, int metaName, unsigned char *propValue, int propLen)
{
	/* Add the given file/metaName/propValue data to the File object */
docPropertyEntry *docProp;
	/* new prop object */
	if(propLen)
	{
		docProp=(docPropertyEntry *) emalloc(sizeof(docPropertyEntry));
		docProp->metaName = metaName;
		docProp->propValue = (char *)emalloc(propLen);
		memcpy(docProp->propValue, propValue,propLen);
		docProp->propLen=propLen;
			
		/* insert at head of file objects list of properties */
		docProp->next = *docProperties;	/* "*docProperties" is the ptr to the head of the list */
		*docProperties = docProp;	/* update head-of-list ptr */
	}
}
/*#### */

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
/* #### Modified to use propLen */
unsigned char *storeDocProperties(docProperties, datalen)
docPropertyEntry *docProperties;
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
		len = docProperties->propLen;
		/* Realloc buffer size if needed */
		if(lenbuffer<(*datalen+len+8*2))
		{
			lenbuffer +=(*datalen)+len+8*2+500;
			buffer=erealloc(buffer,lenbuffer);
		}
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
		docProperties = docProperties->next;
	}
	return buffer;
}
/* #### */

/* #### Added propLen support and simplify it */
/* Read one entry and return it; also set the metaName.
 * In all cases, metaName will be zero when the end of the
 * set is reached.
 */
unsigned char* readNextDocPropEntry(char **buf, int *metaName, int *propLen)
{
char* propValueBuf=NULL;
int tempPropID;
int len;
char *p=*buf;
	uncompress2(tempPropID,p);
	if (!tempPropID) return NULL;		/* end of list */

	*metaName = --tempPropID;

	/* grab the data length */
	uncompress2(len,p);

	/* allocate buffer for prop value */
	/* BTW, len must no be 0 */
	propValueBuf=(char *)emalloc(len);
	memcpy(propValueBuf, p, len);
	p+=len;
	*propLen=len;
	*buf=p;
	return propValueBuf;
}
/* #### */

/*
 * Read the docProperties section that the buffer pointer is
 * currently pointing to.
 */
/* #### Added support for propLen */
docPropertyEntry *fetchDocProperties(char *buf)
{
docPropertyEntry *docProperties=NULL;
char* tempPropValue=NULL;
int tempPropLen=0;
int tempMetaName=0;

	/* read all of the properties */
	while((tempPropValue = readNextDocPropEntry(&buf, &tempMetaName, &tempPropLen)) && tempMetaName > 0)
	{
			/* add the entry to the list of properties */
		addDocProperty(&docProperties, tempMetaName, tempPropValue, tempPropLen );
		efree(tempPropValue);
	}
	return docProperties;
}
/* #### */


/* #### Added propLen support */
/* removed lookupDocPropertyValue. Not used */
/* #### */

int getnumPropertiesToDisplay(SWISH *sw)
{
	if(sw)
		return sw->numPropertiesToDisplay;
	return 0;
}

void addSearchResultDisplayProperty(SWISH *sw, char *propName)
{
IndexFILE *indexf;
	/* add a property to the list of properties that will be displayed */
	if (sw->numPropertiesToDisplay >= sw->currentMaxPropertiesToDisplay)
	{
		if(sw->currentMaxPropertiesToDisplay) {
			sw->currentMaxPropertiesToDisplay+=2;
			sw->propNameToDisplay=(char **)erealloc(sw->propNameToDisplay,sw->currentMaxPropertiesToDisplay*sizeof(char *));
			for(indexf=sw->indexlist;indexf;indexf=indexf->next)
				indexf->propIDToDisplay=(int *)erealloc(indexf->propIDToDisplay,sw->currentMaxPropertiesToDisplay*sizeof(int));
		} else {
			sw->currentMaxPropertiesToDisplay=5;
			sw->propNameToDisplay=(char **)emalloc(sw->currentMaxPropertiesToDisplay*sizeof(char *));
			for(indexf=sw->indexlist;indexf;indexf=indexf->next)
				indexf->propIDToDisplay=(int *)emalloc(sw->currentMaxPropertiesToDisplay*sizeof(int));
		}
	}
	sw->propNameToDisplay[sw->numPropertiesToDisplay++] = estrdup(propName);
}

void addSearchResultSortProperty(SWISH *sw, char *propName,int mode)
{
IndexFILE *indexf;
	/* add a property to the list of properties that will be displayed */
	if (sw->numPropertiesToSort >= sw->currentMaxPropertiesToSort)
	{
		if(sw->currentMaxPropertiesToSort) {
			sw->currentMaxPropertiesToSort+=2;
			sw->propNameToSort=(char **)erealloc(sw->propNameToSort,sw->currentMaxPropertiesToSort*sizeof(char *));
			for(indexf=sw->indexlist;indexf;indexf=indexf->next)
				indexf->propIDToSort=(int *)erealloc(indexf->propIDToSort,sw->currentMaxPropertiesToSort*sizeof(int));
			sw->propModeToSort=(int *)erealloc(sw->propModeToSort,sw->currentMaxPropertiesToSort*sizeof(int));
		} else {
			sw->currentMaxPropertiesToSort=5;
			sw->propNameToSort=(char **)emalloc(sw->currentMaxPropertiesToSort*sizeof(char *));
			for(indexf=sw->indexlist;indexf;indexf=indexf->next)
				indexf->propIDToSort=(int *)emalloc(sw->currentMaxPropertiesToSort*sizeof(int));
			sw->propModeToSort=(int *)emalloc(sw->currentMaxPropertiesToSort*sizeof(int));
		}
	}
	sw->propNameToSort[sw->numPropertiesToSort] = estrdup(propName);
	sw->propModeToSort[sw->numPropertiesToSort++] = mode;
}


int initSortResultProperties(SWISH *sw)
{
int i;
IndexFILE *indexf;
        if (sw->numPropertiesToSort == 0)
                return RC_OK;
        for (i = 0; i<sw->numPropertiesToSort; i++)
        {
                makeItLow(sw->propNameToSort[i]);
		/* Get ID for each index file */
		for(indexf=sw->indexlist;indexf;indexf=indexf->next)
		{
                	indexf->propIDToSort[i] = getMetaName(indexf, sw->propNameToSort[i]);
                	if (indexf->propIDToSort[i] == 1)
                	{
				sw->errorstr=BuildErrorString(sw->errorstr, &sw->lenerrorstr, "err: Unknown Sort property name \"%s\" in one of the index files\n.\n", sw->propNameToSort[i]);
				return (sw->lasterror=UNKNOWN_PROPERTY_NAME_IN_SEARCH_SORT);
                	}
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
IndexFILE *indexf;
int i;
	/* lookup selected property names */

	if (sw->numPropertiesToDisplay == 0)
		return RC_OK;
	for (i = 0; i<sw->numPropertiesToDisplay; i++)
	{
		makeItLow(sw->propNameToDisplay[i]);
		/* Get ID for each index file */
		for(indexf=sw->indexlist;indexf;indexf=indexf->next)
		{
			indexf->propIDToDisplay[i] = getMetaName(indexf, sw->propNameToDisplay[i]);
			if (indexf->propIDToDisplay[i] == 1)
			{
				sw->errorstr=BuildErrorString(sw->errorstr, &sw->lenerrorstr, "err: Unknown Display property name \"%s\"\n.\n", sw->propNameToDisplay[i]);
				return (sw->lasterror=UNKNOWN_PROPERTY_NAME_IN_SEARCH_DISPLAY);
			}
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

char **getResultProperties(SWISH *sw, IndexFILE *indexf, docPropertyEntry *docProperties)
{
int i;
char **props;      /* Array to Store properties */
docPropertyEntry *p;

	if (sw->numPropertiesToDisplay == 0)
		return NULL;

	props=(char **)emalloc(sw->numPropertiesToDisplay * sizeof(char *));
	for (i = 0; i<sw->numPropertiesToDisplay; i++)
	{
		for(p=docProperties;p;p=p->next)
		{
			if(indexf->propIDToDisplay[i]==p->metaName) break;
		}
		if(!p)
			props[i] = estrdup("");
		else
                	props[i] = getPropAsString(indexf,p);
	}
	return props;
}


char **getResultSortProperties(SWISH *sw, IndexFILE *indexf, docPropertyEntry *docProperties)
{
int i;
char **props=NULL;      /* Array to Store properties */
docPropertyEntry *p;

        if (sw->numPropertiesToSort == 0)
                return NULL;
	
	props=(char **) emalloc(sw->numPropertiesToSort*sizeof(char *));
        for (i = 0; i<sw->numPropertiesToSort; i++)
	{
		for(p=docProperties;p;p=p->next)
		{
			if(indexf->propIDToSort[i]==p->metaName) break;
		}
		if(!p)
			props[i] = estrdup("");
		else
                	props[i] = getPropAsString(indexf,p);
	}
	return props;
}

void swapDocPropertyMetaNames(docProperties, metaFile)
     docPropertyEntry *docProperties;
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
docPropertyEntry *DupProps(docPropertyEntry *dp)
{
docPropertyEntry *new=NULL,*tmp=NULL,*last=NULL;
	if(!dp) return NULL;
	while(dp)
	{
		tmp=emalloc(sizeof(docPropertyEntry));
		tmp->metaName=dp->metaName;
		tmp->propValue=emalloc(dp->propLen);
		memcpy(tmp->propValue,dp->propValue,dp->propLen);
		tmp->propLen=dp->propLen;
		tmp->next=NULL;
		if(!new) new=tmp;
		if(last) last->next=tmp;
		last=tmp;
		dp=dp->next;
	}
	return new;
}


/* Frees memory of vars used by Ouutput properties configuration */
void FreeOutputPropertiesVars(SWISH *sw)
{
int i;
IndexFILE *tmpindexlist;
		/* First the common part to all the index files */
	if (sw->propNameToDisplay) 
	{
		for(i=0;i<sw->numPropertiesToDisplay;i++)
			efree(sw->propNameToDisplay[i]);
		efree(sw->propNameToDisplay);
		sw->propNameToDisplay=NULL;
	}
	if (sw->propNameToSort) 
	{
		for(i=0;i<sw->numPropertiesToSort;i++)
			efree(sw->propNameToSort[i]);
		efree(sw->propNameToSort);
		sw->propNameToSort=NULL;
	}
	if (sw->propModeToSort) efree(sw->propModeToSort);sw->propModeToSort=NULL;
	sw->numPropertiesToDisplay=sw->currentMaxPropertiesToDisplay=sw->numPropertiesToSort=sw->currentMaxPropertiesToSort=0;
		/* Now the IDs of each index file */
	for(tmpindexlist=sw->indexlist;tmpindexlist;tmpindexlist=tmpindexlist->next)
	{
		if (tmpindexlist->propIDToDisplay) efree(tmpindexlist->propIDToDisplay);tmpindexlist->propIDToDisplay=NULL;
		if (tmpindexlist->propIDToSort) efree(tmpindexlist->propIDToSort);tmpindexlist->propIDToSort=NULL;
	}
}


/* Returns the contents of the property "propertyname" of result r */
/* Returns NULL if not found */
char * getResultPropertyByName (SWISH *sw, char *propertyname, RESULT *r)
{
int i;
char *p,*pname;
char *propdata;  /* default value to NULL */

		/* To avoid problems and for faster scan make propertyname */
		/* to lowercase */
	pname=estrdup(propertyname);     /* preserve original value */
	for(p=pname;*p;p++)*p=tolower((int)((char)p[0])); /* go lowercase */
		/* Search for the property name */
	for(i=0;i<sw->numPropertiesToDisplay;i++)
	{
		if(!strcmp(sw->propNameToDisplay[i],pname)) break;
	}

	if(i!=sw->numPropertiesToDisplay)    
		propdata = r->Prop[i];     /* found !! */
	else
		propdata = NULL;   /* not found */
	efree(pname);
	return propdata;
}

/* #### Function to format the property as a string */
char *getPropAsString(IndexFILE *indexf,docPropertyEntry *p)
{
char *s=NULL;
unsigned long i;
struct metaEntry *q;
	q=getMetaIDData(indexf,p->metaName); /* BTW metaName is de ID !!!*/
	if(!q) return estrdup("");

	if(is_meta_string(q))      /* check for ascii/string data */
	{
		s=bin2string(p->propValue,p->propLen);
	} else if(is_meta_date(q))  /* check for a date */
	{
		s=emalloc(20);
		i=*(unsigned long *)p->propValue;  /* read binary */
						  /* as unsigned long */
		UNPACKLONG(i);     /* Convert the portable number */
				/* Conver to ISO datetime */
		strftime(s,20,"%Y/%m/%d %H:%M:%S",(struct tm *)localtime((time_t *)&i));
	} else if(is_meta_number(q))  /* check for a number */
	{
		s=emalloc(14);
		i=*(unsigned long *)p->propValue;  /* read binary */
						  /* as unsigned long */
		UNPACKLONG(i);     /* Convert the portable number */
				/* Conver to ISO datetime */
		sprintf(s,"%.013lu",i);
	} else s=estrdup("");
	return s;
}
