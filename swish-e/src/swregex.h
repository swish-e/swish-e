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

#ifndef __HasSeenModule_regex
#define __HasSeenModule_regex       1

void add_regex_patterns( char *name, regex_list **reg_list, char **params, int regex_pattern );
void  add_replace_expression( char *name, regex_list **reg_list, char *expression );

int match_regex_list( char *str, regex_list *regex, char *comment );
char *process_regex_list( char *str, regex_list *regex, int *matched );

void free_regex_list( regex_list **reg_list );
void add_regular_expression( regex_list **reg_list, char *pattern, char *replace, int cflags, int global, int negate );

#endif /* __HasSeenModule_regex */
