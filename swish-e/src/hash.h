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

unsigned hash _AP ((char *));
unsigned numhash _AP ((int));
unsigned bighash _AP ((char *));
unsigned bignumhash _AP ((int));
unsigned searchhash _AP ((char *));
void readdefaultstopwords _AP ((IndexFILE *));
void addstophash _AP ((IndexFILE *, char *));
void addusehash _AP ((IndexFILE *, char *));
int isstopword _AP ((IndexFILE *, char *));
int isuseword _AP ((IndexFILE *, char *));
void mergeresulthashlist _AP ((SWISH *, RESULT *));
void initresulthashlist _AP ((SWISH *sw));
void addStopList _AP ((IndexFILE *, char *));
void freestophash _AP ((IndexFILE *));
void freeStopList _AP ((IndexFILE *));
