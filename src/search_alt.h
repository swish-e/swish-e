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
** 2001-03-02 rasc   initial coding
*/


#ifndef __HasSeenModule_SearchAlt
#define __HasSeenModule_SearchAlt	1


/* exported Prototypes */

void initModule_SearchAlt   (SWISH *sw);
void freeModule_SearchAlt   (SWISH *sw);
int  configModule_SearchAlt (SWISH *sw, StringList *sl);

char *convAltSearch2SwishStr (char *str);


#endif

