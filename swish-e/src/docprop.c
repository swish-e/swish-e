/*
$Id$
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
** 2001-01    rasc    getResultPropertyByName rewritten, datatypes for properties.
** 2001-02    rasc    isAutoProperty
**                    printSearchResultProperties changed
** 2001-03-15 rasc    Outputdelimiter var name changed
** 2001-06-08 wsm     Store propValue at end of docPropertyEntry to save memory
** 2001-06-14 moseley Much of this was rewritten.
** 
*/

#include <limits.h>     // for ULONG_MAX
#include "swish.h"
#include "string.h"
#include "file.h"
#include "hash.h"
#include "mem.h"
#include "merge.h"
#include "error.h"
#include "search.h"
#include "index.h"
#include "docprop.h"
#include "error.h"
#include "compress.h"
#include "metanames.h"
#include "result_output.h"
#include "result_sort.h"
#include "entities.h"



void freeDocProperties(docProperties *docProperties)
{
	/* delete the linked list of doc properties */
	struct propEntry *prop;
	int i;

	for(i=0;i<docProperties->n;i++)
	{
		prop = docProperties->propEntry[i];
		while (prop)
		{
			propEntry *nextOne = prop->next;
			efree(prop);

			prop = nextOne;
		}
	}
	efree(docProperties);

	
}


/*******************************************************************
*   Converts a property into a string, based on it's type.
*   Numbers are zero filled
*
*   Call with:
*       *metaEntry
*       *propEntry
*
*   Returns:
*       malloc's a new string.  Caller must call free().
*
*
********************************************************************/

static char *DecodeDocProperty( struct metaEntry *meta_entry, propEntry *prop )
{
    char *s;
    unsigned long i;
    
    if  ( is_meta_string(meta_entry) )      /* check for ascii/string data */
        return bin2string(prop->propValue,prop->propLen);


    if ( is_meta_date(meta_entry) )
    {
        s=emalloc(20);
        i = *(unsigned long *) prop->propValue;  /* read binary */
        i = UNPACKLONG(i);     /* Convert the portable number */
        strftime(s,20,"%Y-%m-%d %H:%M:%S",(struct tm *)localtime((time_t *)&i));
        return s;
    }


    
    if ( is_meta_number(meta_entry) )
    {
        s=emalloc(14);
        i=*(unsigned long *)prop->propValue;  /* read binary */
        i = UNPACKLONG(i);     /* Convert the portable number */
        sprintf(s,"%.013lu",i);
        return s;
    }

    progwarn("Invalid property type for property '%s'\n", meta_entry->metaName );
    return estrdup("");
}

/*******************************************************************
*   Returns a property (really the head of the list)
*   for the specified property
*
*   Call with:
*       *RESULT
*       *metaEntry - pointer to related meta entry
*       metaID - OR, if metaEntry is NULL uses this to lookup metaEntry
*
*   Returns:
*       *propEntry
*
*   Warning:
*       Only returns first property in list (which is the last property added)
*
*
********************************************************************/

static propEntry *getDocProperty( RESULT *result, struct metaEntry **meta_entry, int metaID )
{
    IndexFILE *indexf = result->indexf; 
    propEntry *prop = NULL;
    struct file *fi;
    SWISH *sw = (SWISH *) result->sw;


    /* Grab the meta structure for this ID, unless one was passed in */

    if ( *meta_entry )
        metaID = (*meta_entry)->metaID;

    else if ( !(*meta_entry = getMetaIDData(&indexf->header, metaID )) )
        return NULL;

    if ( ! ( (*meta_entry)->metaType & META_PROP) )
        progerr( "'%s' is not a PropertyName", (*meta_entry)->metaName );


    /* Read in the file info for this file - this can be a cached read */

    if ( ! (fi = readFileEntry( sw, indexf, result->filenum)) )
        return NULL;


    /* is there a property this high? */
    if ( metaID > fi->docProperties->n -1  )
        return NULL;


    if ( (prop = fi->docProperties->propEntry[metaID]))
        return prop;

    return NULL;
}


