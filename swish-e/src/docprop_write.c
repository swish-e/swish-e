/*
** $Id$
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
** You should have received a copy of the GNU (Library) General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
**
**
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


static unsigned char *compress_property( propEntry *prop, int propID, SWISH *sw, int *buf_len, int *uncompressed_len );


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

        buf = compress_property( prop, propID, sw, &buf_len, &uncompressed_len );

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
*       propID          - current property
*       SWISH           - to get access to the common buffer
*       *uncompress_len - returns the length of the original buffer, or zero if not compressed
*       *buf_len        - the length of the returned buffer
*
*   Returns:
*       pointer the buffer of buf_len size
*
*
*********************************************************************/

static unsigned char *compress_property( propEntry *prop, int propID, SWISH *sw, int *buf_len, int *uncompressed_len )
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


