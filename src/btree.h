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


typedef struct BTREE_Page
{
    sw_off_t           next;    /* Next Page */
    sw_off_t           prev;    /* Previous Page */
    SWUINT_T       size;    /* Size of page */
    SWUINT_T       n;       /* Number of keys in page */
    SWUINT_T       flags;
    SWUINT_T       data_end;

    sw_off_t           page_number;
    SWINT_T                modified;
    SWINT_T                in_use;

    struct BTREE_Page  *next_cache;

    unsigned char data[0];        /* Page data */
} BTREE_Page;

#define BTREE_CACHE_SIZE 97

typedef struct BTREE
{
    sw_off_t root_page;
    SWINT_T page_size;
    struct BTREE_Page *cache[BTREE_CACHE_SIZE];
    SWINT_T levels;
    sw_off_t tree[1024];
          /* Values for sequential reading */
    sw_off_t current_page;
    SWUINT_T current_position;

    FILE *fp;
} BTREE;

BTREE *BTREE_Create(FILE *fp, SWUINT_T size);
BTREE *BTREE_Open(FILE *fp, SWINT_T size, sw_off_t root_page);
sw_off_t BTREE_Close(BTREE *bt);
SWINT_T BTREE_Insert(BTREE *b, unsigned char *key, SWINT_T key_len, sw_off_t data_pointer);
sw_off_t BTREE_Search(BTREE *b, unsigned char *key, SWINT_T key_len, unsigned char **found, SWINT_T *found_len, SWINT_T exact_match);
sw_off_t BTREE_Next(BTREE *b, unsigned char **found, SWINT_T *found_len);
SWINT_T BTREE_Update(BTREE *b, unsigned char *key, SWINT_T key_len, sw_off_t new_data_pointer);