/*******************************************************************
*   Returns a string for the property ID supplied
*   Numbers are zero filled
*
*   Call with:
*       *RESULT
*       metaID
*
*   Returns:
*       malloc's a new string.  Caller must call free().
*
*   Bugs:
*       Only returns first property in list (which is the last property)
*
*
********************************************************************/

char *getResultPropAsString(RESULT *result, int ID)
{
    char *s=NULL;

    
	if( !result )
	    return estrdup("");  // when would this happen?


    /* Deal with internally generated props */
    
    if ( ID == AUTOPROP_ID__REC_COUNT )
    {
    	s=emalloc(14);
    	sprintf(s,"%.013lu", (unsigned long)0 );
    	return s;
    }

    if ( ID == AUTOPROP_ID__RESULT_RANK )
    {
        s=emalloc(14);
        sprintf(s,"%.013lu", (unsigned long)result->rank );
        return s;
    }

    if ( ID == AUTOPROP_ID__INDEXFILE )
        return estrdup( result->indexf->line );


    /* "Real" properties */
    {
        propEntry *prop;
        struct metaEntry *meta_entry = NULL;

        if ( !(prop = getDocProperty( result, &meta_entry, ID )) )
            return estrdup("");

        /* $$$ Ignores other properties! */
        return DecodeDocProperty( meta_entry, prop );
    }
}

/*******************************************************************
*   Returns a property as a *propValue, which is a union of different
*   data types, with a flag to indicate the type
*   Can be called with either a metaname, or a metaID.
*
*   Call with:
*       *SWISH
*       *RESULT
*       *metaName -- String name of meta entry
*       metaID    -- OR - meta ID number
*
*   Returns:
*       pointer to a propValue structure if found -- caller MUST free
*
*
********************************************************************/

PropValue *getResultPropValue (SWISH *sw, RESULT *r, char *pname, int ID )
{
    PropValue *pv;
    struct metaEntry *meta_entry;


    /* create a propvalue to return to caller */
    pv = (PropValue *) emalloc (sizeof (PropValue));
    pv->datatype = UNDEFINED;
    pv->destroy = 0;

    if ( pname )
        ID = isAutoProperty ( pname );  // I'd rather that everything had a metaEntry!
            
    if ( ID == AUTOPROP_ID__REC_COUNT )
    {
        pv->datatype = INTEGER;
        pv->value.v_int = r->count;
        return pv;
    }

    if ( ID == AUTOPROP_ID__RESULT_RANK )
    {
        pv->datatype = INTEGER;
        pv->value.v_int = r->rank;
        return pv;
    }

    if ( ID == AUTOPROP_ID__INDEXFILE )
    {
        pv->datatype = STRING;
        pv->value.v_str = r->indexf->line;
        return pv;
    }


    /* meta name exists? */
    if ( pname )
    {
        if ( !(meta_entry = getMetaNameData( &r->indexf->header, pname )) )
            return NULL;
    }
    else
        if ( !(meta_entry = getMetaIDData(&r->indexf->header, ID)) )
            return NULL;


    /* Now return a "real" property */
    {
        propEntry *prop;

        /* This may return false */
        prop = getDocProperty( r, &meta_entry, 0 );
            

        if ( is_meta_string(meta_entry) )      /* check for ascii/string data */
        {
            if ( !prop )
            {
                pv->datatype = STRING;
                pv->value.v_str = "";
                return pv;
            }

            pv->datatype = STRING;
            pv->destroy++;       // caller must free this
            pv->value.v_str = bin2string(prop->propValue,prop->propLen);
            return pv;
        }


        /* dates and numbers should return null to tell apart from zero */
        if ( !prop )
        {
            efree( pv );
            return NULL;
        }


        if ( is_meta_number(meta_entry) )
        {
            unsigned long i;
            i = *(unsigned long *) prop->propValue;  /* read binary */
            i = UNPACKLONG(i);     /* Convert the portable number */
            pv->datatype = ULONG;
            pv->value.v_ulong = i;
            return pv;
        }

   
        if ( is_meta_date(meta_entry) )
        {
            unsigned long i;
            i = *(unsigned long *) prop->propValue;  /* read binary */
            i = UNPACKLONG(i);     /* Convert the portable number */
            pv->datatype = DATE;
            pv->value.v_date = (time_t)i;
            return pv;
        }
    }

 
	if (pv->datatype == UNDEFINED) {	/* nothing found */
	  efree (pv);
	  pv = NULL;
	}

	return pv;
}

