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
** 2001-09-21  new HTML parser using libxml2 (http://www.xmlsoft.org/) Bill Moseley
**
** TODO:
**
**  - FileRules title (and all abort_parsing calls - define some constants)
**
**  - There are two buffers that are created for every file, but these could be done once
**    and only expanded when needed.  If that would make any difference in indexing speed.
**
**  - There is also a read chunk stack variable that's 1024 bytes.  Is stack space an issue?
**
**  - Note that these parse_*() functions get passed a "buffer" which is not used
**    (to be compatible with old swihs-e buffer-based parsers)
**
*/

#include <stdarg.h>  // for va_list
#include "swish.h"
#include "fs.h"  // for the title check
#include "merge.h"
#include "mem.h"
#include "string.h"
#include "docprop.h"
#include "error.h"
#include "index.h"
#include "metanames.h"

/* libxml2 */
#include <libxml/HTMLparser.h>
#include <libxml/xmlerror.h>

/* Should be in config.h */

#define BUFFER_CHUNK_SIZE 10000 // This is the size of buffers used to accumulate text
#define READ_CHUNK_SIZE 4096    // The size of chunks read from the stream

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
    CHAR_BUFFER         text_buffer;    // buffer for collecting text
 // CHAR_BUFFER         prop_buffer;  // someday, may want a separate property buffer if want to collect tags within props
    SUMMARY_INFO        summary;     // argh.
    char               *ignore_tag;     // tag that triggered ignore (currently used for both)
    int                 total_words;
    int                 word_pos;
    int                 filenum;
    INDEXDATAHEADER    *header;
    SWISH              *sw;
    FileProp           *fprop;
    struct file        *thisFileEntry;
    int                 structure;
    int                 in_meta;            // counter of nested metas, so structure will work
    int                 parsing_html;
    struct metaEntry   *titleProp;
    int                 flush_word;         // flag to flush buffer next time there's a white space.
    xmlSAXHandlerPtr    SAXHandler;  // for title only 
    xmlParserCtxtPtr    ctxt;
    CHAR_BUFFER         ISO_Latin1;         // buffer to hold UTF-8 -> ISO Latin-1 converted text
    int                 abort;              // flag to stop parsing
} PARSE_DATA;


/* Prototypes */
static void start_hndl(void *data, const char *el, const char **attr);
static void end_hndl(void *data, const char *el);
static void char_hndl(void *data, const char *txt, int txtlen);
static void append_buffer( CHAR_BUFFER *buf, const char *txt, int txtlen );
static void flush_buffer( PARSE_DATA  *parse_data, int clear );
static void comment_hndl(void *data, const char *txt);
static char *isIgnoreMetaName(SWISH * sw, char *tag);
static void error(void *data, const char *msg, ...);
static void warning(void *data, const char *msg, ...);
static void process_htmlmeta( PARSE_DATA *parse_data, const char ** atts );
static int check_html_tag( PARSE_DATA *parse_data, char * tag, int start );
static void start_metaTag( PARSE_DATA *parse_data, char * tag );
static void end_metaTag( PARSE_DATA *parse_data, char * tag );
static void init_sax_handler( xmlSAXHandlerPtr SAXHandler, SWISH * sw );
static void init_parse_data( PARSE_DATA *parse_data, SWISH * sw, FileProp * fprop, xmlSAXHandlerPtr SAXHandler );
static void free_parse_data( PARSE_DATA *parse_data ); 
static int Convert_to_latin1( PARSE_DATA *parse_data, const char *txt, int txtlen );
static int parse_chunks( PARSE_DATA *parse_data );
static void extract_html_links( PARSE_DATA *parse_data, const char **atts, struct metaEntry *meta_entry );
static int read_next_chunk( FileProp *fprop, char *buf, int buf_size, int max_size );
static void abort_parsing( PARSE_DATA *parse_data, int abort_code );


/*********************************************************************
*   XML Push parser
*
*   Returns:
*       Count of words indexed
*
*
*********************************************************************/

