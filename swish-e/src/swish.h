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
**-------------------------------------------------
** Added support for METADATA
** G. Hill  ghill@library.berkeley.edu   3/18/97
**
** Added Document Properties support
** Mark Gaulin gaulin@designinfo.com  11/24/98
**
** Added safestrcpy() macro to avoid corruption from strcpy overflow
** SRE 11/17/99
**
** Added Document Filter support (e.g. PDF, Winword)
** Rainer.Scherg@t-online.de   (rasc)  1998-08-07, 1999-05-05, 1999-05-28
**
** Added some definitions for phrase search
** Structure location modified to add frequency and word positions
** Structure entry modified to add link hash values for direct search
**
** Jose Ruiz jmruiz@boe.es 04/04/00
**
** 2000-11-15 Rainer Scherg (rasc)  FileProp type and routines
**
** 2001-01-01 Jose Ruiz Added ISOTime 
**
*/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <locale.h>
#include <ctype.h>
#include <errno.h>
#include "config.h"

#ifdef NEXTSTEP
#include <sys/dir.h>
#else

#define snprintf snprintf_is_not_ANSI

#ifdef _WIN32
#include "win32/dirent.h"
#include "Win32/regex.h"
#define pclose _pclose
#define popen _popen
#define strcasecmp stricmp
#else    
#include <dirent.h>
#include <regex.h>
#endif

#endif

#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <setjmp.h>


#define SWISH_VERSION "2.1.10-devel13"

#define SWISH_MAGIC 21076321L

#define INDEXFILE "index.swish-e"

#define BASEHEADER 1

#define INDEXHEADER "# SWISH format: 2.1"
#define INDEXHEADER_ID BASEHEADER + 1 
#define INDEXVERSION "# Swish-e format: 2.1"
#define INDEXVERSION_ID BASEHEADER + 2


#define NAMEHEADER "# Name:"
#define NAMEHEADER_ID BASEHEADER + 3
#define SAVEDASHEADER "# Saved as:"
#define SAVEDASHEADER_ID BASEHEADER + 4
#define COUNTSHEADER "# Counts:"
#define COUNTSHEADER_ID BASEHEADER + 5
#define INDEXEDONHEADER "# Indexed on:"
#define INDEXEDONPARAMNAME "Indexed on"
#define INDEXEDONHEADER_ID BASEHEADER + 6
#define DESCRIPTIONHEADER "# Description:"
#define DESCRIPTIONPARAMNAME "Description"
#define DESCRIPTIONHEADER_ID BASEHEADER + 7
#define POINTERHEADER "# Pointer:"
#define POINTERPARAMNAME "IndexPointer"
#define POINTERHEADER_ID BASEHEADER + 8
#define MAINTAINEDBYHEADER "# Maintained by:"
#define MAINTAINEDBYPARAMNAME "IndexAdmin"
#define MAINTAINEDBYHEADER_ID BASEHEADER + 9
#define WORDCHARSHEADER "# WordCharacters:"
#define WORDCHARSPARAMNAME "WordCharacters"
#define WORDCHARSHEADER_ID BASEHEADER + 10
#define MINWORDLIMHEADER "# MinWordLimit:"
#define MINWORDLIMHEADER_ID BASEHEADER + 11
#define MAXWORDLIMHEADER "# MaxWordLimit:"
#define MAXWORDLIMHEADER_ID BASEHEADER + 12
#define BEGINCHARSHEADER "# BeginCharacters:"
#define BEGINCHARSPARAMNAME "BeginCharacters"
#define BEGINCHARSHEADER_ID BASEHEADER + 13
#define ENDCHARSHEADER "# EndCharacters:"
#define ENDCHARSPARAMNAME "EndCharacters"
#define ENDCHARSHEADER_ID BASEHEADER + 14
#define IGNOREFIRSTCHARHEADER "# IgnoreFirstChar:"
#define IGNOREFIRSTCHARPARAMNAME "IgnoreFirstChar"
#define IGNOREFIRSTCHARHEADER_ID BASEHEADER + 15
#define IGNORELASTCHARHEADER "# IgnoreLastChar:"
#define IGNORELASTCHARPARAMNAME "IgnoreLastChar"
#define IGNORELASTCHARHEADER_ID BASEHEADER + 16

