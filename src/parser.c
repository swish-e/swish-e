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
#include <libxml/uri.h>

/* Should be in config.h */

#define BUFFER_CHUNK_SIZE 10000 // This is the size of buffers used to accumulate text
#define READ_CHUNK_SIZE 2048    // The size of chunks read from the stream (4096 seems to cause problems)

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

#define STACK_SIZE 255  // stack size, but can grow.

typedef struct MetaStackElement {
    struct MetaStackElement *next;      // pointer to sibling, if more one
    struct metaEntry        *meta;      // pointer to meta that's inuse
    int                      ignore;    // flag that this meta turned on ignore
    char                     tag[1];    // tag to look for
} MetaStackElement, *MetaStackElementPtr;

typedef struct {
    int                 pointer;        // next empty slot in stack
    int                 maxsize;        // size of stack
    int                 ignore_flag;    // count of ignores
    MetaStackElementPtr *stack;         // pointer to an array of stack data
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
    struct file        *thisFileEntry;
    int                 structure[STRUCTURE_END+1];
    int                 parsing_html;
    struct metaEntry   *titleProp;
    int                 flush_word;         // flag to flush buffer next time there's a white space.
    xmlSAXHandlerPtr    SAXHandler;         // for aborting, I guess.
    xmlParserCtxtPtr    ctxt;
    CHAR_BUFFER         ISO_Latin1;         // buffer to hold UTF-8 -> ISO Latin-1 converted text
    int                 abort;              // flag to stop parsing
    char               *baseURL;            // for fixing up relative links
} PARSE_DATA;