int     parse_XML(SWISH * sw, FileProp * fprop, char *buffer)
{
    xmlSAXHandler       SAXHandlerStruct;
    xmlSAXHandlerPtr    SAXHandler = &SAXHandlerStruct;
    PARSE_DATA          parse_data;


    init_sax_handler( SAXHandler, sw );
    init_parse_data( &parse_data, sw, fprop, SAXHandler );
    

    /* Now parse the XML file */
    return parse_chunks( &parse_data );

}

/*********************************************************************
*   HTML Push parser
*
*   Returns:
*       Count of words indexed
*
*********************************************************************/

int     parse_HTML(SWISH * sw, FileProp * fprop, char *buffer)
{
    htmlSAXHandler       SAXHandlerStruct;
    htmlSAXHandlerPtr    SAXHandler = &SAXHandlerStruct;
    PARSE_DATA           parse_data;

    init_sax_handler( (xmlSAXHandlerPtr)SAXHandler, sw );
    init_parse_data( &parse_data, sw, fprop, (xmlSAXHandlerPtr)SAXHandler );
    

    parse_data.parsing_html = 1;
    parse_data.titleProp    = getPropNameByName( parse_data.header, AUTOPROPERTY_TITLE );
    parse_data.structure    = IN_FILE;
    
    /* Now parse the HTML file */
    return parse_chunks( &parse_data );

}

/*********************************************************************
*   TXT "Push" parser
*
*   Returns:
*       Count of words indexed
*
*********************************************************************/

int     parse_TXT(SWISH * sw, FileProp * fprop, char *buffer)
{
    PARSE_DATA          parse_data;
    int                 res;
    char                chars[READ_CHUNK_SIZE];
    IndexFILE          *indexf = sw->indexlist;
    struct MOD_Index   *idx = sw->Index;

    init_parse_data( &parse_data, sw, fprop, NULL );
    parse_data.structure    = IN_FILE;


    /* Document Summary */
    if ( parse_data.summary.meta && parse_data.summary.meta->max_len )
        parse_data.summary.active++;

    
    while ( (res = read_next_chunk( fprop, chars, READ_CHUNK_SIZE, sw->truncateDocSize )) )
    {
        append_buffer( &parse_data.text_buffer, chars, res );
        flush_buffer( &parse_data, 0 );  // flush upto whitespace

        /* turn off summary when we exceed size */
        if ( parse_data.summary.meta && parse_data.summary.meta->max_len && fprop->bytes_read > parse_data.summary.meta->max_len )
            parse_data.summary.active = 0;

    }

    flush_buffer( &parse_data, 1 );
    addtofwordtotals(indexf, idx->filenum, parse_data.total_words);
    free_parse_data( &parse_data );
    return parse_data.total_words;
}


