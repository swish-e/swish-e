/*
** Copyright (C) 1995, 1996, 1997, 1998 Hewlett-Packard Company
** Originally by Kevin Hughes, kev@kevcom.com, 3/11/94
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

/*
** use _AP() for easier cross-compiler (non-ANSI) porting 
** <return value> <functionname> _AP( (<arg prototypes>) );
*/

int isokword _AP ((SWISH *,char *,IndexFILE *));
int hasokchars _AP ((IndexFILE *, char *));
char *hasfilter _AP((char *filename, struct filter *filterlist));
int getdoctype _AP((char *filename, struct IndexContents *indexcontents));