/*******************************************************************
*   Displays the "old" style properties for -p
*
*   Call with:
*       *RESULT
*
*   I think this could be done in result_output.c by creating a standard
*   -x format (plus properites) for use when there isn't one already.
*
*
********************************************************************/
void printStandardResultProperties(SWISH *sw, FILE *f, RESULT *r)
{
    int     i;
    struct  MOD_Search *srch = sw->Search;
    char   *s;
    char   *propValue;
    int    *metaIDs;

    metaIDs = r->indexf->propIDToDisplay;

    if (srch->numPropertiesToDisplay == 0)
        return;

    for ( i = 0; i < srch->numPropertiesToDisplay; i++ )
    {
        propValue = s = getResultPropAsString( r, metaIDs[ i ] );

        if (sw->ResultOutput->stdResultFieldDelimiter)
            fprintf(f, "%s", sw->ResultOutput->stdResultFieldDelimiter);
        else
            fprintf(f, " \"");	/* default is to quote the string, with leading space */

        /* print value, handling newlines and quotes */
        while (*propValue)
        {
            if (*propValue == '\n')
                fprintf(f, " ");

            else if (*propValue == '\"')	/* should not happen */
                fprintf(f,"&quot;");

            else
                fprintf(f,"%c", *propValue);

            propValue++;
        }

        //fprintf(f,"%s", propValue);

        if (!sw->ResultOutput->stdResultFieldDelimiter)
            fprintf(f,"\"");	/* default is to quote the string */

        efree( s );            
    }
}


/*******************************************************************
*   Converts a string into a string for saving as a property
*   Which means will either return a duplicated string,
*   or a packed unsigned long.
*
*   Call with:
*       *metaEntry
*       **encodedStr (destination)
*       *string
*
*   Returns:
*       malloc's a new string, stored in **encodedStr.  Caller must call free().
*       length of encoded string, or zero if an error
*       (zero length strings are not for encoding anyway, I guess)
*
*   QUESTION: ???
*       should this return a *docproperty instead?
*       numbers are unsigned longs.  What if someone
*       wanted to store signed numbers?
*
*   ToDO:
*       What about convert entities here?
*
********************************************************************/
int EncodeProperty( struct metaEntry *meta_entry, char **encodedStr, char *propstring )
{
    unsigned long int num;
    char     *newstr;
    char     *badchar;
    char     *tmpnum;
    char     *string;
    
    string = propstring;

    /* skip leading white space */
    while ( isspace( (int)*string) )
        string++;

    if ( !string || !*string )
    {
        // progwarn("Null string passed to EncodeProperty for meta '%s'", meta_entry->metaName);
        return 0;
    }


    /* make a working copy */
    string = estrdup( string );

    /* remove trailing white space  */
    {
        int i = strlen( string );

        while ( i  && isspace( (int)string[i-1]) )
            string[--i] = '\0';
    }


    if (is_meta_number( meta_entry ) || is_meta_date( meta_entry ))
    {
        int j;

        newstr = emalloc( sizeof( num ) + 1 );
        num = strtoul( string, &badchar, 10 ); // would base zero be more flexible?
        
        if ( num == ULONG_MAX )
        {
            progwarnno("Attempted to convert '%s' to a number", string );
            efree(string);
            return 0;
        }

        if ( *badchar ) // I think this is how it works...
        {
            progwarn("Invalid char '%c' found in string '%s'", badchar[0], string);
            efree(string);
            return 0;
        }
        /* I'll bet there's an easier way */
        num = PACKLONG(num);
        tmpnum = (unsigned char *)&num;

        for ( j=0; j <= sizeof(num)-1; j++ )
            newstr[j] = (unsigned char)tmpnum[j];
        
        newstr[ sizeof(num) ] = '\0';

        *encodedStr = newstr;

        efree(string);

        return (int)sizeof(num);
    }
        

    if ( is_meta_string(meta_entry) )
    {
        *encodedStr = string;
        return (int)strlen( string );
    }


    progwarn("EncodeProperty called but doesn't know the property type :(");
    return 0;
}

