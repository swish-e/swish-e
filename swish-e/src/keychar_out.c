/*
$Id$
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
** 2001-03-20 rasc   own module for this routine  (from swish.c)
**
*/

#include "swish.h"
#include "string.h"
#include "error.h"
#include "mem.h"
#include "search.h"
#include "result_output.h"
#include "db.h"
#include "keychar_out.h"




/*
** ----------------------------------------------
** 
**  Module code starts here
**
** ----------------------------------------------
*/



/*
  -- output all indexed words starting with character "keychar"
  -- keychar == '*' prints all words
*/


void OutputKeyChar (SWISH *sw, int keychar)
{
 IndexFILE *tmpindexlist;
 int       keychar2;
 char      *keywords;
 int       rc;

	rc=SwishAttach(sw,1);
	switch(rc) {
		case INDEX_FILE_NOT_FOUND:
			resultHeaderOut(sw,1, "# Name: unknown index\n");
			progerrno("could not open index file %s: ",sw->indexlist->line);
			break;
		case UNKNOWN_INDEX_FILE_FORMAT:
			progerr("the index file format is unknown");
			break;
	}

	resultHeaderOut(sw,1, "%s\n", INDEXHEADER);
		/* print out "original" search words */
	for(tmpindexlist=sw->indexlist;tmpindexlist;tmpindexlist=tmpindexlist->next)
	{
		resultHeaderOut(sw,1, "%s:",tmpindexlist->line);
		if(keychar=='*')
		{
			for(keychar2=1;keychar2<256;keychar2++)
			{
				keywords=getfilewords(sw,(unsigned char )keychar2,tmpindexlist);
				for(;keywords && keywords[0];keywords+=strlen(keywords)+1)
					resultHeaderOut(sw,1, " %s",keywords);
			}
		} else {
			keywords=getfilewords(sw,keychar,tmpindexlist);
			for(;keywords && keywords[0];keywords+=strlen(keywords)+1)
					resultHeaderOut(sw,1, " %s",keywords);
		}
		resultHeaderOut(sw,1, "\n");
	}

	return;
}