#define STEMMINGHEADER	"# Stemming Applied:"
#define STEMMINGPARAMNAME "Stemming"
#define STEMMINGHEADER_ID BASEHEADER + 17
#define SOUNDEXHEADER "# Soundex Applied:"
#define SOUNDEXPARAMNAME "Soundex"
#define SOUNDEXHEADER_ID BASEHEADER + 18
#define MERGED_ID BASEHEADER + 19

#define DOCPROPHEADER "# DocProperty"
#define DOCPROPHEADER_ID BASEHEADER + 20
#define DOCPROPENHEADER "# DocumentProperties:"
#define DOCPROPENHEADER_ID BASEHEADER + 21
#define SORTDOCPROPHEADER_ID BASEHEADER + 22

#define IGNORETOTALWORDCOUNTWHENRANKING "# IgnoreTotalWordCountWhenRanking:"
#define IGNORETOTALWORDCOUNTWHENRANKINGPARAMNAME "IgnoreTotalWordCountWhenRanking"
#define IGNORETOTALWORDCOUNTWHENRANKING_ID BASEHEADER + 23

#define FILEINFOCOMPRESSION "# FileInfoCompression:"
#define FILEINFOCOMPRESSIONPARAMNAME "FileInfoCompression"
#define FILEINFOCOMPRESSION_ID BASEHEADER + 24

#define MAXFILELEN 1000
#define MAXSTRLEN 2000
#define MAXWORDLEN 1000
#define MAXTITLELEN 300
#define MAXSUFFIXLEN 10
#define MAXENTLEN 10
#define HASHSIZE 101
#define BIGHASHSIZE 1009
#define SEARCHHASHSIZE 10001
#define MAXPAR 10
#define MAXCHARDEFINED 256

#define NOWORD "thisisnotaword"
#define SECSPERMIN 60

#define NO_RULE 0
#define AND_RULE 1
#define OR_RULE 2
#define NOT_RULE 3
#define PHRASE_RULE 4
#define AND_NOT_RULE 5

#define IN_FILE 1
#define IN_TITLE 2
#define IN_HEAD 4
#define IN_BODY 8
#define IN_COMMENTS 16
#define IN_HEADER 32
#define IN_EMPHASIZED 64
#define IN_ALL IN_FILE|IN_TITLE|IN_HEAD|IN_BODY|IN_COMMENTS|IN_HEADER|IN_EMPHASIZED

#define MAXLONGLEN 4
#define MAXCHARS 266    /* 255 for chars plus ten more for other data */
#define METANAMEPOS MAXCHARS - 7
#define STOPWORDPOS MAXCHARS - 6
#define FILELISTPOS MAXCHARS - 5
#define FILEOFFSETPOS MAXCHARS - 4
#define LOCATIONLOOKUPTABLEPOS MAXCHARS - 3
#define PATHLOOKUPTABLEPOS MAXCHARS - 2
#define DEFLATEDICTPOS MAXCHARS - 1


/* Document Types */
#define BASEDOCTYPE 0
#define NODOCTYPE BASEDOCTYPE
#define TXT BASEDOCTYPE+1
#define HTML BASEDOCTYPE+2
#define XML BASEDOCTYPE+3
#define MULTITXT BASEDOCTYPE+4
#define WML BASEDOCTYPE+5

typedef struct docPropertyEntry {
	int metaName;		/* meta field identifier; from getMetaName() */
	char *propValue;	/* string from META's CONTENTS attribute */

	struct docPropertyEntry *next;
} docPropertyEntry;

struct metaEntry {
	char* metaName;
	int index;
	
	/* is this meta field a Document Property? */
	int isDocProperty;		/* true is doc property */
	int isOnlyDocProperty;	/* true if NOT an indexable meta tag (ie: not in MetaNames) */
	
	struct metaEntry* next;
};

typedef struct {
	int lookup_path;
	char *filename;
	unsigned long mtime;
	char *title;
	char *summary;
	int start;
	int size;
} FILEINFO;

typedef struct {
	long filelong;
	int ftotalwords;
} FILEOFFSET;

struct file {
	FILEINFO fi;
	int read;
	struct docPropertyEntry* docProperties;
};


/*
 -- FileProperties 
 -- store for information about a file to be indexed...
 -- Unused items may be NULL (e.g. if File is not opened, fp == NULL)
 -- (2000-11 rasc)

 -- (2000-12 Jose Ruiz)
 -- Added StoreDescription
*/

