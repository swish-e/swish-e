/* jmruiz - 02/2001 - Sorting results ... */
#include "swish.h"
#include "mem.h"
#include "merge.h"
#include "docprop.h"
#include "metanames.h"
#include "compress.h"
#include "search.h"
#include "error.h"
#include "string.h"


/* preprocess Sort Result Properties to get the ID */
/* If there is not a sort properties then use rank */
int initSortResultProperties(SWISH *sw)
{
int i;
IndexFILE *indexf;
	if (sw->numPropertiesToSort == 0)
	{
			/* hack -> If no sort perperties have been specified then
			use rank in descending mode */
		addSearchResultSortProperty(sw,AUTOPROPERTY_RESULT_RANK,1);
		for(indexf=sw->indexlist;indexf;indexf=indexf->next)
			indexf->propIDToSort[0] = AUTOPROP_ID__RESULT_RANK;			

		return RC_OK;
	}
	for (i = 0; i<sw->numPropertiesToSort; i++)
	{
		makeItLow(sw->propNameToSort[i]);
		/* Get ID for each index file */
		for(indexf=sw->indexlist;indexf;indexf=indexf->next)
		{
			indexf->propIDToSort[i] = getMetaNameID(indexf, sw->propNameToSort[i]);
			if (indexf->propIDToSort[i] == 1)
			{
				progerr ("Unknown Sort property name \"%s\" in one of the index files", sw->propNameToSort[i]);
				return (sw->lasterror=UNKNOWN_PROPERTY_NAME_IN_SEARCH_SORT);
			}
		}
	}
	return RC_OK;
}

/* 02/2001 Jose Ruiz */
/* function for comparing data in order to
get sorted results with qsort (including combinations of asc and descending
fields */
int compResultsByNonSortedProps(const void *s1,const void *s2)
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

/* 02/2001 Jose Ruiz */
/* function for comparing data in order to
get sorted results with qsort (including combinations of asc and descending
fields */
/* This routine uses the presorted tables built during the index proccess */
int compResultsBySortedProps(const void *s1,const void *s2)
{
RESULT *r1=*(RESULT* const *)s1;
RESULT *r2=*(RESULT* const *)s2;
register int i,num_fields;
int rc,sortmode;
SWISH *sw=(SWISH *)r1->sw;

	num_fields=sw->numPropertiesToSort;
	for(i=0;i<num_fields;i++){
		sortmode=sw->propModeToSort[i];
		rc=sortmode*(r1->iPropSort[i]-r2->iPropSort[i]);
		if(rc)return rc;
	}
	return 0;
}

/* Adds the results of a search, sorts them by rank.
*/

/* Jose Ruiz 04/00
** Complete rewrite
** Sort was made before calling this function !! -> FASTER!!
** This one just reverses order
*/
RESULT *addsortresult(sw, sphead, r)
SWISH *sw;
RESULT *sphead;
RESULT *r;
{
	if (r->rank > sw->bigrank)
		sw->bigrank = r->rank;
	if (sphead == NULL) {
		r->nextsort = NULL;
	}
	else {
		r->nextsort = sphead;
	}
	return r;
}

/* Routine to load from the index file the presorted data */
int *readSortedData(IndexFILE *indexf,struct metaEntry *m)
{
FILE *fp=indexf->fp;
int j,value;
int *data=emalloc(indexf->filearray_cursize*sizeof(int));
	fseek(fp,m->sort_offset,SEEK_SET);
	for(j=0;j<indexf->filearray_cursize;j++)
	{
		uncompress1(value,fp);
		data[j]=value-1;    /* It was stored as value + 1 */
	}
	return data;
}

/* Routine to get the presorted lookupdata for a result for all the specified properties */
int *getLookupResultSortedProperties(RESULT *r)
{
int i;
int *props=NULL;      /* Array to Store properties Lookups */
struct metaEntry *m=NULL;
IndexFILE *indexf=r->indexf;
SWISH *sw=(SWISH *)r->sw;

	props=(int *) emalloc(sw->numPropertiesToSort*sizeof(int));
	for (i = 0; i<sw->numPropertiesToSort; i++)
	{
		switch(indexf->propIDToSort[i])
		{
			case AUTOPROP_ID__REC_COUNT:
				props[i]=0;
				break;
			case AUTOPROP_ID__RESULT_RANK:
				props[i]=r->rank;
				break;
			case AUTOPROP_ID__DOCPATH:
				props[i]=r->filenum;
				break;
			case AUTOPROP_ID__INDEXFILE:
				props[i]=0;         /* Same value for all of them */
				break; 
			case AUTOPROP_ID__DOCSIZE:
			case AUTOPROP_ID__LASTMODIFIED:
			case AUTOPROP_ID__STARTPOS:
			case AUTOPROP_ID__TITLE:
			case AUTOPROP_ID__SUMMARY:
			default:   /* User properties */
				m=getMetaIDData(indexf,indexf->propIDToSort[i]);
				if(m)
				{
					/* Read presorted data if not yet loaded (it will load only once */
					if(!m->sorted_data) m->sorted_data=readSortedData(indexf,m);
					props[i]=m->sorted_data[r->filenum-1];
				} else props[i]=0;
				break;
		}
	}
	return props;
}


char **getResultSortProperties(RESULT *r)
{
int i;
char **props=NULL;      /* Array to Store properties */
IndexFILE *indexf=r->indexf;
SWISH *sw=(SWISH *)r->sw;

    if (sw->numPropertiesToSort == 0) return NULL;
	
	props=(char **) emalloc(sw->numPropertiesToSort*sizeof(char *));
	for (i = 0; i<sw->numPropertiesToSort; i++)
	{
		props[i] = getResultPropAsString(r, indexf->propIDToSort[i]);
	}
	return props;
}