/*********************************************************************
*   Parse chunks (used for both XML and HTML parsing)
*   Creates the parsers, reads in chunks as one might expect
*
*
*********************************************************************/
static int parse_chunks( PARSE_DATA *parse_data )
{
    SWISH              *sw = parse_data->sw;
    IndexFILE          *indexf = sw->indexlist;
    struct MOD_Index   *idx = sw->Index;
    FileProp           *fprop = parse_data->fprop;
    xmlSAXHandlerPtr    SAXHandler = parse_data->SAXHandler;
    int                 res;
    char                chars[READ_CHUNK_SIZE];
    xmlParserCtxtPtr    ctxt;

    res = read_next_chunk( fprop, chars, READ_CHUNK_SIZE, sw->truncateDocSize );
    if (res == 0)
        return 0;

    /* Create parser */
    if ( parse_data->parsing_html )
        ctxt = (xmlParserCtxtPtr)htmlCreatePushParserCtxt((htmlSAXHandlerPtr)SAXHandler, parse_data, chars, res, fprop->real_path,0);
    else
        ctxt = xmlCreatePushParserCtxt(SAXHandler, parse_data, chars, res, fprop->real_path);

    parse_data->ctxt = ctxt; // save


    while ( (res = read_next_chunk( fprop, chars, READ_CHUNK_SIZE, sw->truncateDocSize )) )
    {
        if ( parse_data->parsing_html )
            htmlParseChunk((htmlParserCtxtPtr)ctxt, chars, res, 0);
        else
            xmlParseChunk(ctxt, chars, res, 0);


        /* Check for abort condition set while parsing (isoktitle, NoContents) */

        if ( parse_data->abort )
        {
            /* For NoContents add path name if title was not found */

            if ( fprop->index_no_content && !parse_data->total_words )
            {
                append_buffer( &parse_data->text_buffer, fprop->real_path, strlen(fprop->real_path) );
                flush_buffer( parse_data, 3 );
            }

            break;
        }
    }

    /* Tell the parser we are done, and free it */
    if ( parse_data->parsing_html )
    {
        htmlParseChunk( (htmlParserCtxtPtr)ctxt, chars, 0, 1 );
        htmlFreeParserCtxt( (htmlParserCtxtPtr)ctxt);
    }
    else
    {
        xmlParseChunk(ctxt, chars, 0, 1);
        xmlFreeParserCtxt(ctxt);
    }

    /* Not sure if this is needed */
    xmlCleanupParser();

    
    /* Flush any text left in the buffer */

    if ( !parse_data->abort )
        flush_buffer( parse_data, 3 );

    addtofwordtotals(indexf, idx->filenum, parse_data->total_words);


    free_parse_data( parse_data );

    // $$$ This doesn't work since the file (and maybe some words) already added
    // $$$ need a way to "remove" the file entry and words already added 

    if ( parse_data->abort < 0 )
        return parse_data->abort;

    return parse_data->total_words;
}

/*********************************************************************
*   read_next_chunk - read another chunk from the stream
*
*   Call with:
*       fprop
*       *buf        - where to save the data
*       *buf_size   - max size of buffer
*       *max_size   - limit of *total* bytes read from this stream (for truncate)
*
*   Returns:
*       number of bytes read (as returned from fread)
*
*
*********************************************************************/
static int read_next_chunk( FileProp *fprop, char *buf, int buf_size, int max_size )
{
    int size;
    int res;

    if ( fprop->done )
        return 0;

    /* For -S prog, only read in the right amount of data */
    if ( fprop->external_program && (fprop->bytes_read >= fprop->fsize ))
        return 0;


    /* fprop->external_program is set if -S prog and NOT reading from a filter */

    size = fprop->external_program && (( fprop->fsize - fprop->bytes_read ) < buf_size)
           ? fprop->fsize - fprop->bytes_read
           : buf_size;

    if ( !fprop->bytes_read && size > 4 )
        size = 4;

               

    /* Truncate -- safety feature from Rainer.  No attempt is made to backup to a whole word */
    if ( max_size && fprop->bytes_read + size > max_size )
    {
        fprop->done++;  // flag that we are done
        size = max_size - fprop->bytes_read;
    }


    res = fread(buf, 1, size, fprop->fp);
            
    fprop->bytes_read += res;

    return res;
}



/*********************************************************************
*   Init a sax handler structure
*   Must pass in the structure
*
*********************************************************************/
static void init_sax_handler( xmlSAXHandlerPtr SAXHandler, SWISH * sw )
{
    /* Set event handlers for libxml2 parser */
    memset( SAXHandler, 0, sizeof( xmlSAXHandler ) );

    SAXHandler->startElement   = (startElementSAXFunc)&start_hndl;
    SAXHandler->endElement     = (endElementSAXFunc)&end_hndl;
    SAXHandler->characters     = (charactersSAXFunc)&char_hndl;

    if( sw->indexComments )
        SAXHandler->comment    = (commentSAXFunc)&comment_hndl;

    if ( sw->parser_warn_level >= 1 )
        SAXHandler->fatalError     = (fatalErrorSAXFunc)&error;

    if ( sw->parser_warn_level >= 2 )
        SAXHandler->error          = (errorSAXFunc)&error;

    if ( sw->parser_warn_level >= 3 )
        SAXHandler->warning        = (warningSAXFunc)&warning;

}


