/*
$Id$
**
**
** This program and library is free software; you can redistribute it and/or
** modify it under the terms of the GNU (Library) General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU (Library) General Public License for more details.
**
**
** 2001-03-17  rasc  save real_filename as title (instead full real_path)
**                   was: compatibility issue to v 1.x.x
** 2001-05-09  rasc  entities changed (new module)
**
** 2001-07-25  moseley complete rewrite to use James Clark's Expat parser
**
** BUGS:
**      UndefinedMetaTags ignore is not coded
*/

#include "swish.h"
#include "merge.h"
#include "mem.h"
#include "string.h"
#include "search.h"
#include "docprop.h"
#include "error.h"
#include "index.h"
#include "metanames.h"

#include "xmlparse.h"   // James Clark's Expat

#define BUFFER_CHUNK_SIZE 20000

typedef struct {
    char   *buffer;     // text for buffer
    int     cur;        // pointer to end of buffer
    int     max;        // max size of buffer
    int     defaultID;  // default ID for no meta names.
} CHAR_BUFFER;


// I think that the property system can deal with StoreDescription in a cleaner way.
// This code shouldn't need to know about that StoreDescription.

typedef struct {
    struct metaEntry    *meta;
    int                 save_size;   /* save max size */
    char                *tag;        /* summary tag */
    int                 active;      /* inside summary */
} SUMMARY_INFO;
    

typedef struct {
    CHAR_BUFFER text_buffer;    // buffer for collecting text

    // CHAR_BUFFER prop_buffer;  // someday, may want a separate property buffer if want to collect tags within props

    SUMMARY_INFO    summary;     // argh.

    char       *ignore_tag;     // tag that triggered ignore (currently used for both)
    int         total_words;
    int         word_pos;
    int         filenum;
    XML_Parser *parser;
    INDEXDATAHEADER *header;
    SWISH      *sw;
    FileProp   *fprop;
    FileRec    *thisFileEntry;
    
} PARSE_DATA;


/* Prototypes */
static void start_hndl(void *data, const char *el, const char **attr);
static void end_hndl(void *data, const char *el);
static void char_hndl(void *data, const char *txt, int txtlen);
static void append_buffer( CHAR_BUFFER *buf, const char *txt, int txtlen );
static void flush_buffer( PARSE_DATA  *parse_data );
static void comment_hndl(void *data, const char *txt);
static char *isIgnoreMetaName(SWISH * sw, char *tag);




/*********************************************************************
*   Entry to index an XML file.
*
*   Creates an XML_Parser object and parses buffer
*
*   Returns:
*       Count of words indexed
*
*   ToDo:
*       This is a stream parser, so could avoid loading entire document into RAM before parsing
*
*********************************************************************/

int countwords_XML (SWISH *sw, FileProp *fprop, FileRec *fi, char *buffer)
{
    PARSE_DATA          parse_data;
    XML_Parser          p = XML_ParserCreate(NULL);
    IndexFILE          *indexf = sw->indexlist;
    struct StoreDescription *stordesc = fprop->stordesc;


    /* Set defaults  */
    memset(&parse_data, 0, sizeof(parse_data));

    parse_data.header = &indexf->header;
    parse_data.parser = p;
    parse_data.sw     = sw;
    parse_data.fprop  = fprop;
    parse_data.filenum = fi->filenum;
    parse_data.word_pos= 1;  /* compress doesn't like zero */
    parse_data.thisFileEntry = fi;


    /* Don't really like this, as mentioned above */
    if ( stordesc && (parse_data.summary.meta = getPropNameByName(parse_data.header, AUTOPROPERTY_SUMMARY)))
    {
        /* Set property limit size for this document type, and store previous size limit */
        parse_data.summary.save_size = parse_data.summary.meta->max_len;
        parse_data.summary.meta->max_len = stordesc->size;
        parse_data.summary.tag = stordesc->field;
    }
        
    
    addCommonProperties(sw, fprop, fi, NULL,NULL, 0);



    if (!p)
        progerr("Failed to create XML parser object for '%s'", fprop->real_path );


    /* Set event handlers */
    XML_SetUserData( p, (void *)&parse_data );          // local data to pass around
    XML_SetElementHandler(p, start_hndl, end_hndl);
    XML_SetCharacterDataHandler(p, char_hndl);

    if( sw->indexComments )
        XML_SetCommentHandler( p, comment_hndl );

    //XML_SetProcessingInstructionHandler(p, proc_hndl);

    if ( !XML_Parse(p, buffer, fprop->fsize, 1) )
        progwarn("XML parse error in file '%s' line %d.  Error: %s",
                     fprop->real_path, XML_GetCurrentLineNumber(p),XML_ErrorString(XML_GetErrorCode(p))); 


    /* clean up */
    XML_ParserFree(p);

    /* Flush any text left in the buffer, and free the buffer */
    flush_buffer( &parse_data );

    if ( parse_data.text_buffer.buffer )
        efree( parse_data.text_buffer.buffer );


    /* Restore the size in the StoreDescription property */
    if ( parse_data.summary.save_size )
        parse_data.summary.meta->max_len = parse_data.summary.save_size;
        
    return parse_data.total_words;
}
    
