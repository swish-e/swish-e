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


unsigned hash (char *);
unsigned numhash (int);
unsigned bighash (char *);
unsigned bignumhash (int);
unsigned searchhash (char *);
void readdefaultstopwords (IndexFILE *);
void addstophash (IndexFILE *, char *);
void addusehash (IndexFILE *, char *);
int isstopword (IndexFILE *, char *);
int isuseword (IndexFILE *, char *);
void mergeresulthashlist (SWISH *, RESULT *);
void initresulthashlist (SWISH *sw);
void addStopList (IndexFILE *, char *);
void freestophash (IndexFILE *);
void freeStopList (IndexFILE *);