/* Prototypes */
static void start_hndl(void *data, const char *el, const char **attr);
static void end_hndl(void *data, const char *el);
static void char_hndl(void *data, const char *txt, int txtlen);
static void ignorableWhitespace(void *data, const char *txt, int txtlen);
static void append_buffer( CHAR_BUFFER *buf, const char *txt, int txtlen );
static void flush_buffer( PARSE_DATA  *parse_data, int clear );
static void comment_hndl(void *data, const char *txt);
static char *isIgnoreMetaName(SWISH * sw, char *tag);
static void error(void *data, const char *msg, ...);
static void warning(void *data, const char *msg, ...);
static void process_htmlmeta( PARSE_DATA *parse_data, const char ** attr );
static int check_html_tag( PARSE_DATA *parse_data, char * tag, int start );
static void start_metaTag( PARSE_DATA *parse_data, char * tag, char *endtag, int *meta_append, int *prop_append );
static void end_metaTag( PARSE_DATA *parse_data, char * tag );
static void init_sax_handler( xmlSAXHandlerPtr SAXHandler, SWISH * sw );
static void init_parse_data( PARSE_DATA *parse_data, SWISH * sw, FileProp * fprop, xmlSAXHandlerPtr SAXHandler );
static void free_parse_data( PARSE_DATA *parse_data ); 
static int Convert_to_latin1( PARSE_DATA *parse_data, const char *txt, int txtlen );
static int parse_chunks( PARSE_DATA *parse_data );
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


    /* This does stuff that's not needed for txt */
    init_parse_data( &parse_data, sw, fprop, NULL );


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
        if ( !parse_data->abort ) // bug in libxml
            htmlParseChunk( (htmlParserCtxtPtr)ctxt, chars, 0, 1 );
        htmlFreeParserCtxt( (htmlParserCtxtPtr)ctxt);
    }
    else
    {
        if ( !parse_data->abort ) // bug in libxml
            xmlParseChunk(ctxt, chars, 0, 1);
        xmlFreeParserCtxt(ctxt);
    }

    /* Not sure if this is needed */
    xmlCleanupParser();

    /* Check for abort condition set while parsing (isoktitle, NoContents) */

    if ( parse_data->abort && fprop->index_no_content && !parse_data->total_words )
    {
        append_buffer( &parse_data->text_buffer, fprop->real_path, strlen(fprop->real_path) );

        parse_data->meta_stack.ignore_flag = 0;  /* make sure we can write */
        flush_buffer( parse_data, 3 );
    }
    
    
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
    SAXHandler->characters     = (charactersSAXFunc)&char_hndl;
    SAXHandler->ignorableWhitespace = (ignorableWhitespaceSAXFunc)&ignorableWhitespace;

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


    /* Initialize the meta and property stacks */
    /* Not needed for TXT processing, of course */
    {
        MetaStack   *s;

        s = &parse_data->meta_stack;
        s->maxsize = STACK_SIZE;
        s->stack = (MetaStackElementPtr *)emalloc( sizeof( MetaStackElementPtr ) * s->maxsize );
        if ( fprop->index_no_content )
            s->ignore_flag++;

        s = &parse_data->prop_stack;
        s->maxsize = STACK_SIZE;
        s->stack = (MetaStackElementPtr *)emalloc( sizeof( MetaStackElementPtr ) * s->maxsize );
        if ( fprop->index_no_content )  /* only works for HTML */
            s->ignore_flag++;
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
    int         is_html_tag = 0;   // to allow <foo> type of meta tags in HTML
    int         meta_append = 0;   // used allow siblings metanames
    int         prop_append = 0;



    if(strlen(el) >= MAXSTRLEN)  // easy way out
    {
        warning("Warning: Tag found in %s is too long: '%s'\n", parse_data->fprop->real_path, el );
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
            /** Special handling for <A>, <IMG>, and <BASE> tags **/

            /* Extract out links - currently only keep <a> links */
            if ( strcmp( tag, "a") == 0 )
                extract_html_links( parse_data, attr, parse_data->sw->links_meta, "href" );


            /* Extract out links from images */
            else if ( strcmp( tag, "img") == 0 )
                extract_html_links( parse_data, attr, parse_data->sw->images_meta, "src" );


            /* Extract out the BASE URL for fixups */
            else if ( strcmp( tag, "base") == 0 )
                parse_data->baseURL = estrdup( extract_html_links( parse_data, attr, NULL, "href" ) );
        }
        
    }


    /* Now check if we are in a meta tag */
    if ( !is_html_tag )
        start_metaTag( parse_data, tag, tag, &meta_append, &prop_append );


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
    int         is_html_tag = 0;  // to allow <foo> type of metatags in html.

    if(strlen(el) > MAXSTRLEN)
    {
        warning("Warning: Tag found in %s is too long: '%s'\n", parse_data->fprop->real_path, el );
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
    if ( parse_data->meta_stack.ignore_flag && parse_data->prop_stack.ignore_flag )
        return;

    /* $$$ this was added to limit the buffer size */
    if ( parse_data->text_buffer.cur + txtlen >= BUFFER_CHUNK_SIZE )
        flush_buffer( parse_data, 0 );  // flush upto last word - somewhat expensive



    if ( (ret = Convert_to_latin1( parse_data, txt, txtlen )) != 0 )
    {
        if ( parse_data->sw->parser_warn_level >= 1 )
            xmlParserWarning(parse_data->ctxt, "Failed to convert internal UTF-8 to Latin-1.\nIndexing w/o conversion.\n");

        if ( DEBUG_MASK & DEBUG_PARSED_TEXT )
            debug_show_parsed_text( parse_data, (char *)txt, txtlen );

        append_buffer( &parse_data->text_buffer, txt, txtlen );
        return;
    }

    if ( DEBUG_MASK & DEBUG_PARSED_TEXT )
        debug_show_parsed_text( parse_data, parse_data->ISO_Latin1.buffer, parse_data->ISO_Latin1.cur );

    /* Buffer the text */
    append_buffer( &parse_data->text_buffer, parse_data->ISO_Latin1.buffer, parse_data->ISO_Latin1.cur );

    /* Some day, might want to have a separate property buffer if need to collect more than plain text */
    // append_buffer( &parse_data->prop_buffer, txt, txtlen );


    /* attempt to flush on a word boundry, if possible */
    if ( parse_data->flush_word )
        flush_buffer( parse_data, 0 );

}

/*********************************************************************
*   ignorableWhitespace handler
*
*   Just adds a space to the buffer
*
*
*********************************************************************/

static void ignorableWhitespace(void *data, const char *txt, int txtlen)
{
    PARSE_DATA         *parse_data = (PARSE_DATA *)data;

    append_buffer( &parse_data->text_buffer, " ", 1 );  // could flush buffer, I suppose
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

    return UTF8Toisolat1( (unsigned char *)buf->buffer, &buf->cur, (unsigned char *)txt, &inlen );
}




