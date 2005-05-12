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

#ifndef __HasSeenModule_PropLimit
#define __HasSeenModule_PropLimit  1


void SwishResetSearchLimit( SEARCH_OBJECT *srch );
int SwishSetSearchLimit(SEARCH_OBJECT *srch, char *propertyname, char *low, char *hi);


/* internal use */
void ClearLimitParams( LIMIT_PARAMS *params );

LIMIT_PARAMS *setlimit_params( SWISH *sw, LIMIT_PARAMS *params, char *propertyname, char *low, char *hi );

int Prepare_PropLookup(SEARCH_OBJECT * srch );



int LimitByProperty( IndexFILE *indexf, PROP_LIMITS *prop_limits, int filenum );


#endif



