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


void OutputKeyChar (SWISH *sw, int keychar)
{
 IndexFILE *tmpindexlist;
 int       keychar2;
 char      *keywords;

	if ( !SwishAttach(sw) )
            SwishAbortLastError( sw );

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



