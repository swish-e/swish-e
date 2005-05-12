/*
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
    
** Mon May  9 18:15:43 CDT 2005
** added GPL

*/

#ifndef SWISH_WORDS_H
#define  SWISH_WORDS_H 1

#ifdef __cplusplus
extern "C" {
#endif


/* internal representation,  may not be changed */
/* BTW, keep strlen(MAGIC_NOT_WORD) >= strlen(NOT_WORD) to avoid string reallocation in the code */
#define AND_WORD       "<and>" 
#define OR_WORD        "<or>"
#define NOT_WORD       "<not>"
#define MAGIC_NOT_WORD "<__not__>"
#define PHRASE_WORD    "<precd>"
#define AND_NOT_WORD   "<andnot>"

/* internal search rule numbers */
#define NO_RULE 0
#define AND_RULE 1
#define OR_RULE 2
#define NOT_RULE 3
#define PHRASE_RULE 4
#define AND_NOT_RULE 5



struct swline *parse_swish_query( DB_RESULTS *db_results );

void initModule_Swish_Words (SWISH *sw);
void freeModule_Swish_Words (SWISH *sw);


void    stripIgnoreFirstChars(INDEXDATAHEADER *, char *);
void    stripIgnoreLastChars(INDEXDATAHEADER *, char *);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

