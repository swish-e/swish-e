/*
** $Id$
**
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
** 2001-01-xx Rainer Scherg (rasc) Added property type structures, etc.
** 2001-01-xx Rainer Scherg (rasc) cmd-opt should be own structure in SWISH * (started)
**
** 2001-02-xx rasc   replaced ISOTime by binary value
**                   removed SWISH.errorstr, etc. 
**                   ResultExtFmtStrList & var
**
** 2001-02-28 rasc   some cleanup, ANSI compliant
** 2001-03-12 rasc   logical search operators via config changable
**                   moved some parts to config.h
**
** 2001-03-16 rasc   truncateDocSize
** 2001-03-17 rasc   fprop enhanced by real_filename
** 2001-04-09 rasc   filters changed and enhanced
** 2001-06-08 wsm    Add word to end of ENTRY and propValue to end of docPropertyEntry
**                     to save memory and less malloc/free
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
#include <time.h>
#ifdef HAVE_CONFIG_H
#include "acconfig.h"           /* These are defines created by autoconf */
#endif
#include "config.h"

#ifdef NEXTSTEP
#include <sys/dir.h>
#else


#ifdef _WIN32
#include "win32/config.h"
#include "win32/dirent.h"
#include "win32/regex.h"
#define strcasecmp stricmp
#elif defined(__VMS)
#include "vms/regex.h"
#include <dirent.h>
#include <stdarg.h>
extern int snprintf(char *, size_t, const char *, /*args */ ...);
extern int vsnprintf(char *, size_t, const char *, va_list);
#else
#include <dirent.h>
#include <regex.h>
#endif

#endif

#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <setjmp.h>


#define SWISH_MAGIC 21076321L

#define INDEXFILE "index.swish-e"


#define BASEHEADER 1
#define INDEXHEADER "# SWISH format: " SWISH_VERSION
#define INDEXHEADER_ID BASEHEADER + 1
#define INDEXVERSION "# Swish-e format: " SWISH_VERSION
#define INDEXVERSION_ID (BASEHEADER + 2)


#define NAMEHEADER "# Name:"
#define NAMEHEADER_ID (BASEHEADER + 3)
#define SAVEDASHEADER "# Saved as:"
#define SAVEDASHEADER_ID (BASEHEADER + 4)
#define COUNTSHEADER "# Counts:"
#define COUNTSHEADER_ID (BASEHEADER + 5)
#define INDEXEDONHEADER "# Indexed on:"
#define INDEXEDONPARAMNAME "Indexed on"
#define INDEXEDONHEADER_ID (BASEHEADER + 6)
#define DESCRIPTIONHEADER "# Description:"
#define DESCRIPTIONPARAMNAME "Description"
#define DESCRIPTIONHEADER_ID (BASEHEADER + 7)
#define POINTERHEADER "# Pointer:"
#define POINTERPARAMNAME "IndexPointer"
#define POINTERHEADER_ID (BASEHEADER + 8)
#define MAINTAINEDBYHEADER "# Maintained by:"
#define MAINTAINEDBYPARAMNAME "IndexAdmin"
#define MAINTAINEDBYHEADER_ID (BASEHEADER + 9)
#define WORDCHARSHEADER "# WordCharacters:"
#define WORDCHARSPARAMNAME "WordCharacters"
#define WORDCHARSHEADER_ID (BASEHEADER + 10)
#define MINWORDLIMHEADER "# MinWordLimit:"
#define MINWORDLIMHEADER_ID (BASEHEADER + 11)
#define MAXWORDLIMHEADER "# MaxWordLimit:"
#define MAXWORDLIMHEADER_ID (BASEHEADER + 12)
#define BEGINCHARSHEADER "# BeginCharacters:"
#define BEGINCHARSPARAMNAME "BeginCharacters"
#define BEGINCHARSHEADER_ID (BASEHEADER + 13)
#define ENDCHARSHEADER "# EndCharacters:"
#define ENDCHARSPARAMNAME "EndCharacters"
#define ENDCHARSHEADER_ID (BASEHEADER + 14)
#define IGNOREFIRSTCHARHEADER "# IgnoreFirstChar:"
#define IGNOREFIRSTCHARPARAMNAME "IgnoreFirstChar"
#define IGNOREFIRSTCHARHEADER_ID (BASEHEADER + 15)
#define IGNORELASTCHARHEADER "# IgnoreLastChar:"
#define IGNORELASTCHARPARAMNAME "IgnoreLastChar"
#define IGNORELASTCHARHEADER_ID (BASEHEADER + 16)

