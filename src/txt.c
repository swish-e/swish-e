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
    along  with Swish-e; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    
    See the COPYING file that accompanies the Swish-e distribution for details
    of the GNU GPL and the special exception available for linking against
    the Swish-e library.
    
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
SWINT_T countwords_TXT (SWISH *sw, FileProp *fprop, FileRec *fi, char *buffer)

{
    SWINT_T     metaID;
    SWINT_T     positionMeta;    /* Position of word in file */
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