typedef struct  {
	FILE   *fp;	/* may be also a filter stream or NULL if not opened */
	char   *real_path;	/* org. path/URL to indexed file */
	char   *work_path;	/* path to file to index (may be tmpfile or real_path) */
	long   fsize;		/* size of the original file (not filtered) */
	time_t mtime;           /* Date of last mod of or. file */
	int    doctype;		/* Type of document HTML, TXT, XML, ... */
	int    index_no_content;/* Flag, index "filename/real_path" only! */
	struct StoreDescription *stordesc;  
				/* Null if no description/summary */
	char   *filterprog;     /* path to a filterscript or NULL */
} FileProp;


typedef struct {
	int metaName;
	int filenum;
	int structure;
	int frequency;
	int position[1];
} LOCATION;

typedef struct ENTRY {
	char *word;
	int tfrequency;
	LOCATION **locationarray;
	struct {
		long fileoffset;
		int max_locations;
	} u1;
		/* ths union is just for saving memory */
	union {
		struct ENTRY *nexthash;
		int currentlocation;
	} u2;
} ENTRY;

struct swline {
	char *line;
	struct swline *next;
	struct swline *nodep;
};

typedef struct {
	/* vars for WordCharacters */
    int lenwordchars;
    char *wordchars;
	/* vars for BeginCharacters */
    int lenbeginchars;
    char *beginchars;
	/* vars for EndCharacters */
    int lenendchars;
    char *endchars;
	/* vars for IgnoreLastChar */
    int lenignorelastchar;
    char *ignorelastchar;
	/* vars for IgnoreFirstChar */
    int lenignorefirstchar;
    char *ignorefirstchar;
	/* vars for TranslateCharacters */
    int lentranslatechars1;
    char *translatechars1;
    int lentranslatechars2;
    char *translatechars2;
	/* vars for WordCharacters */
    int lenbumpposchars;
    char *bumpposchars;
	/* vars for header values */
    char *savedasheader;
    int lensavedasheader;
    int lenindexedon;
    char *indexedon;
    int lenindexn;
    char *indexn;
    int lenindexd;
    char *indexd;
    int lenindexp;
    char *indexp;
    int lenindexa;
    char *indexa;
    int minwordlimit;
    int maxwordlimit;
    int applyStemmingRules;		/* added 11/24/98 - MG */
    int applySoundexRules;              /* added 09/01/99 - DN */
    int applyFileInfoCompression;       /* added 10/00 Jose Ruiz */
		/* Total files and words in index file */
    int totalwords;
    int totalfiles;
	/* var to specify how to ranking while indexing */
    int ignoreTotalWordCountWhenRanking;	/* added 11/24/98 - MG */
	/* Lookup tables for fast access */
    int wordcharslookuptable[256];
    int begincharslookuptable[256];
    int endcharslookuptable[256];
    int ignorefirstcharlookuptable[256];
    int ignorelastcharlookuptable[256];
    int bumpposcharslookuptable[256];
} INDEXDATAHEADER;

typedef struct IndexFILE {
	char *line;
		/* Offsets to words */
	long offsets[MAXCHARS];
	long hashoffsets[SEARCHHASHSIZE];
		/* file index info */
	struct file **filearray;
	int filearray_cursize;
	int filearray_maxsize;
	FILEOFFSET **fileoffsetarray;
	int fileoffsetarray_cursize;
	int fileoffsetarray_maxsize;

		/* Offsets of words in index file */
	long wordpos; 
       		 /* Values for fields (metanames) */
	struct metaEntry* metaEntryList;
	int Metacounter;
		
		/* values for handling stopwords */
	struct swline *hashstoplist[HASHSIZE];
	char **stopList;
	int stopMaxSize;
	int stopPos;

	/* values for handling "use" words - > Unused in the search proccess */
	int is_use_words_flag;
	struct swline *hashuselist[HASHSIZE];

	/* File handle */
	FILE *fp;
	
	/* Header Info */
	INDEXDATAHEADER header;

	/* Lookup tables for repetitive values of locations (frequency,structureand metaname */
	/* Index use all of them */
	/* search does not use locationlookup */
	struct int_lookup_st *locationlookup;
	struct int_lookup_st *structurelookup;
	struct int_lookup_st *structfreqlookup;
	/* Lookup table for repetitive values of the path */
	struct char_lookup_st *pathlookup;

	/* Pointer to cache the keywords */
	char *keywords[256];

	struct IndexFILE *next;
	struct IndexFILE *nodep;

	int n_dict_entries;
	unsigned char **dict;     /* Used to store the patterns when
				  ** DEFLATE_FILES is enabled */
		/* props IDs */
	int *propIDToDisplay;
	int *propIDToSort;
} IndexFILE;