#define STEMMINGHEADER	"# Stemming Applied:"
#define STEMMINGPARAMNAME "Stemming"
#define STEMMINGHEADER_ID (BASEHEADER + 17)
#define SOUNDEXHEADER "# Soundex Applied:"
#define SOUNDEXPARAMNAME "Soundex"
#define SOUNDEXHEADER_ID (BASEHEADER + 18)
#define MERGED_ID (BASEHEADER + 19)

#define DOCPROPHEADER "# DocProperty"
#define DOCPROPHEADER_ID (BASEHEADER + 20)
#define DOCPROPENHEADER "# DocumentProperties:"
#define DOCPROPENHEADER_ID (BASEHEADER + 21)
#define SORTDOCPROPHEADER_ID (BASEHEADER + 22)

#define IGNORETOTALWORDCOUNTWHENRANKING "# IgnoreTotalWordCountWhenRanking:"
#define IGNORETOTALWORDCOUNTWHENRANKINGPARAMNAME "IgnoreTotalWordCountWhenRanking"
#define IGNORETOTALWORDCOUNTWHENRANKING_ID (BASEHEADER + 23)

/* Removed - Patents ...
#define FILEINFOCOMPRESSION "# FileInfoCompression:"
#define FILEINFOCOMPRESSIONPARAMNAME "FileInfoCompression"
#define FILEINFOCOMPRESSION_ID (BASEHEADER + 24)
*/

#define TRANSLATECHARTABLEHEADER "# TranslateCharacterTable:"
#define TRANSLATECHARTABLEPARAMNAME "TranslateCharacterTable"
#define TRANSLATECHARTABLE_ID (BASEHEADER + 25)

#define STOPWORDS_ID (BASEHEADER + 26)
#define METANAMES_ID (BASEHEADER + 27)
#define LOCATIONLOOKUPTABLE_ID (BASEHEADER + 28)
#define PATHLOOKUPTABLE_ID (BASEHEADER + 29)
#define BUZZWORDS_ID (BASEHEADER + 30) /* 2001-04-24 moseley */
#define WORDSPERDOC_ID (BASEHEADER + 31)

#define MAXFILELEN 1000
#define MAXSTRLEN 2000
#define MAXWORDLEN 1000
#define MAXTITLELEN 300
#define MAXENTLEN 10
#define HASHSIZE 101
#define BIGHASHSIZE 1009
#define SEARCHHASHSIZE 10001
#define MAXPAR 10
#define MAXCHARDEFINED 256
#define RD_BUFFER_SIZE  65356   /* init size, larger to avoid often reallocs  (2001-03-16 rasc) */

#define NOWORD "thisisnotaword"
#define SECSPERMIN 60

#define IN_FILE 1
#define IN_TITLE 2
#define IN_HEAD 4
#define IN_BODY 8
#define IN_COMMENTS 16
#define IN_HEADER 32
#define IN_EMPHASIZED 64
#define IN_META 128
#define IN_ALL (IN_FILE|IN_TITLE|IN_HEAD|IN_BODY|IN_COMMENTS|IN_HEADER|IN_EMPHASIZED|IN_META)

#define MAXLONGLEN 4

