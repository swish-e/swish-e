/*
**
** $Id$
**
** This program and library is free software; you can redistribute it and/or
** modify it under the terms of the GNU (Library) General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU (Library) General Public License for more details.
**
** You should have received a copy of the GNU (Library) General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/


unsigned hash (char *);
unsigned numhash (int);
unsigned bighash (char *);
unsigned bignumhash (int);
unsigned verybighash (char *);

void add_word_to_hash_table( WORD_HASH_TABLE *table_ptr, char *word);
int is_word_in_hash_table( WORD_HASH_TABLE table, char *word);
void free_word_hash_table( WORD_HASH_TABLE *table_ptr);