/*********************************************************************
*   Start Tag Event Handler
*
*   These routines check to see if a given meta tag should be indexed
*   and if the tags should be added as a property
*
*   To Do:
*       deal with attributes!
*
*********************************************************************/


static void start_hndl(void *data, const char *el, const char **attr)
{
    PARSE_DATA *parse_data = (PARSE_DATA *)data;
    struct metaEntry *m;
    SWISH *sw = parse_data->sw;
    char  tag[MAXSTRLEN + 1];


    /* return if within an ignore block */
    if ( parse_data->ignore_tag )
        return;

    /* Flush any text in the buffer */
    flush_buffer( parse_data );


    if(strlen(el) >= MAXSTRLEN)  // easy way out
    {
        progwarn("Warning: Tag found in %s is too long: '%s'", parse_data->fprop->real_path, el );
        return;
    }

    strcpy(tag,(char *)el);
    strtolower( tag );  // $$$ swish ignores case in xml tags!



    /* Bump on all meta names, unless overridden */
    /* Done before the ignore tag check since still need to bump */

    if (!isDontBumpMetaName(sw->dontbumpstarttagslist, tag))
        parse_data->word_pos++;


    /* check for ignore tag (should propably remove char handler for speed) */
    if ( (parse_data->ignore_tag = isIgnoreMetaName( sw, tag )))
        return;


    /* Check for metaNames */

    if ( (m  = getMetaNameByName( parse_data->header, tag)) )
        m->in_tag++;

    else
    {
        if (sw->UndefinedMetaTags == UNDEF_META_AUTO)
        {
            if (sw->verbose)
                printf("!!!Adding automatic MetaName '%s' found in file '%s'\n", tag, parse_data->fprop->real_path);

            addMetaEntry( parse_data->header, tag, META_INDEX, 0)->in_tag++;
        }


        /* If set to "error" on undefined meta tags, then error */
        if (sw->UndefinedMetaTags == UNDEF_META_ERROR)
            progerr("UndefinedMetaNames=error.  Found meta name '%s' in file '%s', not listed as a MetaNames in config", tag, parse_data->fprop->real_path);
    }


    /* Check property names */

    if ( (m  = getPropNameByName( parse_data->header, tag)) )
        m->in_tag++;


    /* Look to enable StoreDescription */
    {
        SUMMARY_INFO    *summary = &parse_data->summary;
        if ( summary->tag && (strcasecmp( tag, summary->tag ) == 0 ))
            summary->active++;
    }

}


/*********************************************************************
*   End Tag Event Handler
*
*
*
*********************************************************************/


static void end_hndl(void *data, const char *el)
{
    PARSE_DATA *parse_data = (PARSE_DATA *)data;
    char  tag[MAXSTRLEN + 1];
    struct metaEntry *m;

    if(strlen(el) > MAXSTRLEN)
    {
        progwarn("Warning: Tag found in %s is too long: '%s'", parse_data->fprop->real_path, el );
        return;
    }

    strcpy(tag,(char *)el);
    strtolower( tag );

    if ( parse_data->ignore_tag )
    {
        if  (strcmp( parse_data->ignore_tag, tag ) == 0)
            parse_data->ignore_tag = NULL;  // don't free since it's a pointer to the config setting
        return;
    }

    /* Flush any text in the buffer */
    flush_buffer( parse_data );


    /* Don't allow matching across tag boundry */
    if (!isDontBumpMetaName(parse_data->sw->dontbumpendtagslist, tag))
       parse_data->word_pos++;
    


    /* Flag that we are not in tag anymore - tags must be balanced, of course. */

    if ( ( m = getMetaNameByName( parse_data->header, tag) ) )
        if ( m->in_tag )
            m->in_tag--;


    if ( ( m = getPropNameByName( parse_data->header, tag) ) )
        if ( m->in_tag )
            m->in_tag--;


    /* Look to disable StoreDescription */
    {
        SUMMARY_INFO    *summary = &parse_data->summary;
        if ( summary->tag && (strcasecmp( tag, summary->tag ) == 0 ))
            summary->active--;
    }

}

