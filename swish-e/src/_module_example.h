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
** 2001-02-28 rasc    remarks for code changes
**
*/


#ifndef __HasSeenModule_#modulename#
#define __HasSeenModule_#modulename#	1



//   use the following methode/layout to access global module data:
//     sw->#modulname#->data
//     in swish.h:
//          ... SWISH structure {  
//                ...
//                struct MOD_#modulename# *#modulename#;
//
//     Hint: Just copy the module and replace #modulename# by the Name... ;-) 
//

/* Global module data */

struct MOD_#modulename# {
   /* public:  */
   .... module private data, can be accessed outside the module
   .... use it with care. Most data should be private, if possible.
   .... use functions/methods instead to access/modify these data.

   /* private: don't use outside this module! */
   .... here private module data...
   .... don't use this data outside this module!
};





void initModule_#modulename# (SWISH *sw);
void freeModule_#modulename# (SWISH *sw);
void resetModule_#modulename# (SWISH *sw);
int  configModule_#modulename# (SWISH *sw, StringList *sl);
int  cmdlineModule_#modulename# (cmdline handling still to be defined...);
.....

#endif