typedef struct RESULT {
	int filenum;
	int rank;
	int structure;
	int frequency;
	int *position;
	struct RESULT *next;
	struct RESULT *head;
	struct RESULT *nextsort;   /* Used while sorting results */
	char *filename;
	char ISOTime[20];
	char *title;
	char *summary;
	int start;
	int size;
	/* file position where this document's properties are stored */
	char **Prop;
	char **PropSort;
	IndexFILE *indexf;
} RESULT;

struct multiswline {
    struct swline *list;
    struct multiswline *next;
};


struct filter {
        char *suffix;
        char *prog;
        struct filter *next;
        struct filter *nodep;
};

typedef struct {
	int currentsize;
	int maxsize;
	ENTRY **elist;
} ENTRYARRAY;


typedef struct {
        int currentsize;
        int maxsize;
        char **filenames;
} DOCENTRYARRAY;

struct dev_ino {
                dev_t  dev;
                ino_t  ino;
                struct dev_ino *next;
};

struct url_info {
    char *url;
    struct url_info *next;
};

struct IndexContents {
	int DocType;
	struct swline *patt;
	struct IndexContents *next;
};

struct StoreDescription {
	int DocType;
	char *field;
	int size;
	struct StoreDescription *next;
};

/* These two structs are used for lookuptables in order to save memory */
/* Normally Metaname, frequency and structure are repetitive schemas */
/* and usually have also low values */
/* In this way three values can be fit in just one using a lookup table*/
/* Structure itself can use its own lookuptable */
struct int_st
{
        int index;
        struct int_st *next;
        int val[1];
};

struct int_lookup_st
{
        int n_entries;
        struct int_st *hash_entries[HASHSIZE];
        struct int_st *all_entries[1];
};

/* These two structs are used for lookuptables in order to save memory */
/* Normally part of the path/url are repetitive schemas */
/* and usually have also low values */
struct char_st
{
        int index;
        struct char_st *next;
        char *val;
};

struct char_lookup_st
{
        int n_entries;
        struct char_st *hash_entries[HASHSIZE];
        struct char_st *all_entries[1];
};

#ifndef GLOBAL_VARS
#define VAR extern
#else
#define VAR
#endif


typedef struct {
	/* All files header info */
    INDEXDATAHEADER mergedheader;


	/* entry vars */
    ENTRYARRAY *entrylist;
    ENTRY *hashentries[SEARCHHASHSIZE];

	/* 08/00 Jose Ruiz Values for document type support */
    int DefaultDocType;
    struct IndexContents *indexcontents;
	/* 12/00 Jose Ruiz Values for summary support */
    struct StoreDescription *storedescription;

	/* structure for handling replace config data while searching */
    struct swline *replacelist;
	/* structure for handling NoContents config data while searching */
    struct swline *nocontentslist;
	/* structure for handling all the directories while indexing  */
    struct swline *dirlist;
	/* structure for handling IndexOnly config data while indexing */
    struct swline *suffixlist;

	/* structure for handling all the index files while searching  */
    IndexFILE *indexlist;


	/* structure to store the parsed search string */
    struct swline *searchwordlist;

    RESULT *resulthashlist[BIGHASHSIZE];

	/* Compression Work buffer while compression locations in index
	** proccess */
    unsigned char *compression_buffer;
    int len_compression_buffer;


	/* Total words and files in all index files */
    int TotalWords;
    int TotalFiles;
	/* verbose flag */
    int verbose;
	/* File counter */
    int filenum;
    int bigrank;
    int beginhits;
    int maxhits;
    int followsymlinks;
	/* Error vars */
    int commonerror;
    int lasterror;
    int lenerrorstr;
    char *errorstr;

    int indexComments;
	/* Custom delimiter vars */
    int useCustomOutputDelimiter;	/* added 11/24/98 - MG */
    int lencustomOutputDelimiter;
    char *customOutputDelimiter;	/* added 11/24/98 - MG */
	/* Filter vars */
    struct filter *filterlist;                  /* 1998-08-07 rasc */
    int lenfilterdir;
    char *filterdir;                  /* 1998-08-07 rasc */

                /* 06/00 Jose Ruiz */
    int applyautomaticmetanames;
    int isvowellookuptable[256];

	/* Properties vars */
    int numPropertiesToDisplay;
    int currentMaxPropertiesToDisplay;
    char **propNameToDisplay;
    int numPropertiesToSort;
    int currentMaxPropertiesToSort;
    char **propNameToSort;
    int *propModeToSort;
		
	/* http proccessing */
    int lentmpdir;
    char *tmpdir;

    int lenspiderdirectory;
    char *spiderdirectory;

    /* Values for IgnoreLimit */
    long plimit;
    long flimit;

    /* Values for handling results */
    RESULT *resultlist;
    RESULT *sortresultlist;
    RESULT *currentresult;

	/* MetaName indexing options */
	int ReqMetaName;
	int OkNoMeta;
        /* Flag to specify if AsciiEntities are indexing */
	int AsciiEntities;

    /* file system specific configuration parameters */
    struct swline *pathconlist;
    struct swline *dirconlist;
    struct swline *fileconlist;
    struct swline *titconlist;
    struct swline *fileislist;
    struct dev_ino *inode_hash[BIGHASHSIZE];

    /* http system specific configuration parameters */
    int maxdepth;
    int delay;
    struct multiswline *equivalentservers;
    struct url_info *url_hash[BIGHASHSIZE];

    /* Phrase delimiter char */
    int PhraseDelimiter;

    /* If true. Swap part of the info to disk while indexing */
    /* Save memory */
    int swap_flag;
    /* Filenames of the swap files */
    char *swap_file_name;   /* File and properties file */
    char *swap_location_name;   /* Location info file */
    /* handlers for both files */
    FILE *fp_loc_write;        /* Location (writing) */
    FILE *fp_loc_read;       /* Location (writing) */
    FILE *fp_file_write;     /* File (writing) */
    FILE *fp_file_read;      /* File (read) */
} SWISH;

