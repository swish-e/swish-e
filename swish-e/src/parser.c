/*
$Id$
**
**
** This program and library is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
**
** 2001-09-21  new HTML parser using libxml2 (http://www.xmlsoft.org/)
**
** TODO:  grab title only.
*/

#include "swish.h"
#include "merge.h"
#include "mem.h"
#include "string.h"
#include "docprop.h"
#include "error.h"
#include "index.h"
#include "metanames.h"
#include <stdarg.h>  // for va_list

/* libxml2 */
#include <libxml/HTMLparser.h>

#define BUFFER_CHUNK_SIZE 20000

/* to buffer text until an end tag is found */

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


/* This struct is returned in all call-back functions as user data */

typedef struct {
    CHAR_BUFFER text_buffer;    // buffer for collecting text

    // CHAR_BUFFER prop_buffer;  // someday, may want a separate property buffer if want to collect tags within props

    SUMMARY_INFO    summary;     // argh.

    char       *ignore_tag;     // tag that triggered ignore (currently used for both)
    int         total_words;
    int         word_pos;
    int         filenum;
    INDEXDATAHEADER *header;
    SWISH      *sw;
    FileProp   *fprop;
    struct file *thisFileEntry;
    int         structure;
    int         in_html_meta;       // flag that we are in a <meta name=...> tag
    int         in_meta;            // counter of nested metas, so structure will work
    int         parsing_html;
    struct metaEntry *titleProp;
    int         flush_word;         // flag to flush buffer next time there's a white space.
    htmlSAXHandlerPtr   SAXHandler;
    
} PARSE_DATA;


/* Prototypes */
static void start_hndl(void *data, const char *el, const char **attr);
static void end_hndl(void *data, const char *el);
static void char_hndl(void *data, const char *txt, int txtlen);
static void append_buffer( CHAR_BUFFER *buf, const char *txt, int txtlen );
static void flush_buffer( PARSE_DATA  *parse_data, int clear );
static void comment_hndl(void *data, const char *txt);
static char *isIgnoreMetaName(SWISH * sw, char *tag);
static void warning(void *data, const char *msg, ...);
static void error(void *data, const char *msg, ...);
static void fatalError(void *data, const char *msg, ...);
static void process_htmlmeta( PARSE_DATA *parse_data, const char ** atts );
static int check_html_tag( PARSE_DATA *parse_data, char * tag, int start );
static void start_metaTag( PARSE_DATA *parse_data, char * tag );
static void end_metaTag( PARSE_DATA *parse_data, char * tag );
static void start_title_tag(void *data, const char *el, const char **attr);
static void end_title_tag(void *data, const char *el);


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

