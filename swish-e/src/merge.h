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
**-----------------------------------------------------------
** Added remapVar and initmapentrylist to fix the merge option -M
** G. Hill 3/7/97
**
*/

struct mergeindexfileinfo {
	int filenum;
	char *path;
	int start;
	int size;
	struct mergeindexfileinfo *next;
};

struct mapentry {
	int oldnum;
	int newnum;
	struct mapentry *next;
};

struct markentry {
	int num;
	struct markentry *next;
};

struct markentryMerge {
	int num;
	int metaName;
	struct markentryMerge *next;
};	

struct metaMergeEntry {
	char* metaName;
	int oldMetaID;
	int newMetaID;
	int metaType;
	struct metaMergeEntry* next;
};

struct mergeindexfileinfo *indexfilehashlist[BIGHASHSIZE];
struct mapentry *mapentrylist[BIGHASHSIZE];
struct markentry *markentrylist[BIGHASHSIZE];
struct markentryMerge *markentrylistMerge[BIGHASHSIZE];


void readmerge (char *, char *, char *, int);
void addfilenums (ENTRY *, int);
ENTRY *readindexline (SWISH *, IndexFILE *, struct metaMergeEntry *, int, int *);
void addindexfilelist (SWISH *, int , char *, time_t, char *, char *, int, int, struct docProperties *, int *, int, struct metaMergeEntry *);
struct mergeindexfileinfo *lookupindexfilenum (int ,struct docPropertyEntry**);
ENTRY *mergeindexentries (ENTRY *,ENTRY *, int);
int lookupindexfilepath (char *, int, int);
void remap (int, int);
int getmap (int);
void marknum (int);
int ismarked (int);
void initmarkentrylist (void);
void initindexfilehashlist (void);
void initmapentrylist (void);
struct metaMergeEntry *readMergeMeta (SWISH *,int, struct metaEntry **); 
struct metaEntry **createMetaMerge (struct metaMergeEntry *, struct metaMergeEntry*, int *);
struct metaEntry **addMetaMergeArray (struct metaEntry **, struct metaMergeEntry*, int*);

void addentryMerge (SWISH *, ENTRY *);