/*
 * This structure defines all of the functions that need to
 * be implemented to an Indexing Data Source.
 * Right now there are two Indexing Data Source types:
 *  file-system based and an HTTP web crawler.
 * Any Data Source can be created as long as all of the
 * functions below are properly initialized.
 */
struct _indexing_data_source_def
{
  const char* IndexingDataSourceName;           /* long name for data source */
  const char* IndexingDataSourceId;             /* short name for data source */
  void (*indexpath_fn)(SWISH *sw, char *path);             /* routine to index a "path" */
  int (*parseconfline_fn)(SWISH *sw, char *line);          /* parse config file lines */
};

#ifdef GLOBAL_VARS

VAR struct _indexing_data_source_def *IndexingDataSource;
VAR char *indexchars ="abcdefghijklmnopqrstuvwxyz¡¬√»˝ ÀÃÆ–◊›ﬁÕŒœ“”‘’ÿŸ€ÓË„ıöõúñùÄﬂÇÉÑÖÜáàâäãåçéè±¯ü˜∞£‹û∑îìí≤ëê&#;0123456789_\\|/-+=?!@$%^'\"`~,.<>[]{}";

VAR char *defaultstopwords[] = {
"a", "above", "according", "across", "actually", "adj", "after", 
"afterwards", "again", "against", "all", "almost", "alone", "along", 
"already", "also", "although", "always", "among", "amongst", "an", "and", 
"another", "any", "anyhow", "anyone", "anything", "anywhere", "are", "aren", 
"aren't", "around", "as", "at", "be", "became", "because", "become", "becomes", 
"becoming", "been", "before", "beforehand", "begin", "beginning", "behind", 
"being", "below", "beside", "besides", "between", "beyond", "billion", "both", 
"but", "by", "can", "can't", "cannot", "caption", "co", "could", "couldn",
"couldn't", "did", "didn", "didn't", "do", "does", "doesn", "doesn't", "don",
"don't", "down", "during", "each", "eg", "eight", "eighty", "either", "else",
"elsewhere", "end", "ending", "enough", "etc", "even", "ever", "every",
"everyone", "everything", "everywhere", "except", "few", "fifty", "first",
"five", "for", "former", "formerly", "forty", "found", "four", "from",
"further", "had", "has", "hasn", "hasn't", "have", "haven", "haven't",
"he", "hence", "her", "here", "hereafter", "hereby", "herein", "hereupon", 
"hers", "herself", "him", "himself", "his", "how", "however", "hundred", 
"ie", "i.e.", "if", "in", "inc", "inc.", "indeed", "instead", "into", "is",
"isn", "isn't", "it", "its", "itself", "last", "later", "latter", "latterly",
"least", "less", "let", "like", "likely", "ll", "ltd", "made", "make",
"makes", "many", "maybe", "me", "meantime", "meanwhile", "might", "million",
"miss", "more", "moreover", "most", "mostly", "mr", "mrs", "much", "must",
"my", "myself", "namely", "neither", "never", "nevertheless", "next", "nine",
"ninety", "no", "nobody", "none", "nonetheless", "noone", "nor", "not",
"nothing", "now", "nowhere", "of", "off", "often", "on", "once", "one",
"only", "onto", "or", "others", "otherwise", "our", "ours",
"ourselves", "out", "over", "overall", "own", "per", "perhaps", "rather",
"re", "recent", "recently", "same", "seem", "seemed", "seeming", "seems",
"seven", "seventy", "several", "she", "should", "shouldn", "shouldn't",
"since", "six", "sixty", "so", "some", "somehow", "someone", "something",
"sometime", "sometimes", "somewhere", "still", "stop", "such", "taking",
"ten", "than", "that", "the", "their", "them", "themselves", "then",
"thence", "there", "thereafter", "thereby", "therefore", "therein",
"thereupon", "these", "they", "thirty", "this", "those", "though",
"thousand", "three", "through", "throughout", "thru", "thus", "to",
"together", "too", "toward", "towards", "trillion", "twenty", "two", "under",
"unless", "unlike", "unlikely", "until", "up", "upon", "us", "used", "using",
"ve", "very", "via", "was", "wasn", "we", "we", "well", "were", "weren",
"weren't", "what", "whatever", "when", "whence", "whenever", "where", 
"whereafter", "whereas", "whereby", "wherein", "whereupon", "wherever", 
"whether", "which", "while", "whither", "who", "whoever", "whole", "whom", 
"whomever", "whose", "why", "will", "with", "within", "without", "won", 
"would", "wouldn", "wouldn't", "yes", "yet", "you", "your", "yours",
"yourself", "yourselves", NULL };