/*********************************************************************
*   Init the parer data structure
*   Must pass in the structure
*
*********************************************************************/
static void init_parse_data( PARSE_DATA *parse_data, SWISH * sw, FileProp * fprop, xmlSAXHandlerPtr SAXHandler  )
{
    IndexFILE          *indexf = sw->indexlist;
    struct MOD_Index   *idx = sw->Index;
    struct StoreDescription *stordesc = fprop->stordesc;

    /* Set defaults  */
    memset( parse_data, 0, sizeof(PARSE_DATA));

    parse_data->header      = &indexf->header;
    parse_data->sw          = sw;
    parse_data->fprop       = fprop;
    parse_data->filenum     = idx->filenum + 1;
    parse_data->word_pos    = 1;  /* compress doesn't like zero */
    parse_data->SAXHandler  = SAXHandler;


    /* Don't really like this, as mentioned above */
    if ( stordesc && (parse_data->summary.meta = getPropNameByName(&indexf->header, AUTOPROPERTY_SUMMARY)))
    {
        /* Set property limit size for this document type, and store previous size limit */
        parse_data->summary.save_size = parse_data->summary.meta->max_len;
        parse_data->summary.meta->max_len = stordesc->size;
        parse_data->summary.tag = stordesc->field;
    }



    /* I have no idea why addtofilelist doesn't do this! */
    idx->filenum++;

    /* Create file entry in index */

    addtofilelist(sw, indexf, fprop->real_path, &(parse_data->thisFileEntry) );
    addCommonProperties(sw, indexf, fprop->mtime, NULL,NULL, 0, fprop->fsize);


}    


/*********************************************************************
*   Free any data used by the parse_data struct
*
*********************************************************************/
static void free_parse_data( PARSE_DATA *parse_data )
{
    
    if ( parse_data->ISO_Latin1.buffer )
        efree( parse_data->ISO_Latin1.buffer );

    if ( parse_data->text_buffer.buffer )
        efree( parse_data->text_buffer.buffer );


    /* Restore the size in the StoreDescription property */
    if ( parse_data->summary.save_size )
        parse_data->summary.meta->max_len = parse_data->summary.save_size;

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
            process_htmlmeta( parse_data, attr );
            return;
        }


        /* Deal with structure */
        if ( (is_html_tag = check_html_tag( parse_data, tag, 1 )) )
        {
            /* And if we are in title, flag to save content as a doc property */
            if ( !strcmp( tag, "title" ) && parse_data->titleProp )
                parse_data->titleProp->in_tag++;

            /* Extract out links - currently only keep <a> links */
            if ( (strcmp( tag, "a") == 0) && attr && parse_data->sw->links_meta  )
                extract_html_links( parse_data, attr, parse_data->sw->links_meta );
        }

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
            return;  // this was flushed at end tag



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
    int ret;


   
    /* Have we been disabled? */
    if ( !parse_data->SAXHandler->characters )
        return;


    /* If currently in an ignore block, then return */
    if ( parse_data->ignore_tag )
        return;


    /* $$$ this was added to limit the buffer size */
    if ( parse_data->text_buffer.cur + txtlen >= BUFFER_CHUNK_SIZE )
        flush_buffer( parse_data, 0 );  // flush upto last word - somewhat expensive


    
    if ( (ret = Convert_to_latin1( parse_data, txt, txtlen )) != 0 )
    {
        if ( parse_data->sw->parser_warn_level >= 1 )
            xmlParserWarning(parse_data->ctxt, "Failed to convert internal UTF-8 to Latin-1.\nIndexing w/o conversion.\n");
            
        append_buffer( &parse_data->text_buffer, txt, txtlen );
        return;
    }


    /* Buffer the text */
    append_buffer( &parse_data->text_buffer, parse_data->ISO_Latin1.buffer, parse_data->ISO_Latin1.cur );

    /* Some day, might want to have a separate property buffer if need to collect more than plain text */
    // append_buffer( &parse_data->prop_buffer, txt, txtlen );


    /* attempt to flush on a word boundry, if possible */
    if ( parse_data->flush_word )
        flush_buffer( parse_data, 0 );

}

