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
#ifdef PROPFILE
#include "db.h"
#endif

/* Delete a property entry (and any linked properties) */
void freeProperty( propEntry *prop )
{
	while (prop)
	{
		propEntry *nextOne = prop->next;
		efree(prop);

		prop = nextOne;
	}
}	


/* delete the linked list of doc properties */

void freeDocProperties(docProperties *docProperties)
{
	int i;

	for( i = 0; i < docProperties->n; i++ )
	{
	    freeProperty( docProperties->propEntry[i] );
	    docProperties->propEntry[i] = NULL;
	}

	efree(docProperties);
	docProperties = NULL;
	
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

char *DecodeDocProperty( struct metaEntry *meta_entry, propEntry *prop )
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
*   Notes:
*       with PROPFILE, caller is expected to destroy the property
*
*
********************************************************************/

static propEntry *getDocProperty( RESULT *result, struct metaEntry **meta_entry, int metaID )
{
    IndexFILE *indexf = result->indexf; 
    propEntry *prop = NULL;
    struct file *fi;
    SWISH *sw = (SWISH *) result->sw;
    int     error_flag;
    unsigned long num;


    /* Grab the meta structure for this ID, unless one was passed in */

    if ( *meta_entry )
        metaID = (*meta_entry)->metaID;

    else if ( !(*meta_entry = getMetaIDData(&indexf->header, metaID )) )
        return NULL;

    if ( ! ( (*meta_entry)->metaType & META_PROP) )
        progerr( "'%s' is not a PropertyName", (*meta_entry)->metaName );


    /* This is a memory leak if not using PROPFILE */

    /* Some properties are generated during a search */
    if ( is_meta_internal( *meta_entry ) )
    {
        if ( is_meta_entry( *meta_entry, AUTOPROPERTY_RESULT_RANK ) )
        {
            num = PACKLONG( (unsigned long)result->rank );
            return CreateProperty( *meta_entry, (unsigned char *)&num, sizeof( num ), 1, &error_flag );
        }

        if ( is_meta_entry( *meta_entry, AUTOPROPERTY_REC_COUNT ) )
        {
            num = PACKLONG( (unsigned long)result->count );
            return CreateProperty( *meta_entry, (unsigned char *)&num, sizeof( num ), 1, &error_flag );
        }

        if ( is_meta_entry( *meta_entry, AUTOPROPERTY_FILENUM ) )
        {
            num = PACKLONG( (unsigned long)result->filenum );
            return CreateProperty( *meta_entry, (unsigned char *)&num, sizeof( num ), 1, &error_flag );
        }

            
        if ( is_meta_entry( *meta_entry, AUTOPROPERTY_INDEXFILE ) )
            return CreateProperty( *meta_entry, result->indexf->line, strlen( result->indexf->line ), 0, &error_flag );
    }
                   

#ifdef PROPFILE
    return ReadSingleDocPropertiesFromDisk(sw, indexf, result->filenum, metaID, 0 );
#endif;    



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
    char *s = NULL;
    propEntry *prop;
    struct metaEntry *meta_entry = NULL;

    
	if( !result )
	    return estrdup("");  // when would this happen?



    if ( !(prop = getDocProperty( result, &meta_entry, ID )) )
        return estrdup("");

    /* $$$ Ignores possible other properties that are linked to this one */
    s = DecodeDocProperty( meta_entry, prop );

#ifdef PROPFILE
    freeProperty( prop );
#endif        

    return s;
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
    struct metaEntry *meta_entry = NULL;
    propEntry *prop;


    /* create a propvalue to return to caller */
    pv = (PropValue *) emalloc (sizeof (PropValue));
    pv->datatype = UNDEFINED;
    pv->destroy = 0;

    /* Lookup by property name, if supplied */
    if ( pname )
        if ( !(meta_entry = getMetaNameData( &r->indexf->header, pname )) )
            return NULL;


    /* This may return false */
    prop = getDocProperty( r, &meta_entry, ID );


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
#ifdef PROPFILE
        freeProperty( prop );
#endif        
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
#ifdef PROPFILE
        freeProperty( prop );
#endif        
        return pv;
    }

   
    if ( is_meta_date(meta_entry) )
    {
        unsigned long i;
        i = *(unsigned long *) prop->propValue;  /* read binary */
        i = UNPACKLONG(i);     /* Convert the portable number */
        pv->datatype = DATE;
        pv->value.v_date = (time_t)i;
#ifdef PROPFILE
        freeProperty( prop );
#endif        
        return pv;
    }

#ifdef PROPFILE
    freeProperty( prop );
#endif        


 
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
*       *error_flag - integer to indicate the difference between an error and a blank property
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
static int EncodeProperty( struct metaEntry *meta_entry, char **encodedStr, char *propstring, int *error_flag )
{
    unsigned long int num;
    char     *newstr;
    char     *badchar;
    char     *tmpnum;
    char     *string;
  
    
    string = propstring;

    *error_flag = 0;

    /* skip leading white space */
    while ( isspace( (int)*string) )
        string++;

    if ( !string || !*string )
    {
        // progwarn("Null string passed to EncodeProperty for meta '%s'", meta_entry->metaName);
#ifdef BLANK_PROP_VALUE
        string = BLANK_PROP_VALUE;  // gets dup'ed below
#else        
        return 0;
#endif        
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
            (*error_flag)++;
            return 0;
        }

        if ( *badchar ) // I think this is how it works...
        {
            progwarn("Invalid char '%c' found in string '%s'", badchar[0], string);
            efree(string);
            (*error_flag)++;
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
*   Creates a document property 
*
*   Call with:
*       *metaEntry
*       *propValue  - string to add
*       *propLen    - length of string to add
*       preEncoded  - flag saying the data is already encoded
*                     (that's for filesize, last modified, start position)
*       *error_flag - integer to indicate the difference between an error and a blank property
*
*   Returns:
*       pointer to a newly created document property
*       NULL indicates property could not be created
*
*
********************************************************************/
propEntry *CreateProperty(struct metaEntry *meta_entry, unsigned char *propValue, int propLen, int preEncoded, int *error_flag )
{
    propEntry *docProp;

    /* convert string to a document property, if not already encoded */
    if ( !preEncoded )
    {
        char *tmp;
        
        propLen = EncodeProperty( meta_entry, &tmp, propValue, error_flag );

        if ( !propLen )  /* Error detected in encode */
            return NULL;
            
        propValue = tmp;
    }


    /* Now create the property */
    docProp=(propEntry *) emalloc(sizeof(propEntry) + propLen);
    docProp->next = NULL;

    memcpy(docProp->propValue, propValue, propLen);
    docProp->propLen = propLen;


    /* EncodeProperty creates a new string */
	if ( !preEncoded )
	    efree( propValue );

    return docProp;	    
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
	int error_flag;


    /* Allocate or extend the property array, if needed */

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


    /* create the document property */
    /* Ignore some errors */

    if ( !(docProp = CreateProperty( meta_entry, propValue, propLen, preEncoded, &error_flag )) )
        return error_flag ? 0 : 1;
    
	docProp->next = dp->propEntry[meta_entry->metaID];	
	dp->propEntry[meta_entry->metaID] = docProp;	/* update head-of-list ptr */

    return 1;	    
}

// #define DEBUGPROP 1
#ifdef DEBUGPROP
static int insidecompare = 0;
#endif

/*******************************************************************
*   Compares two properties for sorting
*
*   Call with:
*       *metaEntry
*       *docPropertyEntry1
*       *docPropertyEntry2
*
*   Returns:
*       0 - two properties are the same
*      -1 - docPropertyEntry1 < docPropertyEntry2
*      +1 - docPropertyEntry1 > docPropertyEntry2
*
*
********************************************************************/
int Compare_Properties( struct metaEntry *meta_entry, propEntry *p1, propEntry *p2 )
{

#ifdef DEBUGPROP
    if ( !insidecompare++ )
    {
        printf("comparing properties for meta %s: returning: %d\n", meta_entry->metaName, Compare_Properties( meta_entry, p1, p2) );
        dump_single_property( p1, meta_entry );
        dump_single_property( p2, meta_entry );
        insidecompare = 0;
    }
#endif
    
    
    if ( !p1 && p2 )
        return -1;

    if ( !p1 && !p2 )
        return 0;

    if ( p1 && !p2 )
        return +1;
        

    if (is_meta_number( meta_entry ) || is_meta_date( meta_entry ))
        return memcmp( (const void *)p1->propValue, (const void *)p2->propValue, p1->propLen );


    if ( is_meta_string(meta_entry) )
    {
        int rc;
        int len = Min( p1->propLen, p2->propLen );

        rc = strncasecmp(p1->propValue, p2->propValue, len);
        if ( rc != 0 )
            return rc;
            
        return p1->propLen - p2->propLen;
    }

    return 0;

}

#ifdef PROPFILE

/*******************************************************************
*   Write Properties to disk, and save seek pointers
*
*   The format is:
*	    <PropID:int><PropValueLen:int><PropValue:not-null-terminated>
*	    ...
*	    <PropID:int><PropValueLen:int><PropValue:not-null-terminated>
*	    <EndofList:null char>
*
* All ints are compressed to save space. Jose Ruiz 04/00
*
* The list is terminated with a PropID with a value of zero
*
*********************************************************************/
void     WritePropertiesToDisk( SWISH *sw )
{
    IndexFILE *indexf = sw->indexlist;
    struct file *fi = indexf->filearray[ indexf->filearray_cursize -1 ];
    docProperties *docProperties;
    int propID;
    int len;
    char *buffer,*p,*q;
    int lenbuffer;
    propEntry *prop;
    int datalen;
    int total_len;


    /* any props exist? */
    if ( !fi->docProperties )
        return;

    docProperties = fi->docProperties;

    /* create an array to hold the properties */

    fi->propLocationsCount = docProperties->n;
    fi->propLocations = (long *) emalloc( fi->propLocationsCount * sizeof( int *) );
    fi->propSize = (long *) emalloc( fi->propLocationsCount * sizeof( int *) );


    total_len = 0;  

    /* maybe this should be sw->Index->propbuffer so it's not allocated for every file */
	buffer = emalloc( (lenbuffer = MAXSTRLEN) );


    for( propID = 0; propID < docProperties->n; propID++ )
    {
        if ( !(prop = docProperties->propEntry[propID]))
        {
            fi->propSize[ propID ] = 0;         // here's the flag!
            fi->propLocations[ propID ] = 0;    // not here!
            continue;
        }
		
        datalen = 0;  /* length of data packed away */


        /* now write a property (or properties if more than one) */
        /* I wonder if it would be smarter to buffer ALL the props and then write */
        /* which would mean I'd have to calculate the offsets, which shouldn't be hard */

        while (prop)
        {
            /* the length of the property value */
            len = prop->propLen;

            /* Realloc buffer size if needed */
            if ( lenbuffer < (datalen+len+8*2) )
            {
                lenbuffer +=(datalen)+len+8*2+500;  // could we learn from the last allocation?
                buffer = erealloc( buffer, lenbuffer );
            }

            /* set start of this entry */
            p = q = buffer + datalen;

            /* Do not store 0!! - compress does not like it */
            p = compress3(propID+1,p);

            /* including the length will make retrieval faster */
            p = compress3(len,p);

            memcpy(p,prop->propValue, len);
            

            datalen += (p-q) + len;
            prop = prop->next;
        }

        /* Now write this property out to disk */

        fi->propLocations[ propID ] = DB_WriteProperty( sw, indexf->filearray_cursize, buffer, datalen, propID, indexf->DB );
        fi->propSize[ propID ] = datalen;
        total_len += datalen;
    }



    fi->propTotalLen = total_len;

    efree( buffer );


    /* Now free the doc properties */
    freeDocProperties( fi->docProperties );
    fi->docProperties = NULL;
    

}


/*******************************************************************
*   Reads the doc properties from disk
*
*   Maybe should return void, and just set?
*   Or maybe should take a filenum, and instead take a position?
*
*********************************************************************/
docProperties *ReadAllDocPropertiesFromDisk( SWISH *sw, IndexFILE *indexf, int filenum )
{
    struct file *fi;
    docProperties *docProperties=NULL;
    char   *tempPropValue=NULL;
    int     tempPropLen=0;
    int     tempPropID;
    struct  metaEntry meta_entry;
    char   *buf;
    char   *propbuf;
    long    seek_pos = -1;
    int     i;

    if ( !(fi = readFileEntry(sw, indexf, filenum)) )
        progerr("Failed to read file entry for file '%d'", filenum );



    /* already loaded? */
    if ( fi->docProperties )
        return fi->docProperties;
        

    if ( !fi->propTotalLen )
        return NULL;

    for ( i = 0; i < fi->propLocationsCount; i++ )
        if ( fi->propSize[i] )
        {
            seek_pos = fi->propLocations[ i ];
            break;
        }

    if ( seek_pos < 0 )
        progerr("failed to find seek postion for first property in file %d", filenum );


    propbuf = buf = ( char * ) emalloc( fi->propTotalLen + 1 );

    DB_ReadProperty( sw, buf, seek_pos, fi->propTotalLen, filenum, indexf->DB );



    *(buf + fi->propTotalLen) = '\0'; /* flag end of buffer */


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
        tempPropValue = buf;
        buf += tempPropLen;

        /* add the entry to the list of properties */
        /* Flag as encoded, so won't encode again */
        meta_entry.metaID = tempPropID;

        addDocProperty(&docProperties, &meta_entry, tempPropValue, tempPropLen, 1 );

        /* found ending null? */
        if (!*buf )
            break;

        tempPropID = uncompress2((unsigned char **)&buf);
    }

    efree( propbuf );

    /* save it in the file entry */
    fi->docProperties = docProperties;

    
    return docProperties;
}

/*******************************************************************
*   Reads a single doc property - this is used for sorting
*   ONLY RETURNS THE FIRST PROPERTY (if more than one exist)
*
*   Caller needs to destroy returned property
*
*
*********************************************************************/
propEntry *ReadSingleDocPropertiesFromDisk( SWISH *sw, IndexFILE *indexf, int filenum, int metaID, int max_size )
{
    struct file *fi;
    char   *buffer;
    char   *propbuf;
    struct  metaEntry meta_entry;
    int     propLen;
    char   *propValue;
    propEntry *docProp;
    int     error_flag;

    if ( !(fi = readFileEntry(sw, indexf, filenum)) )
        progerr("Failed to read file entry for file '%d'", filenum );
    
    /* Any properties? */
    if ( !fi->propTotalLen )
        return NULL;

    /* And do we have one that high? */
    if ( metaID >= fi->propLocationsCount )
        return NULL;


    /* already loaded? -- if so, duplicate the property for the given length */
    if ( fi->docProperties )
    {
        if ( metaID >= fi->docProperties->n )
            return NULL;

        if ( !(docProp = fi->docProperties->propEntry[ metaID ]) )
            return NULL;

        propLen = docProp->propLen;
        if ( max_size && (max_size >= 8) && (max_size < propLen ))
            propLen = max_size;

        /* Duplicate the property */
        meta_entry.metaName = "(default)";
        meta_entry.metaID = metaID;
        return CreateProperty( &meta_entry, docProp->propValue, propLen, 1, &error_flag );
    }               



    /* Any for this metaID? */
    if ( !fi->propSize[ metaID ] )
        return NULL;


    /* allocate a read buffer */
	propbuf = buffer = emalloc( fi->propSize[ metaID ] + 1 );

    DB_ReadProperty( sw, buffer, fi->propLocations[ metaID ], fi->propSize[ metaID ], filenum, indexf->DB );


    *(buffer + fi->propSize[ metaID ]) = '\0'; /* flag end of buffer - but not currently used! */


    meta_entry.metaName = "(default)";

    if ( !(meta_entry.metaID = uncompress2( (unsigned char **) &buffer )) )
    {
        efree( buffer );
        return NULL;
    }
    
    /* Decrease 1 (it was stored as ID+1 to avoid 0 value ) */
    meta_entry.metaID--;

    /* Get the data length */
    propLen = uncompress2((unsigned char **)&buffer);

    /* and limit it's size */
    /* $$$ don't know type, so just limit at 8, but should probably lookup its type */

    if ( max_size && (max_size >= 8) && (max_size < propLen ))
        propLen = max_size;



    propValue = buffer;  /* just to be obvious */

    docProp = CreateProperty( &meta_entry, propValue, propLen, 1, &error_flag );

    efree( propbuf );

    return docProp;
}

/*******************************************************************
*   Packs the file ENTRY's data related to properties into a buffer,
*   which is later written to the index file.
*
*********************************************************************/
unsigned char *PackPropLocations( struct file *fi, int *propbuflen )
{
    int i;
    unsigned char *buf;
    unsigned char *p;

    *propbuflen = 0;

    /* allocate enough memory to hold the data uncompressed */
    p = buf = emalloc( ( fi->propLocationsCount * 2 * sizeof( long ) ) + sizeof( int ) + sizeof( long ) + 10 );

    p = compress3( fi->propLocationsCount+1, p );
    p = compress3( fi->propTotalLen+1, p);

    /* Now save the offsets and sizes - note that this currently saves even the NULLs */
    for ( i = 0; i < fi->propLocationsCount; i++ )
    {
        p = compress3( fi->propLocations[i] + 1, p );
        p = compress3( fi->propSize[i] + 1, p );
    }

    *propbuflen = p - buf;

    return buf;
}

/*******************************************************************
*   Unpacks the file ENTRY's data
*
*********************************************************************/
unsigned char *UnPackPropLocations( struct file *fi, char *buf )
{
    int i;


    fi->propLocationsCount = uncompress2((unsigned char **)&buf) - 1;
    fi->propTotalLen       = uncompress2((unsigned char **)&buf) - 1;

    /* no properties found */
    if ( !fi->propLocationsCount )
    {
        fi->propLocations = NULL;
        fi->propSize      = NULL;
        return ++buf;
    }

    /* allocate memory for storage of seek values, and lengths */
    fi->propLocations = (long *) emalloc( fi->propLocationsCount * sizeof( int *) );
    fi->propSize      = (long *) emalloc( fi->propLocationsCount * sizeof( int *) );

    for ( i = 0; i < fi->propLocationsCount; i++ )
    {
        fi->propLocations[i] = uncompress2((unsigned char **)&buf) - 1;
        fi->propSize[i]      = uncompress2((unsigned char **)&buf) - 1;
    }

    return buf;
}
 
#else

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
unsigned char *fetchDocProperties( struct file *fi, char *buf)
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
	fi->docProperties = docProperties;

	return buf;
}
/* #### */
#endif


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



void dump_single_property( propEntry *prop, struct metaEntry *meta_entry )
{
    char *propstr;
    char proptype = '?';

    if  ( is_meta_string(meta_entry) )
        proptype = 'S';

    else if ( is_meta_date(meta_entry) )
        proptype = 'D';

    else if ( is_meta_number(meta_entry) )
        proptype = 'N';

    printf("  %20s (%2d) %c:", meta_entry->metaName, meta_entry->metaID, proptype );

    if ( !prop )
        printf(" propEntry=NULL");

    for ( ;prop; prop = prop->next )
    {
        propstr = DecodeDocProperty( meta_entry, prop );
        printf(" \"%s\"", propstr );
        efree( propstr );
    }

    printf( "\n" );
}


void dump_file_properties(IndexFILE * indexf, struct  file *fi )
{
    int j;
	propEntry *prop;
    struct metaEntry *meta_entry;

	if ( !fi->docProperties )  /* may not be any properties */
	{
	    printf(" (No Properties)\n");
	    return;
	}

    for (j = 0; j < fi->docProperties->n; j++)
    {
        if ( !fi->docProperties->propEntry[j] )
            continue;

        meta_entry = getMetaIDData( &indexf->header, j );
        prop = fi->docProperties->propEntry[j];
        
        dump_single_property( prop, meta_entry );
    }
}

