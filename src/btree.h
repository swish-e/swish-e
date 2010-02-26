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
    unsigned int       size;    /* Size of page */
    unsigned int       n;       /* Number of keys in page */
    unsigned int       flags;
    unsigned int       data_end;

    sw_off_t           page_number;
    int                modified;
    int                in_use;

    struct BTREE_Page  *next_cache;

    unsigned char data[0];        /* Page data */
} BTREE_Page;

#define BTREE_CACHE_SIZE 97

typedef struct BTREE
{
    sw_off_t root_page;
    int page_size;
    struct BTREE_Page *cache[BTREE_CACHE_SIZE];
    int levels;
    sw_off_t tree[1024];
          /* Values for sequential reading */
    sw_off_t current_page;
    SWUINT_T current_position;

    FILE *fp;
} BTREE;

BTREE *BTREE_Create(FILE *fp, unsigned int size);
BTREE *BTREE_Open(FILE *fp, int size, sw_off_t root_page);
sw_off_t BTREE_Close(BTREE *bt);
int BTREE_Insert(BTREE *b, unsigned char *key, int key_len, sw_off_t data_pointer);
sw_off_t BTREE_Search(BTREE *b, unsigned char *key, int key_len, unsigned char **found, int *found_len, int exact_match);
sw_off_t BTREE_Next(BTREE *b, unsigned char **found, int *found_len);
int BTREE_Update(BTREE *b, unsigned char *key, int key_len, sw_off_t new_data_pointer);

