/*

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
    
Mon May  9 10:57:22 CDT 2005 -- added GPL notice


*/


/*
$Id$
**
**
** 2001-03-17  rasc  save real_filename as title (instead full real_path)
**                   was: compatibility issue to v 1.x.x
*/



#include "swish.h"
#include "txt.h"
#include "mem.h"
#include "swstring.h"
#include "check.h"
#include "merge.h"
#include "search.h"
#include "docprop.h"
#include "error.h"
#include "compress.h"
#include "file.h"
#include "index.h"


/* Indexes all the words in a TXT file and adds the appropriate information
** to the appropriate structures. This is the most simple function.
** Just a call to indexstring
*/
int countwords_TXT (SWISH *sw, FileProp *fprop, FileRec *fi, char *buffer)

{
    int     metaID;
    int     positionMeta;    /* Position of word in file */
    char   *summary=NULL;
    char   *title = "";


	if(fprop->stordesc && fprop->stordesc->size)    /* Let us take the summary */
	{
		/* No fields in a TXT doc. So it is easy to get summary */
		summary=estrndup(buffer,fprop->stordesc->size);
		remove_newlines(summary);			/* 2001-03-13 rasc */
	}

    addCommonProperties( sw, fprop, fi, title, summary, 0 );

	if(summary) efree(summary);


	metaID=1; positionMeta=1; /* No metanames in TXT */

	return indexstring(sw, buffer, fi->filenum, IN_FILE, 1, &metaID, &positionMeta);
}