/* Jose Ruiz 04/00
** Sort results by property
*/
RESULT *sortresults(SWISH *sw, int structure)
{ 
int i, j;
RESULT **ptmp;
RESULT *rtmp;
RESULT *sortresultlist;
RESULT *rp,*tmp;
struct IndexFILE *indexf;
int (*compResults)(const void *,const void *);
	rp=sw->resultlist;
		/* Trivial case */
	if (!rp) return NULL;

	/* get properties if needed */
	/* First. Check out how many index files we have */
	for(i=0, indexf=sw->indexlist; indexf; indexf=indexf->next,i++);
	if(i==1)
	{
		/* Asign comparison routine to be used by qsort */
		compResults=compResultsBySortedProps;
			/* If we are only using 1 index file, we can use the presorted data in the index file */
		for(tmp=rp;tmp;tmp=tmp->next)
		{
			/* Load the presorted data */
			tmp->iPropSort=getLookupResultSortedProperties(tmp);
		}
	} 
	else
	{
		/* Asign comparison routine to be used by qsort */
		compResults=compResultsByNonSortedProps;

		/* We need to read the data for each result */
		for(tmp=rp;tmp;tmp=tmp->next)
		{
			tmp=getproperties(tmp);  /* Reads all data except the properties to sort */
					/* read the properties to sort */
			tmp->PropSort=getResultSortProperties(tmp);
		}

	
	}


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
	qsort(ptmp,i,sizeof(RESULT *),compResults);

		/* Build the list */
	for(j=0;j<i;j++){
		sortresultlist = (RESULT *) addsortresult(sw, sortresultlist, ptmp[j]);
	}
		/* Free the memory of the array */
	efree(ptmp);

	return sortresultlist;
}

/* 01/2001 Jose Ruiz */
/* function for comparing data in order to
get sorted results with qsort (including combinations of asc and descending
fields */
int compFileProps(const void *s1,const void *s2)
{
struct file *r1=*(struct file* const *)s1;
struct file *r2=*(struct file* const *)s2;
docPropertyEntry *p1,*p2;
int metaID=r1->currentSortProp->metaID,rc=0,len=0;
	for(p1=r1->docProperties;p1;p1=p1->next)
		if(metaID==p1->metaID) break;
	for(p2=r2->docProperties;p2;p2=p2->next)
		if(metaID==p2->metaID) break;
	if(!p1 && !p2) return 0;
	if(!p1 && p2) return 1;
	if(p1 && !p2) return -1;
		
	if(is_meta_number(r1->currentSortProp) || is_meta_date(r1->currentSortProp))
	{
		rc=*((int *)p1->propValue) - *((int *)p2->propValue);
	}
	else if(is_meta_string(r1->currentSortProp))
	{
		len=Min(p1->propLen,p2->propLen);
		rc=strncasecmp(p1->propValue,p2->propValue,len);
		if(!rc)
			rc=p1->propLen-p2->propLen;
	}
        return rc;
}

/* 02/2001 jmruiz */
/* Routine to sort properties at index time */
void sortFileProperties(IndexFILE *indexf)
{
int i,j,k,val;
int *sortFilenums;
struct metaEntry *m;
FILE *fp=indexf->fp;

printf("DBG: Starting sorting of properties\n");fflush(stdout);
		/* Array of filenums to store the sorted docs (referenced by its filenum) */
	sortFilenums=emalloc(indexf->fileoffsetarray_cursize*sizeof(int));
		/* Execute for each property */
	for(j=0;j<indexf->metaCounter;j++)
	{
		switch(indexf->metaEntryArray[j]->metaID)
		{
		case AUTOPROP_ID__REC_COUNT:
		case AUTOPROP_ID__RESULT_RANK:
		case AUTOPROP_ID__DOCPATH:
		case AUTOPROP_ID__INDEXFILE:
			break; /* Do nothing : Files are already sorted */
					/* Rec_count and rank are computed in search */
		case AUTOPROP_ID__TITLE:
		case AUTOPROP_ID__DOCSIZE:
		case AUTOPROP_ID__LASTMODIFIED:
		case AUTOPROP_ID__SUMMARY:
		case AUTOPROP_ID__STARTPOS:
		default:   /* User properties */
			m=getMetaIDData(indexf,indexf->metaEntryArray[j]->metaID);
			for(i=0;i<indexf->fileoffsetarray_cursize;i++)
				indexf->filearray[i]->currentSortProp=m;
				/* Sort them using qsort. The main work is done by compFileProps */
			qsort(indexf->filearray,indexf->fileoffsetarray_cursize,sizeof(struct file *),&compFileProps);
				/* Build the sorted table */
			for(i=0,k=1;i<indexf->fileoffsetarray_cursize;i++)
			{
				/* 02/2001 We can have duplicated values - So all them may have the same number asigned  - qsort justs sorts */
				if(i) 
				{
					/* If consecutive elements are different increase the number */
					if((compFileProps(&indexf->filearray[i-1],&indexf->filearray[i]))) k++;
				}
				sortFilenums[indexf->filearray[i]->fi.filenum-1]=k;
			}

				/* Write the sorted results to disk in compressed format */
			    /* Get the offset of the index file */
			m->sort_offset = ftell(fp);
				/* Write the sorted table */
			for(i=0;i<indexf->fileoffsetarray_cursize;i++)
			{
				val=sortFilenums[i]; 
				compress1(val,fp);
			}
			break;
		}
	}
	efree(sortFilenums);

printf("DBG: End sorting of properties\n");fflush(stdout);
}