/* Document Types */
#define BASEDOCTYPE 0
#define NODOCTYPE	(BASEDOCTYPE)
#define TXT 	(BASEDOCTYPE+1)
#define HTML 	(BASEDOCTYPE+2)
#define XML 	(BASEDOCTYPE+3)
#define LST 	(BASEDOCTYPE+4)
#define WML 	(BASEDOCTYPE+5)

typedef struct propEntry
{
    struct propEntry *next;     /* It is possible to have more than 1 */
								/* property for the same metaID */
    unsigned int propLen;       /* Length of buffer */
    unsigned char propValue[0]; /* Actual property value starts here */
}
propEntry;

typedef struct docProperties
{
	int n;                      /* Number of entries in the array */
	struct propEntry *propEntry[0];  /* Array to hold properties */
}
docProperties;

struct metaEntry
{
    char   *metaName;           /* MetaName string */
    int     metaID;             /* Meta ID */
    int     metaType;           /* See metanames.h for values */
    /* If 0, files are not sorted by this metaName/property */
    int    *sorted_data;        /* Sorted data . NULL if not read/done */

    int    *inPropRange;          /* Used for limiting to a range */
};

typedef struct
{
    int     filenum;
    int     lookup_path;
    char   *filename;
    unsigned long mtime;
    char   *title;
    char   *summary;
    int     start;
    int     size;
}
FILEINFO;


struct file
{
    FILEINFO fi;
    struct metaEntry *currentSortProp;
    struct docProperties *docProperties;
};


/*
 -- FileProperties 
 -- store for information about a file to be indexed...
 -- Unused items may be NULL (e.g. if File is not opened, fp == NULL)
 -- (2000-11 rasc)

 -- (2000-12 Jose Ruiz)
 -- Added StoreDescription
  
*/

typedef struct
{
    FILE   *fp;                 /* may be also a filter stream or NULL if not opened */
    char   *real_path;          /* org. path/URL to indexed file */
    char   *work_path;          /* path to file to index (may be tmpfile or real_path) */
    char   *real_filename;      /* basename() of real_path  */
    long    fsize;              /* size of the original file (not filtered) */
    time_t  mtime;              /* Date of last mod of or. file */
    int     doctype;            /* Type of document HTML, TXT, XML, ... */
    int     index_no_content;   /* Flag, index "filename/real_path" only! */
    struct StoreDescription *stordesc;
    /* Null if no description/summary */
    struct FilterList *hasfilter;
    /* NULL if no filter for this file */
}
FileProp;


typedef struct
{
    int     metaID;
    int     filenum;
    int     structure;
    int     frequency;
    int     position[1];
}
LOCATION;

typedef struct ENTRY
{
    struct ENTRY *next;
    int     tfrequency;
    LOCATION **locationarray;
    /* this union is just for saving memory */
    struct
    {
        long    fileoffset;
        int     max_locations;
    }
    u1;
    /* this union is just for saving memory */
    int     currentlocation;
	char	word[0];	/* actual word starts here */
}
ENTRY;

struct swline
{
    char   *line;
    struct swline *next;
    struct swline *nodep;
};

