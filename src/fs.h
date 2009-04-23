/* 
fs.h
$Id$

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



*/
#ifndef __HasSeenModule_FS
#define __HasSeenModule_FS       1


#ifdef __cplusplus
extern "C" {
#endif


/*
   -- module data
*/

typedef struct
{
    regex_list  *pathname;
    regex_list  *dirname;
    regex_list  *filename;
    regex_list  *dircontains;
    regex_list  *title;
    
}
PATH_LIST;

struct MOD_FS
{
    PATH_LIST   filerules;
    PATH_LIST   filematch;
    SWINT_T         followsymlinks;

};


void initModule_FS (SWISH *);
void freeModule_FS (SWISH *);
SWINT_T  configModule_FS (SWISH *, StringList *);

#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif
