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
*/

#include "swish.h"
#include "merge.h"
#include "mem.h"
#include "string.h"
#include "docprop.h"
#include "error.h"
#include "index.h"
#include "metanames.h"

#include "xmlparse.h"  // James Clark's Expat

typedef struct {
    int        *metas;          // array of current metaIDs in use
    int         meta_cnt;       // current end pointer + 1
    int         metasize;

    int        *props;          // array of current propIDs in use
    int         prop_cnt;
    int         propsize;
    
    char       *buffer;         // text buffer for summary
    int         buffmax;        // size of buffer
    int         buffend;        // end of buffer.
    struct metaEntry *summeta;  // summary metaEntry

    char       *ignore_tag;     // tag that triggered ignore (currently used for both)
    int         total_words;
    int         word_pos;
    int         filenum;
    XML_Parser *parser;
    INDEXDATAHEADER *header;
    SWISH      *sw;
    FileProp   *fprop;
    struct file *thisFileEntry;
    
} PARSE_DATA;


/* Prototypes */
static void start_hndl(void *data, const char *el, const char **attr);
static void end_hndl(void *data, const char *el);
static void char_hndl(void *data, const char *txt, int txtlen);
static void comment_hndl(void *data, const char *txt);
static char *isIgnoreMetaName(SWISH * sw, char *tag);
static void add_meta( PARSE_DATA *parse_data, struct metaEntry *m );
static void add_prop( PARSE_DATA *parse_data, struct metaEntry *m );
static void append_summary_text( PARSE_DATA *parse_data, char *buf, int len);
static void write_summary( PARSE_DATA *parse_data );




/*********************************************************************
*   Entry to index an XML file.
*
*   Creates an XML_Parser object and parses buffer
*
*   Returns:
*       Count of words indexed
*
*
*********************************************************************/

int     countwords_XML(SWISH * sw, FileProp * fprop, char *buffer)
{
    PARSE_DATA          parse_data;
    XML_Parser          p = XML_ParserCreate(NULL);
    IndexFILE          *indexf = sw->indexlist;
    struct MOD_Index   *idx = sw->Index;

    /* I have no idea why addtofilelist doesn't do this! */
    idx->filenum++;

    /* Set defaults  */
    memset(&parse_data, 0, sizeof(parse_data));

    parse_data.header = &indexf->header;
    parse_data.parser = p;
    parse_data.sw     = sw;
    parse_data.fprop  = fprop;
    parse_data.filenum = idx->filenum;
    parse_data.word_pos= 1;  /* compress doesn't like zero */


    addtofilelist(sw, indexf, fprop->real_path, &(parse_data.thisFileEntry) );
    addCommonProperties(sw, indexf, fprop->mtime, NULL,NULL, 0, fprop->fsize);



    if (!p)
        progerr("Failed to create XML parser object for '%s'", fprop->real_path );



    /* allocate some space */
    parse_data.propsize = parse_data.metasize = parse_data.header->metaCounter + 100;
    parse_data.props  = (int *) emalloc( sizeof( int *) * parse_data.propsize );
    parse_data.metas  = (int *) emalloc( sizeof( int *) * parse_data.metasize );


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
    char  *tag = estrdup( (char *)el );
    struct StoreDescription *stordesc = parse_data->fprop->stordesc;

    strtolower( tag );


    /* Check for store description */
    if ( stordesc && !parse_data->buffer && ( strcmp(stordesc->field, tag) == 0)
         && (parse_data->summeta = getPropNameByName(parse_data->header, AUTOPROPERTY_SUMMARY) ))
    {
        parse_data->buffmax = stordesc->size < RD_BUFFER_SIZE
                             ? stordesc->size
                             : RD_BUFFER_SIZE;

        parse_data->buffer = (char *) emalloc( parse_data->buffmax + 1 );
        parse_data->buffend = 0;
    }



    /* return if within an ignore block */
    if ( parse_data->ignore_tag )
        return;

    /* Bump on all meta names, unless overridden */
    /* Done before the ignore tag check since still need to bump */

    if (!isDontBumpMetaName(sw->dontbumpstarttagslist, tag))
        parse_data->word_pos++;


    /* check for ignore tag (should propably remove char handler for speed) */
    if ( (parse_data->ignore_tag = isIgnoreMetaName( sw, tag )))
        return;


    

    /* Check for metaNames */

    if ( (m  = getMetaNameByName( parse_data->header, tag)) )
        add_meta( parse_data, m );

    else
    {
        if (sw->applyautomaticmetanames)
        {
            if (sw->verbose)
                printf("Adding automatic MetaName '%s' found in file '%s'\n", tag, parse_data->fprop->real_path);

            add_meta( parse_data, addMetaEntry( parse_data->header, tag, META_INDEX, 0));
        }


        /* If set to "error" on undefined meta tags, then error */
        if (!sw->OkNoMeta)
            progerr("UndefinedMetaNames=error.  Found meta name '%s' in file '%s', not listed as a MetaNames in config", tag, parse_data->fprop->real_path);
    }


    /* Check property names */

    if ( (m  = getPropNameByName( parse_data->header, tag)) )
        add_prop( parse_data, m );


    /* Check for store description */
    

    efree( tag );
}

