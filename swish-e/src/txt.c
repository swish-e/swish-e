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
#include "docprop.h"
#include "error.h"
#include "compress.h"
#include "file.h"
#include "index.h"


/* Indexes all the words in a TXT file and adds the appropriate information
** to the appropriate structures. This is the most simple function.
** Just a call to indexstring
*/
int countwords_TXT(SWISH *sw, FileProp *fprop, char *buffer)

{
    int ftotalwords;
    int metaID;
    int positionMeta;    /* Position of word in file */
    int structure,currentmetanames;
    IndexFILE *indexf=sw->indexlist;
    struct MOD_Index *idx = sw->Index;
    char *summary=NULL;

	idx->filenum++;


	/* external filters can output control chars. So remove them */
	remove_controls(buffer);

	if(fprop->stordesc && fprop->stordesc->size)    /* Let us take the summary */
	{
		/* No fields in a TXT doc. So it is easy to get summary */
		summary=estrndup(buffer,fprop->stordesc->size);
		remove_newlines(summary);			/* 2001-03-13 rasc */
	}

	addtofilelist(sw, indexf, fprop->real_path, NULL );
    addCommonProperties( sw, indexf, fprop->mtime, fprop->real_filename, summary, 0, fprop->fsize );



	ftotalwords=0;
	structure=IN_FILE; /* No HTML tags in TXT , just IN_FILE */
	currentmetanames= 0; /* No metanames in TXT */

	metaID=1; positionMeta=1; /* No metanames in TXT */

	ftotalwords +=indexstring(sw, buffer, idx->filenum, structure, currentmetanames, &metaID, &positionMeta);

	addtofwordtotals(indexf, idx->filenum, ftotalwords);

	if(summary) efree(summary);

	return ftotalwords;
}