/*********************************************************************
*   Start of a MetaTag
*   All XML tags are metatags, but for HTML there's special handling.
*
*   Call with:
*       parse_data
*       tag         = tag to look for as a metaname/property
*       endtag      = tag to look for as the ending tag
*       meta_append = if zero, tells push that this is a new meta
*       prop_append   otherwise, says it's a sibling of a previous call
*
*   <foo class=bar> can start two meta tags "foo" and "foo.bar".  But "bar"
*   will end both tags.
*
*
*********************************************************************/
static void start_metaTag( PARSE_DATA *parse_data, char * tag, char *endtag, int *meta_append, int *prop_append )
{
    SWISH              *sw = parse_data->sw;
    struct metaEntry   *m = NULL;


    /* Bump on all meta names, unless overridden */
    if (!isDontBumpMetaName(sw->dontbumpstarttagslist, tag))
        parse_data->word_pos++;


    /* check for ignore tag (should propably remove char handler for speed) */
    
    if ( isIgnoreMetaName( sw, tag ) )
    {
        /* shouldn't need to flush buffer since it's just blocking out a section and should be balanced */
        push_stack( &parse_data->meta_stack, endtag, NULL, meta_append, 1 );
        push_stack( &parse_data->prop_stack, endtag, NULL, prop_append, 1 );
        parse_data->structure[IN_META_BIT]++;  // so we are in balance with pop_stack
        return;
    }


    /* Check for metaNames */

    if ( !(m = getMetaNameByName( parse_data->header, tag)) )
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


    if ( m )
    {
        if ( DEBUG_MASK & DEBUG_PARSED_TAGS )
            debug_show_tag( tag, parse_data, 1, "(MetaName)" );

        flush_buffer( parse_data, 6 );  /* new meta tag, so must flush */
        push_stack( &parse_data->meta_stack, endtag, m, meta_append, 0 );
        parse_data->structure[IN_META_BIT]++;
    }

    /* If set to "error" on undefined meta tags, then error */
    else if ( sw->UndefinedMetaTags == UNDEF_META_ERROR )
            progerr("Found meta name '%s' in file '%s', not listed as a MetaNames in config", tag, parse_data->fprop->real_path);

    else if ( DEBUG_MASK & DEBUG_PARSED_TAGS )
        debug_show_tag( tag, parse_data, 1, "" );
            



    /* Check property names, again, limited to <meta> for html */


    if ( (m  = getPropNameByName( parse_data->header, tag)) )
    {
        flush_buffer( parse_data, 7 );  // flush since it's a new meta tag
        push_stack( &parse_data->prop_stack, endtag, m, prop_append, 0 );
    }

}    


/*********************************************************************
*   End of a MetaTag
*   All XML tags are metatags, but for HTML there's special handling.
*
*********************************************************************/
static void end_metaTag( PARSE_DATA *parse_data, char * tag )
{

    if ( pop_stack_ifMatch( parse_data, &parse_data->meta_stack, tag ) )
        parse_data->structure[IN_META_BIT]--;


    /* Out of a property? */
    pop_stack_ifMatch( parse_data, &parse_data->prop_stack, tag );


    /* Don't allow matching across tag boundry */
    if (!isDontBumpMetaName(parse_data->sw->dontbumpendtagslist, tag))
       parse_data->word_pos++;

    if ( DEBUG_MASK & DEBUG_PARSED_TAGS )
        debug_show_tag( tag, parse_data, 0, "" );

}


