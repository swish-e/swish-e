/*
** $Id$

    This file is part of Swish-e.

    Swish-e is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Swish-e is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    
    Swish-e includes a library for searching with a well-defined API. The library
    is named libswish-e.
    
    Linking libswish-e statically or dynamically with other modules is making a
    combined work based on Swish-e.  Thus, the terms and conditions of the GNU
    General Public License cover the whole combination.

    As a special exception, the copyright holders of Swish-e give you
    permission to link Swish-e with independent modules that communicate with
    Swish-e solely through the libswish-e API interface, regardless of the license
    terms of these independent modules, and to copy and distribute the
    resulting combined work under terms of your choice, provided that
    every copy of the combined work is accompanied by a complete copy of
    the source code of Swish-e (the version of Swish-e used to produce the
    combined work), being distributed under the terms of the GNU General
    Public License plus this exception.  An independent module is a module
    which is not derived from or based on Swish-e.

    Note that people who make modified versions of Swish-e are not obligated
    to grant this special exception for their modified versions; it is
    their choice whether to do so.  The GNU General Public License gives
    permission to release a modified version without this exception; this
    exception also makes it possible to release a modified version which
    carries forward this exception.
    
** Mon May  9 15:51:39 CDT 2005
** added GPL

**
*/

#include "swish.h"
#include "mem.h"
#include "swstring.h"
#include "error.h"
#include "search.h"
#include "metanames.h"
#include "merge.h"
#include "docprop.h"
#include "db.h"
#ifdef HAVE_ZLIB
#include <zlib.h>
#endif


static unsigned char *compress_property( propEntry *prop, SWISH *sw, int *buf_len, int *uncompressed_len );


/*******************************************************************
*   Write Properties to disk, and save seek pointers
*
*   DB_WriteProperty - should write filenum:propID as the key
*   DB_WritePropPositions - writes the stored positions
*
*
*
*********************************************************************/
void     WritePropertiesToDisk( SWISH *sw , FileRec *fi )
{
    IndexFILE       *indexf = sw->indexlist;
    INDEXDATAHEADER *header = &indexf->header;
    docProperties   *docProperties = fi->docProperties;
    propEntry       *prop;
    int             uncompressed_len;
    unsigned char   *buf;
    int             buf_len;
    int             count;
    int             i;
    

    /* initialize the first time called */
    if ( header->property_count == 0 )
    {
        /* Get the current seek position in the index, since will now write the file info */
        DB_InitWriteProperties(sw, indexf->DB);

        /* build a list of properties that are in use */
        /* And create the prop index to propID (metaID) mapping arrays */
        init_property_list(header);
    }


    if ( (count = header->property_count) <= 0)
        return;


    /* any props exist, unlikely, but need to save a space. */
    if ( !docProperties )
    {
        DB_WritePropPositions( sw, indexf, fi, indexf->DB);
        return;
    }


    for( i = 0; i < count; i++ )
    {
        /* convert the count to a propID */
        int propID = header->propIDX_to_metaID[i];  // here's the array created in init_property_list()


        /* Here's why I need to redo the properties so it's always header->property_count size in the fi rec */
        /* The mapping is all a temporary kludge */
        if ( propID >= docProperties->n ) // Does this file have this many properties?
            continue;


        if ( !(prop = docProperties->propEntry[propID])) // does this file have this prop?
            continue;

        buf = compress_property( prop, sw, &buf_len, &uncompressed_len );

        DB_WriteProperty( sw, indexf, fi, propID, (char *)buf, buf_len, uncompressed_len, indexf->DB );
    }




    /* Write the position data */
    DB_WritePropPositions( sw, indexf, fi, indexf->DB);

    freeDocProperties( docProperties );
    fi->docProperties = NULL;

       

}


/*******************************************************************
*   Compress a Property
*
*   Call with:
*       propEntry       - the in data and its length
*       SWISH           - to get access to the common buffer
*       *uncompress_len - returns the length of the original buffer, or zero if not compressed
*       *buf_len        - the length of the returned buffer
*
*   Returns:
*       pointer the buffer of buf_len size
*
*
*********************************************************************/

static unsigned char *compress_property( propEntry *prop, SWISH *sw, int *buf_len, int *uncompressed_len )
{
#ifndef HAVE_ZLIB
    *buf_len = prop->propLen;
    *uncompressed_len = 0;
    return prop->propValue;

#else
    unsigned char  *PropBuf;     /* For compressing and uncompressing */
    uLongf          dest_size;
    int             zlib_status = 0;


    /* Don't bother compressing smaller items */
    if ( prop->propLen < MIN_PROP_COMPRESS_SIZE )
    {
        *buf_len = prop->propLen;
        *uncompressed_len = 0;
        return prop->propValue;
    }
    
    /* Buffer should be +1% + a few bytes. */
    dest_size = (uLongf)(prop->propLen + ( prop->propLen / 100 ) + 1000);  // way more than should be needed


    /* Get an output buffer */
    PropBuf = allocatePropIOBuffer( sw, dest_size );


    zlib_status = compress2( (Bytef *)PropBuf, &dest_size, prop->propValue, prop->propLen, sw->PropCompressionLevel);
    if ( zlib_status != Z_OK )
        progerr("Property Compression Error.  zlib compress2 returned: %d  Prop len: %d compress buf size: %d compress level:%d", zlib_status, prop->propLen, (int)dest_size,sw->PropCompressionLevel);


    /* Make sure it's compressed enough */
    if ( dest_size >= prop->propLen )
    {
        *buf_len = prop->propLen;
        *uncompressed_len = 0;
        return prop->propValue;
    }

    *buf_len = (int)dest_size;
    *uncompressed_len = prop->propLen;

    return PropBuf;

#endif
}


