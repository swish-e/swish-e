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
** (c) Rainer.Scherg
**
**
** 2001-05-05 rasc    initial coding
**
*/


#ifndef __HasSeenModule_Entities
#define __HasSeenModule_Entities	1



/* Global module data */

struct MOD_Entities {
   int   convertEntities;
};





void initModule_Entities (SWISH *sw);
void freeModule_Entities (SWISH *sw);
int  configModule_Entities (SWISH *sw, StringList *sl);

unsigned char *sw_ConvHTMLEntities2ISO(SWISH *sw, unsigned char *s);
unsigned char *strConvHTMLEntities2ISO (unsigned char *buf);
int charEntityDecode (unsigned char *buf, unsigned char **end);


#endif