typedef struct
{
    /* vars for WordCharacters */
    int     lenwordchars;
    char   *wordchars;
    /* vars for BeginCharacters */
    int     lenbeginchars;
    char   *beginchars;
    /* vars for EndCharacters */
    int     lenendchars;
    char   *endchars;
    /* vars for IgnoreLastChar */
    int     lenignorelastchar;
    char   *ignorelastchar;
    /* vars for IgnoreFirstChar */
    int     lenignorefirstchar;
    char   *ignorefirstchar;
    /* vars for WordCharacters */
    int     lenbumpposchars;
    char   *bumpposchars;
    /* vars for header values */
    char   *savedasheader;
    int     lensavedasheader;
    int     lenindexedon;
    char   *indexedon;
    int     lenindexn;
    char   *indexn;
    int     lenindexd;
    char   *indexd;
    int     lenindexp;
    char   *indexp;
    int     lenindexa;
    char   *indexa;
    int     minwordlimit;
    int     maxwordlimit;
    int     applyStemmingRules; /* added 11/24/98 - MG */
    int     applySoundexRules;  /* added 09/01/99 - DN */
/* Removed - Patents ...
    int     applyFileInfoCompression;
	*/
    /* Total files and words in index file */
    int     totalwords;
    int     totalfiles;
    /* var to specify how to ranking while indexing */
    int     ignoreTotalWordCountWhenRanking; /* added 11/24/98 - MG */
    /* Lookup tables for fast access */
    int     wordcharslookuptable[256];
    int     begincharslookuptable[256];
    int     endcharslookuptable[256];
    int     ignorefirstcharlookuptable[256];
    int     ignorelastcharlookuptable[256];
    int     bumpposcharslookuptable[256];
    int     translatecharslookuptable[256]; /* $$$ rasc 2001-02-21 */

    /* values for handling stopwords */
    struct swline *hashstoplist[HASHSIZE];
    char  **stopList;
    int     stopMaxSize;
    int     stopPos;

    /* Buzzwords hash */
    int    buzzwords_used_flag; /* flag to indicate that buzzwords are being used */
    struct swline *hashbuzzwordlist[HASHSIZE];
    
    /* values for handling "use" words - > Unused in the search proccess */
    int     is_use_words_flag;
    struct swline *hashuselist[HASHSIZE];

	    /* Values for fields (metanames) */
    struct metaEntry **metaEntryArray;
    int     metaCounter;        /* Number of metanames */

    /* Lookup tables for repetitive values of locations (frequency,structureand metaname */
    /* Index use all of them */
    /* search does not use locationlookup */
    struct int_lookup_st *locationlookup;
    struct int_lookup_st *structurelookup;
    struct int_lookup_st *structfreqlookup;
    /* Lookup table for repetitive values of the path */
    struct char_lookup_st *pathlookup;
	    /* Internal Swish meta/props */
    struct metaEntry *filenameProp;
    struct metaEntry *titleProp;
    struct metaEntry *filedateProp;
    struct metaEntry *startProp;
    struct metaEntry *sizeProp;
    struct metaEntry *summaryProp;
		/* Array to handle the number of words per doc */
    int    *filetotalwordsarray;

}
INDEXDATAHEADER;

typedef struct IndexFILE
{
    struct IndexFILE *next;
    struct IndexFILE *nodep;

    char   *line;               /*Name of the index file */

    /* file index info */
    struct file **filearray;
    int     filearray_cursize;
    int     filearray_maxsize;


    /* DB handle */
    void   *DB;

    /* Header Info */
    INDEXDATAHEADER header;

    /* Pointer to cache the keywords */
    char   *keywords[256];


	/* Removed due to patents problems
    int     n_dict_entries;
    unsigned char **dict; */      /* Used to store the patterns when
                                   ** DEFLATE_FILES is enabled */

    /* props IDs */
    int    *propIDToDisplay;
    int    *propIDToSort;

}
IndexFILE;

typedef struct RESULT
{
    int     count;              /* result Entry-Counter */
    int     filenum;
    int     rank;
    int     structure;
    int     frequency;
    int    *position;
    struct RESULT *next;
    struct RESULT *head;
    struct RESULT *nextsort;    /* Used while sorting results */
    char   *filename;
    time_t  last_modified;      /* former ISOTime */
    char   *title;
    char   *summary;
    int     start;
    int     size;
    /* file position where this document's properties are stored */
//    char  **Prop;
    char  **PropSort;
    int    *iPropSort;          /* Used for presorted data */
    IndexFILE *indexf;
    int     read;               /* 0 if file data and properties have not yet been readed from the index file */
    struct SWISH *sw;
}
RESULT;

