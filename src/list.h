/*
** Copyright (C) 1995, 1996, 1997, 1998 Hewlett-Packard Company
** Originally by Kevin Hughes, kev@kevcom.com, 3/11/94
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

#ifdef __cplusplus
extern "C" {
#endif

struct swline *newswline_n(char *line, SWINT_T size);
struct swline *newswline(char *line);
struct swline *addswline (struct swline *rp, char *line);
struct swline *dupswline (struct swline *rp);
void addindexfile(struct SWISH *sw, char *line);
void freeswline (struct swline *ptr);
void init_header (INDEXDATAHEADER *header);
void free_header (INDEXDATAHEADER *header);

#ifdef __cplusplus
}
#endif /* __cplusplus */

