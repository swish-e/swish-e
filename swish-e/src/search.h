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
**-------------------------------------------------------
** Added getMetaName & isMetaName 
** to support METADATA
** G. Hill 3/18/97 ghill@library.berkeley.edu
*/

/*
** use _AP() for easier cross-compiler (non-ANSI) porting 
** <return value> <functionname> _AP( (<arg prototypes>) );
*/

int SwishAttach(SWISH *, int);
int search (SWISH *, char *, int);
RESULT *SwishNext(SWISH *);
struct swline *fixnot(struct swline *);

struct swline *expandphrase(struct swline *, char);

void readheader(IndexFILE *);
void readoffsets(IndexFILE *);
void readhashoffsets(IndexFILE *);
void readstopwords(IndexFILE *);
void readfileoffsets(IndexFILE *);
void readMetaNames(IndexFILE *);
void readlocationlookuptables(IndexFILE *);
void readpathlookuptable(IndexFILE *);

int countResults(RESULT *);
RESULT *parseterm(SWISH *, int, int, IndexFILE *, struct swline **);
RESULT *operate(SWISH *, RESULT *, int, char *, FILE *, int, int, IndexFILE *);
RESULT *getfileinfo(SWISH *, char *, IndexFILE *, int);
char *getfilewords(SWISH *sw, char, IndexFILE *);

int isrule(char *);
int isnotrule(char *);
int isbooleanrule(char *);
int isunaryrule(char *);
int isMetaName(struct swline *);
int getrulenum(char *);

RESULT *andresultlists(SWISH *, RESULT *, RESULT *, int);
RESULT *orresultlists(SWISH *, RESULT *, RESULT *);
RESULT *notresultlist(SWISH *, RESULT *, IndexFILE *);
RESULT *notresultlists(SWISH *, RESULT *, RESULT *);
RESULT *phraseresultlists(SWISH *, RESULT *, RESULT *, int);
RESULT *addtoresultlist(RESULT *, int, int, int, int, int *, IndexFILE *,SWISH *);

RESULT *getproperties(RESULT *);


RESULT *sortresultsbyfilenum(RESULT *);

void getrawindexline(FILE *);
int isokindexheader(FILE *);
int wasStemmingAppliedToIndex(FILE *);
int wasSoundexAppliedToIndex(FILE *);

void freeresultlist(SWISH *);
void freefileoffsets(SWISH *);
void freeresult(SWISH *,RESULT *);
void freefileinfo(struct file *);
struct swline *ignore_words_in_query(SWISH *,IndexFILE *, struct swline *,unsigned char);
struct swline *stem_words_in_query(SWISH *,IndexFILE *, struct swline *);
struct swline *soundex_words_in_query(SWISH *,IndexFILE *, struct swline *);
struct swline *translatechars_words_in_query(SWISH *sw,IndexFILE *indexf,struct swline *searchwordlist);


