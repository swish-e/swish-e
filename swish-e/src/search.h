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

int SwishAttach _AP ((SWISH *, int));
int search _AP ((SWISH *, char *, int, int));
RESULT *SwishNext _AP ((SWISH *));
struct swline *fixnot _AP ((struct swline *));

struct swline *expandphrase _AP ((struct swline *, char));

void readheader _AP ((IndexFILE *));
void readoffsets _AP ((IndexFILE *));
void readhashoffsets _AP ((IndexFILE *));
void readstopwords _AP ((IndexFILE *));
void readfileoffsets _AP ((IndexFILE *));
void readMetaNames _AP ((IndexFILE *));
void readlocationlookuptables _AP ((IndexFILE *));
void readpathlookuptable _AP ((IndexFILE *));

int countResults _AP ((RESULT *));
RESULT *parseterm _AP ((SWISH *, int, int, IndexFILE *, struct swline **));
RESULT *operate _AP ((SWISH *, RESULT *, int, char *, FILE *, int, int, IndexFILE *));
RESULT *getfileinfo _AP ((SWISH *, char *, IndexFILE *, int));
char *getfilewords _AP ((SWISH *sw, char, IndexFILE *));

int isrule _AP ((char *));
int isnotrule _AP ((char *));
int isbooleanrule _AP ((char *));
int isunaryrule _AP ((char *));
int isMetaName _AP ((struct swline *));
int getrulenum _AP ((char *));

RESULT *andresultlists _AP ((SWISH *, RESULT *, RESULT *, int));
RESULT *orresultlists _AP ((SWISH *, RESULT *, RESULT *));
RESULT *notresultlist _AP ((SWISH *, RESULT *, IndexFILE *));
RESULT *notresultlists _AP ((SWISH *, RESULT *, RESULT *));
RESULT *phraseresultlists _AP ((SWISH *, RESULT *, RESULT *, int));
RESULT *addtoresultlist _AP ((RESULT *, int, int, int, int, int *, IndexFILE *));

RESULT *getproperties _AP ((SWISH *, IndexFILE *, RESULT *));

RESULT *addsortresult _AP ((SWISH *, RESULT *sp, RESULT *));

RESULT *sortresultsbyrank _AP ((SWISH *, int structure));
RESULT *sortresultsbyfilenum _AP ((RESULT *rp));
void printsortedresults _AP ((SWISH *,int ));

void getrawindexline _AP ((FILE *));
int isokindexheader _AP ((FILE *));
int wasStemmingAppliedToIndex _AP ((FILE *));
int wasSoundexAppliedToIndex _AP ((FILE *));

void freeresultlist _AP ((SWISH *));
void freefileoffsets _AP ((SWISH *));
void freeresult _AP ((SWISH *,RESULT *));
void freefileinfo _AP ((struct file *));
struct swline *ignore_words_in_query _AP ((SWISH *,IndexFILE *, struct swline *));