int     parse_XML(SWISH * sw, FileProp * fprop, char *buffer)
{
    PARSE_DATA          parse_data;
    IndexFILE          *indexf = sw->indexlist;
    struct MOD_Index   *idx = sw->Index;
    struct StoreDescription *stordesc = fprop->stordesc;
    xmlSAXHandler       SAXHandlerStruct;
    xmlSAXHandlerPtr    SAXHandler = &SAXHandlerStruct;
    int                 SAXerror;



    /* Set event handlers for libxml2 parser */
    memset( SAXHandler, 0, sizeof( xmlSAXHandler ) );

    SAXHandlerStruct.startElement   = (startElementSAXFunc)&start_hndl;
    SAXHandlerStruct.endElement     = (endElementSAXFunc)&end_hndl;
    SAXHandlerStruct.characters     = (charactersSAXFunc)&char_hndl;
    if( sw->indexComments )
        SAXHandlerStruct.comment    = (commentSAXFunc)&comment_hndl;

    if ( sw->parser_warn_level >= 3 )
        SAXHandlerStruct.fatalError     = (fatalErrorSAXFunc)&fatalError;
    if ( sw->parser_warn_level >= 2 )
        SAXHandlerStruct.error          = (errorSAXFunc)&error;
    if ( sw->parser_warn_level >= 1 )
        SAXHandlerStruct.warning        = (warningSAXFunc)&warning;
    

    
    /* I have no idea why addtofilelist doesn't do this! */
    idx->filenum++;

    /* Set defaults  */
    memset(&parse_data, 0, sizeof(parse_data));

    parse_data.header = &indexf->header;
    parse_data.sw     = sw;
    parse_data.fprop  = fprop;
    parse_data.filenum = idx->filenum;
    parse_data.word_pos= 1;  /* compress doesn't like zero */


    /* Don't really like this, as mentioned above */
    if ( stordesc && (parse_data.summary.meta = getPropNameByName(&indexf->header, AUTOPROPERTY_SUMMARY)))
    {
        /* Set property limit size for this document type, and store previous size limit */
        parse_data.summary.save_size = parse_data.summary.meta->max_len;
        parse_data.summary.meta->max_len = stordesc->size;
        parse_data.summary.tag = stordesc->field;
    }
        
    
    /* Create file entry in index */

    addtofilelist(sw, indexf, fprop->real_path, &(parse_data.thisFileEntry) );
    addCommonProperties(sw, indexf, fprop->mtime, NULL,NULL, 0, fprop->fsize);


    /* Now parse the XML file */

    if ( (SAXerror = xmlSAXUserParseMemory( SAXHandler, &parse_data, buffer, strlen( buffer ))) != 0 )
        progwarn("xmlSAXUserParseMemory returned '%d' for file '%s'", SAXerror, fprop->real_path );

    

    /* Flush any text left in the buffer, and free the buffer */
    flush_buffer( &parse_data, 1 );

    if ( parse_data.text_buffer.buffer )
        efree( parse_data.text_buffer.buffer );


    /* Restore the size in the StoreDescription property */
    if ( parse_data.summary.save_size )
        parse_data.summary.meta->max_len = parse_data.summary.save_size;
        


    addtofwordtotals(indexf, idx->filenum, parse_data.total_words);

    return parse_data.total_words;
}



/*********************************************************************
*   Entry to index an HTML file.
*
*   Returns:
*       Count of words indexed
*
*   ToDo:
*       This is a stream parser, so could avoid loading entire document into RAM before parsing
*
*********************************************************************/

int     parse_HTML(SWISH * sw, FileProp * fprop, char *buffer)
{
    PARSE_DATA          parse_data;
    IndexFILE          *indexf = sw->indexlist;
    struct MOD_Index   *idx = sw->Index;
    struct StoreDescription *stordesc = fprop->stordesc;
    htmlSAXHandler      SAXHandlerStruct;
    htmlSAXHandlerPtr   SAXHandler = &SAXHandlerStruct;



    /* Set event handlers for libxml2 parser */
    memset( SAXHandler, 0, sizeof( htmlSAXHandler ) );

    SAXHandlerStruct.startElement   = (startElementSAXFunc)&start_hndl;
    SAXHandlerStruct.endElement     = (endElementSAXFunc)&end_hndl;
    SAXHandlerStruct.characters     = (charactersSAXFunc)&char_hndl;
    if( sw->indexComments )
        SAXHandlerStruct.comment    = (commentSAXFunc)&comment_hndl;

    if ( sw->parser_warn_level >= 3 )
        SAXHandlerStruct.fatalError     = (fatalErrorSAXFunc)&fatalError;
    if ( sw->parser_warn_level >= 2 )
        SAXHandlerStruct.error          = (errorSAXFunc)&error;
    if ( sw->parser_warn_level >= 1 )
        SAXHandlerStruct.warning        = (warningSAXFunc)&warning;
    
    
    /* I have no idea why addtofilelist doesn't do this! */
    idx->filenum++;

    /* Set defaults  */
    memset(&parse_data, 0, sizeof(parse_data));

    parse_data.header = &indexf->header;
    parse_data.sw     = sw;
    parse_data.fprop  = fprop;
    parse_data.filenum = idx->filenum;
    parse_data.word_pos= 1;  /* compress doesn't like zero */
    parse_data.parsing_html = 1;
    parse_data.titleProp = getPropNameByName( parse_data.header, AUTOPROPERTY_TITLE );
    parse_data.structure = IN_FILE;

    /* Don't really like this, as mentioned above */
    if ( stordesc && (parse_data.summary.meta = getPropNameByName(&indexf->header, AUTOPROPERTY_SUMMARY)))
    {
        /* Set property limit size for this document type, and store previous size limit */
        parse_data.summary.save_size = parse_data.summary.meta->max_len;
        parse_data.summary.meta->max_len = stordesc->size;
        parse_data.summary.tag = stordesc->field;
    }
        
    
    /* Create file entry in index */

    addtofilelist(sw, indexf, fprop->real_path, &(parse_data.thisFileEntry) );
    addCommonProperties(sw, indexf, fprop->mtime, NULL,NULL, 0, fprop->fsize);


    /* Now parse the HTML file */

    htmlSAXParseDoc( buffer, NULL, SAXHandler, &parse_data );

    /* Flush any text left in the buffer, and free the buffer */
    flush_buffer( &parse_data, 1 );

    if ( parse_data.text_buffer.buffer )
        efree( parse_data.text_buffer.buffer );


    /* Restore the size in the StoreDescription property */
    if ( parse_data.summary.save_size )
        parse_data.summary.meta->max_len = parse_data.summary.save_size;
        


    addtofwordtotals(indexf, idx->filenum, parse_data.total_words);

    return parse_data.total_words;
}