struct multiswline
{
    struct swline *list;
    struct multiswline *next;
};




typedef struct
{
    int     numWords;
    ENTRY **elist;
}
ENTRYARRAY;


typedef struct
{
    int     currentsize;
    int     maxsize;
    char  **filenames;
}
DOCENTRYARRAY;

struct url_info
{
    char   *url;
    struct url_info *next;
};

struct IndexContents
{
    int     DocType;
    struct swline *patt;
    struct IndexContents *next;
};

struct StoreDescription
{
    int     DocType;
    char   *field;
    int     size;
    struct StoreDescription *next;
};

/* These two structs are used for lookuptables in order to save memory */
/* Normally Metaname, frequency and structure are repetitive schemas */
/* and usually have also low values */
/* In this way three values can be fit in just one using a lookup table*/
/* Structure itself can use its own lookuptable */
struct int_st
{
    int     index;
    struct int_st *next;
    int     val[1];
};

struct int_lookup_st
{
    int     n_entries;
    struct int_st *hash_entries[HASHSIZE];
    struct int_st *all_entries[1];
};

/* These two structs are used for lookuptables in order to save memory */
/* Normally part of the path/url are repetitive schemas */
/* and usually have also low values */
struct char_st
{
    int     index;
    struct char_st *next;
    char   *val;
};

struct char_lookup_st
{
    int     n_entries;
    struct char_st *hash_entries[HASHSIZE];
    struct char_st *all_entries[1];
};






/* -- Property data types 
   -- Result handling structures, (types storage, values)
   -- Warnung! Changing types inflicts outpur routines, etc 
   -- 2001-01  rasc
  
   $$$ ToDO: data types are not yet fully supported by swish
   $$$ Future: to be part of module data_types.c/h
*/


typedef enum
{                               /* Property Datatypes */
    UNDEFINED = -1, UNKNOWN = 0, STRING, INTEGER, FLOAT, DATE, ULONG
}
PropType;

typedef union
{                               /* storage of the PropertyValue */
    char   *v_str;              /* strings */
    int     v_int;              /* Integer */
    time_t  v_date;             /* Date    */
    double  v_float;            /* Double Float */
    unsigned long v_ulong;      /* Unsigned long */
}
u_PropValue1;

typedef struct
{                               /* Propvalue with type info */
    PropType datatype;
    u_PropValue1 value;
}
PropValue;



/* --------------------------------------- */



/* Structure to hold all results per index */
struct DB_RESULTS
{
    /* Values for handling results */
    RESULT *resultlist;
    RESULT *sortresultlist;
    RESULT *currentresult;
    struct DB_RESULTS *next;
};



typedef struct
{
    /* New module design structure data */
    struct MOD_SearchAlt     *SearchAlt;      /* search_alt module data */
    struct MOD_ResultOutput  *ResultOutput;   /* result_output module data */
    struct MOD_Filter        *Filter;         /* filter module data */
    struct MOD_ResultSort    *ResultSort;     /* result_sort module data */
    struct MOD_Entities      *Entities;       /* html entities module data */
    struct MOD_DB            *Db;             /* DB module data */
    struct MOD_Search        *Search;         /* Search module data */
    struct MOD_Index         *Index;          /* Index module data */
    struct MOD_FS            *FS;             /* FileSystem Index module data */
    struct MOD_HTTP          *HTTP;           /* HTTP Index module data */
    struct MOD_Swish_Words   *SwishWords;     /* For parsing into "swish words" */
    struct MOD_Prog          *Prog;           /* For extprog.c */


    struct MOD_PropLimit     *PropLimit;      /* For extprog.c */


    /* 08/00 Jose Ruiz Values for document type support */
    int     DefaultDocType;
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

    /* structure to handle Ignoremeta metanames */
    struct swline *ignoremetalist;
    /* Structure for handling metatags from DontBumpPositionOnMetaTags */
    struct swline *dontbumptagslist;

    /* structure for handling all the index files while searching  */
    IndexFILE *indexlist;

    /* Total words and files in all index files */
    int     TotalWords;
    int     TotalFiles;
    /* verbose flag */
    int     verbose;
    /* Error vars */
    int     commonerror;
    int     lasterror;

    int     indexComments;

    long    truncateDocSize;    /* size of doc, at which it will be truncated (2001-03-16 rasc) */

    /* 06/00 Jose Ruiz */
    int     applyautomaticmetanames;
    int     isvowellookuptable[256];

    /* Limit indexing by a file date */
    time_t  mtime_limit;


    /* MetaName indexing options */
    int     ReqMetaName;
    int     OkNoMeta;

}
SWISH;

