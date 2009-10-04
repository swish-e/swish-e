/*      
$Id: swish.c 2291 2009-03-31 01:56:00Z karpet $
**
** Swish Originally by Kevin Hughes, kev@kevcom.com, 3/11/94
**

** Mon May  9 11:07:38 CDT 2005
** like swish2.c -- how much of this is really Kevin's original work?
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
    
    $Log$
    
*/  


#if 0

// DON'T COMPILE THIS CODE IN UNLESS YOU'RE REPLACING GET_POSITION() and SET_POSDATA()
// MACROS WITH FUNCTION VERSIONS

/* NOT USED */
/* FUNCTION VERSION OF GET_POSITION */
SWUINT_T GET_POSITION( SWUINT_T pos) {
    return pos / 256;    // >> 8
}

/* NOT USED */
/* FUNCTION VERSION OF SET_POSDATA */
SWUINT_T SET_POSDATA( SWUINT_T pos, SWUINT_T str /* str for structure */ ) {
    SWUINT_T r = (pos * 256);           //  the << 8
    SWUINT_T pbytes1234 = r / (2^32); // break into two 4byte parts
    SWUINT_T pbytes5678 = r % (2^32);
    SWUINT_T sbytes1234 = str / (2^32);
    SWUINT_T sbytes5678 = str % (2^32);

    return ((2^32) * ( pbytes1234 | sbytes1234 )) + ( pbytes5678 | sbytes5678 );
}
#endif