/*********************************************************************
*   Grab the title
*
*   Returns:
*       emalloc title, or NULL
*
*********************************************************************/

char *parse_HTML_title(SWISH * sw, FileProp * fprop, char *buffer)
{
    PARSE_DATA          parse_data;
    IndexFILE          *indexf = sw->indexlist;
    htmlSAXHandler      SAXHandlerStruct;
    htmlSAXHandlerPtr   SAXHandler = &SAXHandlerStruct;



    /* Set event handlers for libxml2 parser */
    memset( SAXHandler, 0, sizeof( htmlSAXHandler ) );

    SAXHandlerStruct.startElement   = (startElementSAXFunc)&start_title_tag;
    SAXHandlerStruct.endElement     = (endElementSAXFunc)&end_title_tag;

    
    /* Set defaults  */
    memset(&parse_data, 0, sizeof(parse_data));

    parse_data.titleProp = getPropNameByName( &indexf->header, AUTOPROPERTY_TITLE );
    parse_data.SAXHandler = SAXHandler;

    if ( !parse_data.titleProp )
        return NULL;

    htmlSAXParseDoc( buffer, NULL, SAXHandler, &parse_data );

    if ( parse_data.text_buffer.buffer )
    {
        parse_data.text_buffer.cur = '\0';
        return parse_data.text_buffer.buffer;
    }

    return NULL;
}

