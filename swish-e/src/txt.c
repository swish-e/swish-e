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
int metaName[1];
int positionMeta[1];    /* Position of word in file */
int structure,currentmetanames;
struct file *thisFileEntry = NULL;
IndexFILE *indexf=sw->indexlist;
char *summary=NULL;

	sw->filenum++;
		/* external filters can output control chars. So remove them */
	remove_controls(buffer);

	if(fprop->stordesc && fprop->stordesc->size)    /* Let us take the summary */
	{
		/* No fields in a TXT doc. So it is easy to get summary */
		summary=estrndup(buffer,fprop->stordesc->size);
	}

	addtofilelist(sw,indexf, fprop->real_path, fprop->mtime, fprop->real_path, summary, 0, fprop->fsize, &thisFileEntry);
		/* Init meta info */
	ftotalwords=0;
	structure=IN_FILE; /* No HTML tags in TXT , just IN_FILE */
	currentmetanames= 0; /* No metanames in TXT */
	metaName[0]=1; positionMeta[0]=1; /* No metanames in TXT */
	ftotalwords +=indexstring(sw, buffer, sw->filenum, structure, currentmetanames, metaName, positionMeta);
	addtofwordtotals(indexf, sw->filenum, ftotalwords);
	if(sw->swap_flag)
		SwapFileData(sw, indexf->filearray[sw->filenum-1]);
	if(summary) efree(summary);
	return ftotalwords;
}