/*********************************************************************
*   Checks the HTML tag, and sets the "structure"
*
*   returns false if not a valid HTML tag (which might be a metaname)
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

        /* Check for NoContents - can quit looking now once out of <head> block */

        if ( !start && parse_data->fprop->index_no_content )
            abort_parsing( parse_data, 1 );

    }



    /** TITLE **/
    
    else if ( strcmp( tag, "title" ) == 0 )
    {
        /* Can't flush buffer until we have looked at the title */
        
        if ( !start )
        {
            struct MOD_FS *fs = parse_data->sw->FS;

            /* Check isoktitle - before NoContents? */
            if ( match_regex_list( parse_data->text_buffer.buffer, fs->filerules.title) )
            {
                abort_parsing( parse_data, -2 );
                return 1;
            }

            /* Check for NoContents */
            if ( parse_data->fprop->index_no_content )
                abort_parsing( parse_data, 1 );
        }
        else
            /* In start tag, allow capture of text */
            if ( parse_data->fprop->index_no_content )
                parse_data->meta_stack.ignore_flag--;
        
        
        /* Now it's ok to flush */
        flush_buffer( parse_data, 11 );


        /* If title is a property, turn on the property flag */
        if ( parse_data->titleProp )
            parse_data->titleProp->in_tag = start ? 1 : 0;


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
        flush_buffer( parse_data, 0 );  // flush up to current word

        if ( start )
            parse_data->structure[IN_EMPHASIZED_BIT] += bump;
        else
            parse_data->flush_word = IN_EMPHASIZED_BIT + 1;  // + 1 because we might need to use zero some day
        
        
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
            append_buffer( &parse_data->text_buffer, " ", 1 );  // could flush buffer, I suppose
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
    char tagbuf[256];  /* we have our limits */
    char *t;
    int   i;
    int  taglen = strlen( tag );
    SWISH *sw = parse_data->sw;
    int   found = 0;
    
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
            warning("ClassAttribute on tag '%s' too long\n", tag );
            continue;
        }


        /* All metanames are currently lowercase -- would be better to force this in metanames.c */
        strtolower( tagbuf );
        
        strcpy( t, (char *)attr[i+1] );         /* create tag.attribute metaname */
        start_metaTag( parse_data, tagbuf, tag, meta_append, prop_append );
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
    char tagbuf[256];  /* we have our limits */
    char *content;
    char *t;
    int   i;
    int  meta_append;
    int  prop_append;
    int  taglen = strlen( tag );
    SWISH *sw = parse_data->sw;
    UndefMetaFlag  tmp_undef = sw->UndefinedMetaTags;  // save

    sw->UndefinedMetaTags = sw->UndefinedXMLAttributes;
    

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
            warning("Attribute '%s' on tag '%s' too long to build metaname\n", (char *)attr[i], tag );
            continue;
        }
        
        strcpy( t, (char *)attr[i] );         /* create tag.attribute metaname */
        content = (char *)attr[i+1];

        if ( !*content )
            continue;

        strtolower( tagbuf );



        flush_buffer( parse_data, 1 );
        start_metaTag( parse_data, tagbuf, tagbuf, &meta_append, &prop_append );
        char_hndl( parse_data, content, strlen( content ) );
        end_metaTag( parse_data, tagbuf );
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

        flush_buffer( parse_data, 1 );
        start_metaTag( parse_data, metatag, metatag, &meta_append, &prop_append );
        char_hndl( parse_data, content, strlen( content ) );
        end_metaTag( parse_data, metatag );
        
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
        if ( !parse_data->meta_stack.ignore_flag )
            parse_data->total_words +=
                indexstring( sw, c, parse_data->filenum, structure, 0, NULL, &(parse_data->word_pos) );

        /* Add the properties */
        addDocProperties( parse_data->header, &(parse_data->thisFileEntry->docProperties), (unsigned char *)buf->buffer, buf->cur, parse_data->fprop->real_path );


        /* yuck.  Ok, add to summary, if active */
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




    /* This is to allow inline tags to continue to end of word str<b>ON</b>g  */
    if ( parse_data->flush_word )
    {
        parse_data->structure[parse_data->flush_word-1]--;
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
    int         structure = get_structure( parse_data );
    

    /* Bump position around comments - hard coded, always done to prevent phrase matching */
    parse_data->word_pos++;

    /* Index the text */
    parse_data->total_words +=
        indexstring( sw, (char *)txt, parse_data->filenum, structure | IN_COMMENTS, 0, NULL, &(parse_data->word_pos) );


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

    // if ( abort_code < 0 )
    // $$$ mark_file_deleted( parse_data->indexf, parse_data->filenum );
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

        
    /* return if doesn't match stack */
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

    while ( node )
    {
        this = node;

        if ( node->meta )
            node->meta->in_tag--;

        if ( node->ignore )
            stack->ignore_flag--;
            
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


    indent_buf[0] = '\0';

    for (i=0; i<indent; i++)
        strcat( indent_buf, "    ");
            

    i = 0;
    while ( i < len )
    {
        printf("%s", indent_buf );

        /* skip leading space */
        while ( i < len && isspace((int)txt[i] ) )
            i++;

        /* print text */
        while ( i < len )
        {
            if ( txt[i] == '\n' )
            {
                printf("\n");
                i++;
                last_newline=1;
                continue;
            }

            if ( !isprint((int)txt[i] ))
            {
                i++;
                continue;
            }

            printf("%c", txt[i] );
            i++;

            if ( i + strlen( indent_buf ) > 65 )
            {
                printf("\n");
                last_newline=1;
                continue;
            }
        }
    }


    if ( !last_newline )
        printf("\n");
}
    