/* 06/00 Jose Ruiz
** Structure  StringList. Stores words up to a number of n
*/
typedef struct  {
        int n;
        char **word;
} StringList;

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
    const char *IndexingDataSourceName; /* long name for data source */
    const char *IndexingDataSourceId; /* short name for data source */
    void    (*indexpath_fn) (SWISH * sw, char *path); /* routine to index a "path" */
    int     (*parseconfline_fn) (SWISH * sw, StringList *l); /* parse config file lines */
};




#ifndef GLOBAL_VARS
#define VAR extern
#else
#define VAR
#endif


VAR struct _indexing_data_source_def *IndexingDataSource;


#ifdef MAIN_FILE



int     main(int, char **);
void    usage(void);
void    printversion(void);
void    printrunning(long, long);

#endif



void    allocatedefaults(void);

SWISH  *SwishNew(void);
void    SwishDefaults(SWISH *);
void    SwishFree(SWISH *);

/* strcpy doesn't check for overflow in the 'to' string */
/* strncpy doesn't guarantee null byte termination */
/* can't check strlen of 'from' arg since it is sometimes a function call */
#define safestrcpy(n,to,from)  { strncpy(to,from,n); (to)[(n)-1]='\0'; }

/* Jose Ruiz 04/00
** Macro for copying postions between arrays of integers
** copy num integers on dest (starting at posdest) from
** orig (starting at posorig)
*/
/* 
#define CopyPositions(dest,posdest,orig,posorig,num) \
{int i;for(i=0;i<num,i++) (dest)[i+(posdest)]=(orig)[i+(posorig)];}
*/
#define CopyPositions(dest,posdest,orig,posorig,num) \
 memcpy((char *)((int *)(dest)+(posdest)),(char *)((int *)(orig)+(posorig)),(num)*sizeof(int))


/* Min macro */
#define Min(a,b) ((a)<(b)?(a):(b))



/* C library prototypes */
SWISH  *SwishOpen(char *);
void    SwishClose(SWISH *);
void    SwishResetSearch(SWISH *);
RESULT *SwishNext(SWISH *);
int     SwishSearch(SWISH *, char *, int, char *, char *);
int     SwishSeek(SWISH * sw, int pos);
int     getnumPropertiesToDisplay(SWISH *);


/* These are only checked in dump.c */
#define DEBUG_INDEX_HEADER 1<<0
#define DEBUG_INDEX_WORDS 1<<1
#define DEBUG_INDEX_WORDS_FULL 1<<2
#define DEBUG_INDEX_STOPWORDS 1<<3
#define DEBUG_INDEX_FILES 1<<4
#define DEBUG_INDEX_METANAMES 1<<5
#define DEBUG_INDEX_ALL 1<<6
#define DEBUG_INDEX_WORDS_ONLY 1<<7
#define DEBUG_INDEX_WORDS_META 1<<8

/* These are only checked while indexing */
#define DEBUG_WORDS 1<<0
#define DEBUG_PARSED_WORDS 1<<1
#define DEBUG_PROPERTIES 1<<2

/* These are only checked while searching */

/* These are are checked everywhere (can't share bits) */


extern unsigned int DEBUG_MASK;