/* kind of ugly duplication */
static void add_meta( PARSE_DATA *parse_data, struct metaEntry *m )
{
    if ( parse_data->meta_cnt >=  parse_data->metasize )
    {
        parse_data->metasize += 100;
        parse_data->metas = (int *) erealloc( parse_data->metas, sizeof(int *) * parse_data->metasize);
    }
    parse_data->metas[ parse_data->meta_cnt++ ] = m->metaID;
}

static void add_prop( PARSE_DATA *parse_data, struct metaEntry *m )
{
    if ( parse_data->prop_cnt >= parse_data->propsize )
    {
        parse_data->propsize += 100;
        parse_data->props = (int *) erealloc( parse_data->props, sizeof(int *) * parse_data->propsize);
    }
    parse_data->props[ parse_data->prop_cnt++ ] = m->metaID;
}



/*********************************************************************
*   End Tag Event Handler
*
*   This routine will pop the meta/property tag off the stack
*
*
*********************************************************************/


static void end_hndl(void *data, const char *el)
{
    PARSE_DATA *parse_data = (PARSE_DATA *)data;
    char  *tag = estrdup( (char *)el );


    strtolower( tag );

    /* Check for store description */
    if ( parse_data->buffer && ( strcmp(parse_data->fprop->stordesc->field, tag) == 0))
        write_summary( parse_data );
    

    if ( parse_data->ignore_tag )
    {
        if  (strcmp( parse_data->ignore_tag, tag ) == 0)
        {
            efree( parse_data->ignore_tag );
            parse_data->ignore_tag = NULL;
        }
        return;
    }

    /* Don't allow matching across tag boundry */
    if (!isDontBumpMetaName(parse_data->sw->dontbumpendtagslist, tag))
       parse_data->word_pos++;
    

    /* Tags must be balanced, of course. */

    /* Exiting a metaID? */
    if ( parse_data->meta_cnt &&  getMetaNameByName( parse_data->header, tag) )
        parse_data->meta_cnt--;
        

    /* Exiting a propID? */
    if ( parse_data->prop_cnt &&  getPropNameByName( parse_data->header, tag) )
        parse_data->prop_cnt--;

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
    SWISH              *sw = parse_data->sw;
    int                 i;
    char *buf = (char *)emalloc( txtlen + 1 );

    strncpy( buf, txt, txtlen );
    buf[txtlen] = '\0';


    /* Add text to summary */
    if ( parse_data->buffer && parse_data->buffend < parse_data->fprop->stordesc->size )
        append_summary_text( parse_data, buf, txtlen);


        


    /* If currently in an ignore block, then return */
    if ( parse_data->ignore_tag )
        return;


    /* If outside all meta tags then add default */
    if ( !parse_data->meta_cnt && sw->OkNoMeta )
    {
        struct metaEntry *m = getMetaNameByName( parse_data->header, AUTOPROPERTY_DEFAULT );
        if ( m )
            add_meta( parse_data, m );
    }


    /* Index the text */
    /* 2001-08 jmruiz Change structure from IN_FILE | IN_META to IN_FILE */
    /* Since structure does not have much sense in XML, if we use only IN_FILE 
    ** we will save memory and disk space (one byte per location) */
    if ( parse_data->meta_cnt )
        parse_data->total_words +=
            indexstring(
                sw,
                buf,
                parse_data->filenum,
                IN_FILE,
                parse_data->meta_cnt,
                parse_data->metas,
                &(parse_data->word_pos)
             );

    /* Now store the properties -- will concat any existing property */

    for ( i = 0; i < parse_data->prop_cnt; i++ )
    {
        struct metaEntry *m = getPropNameByID( parse_data->header, parse_data->props[i]);
        if (!addDocProperty(&(parse_data->thisFileEntry->docProperties), m, buf, txtlen, 0))
            progwarn("property '%s' not added for document '%s'\n", m->metaName, parse_data->fprop->real_path);
    }

    efree( buf );

}

