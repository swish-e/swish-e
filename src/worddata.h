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

#define WORDDATA_MAX_REUSABLE_PAGES 16

typedef struct WORDDATA_Reusable_Page
{
    sw_off_t           page_number;
    int                page_size;
} WORDDATA_Reusable_Page;

typedef struct WORDDATA_Page
{
    sw_off_t           page_number;
    int                used_blocks;
    int                n;
    int                modified;
    int                in_use;

    struct WORDDATA_Page  *next_cache;

    unsigned char data[0];        /* Page data */
} WORDDATA_Page;

#define WORDDATA_CACHE_SIZE 97

typedef struct WORDDATA
{
    WORDDATA_Page      *last_put_page;  /* last page after an insert (put) */
    WORDDATA_Page      *last_del_page;  /* last page after a delete (del) */
    WORDDATA_Page      *last_get_page;  /* last page after a read (get) */
    struct WORDDATA_Page *cache[WORDDATA_CACHE_SIZE];
    int                page_counter;
    sw_off_t           lastid;
    int                num_Reusable_Pages;
    WORDDATA_Reusable_Page Reusable_Pages[WORDDATA_MAX_REUSABLE_PAGES];
    FILE *fp;
} WORDDATA;

WORDDATA *WORDDATA_Open(FILE *fp);
void WORDDATA_Close(WORDDATA *bt);
sw_off_t WORDDATA_Put(WORDDATA *b, unsigned int len, unsigned char *data);
unsigned char * WORDDATA_Get(WORDDATA *b, sw_off_t global_id, unsigned int *len);
void WORDDATA_Del(WORDDATA *b, sw_off_t global_id, unsigned int *len);
