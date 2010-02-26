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
    
** Mon May  9 18:19:34 CDT 2005
** added GPL



*/

#ifndef HEADERS_H
#define HEADERS_H 1

#ifdef __cplusplus
extern "C" {
#endif


/*************************************************************************
*  Index File Header access interface
*
*   Notes:
*       Must match up with the public library interface headers.
*
*
**************************************************************************/


typedef enum {
    SWISH_NUMBER,
    SWISH_STRING,
    SWISH_LIST,
    SWISH_BOOL,
    SWISH_WORD_HASH,
    SWISH_OTHER_DATA,
    SWISH_HEADER_ERROR /* must check error in this case */
} SWISH_HEADER_TYPE;

typedef union
{
    const char           *string;
    const char          **string_list;
          SWUINT_T   number;
          int             boolean;
} SWISH_HEADER_VALUE;


void print_index_headers( IndexFILE *indexf );

IndexFILE *indexf_by_name( SWISH *sw, const char *index_name );

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* !HEADERS_H */