/*********************************************************************
*   Add characters to summary
*
*   This REALLY shouldn't be here.
*   Could do better with general purpose properties:size
*
*
*********************************************************************/

static void append_summary_text( PARSE_DATA *parse_data, char *buf, int len)
{
    int j;
    
    /* trim trailing space */
    while ( isspace( buf[len-1] && len > 0 ))
        len--;
        

    /* skip leading space */
    for ( j=0; j < len && isspace( (int)buf[j] ); j++ );


    if ( j >= len )
        return;


    /* Add space between */
    if ( parse_data->buffend )
        parse_data->buffer[parse_data->buffend++] = ' ';
        

    while ( j < len )
    {
        /* Check for max size reached */
        if ( parse_data->buffend >= parse_data->fprop->stordesc->size )
        {
            if ( !isspace( (int)buf[j] ) && !isspace( (int)parse_data->buffer[parse_data->buffend-1] ))
            {
                while ( parse_data->buffend && !isspace( (int)parse_data->buffer[--parse_data->buffend] ));
                parse_data->buffer[parse_data->buffend] = '\0';
            }

            write_summary( parse_data );
            return;
        }

        /* reallocate if needed */
        if ( parse_data->buffend >= parse_data->buffmax )
        {
            parse_data->buffmax = parse_data->buffend + RD_BUFFER_SIZE;
            if ( parse_data->buffmax > parse_data->fprop->stordesc->size )
                parse_data->buffmax = parse_data->fprop->stordesc->size;

            parse_data->buffer = erealloc( parse_data->buffer, parse_data->buffmax+1);
        }

               
            

        parse_data->buffer[parse_data->buffend++] = buf[j++];
    }

        
    parse_data->buffer[parse_data->buffend] = '\0';
}

static void write_summary( PARSE_DATA *parse_data )
{
    if ( parse_data->buffend )
        if ( !addDocProperty(
            &(parse_data->thisFileEntry->docProperties),
            parse_data->summeta,
            parse_data->buffer,
            parse_data->buffend,
            0)
        )
            progwarn("property '%s' not added for document '%s'\n", parse_data->summeta->metaName, parse_data->fprop->real_path);


    efree( parse_data->buffer );
    parse_data->buffer = NULL;
    
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
    PARSE_DATA         *parse_data = (PARSE_DATA *)data;
    SWISH              *sw = parse_data->sw;
    int                 added = 0;



    /* If outside all meta tags then add default */
    if ( !parse_data->meta_cnt )
    {
        struct metaEntry *m = getMetaNameByName( parse_data->header, AUTOPROPERTY_DEFAULT );
        if ( m )
        {
            add_meta( parse_data, m );
            added++;
        }
    }
            

   
    if ( parse_data->meta_cnt )
    {
        /* Bump position around comments - hard coded */
        parse_data->word_pos++;
    
        parse_data->total_words +=
            indexstring(
                sw,
                (char *)txt,
                parse_data->filenum,
                IN_COMMENTS,
                parse_data->meta_cnt,
                parse_data->metas,
                &(parse_data->word_pos)
             );
        parse_data->word_pos++;
    }

    if ( added )
        parse_data->meta_cnt--;             

}



/*********************************************************************
*   check if a tag is an IgnoreTag
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
            return tag;

        tmplist = tmplist->next;
    }

    return NULL;
}


