/*
**
** $Id$
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
    
** Mon May  9 18:19:34 CDT 2005
** added GPL


*/

unsigned string_hash(char *, SWINT_T);
unsigned int_hash(SWINT_T, SWINT_T);
unsigned hash (char *);
unsigned numhash (SWINT_T);
unsigned bighash (char *);
unsigned bignumhash (SWINT_T);
unsigned verybighash (char *);

struct swline *add_word_to_hash_table( WORD_HASH_TABLE *table_ptr, char *word, SWINT_T hash_size);
struct swline * is_word_in_hash_table( WORD_HASH_TABLE table, char *word);
void free_word_hash_table( WORD_HASH_TABLE *table_ptr);


