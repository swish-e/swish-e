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
**
** 2001-01-27  rasc   getPropertyByName rewritten, datatypes for properties.
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


/* 01/01 Jose Ruiz */
/* function for comparing data in order to
get sorted results with qsort (including combinations of asc and descending
fields */
int compResultsBySortProps(const void *s1,const void *s2)
{
RESULT *r1=*(RESULT* const *)s1;
RESULT *r2=*(RESULT* const *)s2;
int i,rc,num_fields,sortmode;
SWISH *sw=(SWISH *)r1->sw;
	num_fields=sw->numPropertiesToSort;
	for(i=0;i<num_fields;i++){
		sortmode=sw->propModeToSort[i];
		if((rc=sortmode*strcmp(r1->PropSort[i],r2->PropSort[i])))return rc;
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
void addDocProperty(docPropertyEntry **docProperties, int metaID, unsigned char *propValue, int propLen)
{
	/* Add the given file/metaName/propValue data to the File object */
docPropertyEntry *docProp;
	/* new prop object */
	if(propLen)
	{
		docProp=(docPropertyEntry *) emalloc(sizeof(docPropertyEntry));
		docProp->metaID = metaID;
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
		propID = docProperties->metaID;
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
/* Read one entry and return it; also set the metaID.
 * In all cases, metaID will be zero when the end of the
 * set is reached.
 */
unsigned char* readNextDocPropEntry(char **buf, int *metaID, int *propLen)
{
char* propValueBuf=NULL;
int tempPropID;
int len;
char *p=*buf;
	uncompress2(tempPropID,p);
	if (!tempPropID) return NULL;		/* end of list */

	*metaID = --tempPropID;

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
                	indexf->propIDToSort[i] = getMetaNameID(indexf, sw->propNameToSort[i]);
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
			indexf->propIDToDisplay[i] = getMetaNameID(indexf, sw->propNameToDisplay[i]);
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
			if(indexf->propIDToDisplay[i]==p->metaID) break;
		}
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
			if(indexf->propIDToSort[i]==p->metaID) break;
		}
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
			if (docProperties->metaID == metaFileTemp->oldMetaID)
			{
				docProperties->metaID = metaFileTemp->newMetaID;
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
int i, j;
RESULT **ptmp;
int tmp;
RESULT *rtmp;
RESULT *sortresultlist;
RESULT *rp;
	rp=sw->resultlist;
		/* Trivial case */
	if (!rp) return NULL;
	sortresultlist = NULL;
		/* Compute number of results */
	for(i=0,rtmp=rp;rtmp;rtmp = rtmp->next) {
		if (rtmp->structure & structure) {
			i++;
		}
	}
		/* Another trivial case */
	if (!i) return NULL;

		/* Compute array size */
	ptmp=(RESULT **)emalloc(i*sizeof(RESULT *));
		/* Build an array with the elements to compare
			 and pointers to data */
	for(j=0,rtmp=rp;rtmp;rtmp = rtmp->next,j++) {
		if (rtmp->structure & structure) {
			ptmp[j]=rtmp;
		}
	}
		/* Sort them */
	qsort(ptmp,i,sizeof(RESULT *),&compResultsBySortProps);
		/* Build the list */
	for(j=0;j<i;j++){
		sortresultlist = (RESULT *) addsortresult(sw, sortresultlist, ptmp[j]);
	}
		/* Free the memory of the array */
	efree(ptmp);

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
		tmp->metaID=dp->metaID;
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



/* #### Function to format the property as a string */
char *getPropAsString(IndexFILE *indexf,docPropertyEntry *p)
{
char *s=NULL;
unsigned long i;
struct metaEntry *q;
	if(!p) return estrdup("");
	q=getMetaIDData(indexf,p->metaID); 
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
				/* Convert to ISO datetime */
		strftime(s,20,"%Y/%m/%d %H:%M:%S",(struct tm *)localtime((time_t *)&i));
	} else if(is_meta_number(q))  /* check for a number */
	{
		s=emalloc(14);
		i=*(unsigned long *)p->propValue;  /* read binary */
						  /* as unsigned long */
		UNPACKLONG(i);     /* Convert the portable number */
				/* Convert to string */
		sprintf(s,"%.013lu",i);
	} else s=estrdup("");
	return s;
}


void getSwishInternalProperties(struct file *fi, IndexFILE *indexf)
{
int i;
char **props;      /* Array to Store properties */
docPropertyEntry *p;
	for(p=fi->docProperties;p;p=p->next)
	{
		if(indexf->filenameProp->metaID==p->metaID) {}
		else if(indexf->titleProp->metaID==p->metaID) 
			fi->fi.title=bin2string(p->propValue,p->propLen);
		else if(indexf->filedateProp->metaID==p->metaID) 
		{
			fi->fi.mtime=*(unsigned long *)p->propValue;
			UNPACKLONG(fi->fi.mtime);
		}
		else if(indexf->startProp->metaID==p->metaID) 
		{
			fi->fi.start=*(unsigned long *)p->propValue;
			UNPACKLONG(fi->fi.start);
		}
		else if(indexf->sizeProp->metaID==p->metaID) 
		{
			fi->fi.size=*(unsigned long *)p->propValue;
			UNPACKLONG(fi->fi.size);
		}
		else if(indexf->summaryProp->metaID==p->metaID) 
			fi->fi.summary=bin2string(p->propValue,p->propLen);
	}
}





/* 
  -- Returns the contents of the property "propertyname" of result r
  -- This routines returns properties in their origin. representation.
  -- This routines knows all internal (auto) and user properties!

  -- Return: NULL if not found, or ptr to alloced structure
             property value and type (structure has to be freed()!)..

  -- 2000-12  jruiz -- inital coding
  -- 2001-01  rasc  -- value and type structures, Auto(internal) properties, rewritten
 */

PropValue * getResultPropertyByName (SWISH *sw, char *propertyname, RESULT *r)
{
int       i;
char      *p,*pname;
char      *propdata;  /* default value to NULL */
PropValue *pv;



	/* To avoid problems and for faster scan make propertyname */
	/* to lowercase */

	pname=estrdup(propertyname);     /* preserve original value */
	for(p=pname;*p;p++)*p=tolower((int)((char)p[0])); /* go lowercase */

		/* Search for the property name */

	pv = (PropValue *) emalloc (sizeof (PropValue));
	pv->datatype = UNDEFINED;

	/*
	  -- check for auto properties ( == internal metanames)
	 */

	if (! strcmp (pname, AUTOPROPERTY_TITLE) ) {
 		pv->datatype = STRING;
		pv->value.v_str = r->title;
	} else if (! strcmp (pname, AUTOPROPERTY_DOCPATH)  ) {
		pv->datatype = STRING;
		pv->value.v_str = r->filename;
	} else if (! strcmp (pname, AUTOPROPERTY_SUMMARY)  ) {
		pv->datatype = STRING;
		pv->value.v_str = r->summary;
	} else if (! strcmp (pname, AUTOPROPERTY_DOCSIZE)  ) {
		pv->datatype = INTEGER;
		pv->value.v_int = r->size;
	} else if (! strcmp (pname, AUTOPROPERTY_LASTMODIFIED)  ) {
		pv->datatype = DATE;
		pv->value.v_date = r->last_modified;
	} else if (! strcmp (pname, AUTOPROPERTY_STARTPOS)  ) {
		pv->datatype = INTEGER;
		pv->value.v_int = r->start;
	} else if (! strcmp (pname, AUTOPROPERTY_DOCSIZE)  ) {
		pv->datatype = INTEGER;
		pv->value.v_int = r->size;
	} else if (! strcmp (pname, AUTOPROPERTY_INDEXFILE)  ) {
		pv->datatype = STRING;
		pv->value.v_str = r->indexf->line;
	} else if (! strcmp (pname, AUTOPROPERTY_RESULT_RANK)  ) {
		pv->datatype = INTEGER;
		pv->value.v_int = r->rank;
	} else if (! strcmp (pname, AUTOPROPERTY_REC_COUNT)  ) {
		pv->datatype = INTEGER;
		pv->value.v_int = r->count;
	} else {

		/* User defined Properties
		   -- so far we only know STRING types as user properties
		   -- $$$ ToDO: other types...
		 */

		for(i=0;i<sw->numPropertiesToDisplay;i++) {
			if(!strcmp(pname,sw->propNameToDisplay[i])) {
				pv->datatype = STRING;
				pv->value.v_str = r->Prop[i];
				break;
			}
		}
	}
 
	efree(pname);
	if (pv->datatype == UNDEFINED) {	/* nothing found */
	  efree (pv);
	  pv = NULL;
	}

	return pv;
}


