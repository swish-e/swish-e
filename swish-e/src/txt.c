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
#include "string.h"
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
