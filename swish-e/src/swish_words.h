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