/*********************************************************************
*   Convert UTF-8 to Latin-1
*
*   Buffer is extended/created if needed
*   Returns true if failed.
*
*********************************************************************/

static int Convert_to_latin1( PARSE_DATA *parse_data, const char *txt, int txtlen )
{
    CHAR_BUFFER *buf = &parse_data->ISO_Latin1;
    int     inlen = txtlen;
    

    /* (re)allocate buf if needed */
    
    if ( txtlen >= buf->max )
    {
        buf->max = ( buf->max + BUFFER_CHUNK_SIZE+1 < txtlen )
                    ? buf->max + txtlen+1
                    : buf->max + BUFFER_CHUNK_SIZE+1;

        buf->buffer = erealloc( buf->buffer, buf->max );
    }

    buf->cur = buf->max;

    return UTF8Toisolat1( buf->buffer, &buf->cur, (unsigned char *)txt, &inlen );
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
        flush_buffer( parse_data, 5 );  // flush since it's a new meta tag
        
        m->in_tag++;
        parse_data->in_meta++;
        parse_data->structure |= IN_META;
    }


    else if ( sw->applyautomaticmetanames)
    {
        if (sw->verbose)
            printf("**Adding automatic MetaName '%s' found in file '%s'\n", tag, parse_data->fprop->real_path);

        flush_buffer( parse_data, 6 );  // flush since it's a new meta tag
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
        flush_buffer( parse_data, 7 );  // flush since it's a new meta tag
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
    

    /* Flag that we are not in tag anymore - tags must be balanced, of course. */

    if ( ( m = getMetaNameByName( parse_data->header, tag) ) )
        if ( m->in_tag )
        {
            flush_buffer( parse_data, 8 );
            m->in_tag--;
            parse_data->in_meta--;
            if ( !parse_data->in_meta )
                parse_data->structure &= ~IN_META;
        }


    if ( ( m = getPropNameByName( parse_data->header, tag) ) )
        if ( m->in_tag )
        {
            flush_buffer( parse_data, 9 );
            m->in_tag--;
        }


    /* Don't allow matching across tag boundry */
    if (!isDontBumpMetaName(parse_data->sw->dontbumpendtagslist, tag))
       parse_data->word_pos++;

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
        flush_buffer( parse_data, 10 );
        structure = IN_HEAD;

        /* Check for NoContents - can quit looking now once out of <head> block */

        if ( !start && parse_data->fprop->index_no_content )
            abort_parsing( parse_data, 1 );

    }


    else if ( strcmp( tag, "title" ) == 0 )
    {
        if ( !start )
        {
            struct MOD_FS *fs = parse_data->sw->FS;

            /* Check isoktitle */
            if ( match_regex_list( parse_data->text_buffer.buffer, fs->filerules.title) )
            {
                abort_parsing( parse_data, -2 );
                return 1;
            }

            /* Check for NoContents */
            if ( parse_data->fprop->index_no_content )
                abort_parsing( parse_data, 1 );
        }
    
        flush_buffer( parse_data, 11 );
        parse_data->word_pos++;
        structure = IN_TITLE;
    }

    else if ( strcmp( tag, "body" ) == 0 )
    {
        flush_buffer( parse_data, 12 );
        structure = IN_BODY;
        parse_data->word_pos++;
    }

    else if ( tag[0] == 'h' && isdigit((int) tag[1]))
    {
        flush_buffer( parse_data, 13 );
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

    /* Don't add any meta data while looking for just the title */
    if ( parse_data->fprop->index_no_content )
        return;

    for ( i = 0; atts[i] && atts[i+1]; i+=2 )
    {
        if ( (strcmp( atts[i], "name" ) == 0 ) && atts[i+1] )
            metatag = (char *)atts[i+1];

        else if ( (strcmp( atts[i], "content" ) == 0 ) && atts[i+1] )
            content = (char *)atts[i+1];
    }

    if ( metatag && content )
    {

        /* Robots exclusion: http://www.robotstxt.org/wc/exclusion.html#meta */
        if ( !strcasecmp( metatag, "ROBOTS") && lstrstr( content, "NOINDEX" ) )
        {
            abort_parsing( parse_data, -3 );
            return;
        }

        /* Process meta tag */

        flush_buffer( parse_data, 99 );  /* just in case it's not a MetaName */
        start_metaTag( parse_data, metatag );
        char_hndl( parse_data, content, strlen( content ) );
        end_metaTag( parse_data, metatag );
        flush_buffer( parse_data, 99 );  /* just in case it's not a MetaName */
        
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
    {
        buf->max = ( buf->max + BUFFER_CHUNK_SIZE+1 < buf->cur + txtlen )
                    ? buf->cur + txtlen+1
                    : buf->max + BUFFER_CHUNK_SIZE+1;

        buf->buffer = erealloc( buf->buffer, buf->max+1 );
    }

    memcpy( (void *) &(buf->buffer[buf->cur]), txt, txtlen );
    buf->cur += txtlen;
    buf->buffer[buf->cur] = '\0';  /* seems like a nice thing to do -- only used now in title check */
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

    if ( !clear && !isspace( (int)buf->buffer[buf->cur-1] ) )  // flush up to current word
    {
        while ( buf->cur > 0 && !isspace( (int)buf->buffer[buf->cur-1] ) )
            buf->cur--;

        if ( !buf->cur )  // then there's only a single word in the buffer
        {
            buf->cur = orig_end;
            if ( buf->cur < BUFFER_CHUNK_SIZE )  // should reall look at indexf->header.maxwordlimit 
                return;                          // but just trying to keep the buffer from growing too large
printf("Warning: File %s has a really long word found\n", parse_data->fprop->real_path );
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




    /* This is to allow inline tags to continue to end of word str<b>ON</b>g  */
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

static void error(void *data, const char *msg, ...)
{
    va_list args;
    PARSE_DATA *parse_data = (PARSE_DATA *)data;
    char str[1000];

    va_start(args, msg);
    vsnprintf(str, 1000, msg, args );
    va_end(args);
    xmlParserError(parse_data->ctxt, str);
}

static void warning(void *data, const char *msg, ...)
{
    va_list args;
    PARSE_DATA *parse_data = (PARSE_DATA *)data;
    char str[1000];

    va_start(args, msg);
    vsnprintf(str, 1000, msg, args );
    va_end(args);
    xmlParserWarning(parse_data->ctxt, str);
}


/*********************************************************************
*   Extract out links for indexing
*
*   Needs to be expanded, but currently just pulls out <a href> links.
*   ExtractHTMLlinks <metaname>
*
*********************************************************************/

static void extract_html_links( PARSE_DATA *parse_data, const char **atts, struct metaEntry *meta_entry )
{
    char *href = NULL;
    int  i;



    for ( i = 0; atts[i] && atts[i+1]; i+=2 )
        if ( (strcmp( atts[i], "href" ) == 0 ) && atts[i+1] )
            href = (char *)atts[i+1];

    if ( !href )
        return;

    /* Index the text */
    parse_data->total_words +=
        indexstring( parse_data->sw, href, parse_data->filenum, parse_data->structure, 1, &meta_entry->metaID, &(parse_data->word_pos) );
}

/* This doesn't look like the best method */
static void abort_parsing( PARSE_DATA *parse_data, int abort_code )
{
    parse_data->abort = abort_code;  /* Flag that the we are all done */
    /* Disable parser */
    parse_data->SAXHandler->startElement   = (startElementSAXFunc)NULL;
    parse_data->SAXHandler->endElement     = (endElementSAXFunc)NULL;
    parse_data->SAXHandler->characters     = (charactersSAXFunc)NULL;

    // if ( abort_code < 0 )
    // $$$ mark_file_deleted( parse_data->indexf, parse_data->filenum );
}

