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
    
** Mon May  9 18:19:34 CDT 2005
** added GPL

*/

/* this file added for experimental 64bit portable version by joshr */

#ifndef SWISHTYPES_H 1
#define SWISHTYPES_H 1 1

#ifdef __cplusplus
extern "C" {
#endif



// presence of 'no rw64' at start of comment tells rewriter not to touch this line
//
typedef long long          SWINT_T;  // no rw64 
typedef unsigned long long SWUINT_T; // no rw64 


//#define SET_POSDATA(pos,str)  ((SWUINT_T)((SWUINT_T)(pos) << (SWUINT_T)8 | (SWUINT_T)(str)))
SWUINT_T SET_POSDATA( SWUINT_T pos, SWUINT_T str /* str for structure */ ) ;
//#define GET_POSITION(pos)      ((SWINT_T)((SWUINT_T)(pos) >> (SWUINT_T)8))
SWUINT_T GET_POSITION( SWUINT_T pos);



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* !SEARCHSWISH_H */