static void start_title_tag(void *data, const char *el, const char **attr)
{
    PARSE_DATA *parse_data = (PARSE_DATA *)data;
    htmlSAXHandlerPtr   SAXHandler = parse_data->SAXHandler;
    
    if ( strcmp( el, "title" ) == 0 )
        SAXHandler->characters  = (charactersSAXFunc)&char_hndl;
        
}
static void end_title_tag(void *data, const char *el)
{
    PARSE_DATA *parse_data = (PARSE_DATA *)data;
    htmlSAXHandlerPtr   SAXHandler = parse_data->SAXHandler;

    if ( strcmp( el, "title" ) == 0 )
    {
        SAXHandler->characters     = (charactersSAXFunc)NULL;
        SAXHandler->startElement   = (startElementSAXFunc)NULL;
        SAXHandler->endElement     = (endElementSAXFunc)NULL;
    }
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
    char        tag[MAXSTRLEN + 1];
    int         is_html_tag = parse_data->parsing_html;


    /* return if within an ignore block */
    if ( parse_data->ignore_tag )
        return;

    if(strlen(el) >= MAXSTRLEN)  // easy way out
    {
        progwarn("Warning: Tag found in %s is too long: '%s'", parse_data->fprop->real_path, el );
        return;
    }

    strcpy(tag,(char *)el);
    strtolower( tag );


    if ( parse_data->parsing_html )
    {
        
        /* handle <meta name="metaname" content="foo"> */
        if ( (strcmp( tag, "meta") == 0) && attr  )
        {
            flush_buffer( parse_data, 1 );  // flush since it's a new meta tag
            process_htmlmeta( parse_data, attr );
            return;
        }

        /* Deal with structure */
        if ( (is_html_tag = check_html_tag( parse_data, tag, 1 )) )
            /* And if we are in title, flag to save content as a doc property */
            if ( !strcmp( tag, "title" ) && parse_data->titleProp )
                parse_data->titleProp->in_tag++;
    }


    /* Now check if we are in a meta tag */
    if ( !is_html_tag )
        start_metaTag( parse_data, tag );


    /* Look to enable StoreDescription - allow any tag */
    /* Don't need to flush since this has it's own buffer */
    {
        SUMMARY_INFO    *summary = &parse_data->summary;

        if ( summary->tag && (strcmp( tag, summary->tag ) == 0 ))
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
    char        tag[MAXSTRLEN + 1];
    int         is_html_tag = parse_data->parsing_html;


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

    if ( parse_data->parsing_html )
    {
        
        /* <meta> tags are closed in start_hndl */
        
        if ( (strcmp( tag, "meta") == 0)   )
        {
            flush_buffer( parse_data, 1 );  // flush since it's a new meta tag
            return;
        }



        /* Deal with structure */
        if ( (is_html_tag = check_html_tag( parse_data, tag, 0 )) )
            if ( !strcmp( tag, "title" ) && parse_data->titleProp )
                parse_data->titleProp->in_tag = 0;

    }
    
    if ( !is_html_tag )
        end_metaTag( parse_data, tag );

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


    /* attempt to flush on a word boundry, if possible */
    if ( parse_data->flush_word )
        flush_buffer( parse_data, 0 );

}


/*********************************************************************
*   Start of a MetaTag
*   All XML tags are metatags, but for HTML there's special handling.
*
*********************************************************************/
static void start_metaTag( PARSE_DATA *parse_data, char * tag )
{
    SWISH *sw = parse_data->sw;
    struct metaEntry *m = NULL;

    /* Bump on all meta names, unless overridden */
    /* Done before the ignore tag check since still need to bump */

    if (!isDontBumpMetaName(sw->dontbumpstarttagslist, tag))
        parse_data->word_pos++;


    /* check for ignore tag (should propably remove char handler for speed) */
    
    if ( (parse_data->ignore_tag = isIgnoreMetaName( sw, tag )))
        return;


    /* Check for metaNames */

    if ( (m = getMetaNameByName( parse_data->header, tag)) )
    {
        flush_buffer( parse_data, 1 );  // flush since it's a new meta tag
        
        m->in_tag++;
        parse_data->in_meta++;
        parse_data->structure |= IN_META;
    }


    else if ( sw->applyautomaticmetanames)
    {
        if (sw->verbose)
            printf("**Adding automatic MetaName '%s' found in file '%s'\n", tag, parse_data->fprop->real_path);

        flush_buffer( parse_data, 1 );  // flush since it's a new meta tag
        addMetaEntry( parse_data->header, tag, META_INDEX, 0)->in_tag++;
        parse_data->in_meta++;
        parse_data->structure |= IN_META;
    }


    /* If set to "error" on undefined meta tags, then error */
    if (!sw->OkNoMeta)
        progerr("Found meta name '%s' in file '%s', not listed as a MetaNames in config", tag, parse_data->fprop->real_path);



    /* Check property names, again, limited to <meta> for html */

    if ( (m  = getPropNameByName( parse_data->header, tag)) )
    {
        flush_buffer( parse_data, 1 );  // flush since it's a new meta tag
        m->in_tag++;
    }

}    


/*********************************************************************
*   End of a MetaTag
*   All XML tags are metatags, but for HTML there's special handling.
*
*********************************************************************/
static void end_metaTag( PARSE_DATA *parse_data, char * tag )
{
    struct metaEntry *m = NULL;
    

    /* Don't allow matching across tag boundry */
    if (!isDontBumpMetaName(parse_data->sw->dontbumpendtagslist, tag))
       parse_data->word_pos++;



    /* Flag that we are not in tag anymore - tags must be balanced, of course. */

    if ( ( m = getMetaNameByName( parse_data->header, tag) ) )
    if ( m->in_tag )
    {
        flush_buffer( parse_data, 1 );
        m->in_tag--;
        parse_data->in_meta--;
        if ( !parse_data->in_meta )
            parse_data->structure &= ~IN_META;
    }


    if ( ( m = getPropNameByName( parse_data->header, tag) ) )
        if ( m->in_tag )
        {
            flush_buffer( parse_data, 1 );
            m->in_tag--;
        }

}


/*********************************************************************
*   Checks the HTML tag, and sets the "structure"
*
*   returns false if not a valid HTML tag (which might be a metaname)
*
*********************************************************************/

static int check_html_tag( PARSE_DATA *parse_data, char * tag, int start )
{
    int     structure = 0;
    int     is_html_tag = 1;

    /* Check for structure bits */

    if ( strcmp( tag, "head" ) == 0 )
    {
        flush_buffer( parse_data, 1 );
        structure = IN_HEAD;
    }

    else if ( strcmp( tag, "title" ) == 0 )
    {
        flush_buffer( parse_data, 1 );
        structure = IN_TITLE;
    }

    else if ( strcmp( tag, "body" ) == 0 )
    {
        flush_buffer( parse_data, 1 );
        structure = IN_BODY;
    }

    else if ( tag[0] == 'h' && isdigit((int) tag[1]))
    {
        flush_buffer( parse_data, 1 );
        structure = IN_HEADER;
    }

    /* These should not be hard coded */
    else if ( !strcmp( tag, "em" ) || !strcmp( tag, "b" ) || !strcmp( tag, "strong" ) || !strcmp( tag, "i" ) )
    {
        /* This is hard.  The idea is to not break up words.  But messes up the structure
         * ie: "this is b<b>O</b>ld word" so this would only flush "this is" on <b>,
         * and </b> would not flush anything.  The PROBLEM is that then will make the next words
         * have a IN_EMPHASIZED structure.  To "fix", I set a flag to flush at next word boundry.
        */
        flush_buffer( parse_data, 0 );  // flush up to current word

        if ( start )
            structure = IN_EMPHASIZED;
        else
            parse_data->flush_word = IN_EMPHASIZED;
        
        
    }


    /* Now, look for reasons to add whitespace */
    {
        const htmlElemDesc *element = htmlTagLookup( tag );

        if ( !element )
            is_html_tag = 0;   // flag that this might be a meta name

        else if ( !element->isinline )
            append_buffer( &parse_data->text_buffer, " ", 1 );  // could flush buffer, I suppose
    }

    if ( structure )
    {
        // done when tag is found because some are block and some are char level
        // flush_buffer( parse_data );

        if ( start )
            parse_data->structure |= structure;
        else
            parse_data->structure &= ~structure;
            
    }
    return is_html_tag;
}


/*********************************************************************
*   Deal with html's <meta name="foo" content="bar">
*   Simply calls start and end meta, and passes content
*
*********************************************************************/

static void process_htmlmeta( PARSE_DATA *parse_data, const char **atts )
{
    char *metatag = NULL;
    char *content = NULL;
    int  i;

    for ( i = 0; atts[i] && atts[i+1]; i+=2 )
    {
        if ( (strcmp( atts[i], "name" ) == 0 ) && atts[i+1] )
            metatag = (char *)atts[i+1];

        else if ( (strcmp( atts[i], "content" ) == 0 ) && atts[i+1] )
            content = (char *)atts[i+1];
    }

    if ( metatag && content )
    {
        parse_data->in_html_meta++;
        start_metaTag( parse_data, metatag );
        char_hndl( parse_data, content, strlen( content ) );
        end_metaTag( parse_data, metatag );
        parse_data->in_html_meta--;
    }

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
        buf->buffer = erealloc( buf->buffer, ( buf->max += BUFFER_CHUNK_SIZE+1 ) );


    memcpy( (void *) &(buf->buffer[buf->cur]), txt, txtlen );
    buf->cur += txtlen;
}




/*********************************************************************
*   Flush buffer - adds words to index, and properties
*
*
*********************************************************************/
static void flush_buffer( PARSE_DATA  *parse_data, int clear )
{
    CHAR_BUFFER *buf = &parse_data->text_buffer;
    SWISH       *sw = parse_data->sw;
    int         structure = parse_data->parsing_html ? parse_data->structure : IN_FILE;
    int         orig_end  = buf->cur;
    char        save_char = '?';


    /* anything to do? */
    if ( !buf->cur )
        return;

    /* look back for word boundry */

    if ( !clear && !isspace( buf->buffer[buf->cur-1] ) )  // flush up to current word
    {
        while ( buf->cur > 0 && !isspace( buf->buffer[buf->cur-1] ) )
            buf->cur--;

        if ( !buf->cur )  // then there's only a single word in the buffer
        {
            buf->cur = orig_end;
            return;
        }

        save_char =  buf->buffer[buf->cur];
    }
            

    /* Mark the end of the buffer - should switch over to using a length to avoid strlen */

    buf->buffer[buf->cur] = '\0';



    /* Index the text */
    parse_data->total_words +=
        indexstring( sw, buf->buffer, parse_data->filenum, structure, 0, NULL, &(parse_data->word_pos) );


    /* Add the properties */
    addDocProperties( parse_data->header, &(parse_data->thisFileEntry->docProperties), (unsigned char *)buf->buffer, buf->cur, parse_data->fprop->real_path );


    /* yuck.  Ok, add to summary, if active */
    {
        SUMMARY_INFO    *summary = &parse_data->summary;
        if ( summary->active )
            addDocProperty( &(parse_data->thisFileEntry->docProperties), summary->meta, (unsigned char *)buf->buffer, buf->cur, 0 );
    }


    /* clear the buffer */

    if ( orig_end && orig_end > buf->cur )
    {
        buf->buffer[buf->cur] = save_char;  // put back the char where null was placed
        memmove( buf->buffer, &buf->buffer[buf->cur], orig_end - buf->cur );
        buf->cur = orig_end - buf->cur;
    }
    else
        buf->cur = 0;

    if ( parse_data->flush_word )
    {
        parse_data->structure &= ~parse_data->flush_word;
        parse_data->flush_word = 0;
    }

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
        indexstring( sw, (char *)txt, parse_data->filenum, parse_data->structure | IN_COMMENTS, 0, NULL, &(parse_data->word_pos) );


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

/******************************************************************
*  Warning and Error Messages
*
******************************************************************/

static void warning(void *data, const char *msg, ...)
{
    va_list args;
    PARSE_DATA *parse_data = (PARSE_DATA *)data;

    va_start(args, msg);
    fprintf(stdout, "\n** %s [WARNING] ", parse_data->fprop->real_path);
    vfprintf(stdout, msg, args);
    va_end(args);
}

static void error(void *data, const char *msg, ...)
{
    va_list args;
    PARSE_DATA *parse_data = (PARSE_DATA *)data;

    va_start(args, msg);
    fprintf(stdout, "\n** %s [ERROR] ", parse_data->fprop->real_path);
    vfprintf(stdout, msg, args);
    va_end(args);
}

static void fatalError(void *data, const char *msg, ...)
{
    va_list args;
    PARSE_DATA *parse_data = (PARSE_DATA *)data;

    va_start(args, msg);
    fprintf(stdout, "\n** %s [FATAL ERROR] ", parse_data->fprop->real_path);
    vfprintf(stdout, msg, args);
    va_end(args);
}

