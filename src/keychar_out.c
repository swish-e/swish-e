/*
$Id$
**
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
    
** Mon May  9 15:23:45 CDT 2005
** added GPL


**
**
**
** 2001-03-20 rasc   own module for this routine  (from swish.c)
**
*/

#include "swish.h"
#include "swstring.h"
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


void OutputKeyChar (SWISH *sw, SWINT_T keychar)
{
 IndexFILE *tmpindexlist;
 SWINT_T       keychar2;
 char      *keywords;

	if ( !SwishAttach(sw) )
            SwishAbortLastError( sw );

	resultHeaderOut(sw,1, "%s\n", INDEXHEADER);
		/* print out "original" search words */
	for(tmpindexlist=sw->indexlist;tmpindexlist;tmpindexlist=tmpindexlist->next)
	{
		resultHeaderOut(sw,1, "%s:", tmpindexlist->line);
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



