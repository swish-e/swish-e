/*
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
**
**
**
** 2001-01  jose   initial coding
**
*/

void    DB_decompress(SWISH * sw, IndexFILE * indexf, int begin, int maxhits);
void dump_file_list( SWISH *sw, IndexFILE *indexf );
void dump_memory_file_list( SWISH *sw, IndexFILE *indexf );
void dump_metanames( SWISH *sw, IndexFILE *indexf, int check_presorted );
void dump_file_properties(IndexFILE * indexf, FileRec *fi );
void dump_single_property( propEntry *prop, struct metaEntry *meta_entry );

