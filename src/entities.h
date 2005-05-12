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
    
** Mon May  9 18:19:34 CDT 2005
** added GPL

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
   /* public:  */
   /* none */
   
   /* private: don't use outside this module! */
   int   convertEntities;
};





void initModule_Entities (SWISH *sw);
void freeModule_Entities (SWISH *sw);
int  configModule_Entities (SWISH *sw, StringList *sl);

unsigned char *sw_ConvHTMLEntities2ISO(SWISH *sw, unsigned char *s);
unsigned char *strConvHTMLEntities2ISO (unsigned char *buf);
int charEntityDecode (unsigned char *buf, unsigned char **end);


#endif


