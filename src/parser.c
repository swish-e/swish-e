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
**  Parser reads from the stream in READ_CHUNK_SIZE (after an initial 4 byte chunk
**  to determine the document encoding).  Text is accumulated in a buffer of
**  BUFFER_CHUNK_SIZE (10K?) size.  The buffer is flushed when a new metatag
**  is found.  That buffer will grow, if needed, but now it will attempt
**  to flush upto the last word boundry if > BUFFER_CHUNK_SIZE.
**
**  The buffer is really only flushed when a real metaName or PropertyName is
**  found, or when the strucutre changes -- anything that changes the
**  properities of the text that might be in the buffer.
**
**  An optional arrangement might be to flush the buffer after processing each
**  READ_CHUNK_SIZE from the stream (flush to last word).  This would limit the
**  character buffer size.  It might be nice to flush on any meta tag (not just
**  tags listed as PropertyNames or MetaNames), but for large XML files one would
**  expect some use of Meta/PropertyNames.  HTML files should flush more often
**  since the structure will change often.  Exceptions to this are large <pre>
**  sections, but then the append_buffer() routine will force a flush when the buffer
**  exceeds BUFFER_CHUNK_SIZE.
**
**  The TXT buffer does flush after every chunk read.
**
**  I doubt messing with any of these would change much...
**
**
** TODO:
**
**  - FileRules title (and all abort_parsing calls - define some constants)
**
**  - There's a lot of mixing of xmlChar and char, which will generate warnings.
**
**  - Add a fprop->orig_path (before ReplaceRules) and a directive BaseURI to be used
**    to fixup relative urls (if no <BASE>).
**    This would save space in the property file, but probably not enough to worry about.
**
**
**  - UndefinedMetaTags ignore might throw things like structure off since
**    processing continues (unlike IgnoreMetaTags).  But everything should balance out.
**
**  - There are two buffers that are created for every file, but these could be done once
**    and only expanded when needed.  If that would make any difference in indexing speed.
**
**  - Note that these parse_*() functions get passed a "buffer" which is not used
**    (to be compatible with old swihs-e buffer-based parsers)
**
**  - XML elements and attributes are all converted to lowercase.
**
*/

/* libxml2 */
#include <libxml/HTMLparser.h>
#include <libxml/xmlerror.h>
#include <libxml/uri.h>


#include <stdarg.h>  // for va_list
#ifdef HAVE_VARARGS_H
#include <varargs.h>  // va_list on Win32
#endif
#include "swish.h"
#include "fs.h"  // for the title check
#include "merge.h"
#include "mem.h"
#include "swstring.h"
#include "search.h"
#include "docprop.h"
#include "error.h"
#include "index.h"
#include "metanames.h"


/* Should be in config.h */

#define BUFFER_CHUNK_SIZE 10000 // This is the size of buffers used to accumulate text
#define READ_CHUNK_SIZE 2048    // The size of chunks read from the stream (4096 seems to cause problems)

/* to buffer text until an end tag is found */

