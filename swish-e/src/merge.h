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
	int oldIndex;
	int newIndex;

	/* is this meta field a Document Property? */
	char isDocProperty;		/* true is doc property */
	char isOnlyDocProperty;	/* true if NOT an indexable meta tag (ie: not in MetaNames) */

	struct metaMergeEntry* next;
};

struct mergeindexfileinfo *indexfilehashlist[BIGHASHSIZE];
struct mapentry *mapentrylist[BIGHASHSIZE];
struct markentry *markentrylist[BIGHASHSIZE];
struct markentryMerge *markentrylistMerge[BIGHASHSIZE];

/*
** use _AP() for easier cross-compiler (non-ANSI) porting 
** <return value> <functionname> _AP( (<arg prototypes>) );
*/

void readmerge _AP ((char *, char *, char *, int));
void addfilenums _AP ((ENTRY *, int));
ENTRY *readindexline _AP ((IndexFILE *, struct metaMergeEntry *));
void addindexfilelist _AP ((SWISH *, int , char *, time_t, char *, char *, int, int, struct docPropertyEntry *, int *, int, struct metaMergeEntry *));
struct mergeindexfileinfo *lookupindexfilenum _AP ((int ,struct docPropertyEntry**));
ENTRY *mergeindexentries _AP ((ENTRY *,ENTRY *, int));
ENTRYARRAY * addentryMerge _AP ((SWISH *, ENTRY *));
int lookupindexfilepath _AP ((char *, int, int));
void remap _AP ((int, int));
int getmap _AP ((int));
void marknum _AP ((int));
int ismarked _AP ((int));
void initmarkentrylist _AP ((void));
void initindexfilehashlist _AP ((void));
void initmapentrylist _AP ((void));
struct metaMergeEntry* readMergeMeta _AP ((SWISH *,struct metaEntry *)); 
struct metaEntry* createMetaMerge _AP ((struct metaMergeEntry*, struct metaMergeEntry*));
struct metaEntry* addMetaMergeList _AP ((struct metaEntry*, struct metaMergeEntry*, int*));