/*********************************************************************
*   Character Data Event Handler
*
*   This does the actual adding of text to the index and adding properties
*   if any tags have been found to index
*
*
*********************************************************************/

static void char_hndl(void *data, const char *txt, int txtlen)
{
    PARSE_DATA         *parse_data = (PARSE_DATA *)data;


    /* If currently in an ignore block, then return */
    if ( parse_data->ignore_tag )
        return;

    /* Buffer the text */
    append_buffer( &parse_data->text_buffer, txt, txtlen );

    /* Some day, might want to have a separate property buffer if need to collect more than plain text */
    // append_buffer( parse_data->prop_buffer, txt, txtlen );

}

/*********************************************************************
*   Append character data to the end of the buffer
*
*   Buffer is extended/created if needed
*
*   ToDo: Flush buffer if it gets too large
*
*
*********************************************************************/

static void append_buffer( CHAR_BUFFER *buf, const char *txt, int txtlen )
{

    if ( !txtlen )  // shouldn't happen
        return;


    /* (re)allocate buf if needed */
    
    if ( buf->cur + txtlen >= buf->max )
    {
        buf->max = ( buf->max + BUFFER_CHUNK_SIZE+1 < buf->cur + txtlen )
                    ? buf->cur + txtlen+1
                    : buf->max + BUFFER_CHUNK_SIZE+1;

        buf->buffer = erealloc( buf->buffer, buf->max+1 );
    }


    memcpy( (void *) &(buf->buffer[buf->cur]), txt, txtlen );
    buf->cur += txtlen;
}




/*********************************************************************
*   Flush buffer - adds words to index, and properties
*
*    2001-08 jmruiz Change structure from IN_FILE | IN_META to IN_FILE
*    Since structure does not have much sense in XML, if we use only IN_FILE 
*    we will save memory and disk space (one byte per location)
*
*
*********************************************************************/
static void flush_buffer( PARSE_DATA  *parse_data )
{
    CHAR_BUFFER *buf = &parse_data->text_buffer;
    SWISH       *sw = parse_data->sw;

    /* anything to do? */
    if ( !buf->cur )
        return;

    buf->buffer[buf->cur] = '\0';


    /* Index the text */
    parse_data->total_words +=
        indexstring( sw, buf->buffer, parse_data->filenum, IN_FILE, 0, NULL, &(parse_data->word_pos) );


    /* Add the properties */
    addDocProperties( parse_data->header, &(parse_data->thisFileEntry->docProperties), (unsigned char *)buf->buffer, buf->cur, parse_data->fprop->real_path );


    /* yuck.  Ok, add to summary, if active */
    {
        SUMMARY_INFO    *summary = &parse_data->summary;
        if ( summary->active )
            addDocProperty( &(parse_data->thisFileEntry->docProperties), summary->meta, (unsigned char *)buf->buffer, buf->cur, 0 );
    }


    /* clear the buffer */
    buf->cur = 0;
}



/*********************************************************************
*   Comments
*
*   Should be able to call the char_hndl
*
*   To Do:
*       Can't use DontBump with comments.  Might need a config variable for that.
*
*********************************************************************/
static void comment_hndl(void *data, const char *txt)
{
    PARSE_DATA  *parse_data = (PARSE_DATA *)data;
    SWISH       *sw = parse_data->sw;
    

    /* Bump position around comments - hard coded, always done to prevent phrase matching */
    parse_data->word_pos++;

    /* Index the text */
    parse_data->total_words +=
        indexstring( sw, (char *)txt, parse_data->filenum, IN_COMMENTS, 0, NULL, &(parse_data->word_pos) );


    parse_data->word_pos++;

}



/*********************************************************************
*   check if a tag is an IgnoreTag
*
*   Note: this returns a pointer to the config set tag, so don't free it!
*
*
*********************************************************************/

static char *isIgnoreMetaName(SWISH * sw, char *tag)
{
    struct swline *tmplist = sw->ignoremetalist;

    if (!tmplist)
        return 0;
        
    while (tmplist)
    {
        if (strcmp(tag, tmplist->line) == 0)
            return tmplist->line;

        tmplist = tmplist->next;
    }

    return NULL;
}