typedef struct {
    char   *buffer;     // text for buffer
    int     cur;        // length
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

#define STACK_SIZE 255  // stack size, but can grow.

typedef struct MetaStackElement {
    struct MetaStackElement *next;      // pointer to *siblings*, if any
    struct metaEntry        *meta;      // pointer to meta that's inuse
    int                      ignore;    // flag that this meta turned on ignore
    char                     tag[1];    // tag to look for
} MetaStackElement, *MetaStackElementPtr;

typedef struct {
    int                 pointer;        // next empty slot in stack
    int                 maxsize;        // size of stack
    int                 ignore_flag;    // count of ignores
    MetaStackElementPtr *stack;         // pointer to an array of stack data
    int                 is_meta;        // is this a metaname or property stack?
} MetaStack;






/* This struct is returned in all call-back functions as user data */

typedef struct {
    CHAR_BUFFER         text_buffer;    // buffer for collecting text
 // CHAR_BUFFER         prop_buffer;    // someday, may want a separate property buffer if want to collect tags within props
    SUMMARY_INFO        summary;        // argh.
    MetaStack           meta_stack;     // stacks for tracking the nested metas
    MetaStack           prop_stack;
    int                 total_words;
    int                 word_pos;
    int                 filenum;
    INDEXDATAHEADER    *header;
    SWISH              *sw;
    FileProp           *fprop;
    FileRec            *thisFileEntry;
    int                 structure[STRUCTURE_END+1];
    int                 parsing_html;
    struct metaEntry   *titleProp;
    struct metaEntry   *titleMeta;
    struct metaEntry   *swishdefaultMeta;
    int                 flush_word;         // flag to flush buffer next time there's a white space.
    xmlSAXHandlerPtr    SAXHandler;         // for aborting, I guess.
    xmlParserCtxtPtr    ctxt;
    CHAR_BUFFER         ISO_Latin1;         // buffer to hold UTF-8 -> ISO Latin-1 converted text
    int                 abort;              // flag to stop parsing
    char               *baseURL;            // for fixing up relative links
    int                 swish_noindex;      // swishindex swishnoindex -- for hiding blocks with comments
} PARSE_DATA;


/* Prototypes */
static void start_hndl(void *data, const char *el, const char **attr);
static void end_hndl(void *data, const char *el);
static void char_hndl(void *data, const char *txt, int txtlen);
static void Whitespace(void *data, const xmlChar *txt, int txtlen);
static void append_buffer( CHAR_BUFFER *buf, const char *txt, int txtlen );
static void flush_buffer( PARSE_DATA  *parse_data, int clear );
static void comment_hndl(void *data, const char *txt);
static char *isIgnoreMetaName(SWISH * sw, char *tag);
static void error(void *data, const char *msg, ...);
static void warning(void *data, const char *msg, ...);
static void process_htmlmeta( PARSE_DATA *parse_data, const char ** attr );
static int check_html_tag( PARSE_DATA *parse_data, char * tag, int start );
static void start_metaTag( PARSE_DATA *parse_data, char * tag, char *endtag, int *meta_append, int *prop_append , int is_html_tag );
static void end_metaTag( PARSE_DATA *parse_data, char * tag, int is_html_tag );
static void init_sax_handler( xmlSAXHandlerPtr SAXHandler, SWISH * sw );
static void init_parse_data( PARSE_DATA *parse_data, SWISH * sw, FileProp * fprop, FileRec *fi, xmlSAXHandlerPtr SAXHandler  );
static void free_parse_data( PARSE_DATA *parse_data );
static void Convert_to_latin1( PARSE_DATA *parse_data, char *txt, int txtlen );
static int parse_chunks( PARSE_DATA *parse_data );

static void index_alt_tab( PARSE_DATA *parse_data, const char **attr );
static char *extract_html_links( PARSE_DATA *parse_data, const char **attr, struct metaEntry *meta_entry, char *tag );
static int read_next_chunk( FileProp *fprop, char *buf, int buf_size, int max_size );
static void abort_parsing( PARSE_DATA *parse_data, int abort_code );
static int get_structure( PARSE_DATA *parse_data );

static void push_stack( MetaStack *stack, char *tag, struct metaEntry *meta, int *append, int ignore );
static int pop_stack_ifMatch( PARSE_DATA *parse_data, MetaStack *stack, char *tag );
static int pop_stack( MetaStack *stack );

static void index_XML_attributes( PARSE_DATA *parse_data, char *tag, const char **attr );
static int  start_XML_ClassAttributes(  PARSE_DATA *parse_data, char *tag, const char **attr, int *meta_append, int *prop_append );
static char *isXMLClassAttribute(SWISH * sw, char *tag);

static void debug_show_tag( char *tag, PARSE_DATA *parse_data, int start, char *message );
static void debug_show_parsed_text( PARSE_DATA *parse_data, char *txt, int len );



/*********************************************************************
*   XML Push parser
*
*   Returns:
*       Count of words indexed
*
*
*********************************************************************/

int parse_XML(SWISH * sw, FileProp * fprop, FileRec *fi, char *buffer)

{
    xmlSAXHandler       SAXHandlerStruct;
    xmlSAXHandlerPtr    SAXHandler = &SAXHandlerStruct;
    PARSE_DATA          parse_data;


    init_sax_handler( SAXHandler, sw );
    init_parse_data( &parse_data, sw, fprop, fi, SAXHandler );


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

int parse_HTML(SWISH * sw, FileProp * fprop, FileRec *fi, char *buffer)
{
    htmlSAXHandler       SAXHandlerStruct;
    htmlSAXHandlerPtr    SAXHandler = &SAXHandlerStruct;
    PARSE_DATA           parse_data;

    init_sax_handler( (xmlSAXHandlerPtr)SAXHandler, sw );
    init_parse_data( &parse_data, sw, fprop, fi, (xmlSAXHandlerPtr)SAXHandler );


    parse_data.parsing_html = 1;
    parse_data.titleProp    = getPropNameByName( parse_data.header, AUTOPROPERTY_TITLE );
    parse_data.titleMeta    = getMetaNameByName( parse_data.header, AUTOPROPERTY_TITLE );
    parse_data.swishdefaultMeta = getMetaNameByName( parse_data.header, AUTOPROPERTY_DEFAULT );

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

int parse_TXT(SWISH * sw, FileProp * fprop, FileRec *fi, char *buffer)
{
    PARSE_DATA          parse_data;
    int                 res;
    char       chars[READ_CHUNK_SIZE];



    /* This does stuff that's not needed for txt */
    init_parse_data( &parse_data, sw, fprop, fi, NULL );


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
    FileProp           *fprop = parse_data->fprop;
    xmlSAXHandlerPtr    SAXHandler = parse_data->SAXHandler;
    int                 res;
    char       chars[READ_CHUNK_SIZE];
    xmlParserCtxtPtr    ctxt;


    /* Now start pulling into the libxml2 parser */

    res = read_next_chunk( fprop, chars, READ_CHUNK_SIZE, sw->truncateDocSize );
    if (res == 0)
        return 0;

    /* Create parser */
    if ( parse_data->parsing_html )
        ctxt = (xmlParserCtxtPtr)htmlCreatePushParserCtxt((htmlSAXHandlerPtr)SAXHandler, parse_data, chars, res, fprop->real_path,0);
    else
        ctxt = xmlCreatePushParserCtxt(SAXHandler, parse_data, chars, res, fprop->real_path);

    parse_data->ctxt = ctxt; // save



    while ( !parse_data->abort && (res = read_next_chunk( fprop, chars, READ_CHUNK_SIZE, sw->truncateDocSize )) )
    {
        if ( parse_data->parsing_html )
            htmlParseChunk((htmlParserCtxtPtr)ctxt, chars, res, 0);
        else
            xmlParseChunk(ctxt, chars, res, 0);

        /* Doesn't seem to make much difference to flush here */
        //flush_buffer( parse_data, 0 );  // flush upto whitespace
    }



    /* Tell the parser we are done, and free it */
    if ( parse_data->parsing_html )
    {
        if ( !parse_data->abort ) // bug in libxml 2.4.5
            htmlParseChunk( (htmlParserCtxtPtr)ctxt, chars, 0, 1 );
        htmlFreeParserCtxt( (htmlParserCtxtPtr)ctxt);
    }
    else
    {
        if ( !parse_data->abort ) // bug in libxml
            xmlParseChunk(ctxt, chars, 0, 1);
        xmlFreeParserCtxt(ctxt);
    }

    /* Daniel Veillard on Nov 21, 2001 says this should not be called for every doc. */
    // But, it probably should be called when done parsing.
    // xmlCleanupParser();


    /* Check for abort condition set while parsing (isoktitle, NoContents) */
    /* (But may not abort if the HTML parser never sees any data) */

    if ( fprop->index_no_content && !parse_data->total_words )
    {
        append_buffer( &parse_data->text_buffer, fprop->real_path, strlen(fprop->real_path) );

        parse_data->meta_stack.ignore_flag = 0;  /* make sure we can write */
        flush_buffer( parse_data, 3 );
    }


    /* Flush any text left in the buffer */

    if ( !parse_data->abort )
        flush_buffer( parse_data, 3 );



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
    SAXHandler->cdataBlock     = (charactersSAXFunc)&char_hndl;
    SAXHandler->ignorableWhitespace = (ignorableWhitespaceSAXFunc)&Whitespace;

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
static void init_parse_data( PARSE_DATA *parse_data, SWISH * sw, FileProp * fprop, FileRec *fi, xmlSAXHandlerPtr SAXHandler  )
{
    IndexFILE          *indexf = sw->indexlist;
    struct StoreDescription *stordesc = fprop->stordesc;

    /* Set defaults  */
    memset( parse_data, 0, sizeof(PARSE_DATA));

    parse_data->header      = &indexf->header;
    parse_data->sw          = sw;
    parse_data->fprop       = fprop;
    parse_data->filenum     = fi->filenum;
    parse_data->word_pos    = 1;  /* compress doesn't like zero */
    parse_data->SAXHandler  = SAXHandler;
    parse_data->thisFileEntry = fi;


    /* Don't really like this, as mentioned above */
    if ( stordesc && (parse_data->summary.meta = getPropNameByName(parse_data->header, AUTOPROPERTY_SUMMARY)))
    {
        /* Set property limit size for this document type, and store previous size limit */
        parse_data->summary.save_size = parse_data->summary.meta->max_len;
        parse_data->summary.meta->max_len = stordesc->size;
        parse_data->summary.tag = stordesc->field;
        if ( parse_data->summary.tag )
            strtolower(parse_data->summary.tag);
    }


    /* Initialize the meta and property stacks */
    /* Not needed for TXT processing, of course */
    {
        MetaStack   *s;

        s = &parse_data->meta_stack;
        s->is_meta = 1;
        s->maxsize = STACK_SIZE;

        s->stack = (MetaStackElementPtr *)emalloc( sizeof( MetaStackElementPtr ) * s->maxsize );
        if ( fprop->index_no_content )
            s->ignore_flag++;

        s = &parse_data->prop_stack;
        s->is_meta = 0;
        s->maxsize = STACK_SIZE;
        s->stack = (MetaStackElementPtr *)emalloc( sizeof( MetaStackElementPtr ) * s->maxsize );
        if ( fprop->index_no_content )  /* only works for HTML */
            s->ignore_flag++;
    }

    addCommonProperties(sw, fprop, fi, NULL, NULL, 0);
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

    if ( parse_data->baseURL )
        efree( parse_data->baseURL );


    /* Pop the stacks */
    while( pop_stack( &parse_data->meta_stack ) );
    while( pop_stack( &parse_data->prop_stack ) );

    /* Free the stacks */
    if ( parse_data->meta_stack.stack )
        efree( parse_data->meta_stack.stack );

    if ( parse_data->prop_stack.stack )
        efree( parse_data->prop_stack.stack );



    /* Restore the size in the StoreDescription property */
    if ( parse_data->summary.save_size )
        parse_data->summary.meta->max_len = parse_data->summary.save_size;

}

/*********************************************************************
*   Start Tag Event Handler
*
*   This is called by libxml2.  It normally just calls start_metaTag()
*   and that decides how to deal with that meta tag.
*   It also converts <meta> and <tag class=foo> into meta tags as swish
*   would expect them (and then calls start_metaTag().
*
*   To Do:
*       deal with attributes!
*
*********************************************************************/


static void start_hndl(void *data, const char *el, const char **attr)
{
    PARSE_DATA *parse_data = (PARSE_DATA *)data;
    char        tag[MAXSTRLEN + 1];
    int         is_html_tag = 0;   // to allow <foo> type of meta tags in HTML
    int         meta_append = 0;   // used to allow siblings metanames
    int         prop_append = 0;


    /* disabeld by a comment? */
    if ( parse_data->swish_noindex )
        return;

    if(strlen(el) >= MAXSTRLEN)  // easy way out
    {
        warning( (void *)data, "Warning: Tag found in %s is too long: '%s'\n", parse_data->fprop->real_path, el );
        return;
    }

    strcpy(tag,(char *)el);
    strtolower( tag );  // xml?


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
            /** Special handling for <A>, <IMG>, and <BASE> tags **/

            /* Extract out links - currently only keep <a> links */
            if ( strcmp( tag, "a") == 0 )
                extract_html_links( parse_data, attr, parse_data->sw->links_meta, "href" );


            /* Extract out links from images */
            else if ( strcmp( tag, "img") == 0 )
            {
                /* Index contents of ALT tag text */
                if (parse_data->sw->IndexAltTag)
                    index_alt_tab( parse_data, attr );

                extract_html_links( parse_data, attr, parse_data->sw->images_meta, "src" );
            }


            /* Extract out the BASE URL for fixups */
            else if ( strcmp( tag, "base") == 0 )
                parse_data->baseURL = estrdup( extract_html_links( parse_data, attr, NULL, "href" ) );
        }

    }


    /* Now check if we are in a meta tag */
    start_metaTag( parse_data, tag, tag, &meta_append, &prop_append, is_html_tag );



    /* Index the content of attributes */

    if ( !parse_data->parsing_html && attr )
    {
        int class_found = 0;

        /* Allow <foo class="bar"> to look like <foo.bar> */

        if ( parse_data->sw->XMLClassAttributes )
            class_found = start_XML_ClassAttributes( parse_data, tag, attr, &meta_append, &prop_append );


        /* Index XML attributes */

        if ( !class_found && parse_data->sw->UndefinedXMLAttributes != UNDEF_META_DISABLE )
            index_XML_attributes( parse_data, tag, attr );
    }

}





/*********************************************************************
*   End Tag Event Handler
*
*   Called by libxml2.
*
*
*
*********************************************************************/


static void end_hndl(void *data, const char *el)
{
    PARSE_DATA *parse_data = (PARSE_DATA *)data;
    char        tag[MAXSTRLEN + 1];
    int         is_html_tag = 0;  // to allow <foo> type of metatags in html.


    /* disabeld by a comment? */
    if ( parse_data->swish_noindex )
        return;

    if(strlen(el) > MAXSTRLEN)
    {
        warning( (void *)data, "Warning: Tag found in %s is too long: '%s'\n", parse_data->fprop->real_path, el );
        return;
    }

    strcpy(tag,(char *)el);
    strtolower( tag );



    if ( parse_data->parsing_html )
    {

        /* <meta> tags are closed in start_hndl */

        if ( (strcmp( tag, "meta") == 0)   )
            return;  // this was flushed at end tag



        /* Deal with structure */
        is_html_tag = check_html_tag( parse_data, tag, 0 );
    }


    end_metaTag( parse_data, tag, is_html_tag );
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


    /* Have we been disabled? */
    if ( !parse_data->SAXHandler->characters )
        return;

    /* disabeld by a comment? */
    if ( parse_data->swish_noindex )
        return;


    /* If currently in an ignore block, then return */
    if ( parse_data->meta_stack.ignore_flag && parse_data->prop_stack.ignore_flag )
        return;

    /* $$$ this was added to limit the buffer size */
    if ( parse_data->text_buffer.cur + txtlen >= BUFFER_CHUNK_SIZE )
        flush_buffer( parse_data, 0 );  // flush upto last word - somewhat expensive



    Convert_to_latin1( parse_data, (char *)txt, txtlen );


    if ( DEBUG_MASK & DEBUG_PARSED_TEXT )
        debug_show_parsed_text( parse_data, parse_data->ISO_Latin1.buffer, parse_data->ISO_Latin1.cur );




    /* Check if we are waiting for a word boundry, and there is white space in the text */
    /* If so, write the word, then reset the structure, then write the rest of the text. */

    if ( parse_data->flush_word  )
    {
        /* look for whitespace */
        char *c = parse_data->ISO_Latin1.buffer;
        int   i;
        for ( i=0; i < parse_data->ISO_Latin1.cur; i++ )
            if ( isspace( (int)c[i] ) )
            {
                append_buffer( &parse_data->text_buffer, parse_data->ISO_Latin1.buffer, i );
                flush_buffer( parse_data, 1 );  // Flush the entire buffer

                parse_data->structure[parse_data->flush_word-1]--;  // now it's ok to turn of the structure bit
                parse_data->flush_word = 0;

                /* flush the rest */
                append_buffer( &parse_data->text_buffer, &c[i], parse_data->ISO_Latin1.cur - i );

                return;
            }
    }



    /* Buffer the text */
    append_buffer( &parse_data->text_buffer, parse_data->ISO_Latin1.buffer, parse_data->ISO_Latin1.cur );

    /* Some day, might want to have a separate property buffer if need to collect more than plain text */
    // append_buffer( &parse_data->prop_buffer, txt, txtlen );



}

/*********************************************************************
*   ignorableWhitespace handler
*
*   Just adds a space to the buffer
*
*
*********************************************************************/

static void Whitespace(void *data, const xmlChar *txt, int txtlen)
{
    PARSE_DATA         *parse_data = (PARSE_DATA *)data;

    append_buffer( &parse_data->text_buffer, " ", 1 );  // could flush buffer, I suppose
}




/*********************************************************************
*   Convert UTF-8 to Latin-1
*
*   Buffer is extended/created if needed
*
*********************************************************************/

static void Convert_to_latin1( PARSE_DATA *parse_data, char *txt, int txtlen )
{
    CHAR_BUFFER     *buf = &parse_data->ISO_Latin1;
    int             inlen = txtlen;
    int             ret;
    char  *start_buf;
    char  *end_buf = txt + txtlen - 1;
    int             used;


    /* (re)allocate buf if needed */

    if ( txtlen >= buf->max )
    {
        buf->max = ( buf->max + BUFFER_CHUNK_SIZE+1 < txtlen )
                    ? buf->max + txtlen+1
                    : buf->max + BUFFER_CHUNK_SIZE+1;

        buf->buffer = erealloc( buf->buffer, buf->max );
    }

    buf->cur = 0;  /* start at the beginning of the buffer */

    while( 1 )
    {
        used = buf->max - buf->cur;             /* size available in buffer */
        start_buf = &buf->buffer[buf->cur];     /* offset into buffer */

        /* Returns 0 for OK */
        ret = UTF8Toisolat1( (unsigned char *)start_buf, &used, (const unsigned char *)txt, &inlen );

        if ( used > 0 )         // tally up total bytes consumed
            buf->cur += used;

        if ( ret >= 0 )         /* libxml2 seems confused about what this should return */
            return;             /* either 0 for ok, or number of chars converted */

        if ( ret == -2 )        // encoding failed
        {
            if ( parse_data->sw->parser_warn_level >= 3 )
                xmlParserWarning(parse_data->ctxt, "Failed to convert internal UTF-8 to Latin-1.\nReplacing non ISO-8859-1 char with char '%c'\n", ENCODE_ERROR_CHAR);


            buf->buffer[buf->cur++] = ENCODE_ERROR_CHAR;


            /* Skip one UTF-8 character -- returns null if not pointing to a UTF-8 char */

            /*
             * xmlUTF8Strpos() calls xmlUTFStrlen() which requires a null-terminated string.
             * so, jump over the utf-8 char here.  Mostly from libxml2's encoding.c.
             *
             *
            if (  !(txt = (char *)xmlUTF8Strpos( (const xmlChar *)(&txt[inlen]), 1) ))
                return;
            */


            {
                char ch;
                txt += inlen;  /* point to the start of the utf-8 char (where conversion left off) */

                /* grab first utf-8 char and check that it's not null */
                if ( 0 == (ch = *txt++) )
                    return;

                /* Make sure valid starting utf-8 char (must be 11xxxxxx) */
                if ( 0xc0 != ( ch & 0xc0 ) )
                    return;

                /* Now skip over the aditional bytes based on the number of high-order bits */
                while ( (ch <<= 1 ) & 0x80 )
                {
                    /* Are we past the end of the buffer? */
                    if ( txt > end_buf )
                    {
                        if ( parse_data->sw->parser_warn_level >= 1 )
                            xmlParserError(parse_data->ctxt, "Incomplete UTF-8 character found\n" );

                        return;
                    }

                    /* all trailing bytes must be 10xx xxxx */
                    if ( 0x80 != (*txt++ & 0xc0) )
                    {
                        if ( parse_data->sw->parser_warn_level >= 1 )
                            xmlParserError(parse_data->ctxt, "Invalid UTF-8 sequence found. A secondary byte was: '0x%2x'\n", (unsigned char)txt[-1] );

                        return;
                    }
                }
            }

            /* Calculate the remaining length of the input string */
            inlen = (unsigned long)end_buf - (unsigned long)txt + 1;
            if ( inlen <= 0 )
                return;

            start_buf += buf->cur-1;
        }
        else
        {
            xmlParserWarning(parse_data->ctxt, "Error '%d' converting internal UTF-8 to Latin-1.\n", ret );
            return;
        }
    }
}


/*********************************************************************
*   Start of a MetaTag
*   All XML tags are metatags, but for HTML there's special handling.
*
*   Call with:
*       parse_data
*       tag         = tag to look for as a metaname/property
*       endtag      = tag to look for as the ending tag (since might be different from start tag)
*       meta_append = if zero, tells push that this is a new meta
*       prop_append   otherwise, says it's a sibling of a previous call
*                     (Argh Jan 29, 2001 -- now I don't remember what that _append does!)
*                     (it's for working with xml attributes)
*       is_html_tag = prevents UndefinedMetaTags from being applied to html tags
*
*   <foo class=bar> can start two meta tags "foo" and "foo.bar".  But "bar"
*   will end both tags.
*
*
*********************************************************************/
static void start_metaTag( PARSE_DATA *parse_data, char * tag, char *endtag, int *meta_append, int *prop_append, int is_html_tag )
{
    SWISH              *sw = parse_data->sw;
    struct metaEntry   *m = NULL;


    /* Bump on all meta names, unless overridden */
    if (!is_html_tag && !isDontBumpMetaName(sw->dontbumpstarttagslist, tag))
        parse_data->word_pos++;

    /* check for ignore tag (should probably remove char handler for speed) */
    // Should specific property names and meta names override this?

    if ( isIgnoreMetaName( sw, tag ) )
    {
        /* shouldn't need to flush buffer since it's just blocking out a section and should be balanced */
        /* but need to due to the weird way the char buffer is used (and shared with props) and how metatags are assigned to the buffer */
        /* basically, since flush_buffer looks at the ignore flag and always clears the buffer, need to do it now */
        /* flush_buffer really should not be in the business of checking the ignore flag, and rather we need to keep two buffers -- or maybe just always flush with any change */

        flush_buffer( parse_data, 1 );

        push_stack( &parse_data->meta_stack, endtag, NULL, meta_append, 1 );
        push_stack( &parse_data->prop_stack, endtag, NULL, prop_append, 1 );
        parse_data->structure[IN_META_BIT]++;  // so we are in balance with pop_stack
        return;
    }


    /* Check for metaNames */

    if ( !(m = getMetaNameByName( parse_data->header, tag)) )
    {

        if ( !is_html_tag )
        {
            if ( sw->UndefinedMetaTags == UNDEF_META_AUTO )
            {
                if (sw->verbose)
                    printf("**Adding automatic MetaName '%s' found in file '%s'\n", tag, parse_data->fprop->real_path);

                m = addMetaEntry( parse_data->header, tag, META_INDEX, 0);
            }


            else if ( sw->UndefinedMetaTags == UNDEF_META_IGNORE )  /* Ignore this block of text for metanames only (props ok) */
            {
                flush_buffer( parse_data, 66 );  // flush because we must still continue to process, and structures might change
                push_stack( &parse_data->meta_stack, endtag, NULL, meta_append, 1 );
                parse_data->structure[IN_META_BIT]++;  // so we are in balance with pop_stack
                /* must fall though to property check */
            }
        }
    }



    if ( m )     /* Is a meta name */
    {
        flush_buffer( parse_data, 6 );  /* new meta tag, so must flush */
        push_stack( &parse_data->meta_stack, endtag, m, meta_append, 0 );
        parse_data->structure[IN_META_BIT]++;
    }

    else if ( !is_html_tag )
    {
        /* If set to "error" on undefined meta tags, then error */
        if ( sw->UndefinedMetaTags == UNDEF_META_ERROR )
                progerr("Found meta name '%s' in file '%s', not listed as a MetaNames in config", tag, parse_data->fprop->real_path);

        else {
            /* In general a single word doesn't span tags */
//            append_buffer( &parse_data->text_buffer, " ", 1 );
              flush_buffer( parse_data, 1 );

            if ( DEBUG_MASK & DEBUG_PARSED_TAGS )
                debug_show_tag( tag, parse_data, 1, "(undefined meta name - no action)" );
        }
    }


    /* Check property names -- allows HTML tags as property names */


    if ( (m  = getPropNameByName( parse_data->header, tag)) )
    {
        if ( is_meta_internal( m ) )
        {
            warning( (void *)parse_data, "Found Swish-e reserved property name '%s'\n", tag );
        }
        else
        {
            flush_buffer( parse_data, 7 );  // flush since it's a new meta tag
            push_stack( &parse_data->prop_stack, endtag, m, prop_append, 0 );
        }
    }



    /* Look to enable StoreDescription - allow any tag */
    /* Don't need to flush since this has it's own buffer */

    // This should really be a property, and use aliasing as needed
    {
        SUMMARY_INFO    *summary = &parse_data->summary;

        if ( summary->tag && (strcmp( tag, summary->tag ) == 0 ))
        {
            /* Flush data in buffer */
            if ( 0 == summary->active )
                flush_buffer( parse_data, 1 );

            summary->active++;
        }
    }

}


/*********************************************************************
*   End of a MetaTag
*   All XML tags are metatags, but for HTML there's special handling.
*
*********************************************************************/
static void end_metaTag( PARSE_DATA *parse_data, char * tag, int is_html_tag )
{

    if ( pop_stack_ifMatch( parse_data, &parse_data->meta_stack, tag ) )
        parse_data->structure[IN_META_BIT]--;


    /* Out of a property? */
    pop_stack_ifMatch( parse_data, &parse_data->prop_stack, tag );


    /* Don't allow matching across tag boundry */
    if (!is_html_tag && !isDontBumpMetaName(parse_data->sw->dontbumpendtagslist, tag))
        parse_data->word_pos++;

    /* Tag normally separate words */
    if (!is_html_tag)
//        append_buffer( &parse_data->text_buffer, " ", 1 );
          flush_buffer( parse_data, 1 );




    /* Look to disable StoreDescription */
    {
        SUMMARY_INFO    *summary = &parse_data->summary;
        if ( summary->tag && (strcasecmp( tag, summary->tag ) == 0 ))
        {
            /* Flush data in buffer */
            if ( 1 == summary->active )
                flush_buffer( parse_data, 1 );  // do first since flush buffer looks at summary->active

            summary->active--;
        }
    }

}


/*********************************************************************
*   Checks the HTML tag, and sets the "structure"
*   Also deals with FileRules title
*   In general, flushes the character buffer due to the change in structure.
*
*   returns false if not a valid HTML tag (which might be a "fake" metaname)
*
*********************************************************************/

static int check_html_tag( PARSE_DATA *parse_data, char * tag, int start )
{
    int     is_html_tag = 1;
    int     bump = start ? +1 : -1;

    /* Check for structure bits */


    /** HEAD **/

    if ( strcmp( tag, "head" ) == 0 )
    {
        flush_buffer( parse_data, 10 );
        parse_data->structure[IN_HEAD_BIT] += bump;

        /* Check for NoContents - can quit looking now once out of <head> block since only looking for <title>*/

        if ( !start && parse_data->fprop->index_no_content )
            abort_parsing( parse_data, 1 );

    }



    /** TITLE **/

    // Note: I think storing the title words by default should be optional.
    // Someone might not want to search title tags, if if they don't they are
    // screwed since title by default ranks higher than body words.


    else if ( strcmp( tag, "title" ) == 0 )
    {
        /* Can't flush buffer until we have looked at the title */

        if ( !start )
        {
            struct MOD_FS *fs = parse_data->sw->FS;

            /* Check isoktitle - before NoContents? */
            if ( match_regex_list( parse_data->text_buffer.buffer, fs->filerules.title, "FileRules title") )
            {
                abort_parsing( parse_data, -2 );
                return 1;
            }

            /* Check for NoContents - abort since all we need is the title text */
            if ( parse_data->fprop->index_no_content )
                abort_parsing( parse_data, 1 );


        }
        else
            /* In start tag, allow capture of text (NoContents sets ignore_flag at start) */
            if ( parse_data->fprop->index_no_content )
                parse_data->meta_stack.ignore_flag--;


        /* Now it's ok to flush */
        flush_buffer( parse_data, 11 );


        /* If title is a property, turn on the property flag */
        if ( parse_data->titleProp )
            parse_data->titleProp->in_tag += bump;


        /* If title is a metaname, turn on the indexing flag */
        if ( parse_data->titleMeta )
        {
            parse_data->titleMeta->in_tag += bump;
            parse_data->swishdefaultMeta->in_tag +=bump;
        }



        parse_data->word_pos++;
        parse_data->structure[IN_TITLE_BIT] += bump;
    }



    /** BODY **/

    else if ( strcmp( tag, "body" ) == 0 )
    {
        flush_buffer( parse_data, 12 );
        parse_data->structure[IN_BODY_BIT] += bump;
        parse_data->word_pos++;
    }



    /** H1 HEADINGS **/

    /* This should be split so know different level for ranking */
    else if ( tag[0] == 'h' && isdigit((int) tag[1]))
    {
        flush_buffer( parse_data, 13 );
        parse_data->structure[IN_HEADER_BIT] += bump;
    }



    /** EMPHASIZED **/

    /* These should not be hard coded */

    else if ( !strcmp( tag, "em" ) || !strcmp( tag, "b" ) || !strcmp( tag, "strong" ) || !strcmp( tag, "i" ) )
    {
        /* This is hard.  The idea is to not break up words.  But messes up the structure
         * ie: "this is b<b>O</b>ld word" so this would only flush "this is" on <b>,
         * and </b> would not flush anything.  The PROBLEM is that then will make the next words
         * have a IN_EMPHASIZED structure.  To "fix", I set a flag to flush at next word boundry.
        */
        flush_buffer( parse_data, 0 );  // flush up to current word (leaving any leading chars in buffer)

        if ( start )
            parse_data->structure[IN_EMPHASIZED_BIT]++;
        else
        {
            /* If there is something in the buffer then delay turning off the flag until whitespace is found */
            if ( parse_data->text_buffer.cur )
                /* Flag to flush at next word boundry */
                parse_data->flush_word = IN_EMPHASIZED_BIT + 1;  // + 1 because we might need to use zero some day
            else
                parse_data->structure[IN_EMPHASIZED_BIT]--;
        }


    }




    /* Now, look for reasons to add whitespace
     * img is not really, as someone might use an image to make up a word, but
     * commonly an image would split up text.
     * other tags: frame?
     */

    if ( !strcmp( tag, "br" ) || !strcmp( tag, "img" ) )
        append_buffer( &parse_data->text_buffer, " ", 1 );  // could flush buffer, I suppose
    else
    {
        const htmlElemDesc *element = htmlTagLookup( (const xmlChar *)tag );

        if ( !element )
            is_html_tag = 0;   // flag that this might be a meta name

        else if ( !element->isinline )
        {
            /* Used to just append a space to the buffer,
               but that didn't prevent phrase matches across tags
            */
            flush_buffer( parse_data, 1 );
            parse_data->word_pos++;
        }
    }




    return is_html_tag;
}

/*********************************************************************
*   Allow <foo class="bar"> to start "foo.bar" meta tag
*
*   Returns true if any found
*
*********************************************************************/
static int  start_XML_ClassAttributes(  PARSE_DATA *parse_data, char *tag, const char **attr, int *meta_append, int *prop_append )
{
    char tagbuf[MAXSTRLEN + 1];
    char *t;
    int   i;
    int  taglen = strlen( tag );
    SWISH *sw = parse_data->sw;
    int   found = 0;

    if(strlen(tag) >= MAXSTRLEN)  // easy way out
    {
        warning( (void *)parse_data, "Warning: Tag found in %s is too long: '%s'\n", parse_data->fprop->real_path, tag );
        return 0;
    }


    strcpy( tagbuf, tag );
    t = tagbuf + taglen;
    *t = '.';  /* hard coded! */
    t++;


    for ( i = 0; attr[i] && attr[i+1]; i+=2 )
    {
        if ( !isXMLClassAttribute( sw, (char *)attr[i]) )
            continue;


        /* Is the tag going to be too long? */
        if ( strlen( (char *)attr[i+1] ) + taglen + 2 > 256 )
        {
            warning( (void *)parse_data, "ClassAttribute on tag '%s' too long\n", tag );
            continue;
        }


        /* All metanames are currently lowercase -- would be better to force this in metanames.c */
        strtolower( tagbuf );

        strcpy( t, (char *)attr[i+1] );         /* create tag.attribute metaname */
        start_metaTag( parse_data, tagbuf, tag, meta_append, prop_append, 0 );
        found++;

        /* Now, nest attributes */
        if ( sw->UndefinedXMLAttributes != UNDEF_META_DISABLE )
            index_XML_attributes( parse_data, tagbuf, attr );

    }

    return found;

}

/*********************************************************************
*   check if a tag is an XMLClassAttributes
*
*   Note: this returns a pointer to the config set tag, so don't free it!
*   Duplicate code!
*
*   This does a case-insensitive lookup
*
*
*********************************************************************/

static char *isXMLClassAttribute(SWISH * sw, char *tag)
{
    struct swline *tmplist = sw->XMLClassAttributes;

    if (!tmplist)
        return 0;

    while (tmplist)
    {
        if (strcasecmp(tag, tmplist->line) == 0)
            return tmplist->line;

        tmplist = tmplist->next;
    }

    return NULL;
}



/*********************************************************************
*   This extracts out the attributes and contents and indexes them
*
*********************************************************************/
static void index_XML_attributes( PARSE_DATA *parse_data, char *tag, const char **attr )
{
    char tagbuf[MAXSTRLEN+1];
    char *content;
    char *t;
    int   i;
    int  meta_append;
    int  prop_append;
    int  taglen = strlen( tag );
    SWISH *sw = parse_data->sw;
    UndefMetaFlag  tmp_undef = sw->UndefinedMetaTags;  // save

    sw->UndefinedMetaTags = sw->UndefinedXMLAttributes;

    if(strlen(tag) >= MAXSTRLEN)  // easy way out
    {
        warning( (void *)parse_data, "Warning: Tag found in %s is too long: '%s'\n", parse_data->fprop->real_path, tag );
        return;
    }


    strcpy( tagbuf, tag );
    t = tagbuf + taglen;
    *t = '.';  /* hard coded! */
    t++;

    for ( i = 0; attr[i] && attr[i+1]; i+=2 )
    {
        meta_append = 0;
        prop_append = 0;

        /* Skip attributes that are XMLClassAttribues */
        if ( isXMLClassAttribute( sw, (char *)attr[i] ) )
            continue;


        if ( strlen( (char *)attr[i] ) + taglen + 2 > 256 )
        {
            warning(" (void *)parse_data, Attribute '%s' on tag '%s' too long to build metaname\n", (char *)attr[i], tag );
            continue;
        }

        strcpy( t, (char *)attr[i] );         /* create tag.attribute metaname */
        content = (char *)attr[i+1];

        if ( !*content )
            continue;

        strtolower( tagbuf );



        flush_buffer( parse_data, 1 ); // isn't needed, right?
        start_metaTag( parse_data, tagbuf, tagbuf, &meta_append, &prop_append, 0 );
        char_hndl( parse_data, content, strlen( content ) );
        end_metaTag( parse_data, tagbuf, 0 );
    }

    sw->UndefinedMetaTags = tmp_undef;
}



/*********************************************************************
*   Deal with html's <meta name="foo" content="bar">
*   Simply calls start and end meta, and passes content
*
*********************************************************************/

static void process_htmlmeta( PARSE_DATA *parse_data, const char **attr )
{
    char *metatag = NULL;
    char *content = NULL;
    int  meta_append = 0;
    int  prop_append = 0;

    int  i;

    /* Don't add any meta data while looking for just the title */
    if ( parse_data->fprop->index_no_content )
        return;

    for ( i = 0; attr[i] && attr[i+1]; i+=2 )
    {
        if ( (strcmp( attr[i], "name" ) == 0 ) && attr[i+1] )
            metatag = (char *)attr[i+1];

        else if ( (strcmp( attr[i], "content" ) == 0 ) && attr[i+1] )
            content = (char *)attr[i+1];
    }


    if ( metatag && content )
    {

        /* Robots exclusion: http://www.robotstxt.org/wc/exclusion.html#meta */
        if ( !strcasecmp( metatag, "ROBOTS") && lstrstr( content, "NOINDEX" ) )
        {
            if ( parse_data->sw->obeyRobotsNoIndex )
                abort_parsing( parse_data, -3 );

            return;
        }

        /* Process as a start -> end tag sequence */
        strtolower( metatag );

        flush_buffer( parse_data, 111 );
        start_metaTag( parse_data, metatag, metatag, &meta_append, &prop_append, 0 );
        char_hndl( parse_data, content, strlen( content ) );
        end_metaTag( parse_data, metatag, 0 );
        flush_buffer( parse_data, 112 );
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
*   If the clear flag is set then the entire buffer is flushed.
*   Otherwise, every thing up to the last *partial* word is flushed.
*   It's partial if there is not white-space at the very end of the buffer.
*
*   This prevents some<b>long</b>word from being flushed into part words.
*
*********************************************************************/
static void flush_buffer( PARSE_DATA  *parse_data, int clear )
{
    CHAR_BUFFER *buf = &parse_data->text_buffer;
    SWISH       *sw = parse_data->sw;
    int         structure = get_structure( parse_data );
    int         orig_end  = buf->cur;
    char        save_char = '?';
    char        *c;

    /* anything to do? */
    if ( !buf->cur )
        return;

    /* look back for word boundry when "clear" is not set */

    if ( !clear && !isspace( (int)buf->buffer[buf->cur-1] ) )  // flush up to current word
    {
        while ( buf->cur > 0 && !isspace( (int)buf->buffer[buf->cur-1] ) )
            buf->cur--;

        if ( !buf->cur )  // then there's only a single word in the buffer
        {
            buf->cur = orig_end;
            if ( buf->cur < BUFFER_CHUNK_SIZE )  // should reall look at indexf->header.maxwordlimit
                return;                          // but just trying to keep the buffer from growing too large
        }

        save_char =  buf->buffer[buf->cur];
    }


    /* Mark the end of the buffer - should switch over to using a length to avoid strlen */

    buf->buffer[buf->cur] = '\0';


    /* Make sure there some non-whitespace chars to print */

    c = buf->buffer;
    while ( *c && isspace( (int)*c ) )
        c++;


    if ( *c )
    {
        /* Index the text */
        if ( !parse_data->meta_stack.ignore_flag )  // this really is wrong -- should not check ignore here.  Fix should be to use two buffers
            parse_data->total_words +=
                indexstring( sw, c, parse_data->filenum, structure, 0, NULL, &(parse_data->word_pos) );

        /* Add the properties */
        addDocProperties( parse_data->header, &(parse_data->thisFileEntry->docProperties), (unsigned char *)buf->buffer, buf->cur, parse_data->fprop->real_path );


        /* yuck - addDocProperties should do this.  Ok, add to summary, if active */
        {
            SUMMARY_INFO    *summary = &parse_data->summary;
            if ( summary->active )
                addDocProperty( &(parse_data->thisFileEntry->docProperties), summary->meta, (unsigned char *)buf->buffer, buf->cur, 0 );
        }
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

}



/*********************************************************************
*   Comments
*
*   Should be able to call the char_hndl
*   Allows comments to enable/disable indexing a block by either:
*
*       <!-- noindex -->
*       <!-- index -->
*       <!-- SwishCommand noindex -->
*       <!-- SwishCommand index -->
*
*
*
*   To Do:
*       Can't use DontBump with comments.  Might need a config variable for that.
*
*********************************************************************/
static void comment_hndl(void *data, const char *txt)
{
    PARSE_DATA  *parse_data = (PARSE_DATA *)data;
    SWISH       *sw = parse_data->sw;
    int         structure = get_structure( parse_data );
    char        *swishcmd;
    char        *comment_text = str_skip_ws( (char *)txt );
    int         found = 0;


    str_trim_ws( comment_text );
    if ( ! *comment_text )
        return;


    /* Strip off SwishCommand - might be for future use */
    if ( ( swishcmd = lstrstr( comment_text, "SwishCommand" )) && swishcmd == comment_text )
    {
        comment_text = str_skip_ws( comment_text + strlen( "SwishCommand" ) );
        found++;
    }

    if ( !strcasecmp( comment_text, "noindex" ) )
    {
        parse_data->swish_noindex++;
        return;
    }
    else if ( !strcasecmp( comment_text, "index" ) )
    {
        if ( parse_data->swish_noindex )
           parse_data->swish_noindex--;

        return;
    }


    if( found || !sw->indexComments )
        return;


    /* Bump position around comments - hard coded, always done to prevent phrase matching */
    parse_data->word_pos++;

    /* Index the text */
    parse_data->total_words +=
        indexstring( sw, comment_text, parse_data->filenum, structure | IN_COMMENTS, 0, NULL, &(parse_data->word_pos) );


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
*   Index ALT tabs
*
*
*********************************************************************/
static void index_alt_tab( PARSE_DATA *parse_data, const char **attr )
{
    int  meta_append = 0;
    int  prop_append = 0;
    char *tagbuf     = parse_data->sw->IndexAltTagMeta;
    char *alt_text   = extract_html_links( parse_data, attr, NULL, "alt");


    if ( !alt_text )
        return;

    /* Index as regular text? */
    if ( !parse_data->sw->IndexAltTagMeta )
    {
        char_hndl( parse_data, alt_text, strlen( alt_text ) );
        return;
    }

    flush_buffer( parse_data, 1 );
    start_metaTag( parse_data, tagbuf, tagbuf, &meta_append, &prop_append, 0 );
    char_hndl( parse_data, alt_text, strlen( alt_text ) );
    end_metaTag( parse_data, tagbuf, 0 );
}




/*********************************************************************
*   Extract out links for indexing
*
*   Pass in a metaname, and a tag
*
*********************************************************************/

static char *extract_html_links( PARSE_DATA *parse_data, const char **attr, struct metaEntry *meta_entry, char *tag )
{
    char *href = NULL;
    int  i;
    int         structure = get_structure( parse_data );
    char       *absoluteURL;
    SWISH      *sw = parse_data->sw;


    if ( !attr )
        return NULL;

    for ( i = 0; attr[i] && attr[i+1]; i+=2 )
        if ( (strcmp( attr[i], tag ) == 0 ) && attr[i+1] )
            href = (char *)attr[i+1];

    if ( !href )
        return NULL;

    if ( !meta_entry ) /* The case for <BASE> */
        return href;


    /* Now, fixup the URL, if possible */

    if ( sw->AbsoluteLinks ) // ?? || parse_data->baseURL??? always fix up if a <BASE> tag?
    {
        char *base = parse_data->baseURL
                     ? parse_data->baseURL
                     : parse_data->fprop->real_path;

        absoluteURL = (char *)xmlBuildURI( (xmlChar *)href, (xmlChar *)base );
    }
    else
        absoluteURL = NULL;



    /* Index the text */
    parse_data->total_words +=
        indexstring( sw, absoluteURL ? absoluteURL : href, parse_data->filenum, structure, 1, &meta_entry->metaID, &(parse_data->word_pos) );

    if ( absoluteURL )
        xmlFree( absoluteURL );

    return href;
}



/* This doesn't look like the best method */

static void abort_parsing( PARSE_DATA *parse_data, int abort_code )
{
    parse_data->abort = abort_code;  /* Flag that the we are all done */
    /* Disable parser */
    parse_data->SAXHandler->startElement   = (startElementSAXFunc)NULL;
    parse_data->SAXHandler->endElement     = (endElementSAXFunc)NULL;
    parse_data->SAXHandler->characters     = (charactersSAXFunc)NULL;
}


/* This sets the current structure context (IN_HEAD, IN_BODY, etc) */

static int get_structure( PARSE_DATA *parse_data )
{
    int structure = IN_FILE;

    /* Set structure bits */
    if ( parse_data->parsing_html )
    {
        int i;
        for ( i = 0; i <= STRUCTURE_END; i++ )
            if ( parse_data->structure[i] )
                structure |= ( 1 << i );
    }
    return structure;
}

/*********************************************************************
*   Push a meta entry onto the stack
*
*   Call With:
*       stack   = which stack to use
*       tag     = Element (tag name) to be used to match end tag
*       met     = metaEntry to save
*       append  = append to current if one (will be incremented)
*       ignore  = if true, then flag as an ignore block and bump ignore counter
*
*   Returns:
*       void
*
*   ToDo:
*       move to Mem_Zone?
*
*
*********************************************************************/

static void push_stack( MetaStack *stack, char *tag, struct metaEntry *meta, int *append, int ignore )
{
    MetaStackElementPtr    node;


    if ( DEBUG_MASK & DEBUG_PARSED_TAGS )
    {
        int i;
        for (i=0; i<stack->pointer; i++)
            printf("    ");

        printf("<%s> (%s [%s]%s)\n", tag, stack->is_meta ? "meta" : "property", !meta ? "no meta name defined" : meta->metaName, ignore ? " *Start Ignore*" : ""  );
    }


    /* Create a new node ( MetaStackElement already has one byte allocated for string ) */
    node = (MetaStackElementPtr) emalloc( sizeof( MetaStackElement ) + strlen( tag ) );
    node->next = NULL;

    /* Turn on the meta */
    if ( (node->meta = meta) )
        meta->in_tag++;

    if ( ( node->ignore = ignore ) )  /* entering a block to ignore */
        stack->ignore_flag++;


    strcpy( node->tag, tag );




    if ( !(*append)++ )
    {
        /* reallocate stack buffer if needed */
        if ( stack->pointer >= stack->maxsize )
        {
            progwarn("swish parser adding more stack space for tag %s. from %d to %d", tag, stack->maxsize, stack->maxsize+STACK_SIZE );

            stack->maxsize += STACK_SIZE;
            stack->stack = (MetaStackElementPtr *)erealloc( stack->stack, sizeof( MetaStackElementPtr ) * stack->maxsize );
        }

        stack->stack[stack->pointer++] = node;
    }
    else // prepend to the list
    {
        if ( !stack->pointer )
            progerr("Tried to append tag %s to stack, but stack is empty", tag );

        node->next = stack->stack[stack->pointer - 1];
        stack->stack[stack->pointer - 1] = node;
    }
}

/*********************************************************************
*   Pop the stack if the tag matches the last entry
*   Will turn off all metas associated with this tag level
*
*   Call With:
*       parse_data = to automatically flush
*       stack   = which stack to use
*       tag     = Element (tag name) to be used for removal
*
*   Returns:
*       true if tag matched
*
*********************************************************************/

static int pop_stack_ifMatch( PARSE_DATA *parse_data, MetaStack *stack, char *tag )
{

    /* return if stack is empty */
    if ( !stack->pointer )
        return 0;



    /* return if doesn't match the tag at the top of the stack */

    if ( strcmp( stack->stack[stack->pointer - 1]->tag, tag ) != 0 )
        return 0;


    flush_buffer( parse_data, 1 );
    pop_stack( stack );

    return 1;
}

/*********************************************************************
*   Pop the stack
*   Will turn off all metas associated with this tag level
*
*   Call With:
*       stack   = which stack to use
*
*   Returns:
*       the stack pointer
*
*********************************************************************/

static int pop_stack( MetaStack *stack )
{
    MetaStackElementPtr    node, this;


    /* return if stack is empty */
    if ( !stack->pointer )
        return 0;

    node =  stack->stack[--stack->pointer];

    /* Now pop the stack. */

    // Note that some end tags can pop more than one tag
    // <foo class="bar"> can be to starting metanames <foo> and <foo:bar>, and </foo> pops all.

    while ( node )
    {
        this = node;

        if ( node->meta )
            node->meta->in_tag--;

        if ( node->ignore )
            stack->ignore_flag--;


        if ( DEBUG_MASK & DEBUG_PARSED_TAGS )
        {
            int i;
            for (i=0; i<stack->pointer; i++)
                printf("    ");

            printf("</%s> (%s)%s\n", node->tag, stack->is_meta ? "meta" : "property", node->ignore ? " end ignore" : "" );
        }


        node = node->next;
        efree( this );
    }

    return stack->pointer;
}

static int debug_get_indent( INDEXDATAHEADER *header )
{
    int i;
    int indent = 0;

    for (i = 0; i < header->metaCounter; i++)
        if ( is_meta_index(header->metaEntryArray[i]) )
            indent += header->metaEntryArray[i]->in_tag;

    return indent;
}



static void debug_show_tag( char *tag, PARSE_DATA *parse_data, int start, char *message )
{
    int  indent = debug_get_indent( &parse_data->sw->indexlist->header);
    int  i;

    for (i=0; i<indent; i++)
        printf("    ");

    printf("<%s%s> %s\n", start ? "" : "/", tag, message );
}

static void debug_show_parsed_text( PARSE_DATA *parse_data, char *txt, int len )
{
    int indent = debug_get_indent( &parse_data->sw->indexlist->header);
    int i;
    char indent_buf[1000];
    int  last_newline = 0;
    int  col = 0;


    indent_buf[0] = '\0';

    for (i=0; i<indent && strlen(indent_buf)<900; i++)
        strcat( indent_buf, "    ");


    i = 0;
    while ( i < len )
    {
        printf("%s", indent_buf );
        col = 0;
        last_newline = 0;

        /* skip leading space */
        while ( i < len && isspace((int)txt[i] ) )
            i++;

        /* print text */
        while ( i < len )
        {
            col++;


            if ( txt[i] == '\n' )
            {
                while ( i < len && isspace((int)txt[i] ))
                    i++;
            }

            if ( !isprint((int)txt[i] ))
            {
                i++;
                continue;
            }

            printf("%c", txt[i] );
            i++;

            if ( (col + strlen( indent_buf ) > 60 && isspace((int)txt[i])) || col + strlen( indent_buf ) > 78 )
            {
                printf("\n");
                last_newline=1;
                break;
            }
        }
    }


    if ( !last_newline )
        printf("\n");
}