/*******************************************************************
*   Adds a document property to the list of properties.
*   Creates or extends the list, as necessary
*
*   Call with:
*       **docProperties - pointer to list of properties
*       *metaEntry
*       *propValue  - string to add
*       *propLen    - length of string to add
*       preEncoded  - flag saying the data is already encoded
*                     (that's for filesize, last modified, start position)
*
*   Returns:
*       true if added property
*       sets address of **docProperties, if list changes size
*
*
********************************************************************/

int addDocProperty( docProperties **docProperties, struct metaEntry *meta_entry, unsigned char *propValue, int propLen, int preEncoded )
{
	struct docProperties *dp = *docProperties; 
    propEntry *docProp;
	int i;


    /* convert string to a document property, if not already encoded */
    if ( !preEncoded )
    {
        char *tmp;

        /* $$$ Just ignore null values -- would probably be better to check before calling */
        if ( !propValue || !*propValue )
            return 1;
        
        propLen = EncodeProperty( meta_entry, &tmp, propValue );
        if ( !propLen )
            return 0;
        propValue = tmp;
    }


    /* Store property if it's not zero length */

	if(propLen)
	{

	    /* First property added, allocate space for the property array */
	    
		if( !dp )
		{
			dp = (struct docProperties *) emalloc(sizeof(struct docProperties) + (meta_entry->metaID + 1) * sizeof(propEntry *));
			*docProperties = dp;

			dp->n = meta_entry->metaID + 1;

			for( i = 0; i < dp->n; i++ )
				dp->propEntry[i] = NULL;
		}

		else /* reallocate if needed */
		{
			if( dp->n <= meta_entry->metaID )
			{
				dp = (struct docProperties *) erealloc(dp,sizeof(struct docProperties) + (meta_entry->metaID + 1) * sizeof(propEntry *));

				*docProperties = dp;
				for( i = dp->n; i <= meta_entry->metaID; i++ )
					dp->propEntry[i] = NULL;
					
				dp->n = meta_entry->metaID + 1;
			}
		}

		docProp=(propEntry *) emalloc(sizeof(propEntry) + propLen);

	    memcpy(docProp->propValue, propValue, propLen);
		docProp->propLen = propLen;
		
		docProp->next = dp->propEntry[meta_entry->metaID];	
		dp->propEntry[meta_entry->metaID] = docProp;	/* update head-of-list ptr */
	}

	if ( !preEncoded )
	    efree( propValue );

    return 1;	    
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
unsigned char *storeDocProperties(docProperties *docProperties, int *datalen)
{
    int propID;
    int len;
    char *buffer,*p,*q;
    int lenbuffer;
    propEntry *prop;

	buffer=emalloc((lenbuffer=MAXSTRLEN));

	*datalen=0;

	for( propID = 0; propID < docProperties->n; propID++ )
	{
		prop = docProperties->propEntry[propID];
		while (prop)
		{
			/* the length of the property value */
			len = prop->propLen;

			/* Realloc buffer size if needed */
			if(lenbuffer<(*datalen+len+8*2))
			{
				lenbuffer +=(*datalen)+len+8*2+500;
				buffer=erealloc(buffer,lenbuffer);
			}
			p= q = buffer + *datalen;

			/* Do not store 0!! - compress does not like it */
			p = compress3(propID+1,p);

			/* including the length will make retrieval faster */
			p = compress3(len,p);

			memcpy(p,prop->propValue, len);

			*datalen += (p-q)+len;
			prop = prop->next;
		}
	}
	return buffer;
}
/* #### */

/*
 * Read the docProperties section that the buffer pointer is
 * currently pointing to.
 */
/* #### Added support for propLen */
docProperties *fetchDocProperties( char *buf)
{
docProperties *docProperties=NULL;
char* tempPropValue=NULL;
int tempPropLen=0;
int tempPropID;
struct metaEntry meta_entry;

    meta_entry.metaName = "(default)";

	/* read all of the properties */
    tempPropID = uncompress2((unsigned char **)&buf);
	while(tempPropID > 0)
	{
		/* Decrease 1 (it was stored as ID+1 to avoid 0 value ) */
		tempPropID--;

		/* Get the data length */
		tempPropLen = uncompress2((unsigned char **)&buf);

		/* BTW, len must no be 0 */
	    tempPropValue=buf;
		buf+=tempPropLen;

		/* add the entry to the list of properties */
		/* Flag as encoded, so won't encode again */
		meta_entry.metaID = tempPropID;
		addDocProperty(&docProperties, &meta_entry, tempPropValue, tempPropLen, 1 );
       	tempPropID = uncompress2((unsigned char **)&buf);
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
		return sw->Search->numPropertiesToDisplay;
	return 0;
}

void addSearchResultDisplayProperty(SWISH *sw, char *propName)
{
struct MOD_Search *srch = sw->Search;

	/* add a property to the list of properties that will be displayed */
	if (srch->numPropertiesToDisplay >= srch->currentMaxPropertiesToDisplay)
	{
		if(srch->currentMaxPropertiesToDisplay) {
			srch->currentMaxPropertiesToDisplay+=2;
			srch->propNameToDisplay=(char **)erealloc(srch->propNameToDisplay,srch->currentMaxPropertiesToDisplay*sizeof(char *));
		} else {
			srch->currentMaxPropertiesToDisplay=5;
			srch->propNameToDisplay=(char **)emalloc(srch->currentMaxPropertiesToDisplay*sizeof(char *));
		}
	}
	srch->propNameToDisplay[srch->numPropertiesToDisplay++] = estrdup(propName);
}



void swapDocPropertyMetaNames(docProperties **docProperties, struct metaMergeEntry *metaFile)
{
    int metaID, i;
    propEntry *prop, *tmpprop;
    struct docProperties *tmpDocProperties;

    if(! *docProperties) return;


	tmpDocProperties = (struct docProperties *)emalloc(sizeof(struct docProperties) + (*docProperties)->n * sizeof(propEntry *));
	tmpDocProperties->n = (*docProperties)->n;

	for ( metaID = 0; metaID < tmpDocProperties->n; metaID++ )
		tmpDocProperties->propEntry[metaID] = NULL;

	/* swap metaName values for properties */
	for ( metaID = 0 ; metaID < (*docProperties)->n ;metaID++ )
	{
		prop = (*docProperties)->propEntry[metaID];
		while (prop)
		{
			struct metaMergeEntry* metaFileTemp;
			propEntry *nextOne = prop->next;


			/* scan the metaFile list to get the new metaName value */
			metaFileTemp = metaFile;
			while (metaFileTemp)
			{
				if (metaID == metaFileTemp->oldMetaID)
				{
					if (tmpDocProperties->n <= metaFileTemp->newMetaID )
					{
						tmpDocProperties = erealloc(tmpDocProperties,sizeof(struct docProperties) + (metaFileTemp->newMetaID + 1) * sizeof(propEntry *));
						for(i=tmpDocProperties->n; i<= metaFileTemp->newMetaID; i++)
							tmpDocProperties->propEntry[i] = NULL;
						tmpDocProperties->n = metaFileTemp->newMetaID + 1;
					}

					tmpprop = emalloc(sizeof(propEntry) + prop->propLen);
					memcpy(tmpprop->propValue,prop->propValue,prop->propLen);
					tmpprop->propLen = prop->propLen;
					tmpprop->next = tmpDocProperties->propEntry[metaFileTemp->newMetaID];
					tmpDocProperties->propEntry[metaFileTemp->newMetaID] = tmpprop;
					break;
				}

				metaFileTemp = metaFileTemp->next;
			}
			prop = nextOne;
		}
	}

	/* Reasign new values */
	*docProperties = tmpDocProperties;

}


/* For faster proccess, get de ID of the properties to sort */
int initSearchResultProperties(SWISH *sw)
{
    IndexFILE *indexf;
    int i;
    struct MOD_Search *srch = sw->Search;
    struct metaEntry *meta_entry;


	/* lookup selected property names */

	if (srch->numPropertiesToDisplay == 0)
		return RC_OK;

	for( indexf = sw->indexlist; indexf; indexf = indexf->next )
		indexf->propIDToDisplay=(int *) emalloc(srch->numPropertiesToDisplay*sizeof(int));

	for (i = 0; i < srch->numPropertiesToDisplay; i++)
	{
		makeItLow(srch->propNameToDisplay[i]);

		/* Get ID for each index file */
		for( indexf = sw->indexlist; indexf; indexf = indexf->next )
		{
		    if ( !(meta_entry = getMetaNameData( &indexf->header, srch->propNameToDisplay[i])))
			{
				progerr ("Unknown Display property name \"%s\"", srch->propNameToDisplay[i]);
				return (sw->lasterror=UNKNOWN_PROPERTY_NAME_IN_SEARCH_DISPLAY);
			}
			else if ( !( meta_entry->metaType & META_PROP) )
			{
				progerr ("Name \"%s\" is not a PropertyName", srch->propNameToDisplay[i]);
				return (sw->lasterror=UNKNOWN_PROPERTY_NAME_IN_SEARCH_DISPLAY);
			}
			else
			    indexf->propIDToDisplay[i] = meta_entry->metaID;
		}
	}
	return RC_OK;
}



/*
  -- check if a propertyname is an internal property
  -- (AutoProperty). Check is case sensitive!
  -- Return: 0 (no AutoProp)  or >0 = ID of AutoProp..
  -- 2001-02-07 rasc
*/


int isAutoProperty (char *propname)

{
  static struct {
	char *name;
	int  id;
  }  *ap, auto_props[] = {
	{ AUTOPROPERTY_REC_COUNT,	AUTOPROP_ID__REC_COUNT },
	{ AUTOPROPERTY_RESULT_RANK,	AUTOPROP_ID__RESULT_RANK },
	{ AUTOPROPERTY_TITLE,		AUTOPROP_ID__TITLE },
	{ AUTOPROPERTY_DOCPATH,		AUTOPROP_ID__DOCPATH },
	{ AUTOPROPERTY_DOCSIZE,		AUTOPROP_ID__DOCSIZE },
	{ AUTOPROPERTY_SUMMARY,		AUTOPROP_ID__SUMMARY },
	{ AUTOPROPERTY_LASTMODIFIED,	AUTOPROP_ID__LASTMODIFIED },
	{ AUTOPROPERTY_INDEXFILE,	AUTOPROP_ID__INDEXFILE },
	{ AUTOPROPERTY_STARTPOS,	AUTOPROP_ID__STARTPOS },

        { NULL,				0 }
  };


  /* -- Precheck... fits start of propname? */

  if (strncasecmp (propname,AUTOPROPERTY__ID_START__,
		sizeof(AUTOPROPERTY__ID_START__)-1)) {
	return 0;
  }

  ap = auto_props;
  while (ap->name) {
	if (! strcasecmp(propname,ap->name)) {
	   return ap->id;
	}
	ap++;
  }
  return 0;
}




void dump_file_properties(IndexFILE * indexf, struct  file *fi )
{
    int j;
	propEntry *prop;
    struct metaEntry *meta_entry;
    char *propstr;

	if ( !fi->docProperties )  /* may not be any properties */
	    return;

    for (j = 0; j < fi->docProperties->n; j++)
    {
        if ( !fi->docProperties->propEntry[j] )
            continue;

        meta_entry = getMetaIDData( &indexf->header, j );

        printf("  %20s (%2d):", meta_entry->metaName, meta_entry->metaID );

        for ( prop = fi->docProperties->propEntry[j]; prop; prop = prop->next )
        {
            propstr = DecodeDocProperty( meta_entry, prop );
            
            printf(" \"%s\"", propstr );

            efree( propstr );
        }

        printf( "\n" );;
    }
}

