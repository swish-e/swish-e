/*

$Id: swish-e.h 2307 2009-04-09 16:14:41Z karpet $

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

    $Log$
    
*/

/* this file added for experimental 64bit portable version by joshr */

#ifndef SWISHTYPES_H 
#define SWISHTYPES_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include "inttypes.h"   /* quotes, not <>, because this is not a system header */

// presence of 'no rw64' at start of comment tells rewriter not to touch this line
//
//
typedef int64_t          SWINT_T;  // no rw64 
typedef uint64_t         SWUINT_T; // no rw64 

#define SWINT_FORMAT    PRIi64
#define SWUINT_FORMAT   PRIu64
#define SWXINT_FORMAT   PRIx64   /* lowercase 64bit hex */


// SET_POSDATA() and GET_POSITION() are still defined in swish.h

#if 0
// experimental replacements of SET_POSDATA and GET_POSITION macros.
//#define SET_POSDATA(pos,str)  ((SWUINT_T)((SWUINT_T)(pos) << (SWUINT_T)8 | (SWUINT_T)(str)))
//SWUINT_T SET_POSDATA( SWUINT_T pos, SWUINT_T str /* str for structure */ ) ;
//#define GET_POSITION(pos)      ((SWINT_T)((SWUINT_T)(pos) >> (SWUINT_T)8))
//SWUINT_T GET_POSITION( SWUINT_T pos);
#endif



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* !SWISHTYPES_H */