#else
VAR struct _indexing_data_source_def *IndexingDataSource;
VAR char *indexchars;
VAR char *defaultstopwords[];
#endif

#ifdef MAIN_FILE

/*
** use _AP() for easier cross-compiler (non-ANSI) porting 
** <return value> <functionname> _AP( (<arg prototypes>) );
*/

int main _AP ((int, char **));
void usage _AP ((void));
void printversion _AP ((void));
void printrunning _AP ((long, long));

#endif

long getthetime _AP ((void));

void allocatedefaults _AP((void));

SWISH *SwishNew _AP ((void));
void SwishDefaults _AP((SWISH *));
void SwishFree _AP((SWISH *));

/* 04/00 Jose Ruiz
** Functions to read/write longs from index file
*/
void printlong _AP ((FILE *, long));
long readlong _AP ((FILE *));
/* strcpy doesn't check for overflow in the 'to' string */
/* strncpy doesn't guarantee null byte termination */
/* can't check strlen of 'from' arg since it is sometimes a function call */
#define safestrcpy(n,to,from)  { strncpy(to,from,n); to[n-1]='\0'; }

/* Jose Ruiz 04/00
** Macro for copying postions between arrays of integers
** copy num integers on dest (starting at posdest) from
** orig (starting at posorig)
*/
/* 
#define CopyPositions(dest,posdest,orig,posorig,num) \
{int i;for(i=0;i<num,i++) dest[i+posdest]=orig[i+posorig];}
*/
#define CopyPositions(dest,posdest,orig,posorig,num)  memcpy((char *)((int *)dest+posdest),(char *)((int *)orig+posorig),num*sizeof(int))


/* Min macro */
#define Min(a,b) (a<b?a:b)

/* Definitions of word commands and, or, not, ... */
/* Change them here */
#define _AND_WORD "and"
#define _OR_WORD "or"
#define _NOT_WORD "not"
#define AND_WORD "<and>"
#define OR_WORD "<or>"
#define NOT_WORD "<not>"
#define PHRASE_WORD "<precd>"
#define AND_NOT_WORD "<andnot>"

/* C library prototypes */
SWISH * SwishOpen _AP ((char *));
void SwishClose _AP ((SWISH *));
void SwishResetSearch _AP ((SWISH *));
RESULT * SwishNext _AP ((SWISH *));
int SwishSearch _AP ((SWISH *, char *, int , char *, char *));
int getnumPropertiesToDisplay _AP ((SWISH *));


