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
** 2001-08-12 jmruiz ENTRY struct modified to index in chunks
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

/* Admin header */
#define NAMEHEADERPARAMNAME "IndexName"
#define DESCRIPTIONPARAMNAME "IndexDescription"
#define POINTERPARAMNAME "IndexPointer"
#define MAINTAINEDBYPARAMNAME "IndexAdmin"


/* Other headers that can be looked via the swish-e library */
#define INDEXEDONPARAMNAME "IndexedOn"
#define WORDCHARSPARAMNAME "WordCharacters"
#define BEGINCHARSPARAMNAME "BeginCharacters"
#define ENDCHARSPARAMNAME "EndCharacters"
#define IGNOREFIRSTCHARPARAMNAME "IgnoreFirstChar"
#define IGNORELASTCHARPARAMNAME "IgnoreLastChar"
#define STEMMINGPARAMNAME "Stemming"
#define SOUNDEXPARAMNAME "Soundex"



/* Headers for output, and their offsets */
#define NAMEHEADER "# Name:"
#define NAMEHEADER_ID (BASEHEADER + 3)

#define SAVEDASHEADER "# Saved as:"
#define SAVEDASHEADER_ID (BASEHEADER + 4)

#define COUNTSHEADER "# Counts:"
#define COUNTSHEADER_ID (BASEHEADER + 5)

#define INDEXEDONHEADER "# Indexed on:"
#define INDEXEDONHEADER_ID (BASEHEADER + 6)

#define DESCRIPTIONHEADER "# Description:"
#define DESCRIPTIONHEADER_ID (BASEHEADER + 7)

#define POINTERHEADER "# Pointer:"
#define POINTERHEADER_ID (BASEHEADER + 8)

#define MAINTAINEDBYHEADER "# Maintained by:"
#define MAINTAINEDBYHEADER_ID (BASEHEADER + 9)

#define WORDCHARSHEADER "# WordCharacters:"
#define WORDCHARSHEADER_ID (BASEHEADER + 10)

#define MINWORDLIMHEADER "# MinWordLimit:"
#define MINWORDLIMHEADER_ID (BASEHEADER + 11)

#define MAXWORDLIMHEADER "# MaxWordLimit:"
#define MAXWORDLIMHEADER_ID (BASEHEADER + 12)

#define BEGINCHARSHEADER "# BeginCharacters:"
#define BEGINCHARSHEADER_ID (BASEHEADER + 13)

#define ENDCHARSHEADER "# EndCharacters:"
#define ENDCHARSHEADER_ID (BASEHEADER + 14)

#define IGNOREFIRSTCHARHEADER "# IgnoreFirstChar:"
#define IGNOREFIRSTCHARHEADER_ID (BASEHEADER + 15)

#define IGNORELASTCHARHEADER "# IgnoreLastChar:"
#define IGNORELASTCHARHEADER_ID (BASEHEADER + 16)

#define STEMMINGHEADER	"# Stemming Applied:"
#define STEMMINGHEADER_ID (BASEHEADER + 17)

#define SOUNDEXHEADER "# Soundex Applied:"
#define SOUNDEXHEADER_ID (BASEHEADER + 18)

#define MERGED_ID (BASEHEADER + 19)

/* vv not used vv */
#define DOCPROPHEADER "# DocProperty"
#define DOCPROPHEADER_ID (BASEHEADER + 20)
/* ^^ not used ^^ */

#define DOCPROPENHEADER "# DocumentProperties:"
#define DOCPROPENHEADER_ID (BASEHEADER + 21)

#define SORTDOCPROPHEADER_ID (BASEHEADER + 22)

#define IGNORETOTALWORDCOUNTWHENRANKING "# IgnoreTotalWordCountWhenRanking:"
#define IGNORETOTALWORDCOUNTWHENRANKINGPARAMNAME "IgnoreTotalWordCountWhenRanking"
#define IGNORETOTALWORDCOUNTWHENRANKING_ID (BASEHEADER + 23)

#define TRANSLATECHARTABLEHEADER "# TranslateCharacterTable:"
#define TRANSLATECHARTABLEPARAMNAME "TranslateCharacterTable"
#define TRANSLATECHARTABLE_ID (BASEHEADER + 25)

#define STOPWORDS_ID (BASEHEADER + 26)
#define METANAMES_ID (BASEHEADER + 27)
#define LOCATIONLOOKUPTABLE_ID (BASEHEADER + 28)
#define BUZZWORDS_ID (BASEHEADER + 29) /* 2001-04-24 moseley */

#ifndef USE_BTREE
#define TOTALWORDSPERFILE_ID (BASEHEADER + 30)  /* total words per file array */
#endif

/* -- end of headers */

#define MAXFILELEN 1000
#define MAXSTRLEN 2000
#define MAXWORDLEN 1000
#define MAXTITLELEN 300
#define MAXENTLEN 10

// #define HASHSIZE 101
// #define BIGHASHSIZE 1009
// #define SEARCHHASHSIZE 10001

// Change as suggested by Jean-François PIÉRONNE <jfp@altavista.net>
// on Fri, 28 Dec 2001 07:37:26 -0800 (PST)
#define HASHSIZE 1009
#define BIGHASHSIZE 10001
#define SEARCHHASHSIZE 100003


#define MAXPAR 10
#define MAXCHARDEFINED 256
#define RD_BUFFER_SIZE  65356   /* init size, larger to avoid often reallocs  (2001-03-16 rasc) */

#define NOWORD "thisisnotaword"
#define SECSPERMIN 60

#define IN_FILE_BIT     0
#define IN_TITLE_BIT    1
#define IN_HEAD_BIT     2
#define IN_BODY_BIT     3
#define IN_COMMENTS_BIT 4
#define IN_HEADER_BIT   5
#define IN_EMPHASIZED_BIT   6
#define IN_META_BIT     7
#define STRUCTURE_END 7


#define IN_FILE		(1<<IN_FILE_BIT)
#define IN_TITLE	(1<<IN_TITLE_BIT)
#define IN_HEAD		(1<<IN_HEAD_BIT)
#define IN_BODY		(1<<IN_BODY_BIT)
#define IN_COMMENTS	(1<<IN_COMMENTS_BIT)
#define IN_HEADER	(1<<IN_HEADER_BIT)
#define IN_EMPHASIZED (1<<IN_EMPHASIZED_BIT)
#define IN_META		(1<<IN_META_BIT)
#define IN_ALL (IN_FILE|IN_TITLE|IN_HEAD|IN_BODY|IN_COMMENTS|IN_HEADER|IN_EMPHASIZED|IN_META)

#define MAXLONGLEN 4

/* Document Types */
enum {
	BASEDOCTYPE = 0, TXT, HTML, XML, WML, XML2, HTML2, TXT2
};

#define NODOCTYPE BASEDOCTYPE

// This is used to build the property to read/write to disk
// It's here so the buffer can live between writes



typedef struct propEntry
{
    unsigned int propLen;       /* Length of buffer */
    unsigned char propValue[1]; /* Actual property value starts here */
}
propEntry;



typedef struct docProperties
{
    int n;  /* to be removed - can just use count of properties */
	struct propEntry *propEntry[1];  /* Array to hold properties */
}
docProperties;

#define RANK_BIAS_RANGE 10 // max/min range ( -10 -> 10, with zero being no bias )

/* This structure is for storing both properties and metanames -- probably should be two lists */
struct metaEntry
{
    char       *metaName;           /* MetaName string */
    int         metaID;             /* Meta ID */
    int         metaType;           /* See metanames.h for values */
    int        *inPropRange;        /* Used for limiting to a range */
    int         in_tag;             /* Flag to indicate that we are within this tag */
    int         max_len;            /* If non-zero, limits properties to this length (for storedescription) */
    char       *extractpath_default; /* String to index under this metaname if none found with ExtractPath */
    propEntry  *loPropRange;
    propEntry  *hiPropRange;
    int         alias;              /* if non-zero, this is an alias to the listed metaID */
    int         rank_bias;          /* An integer used to bias hits on this metaname 0 = no bias */
    int        *sorted_data;        /* Sorted data . NULL if not read/done */
                                    /* If 0, files are not sorted by this metaName/property */
};



typedef struct
{
    long    length;
    long    seek;
} PROP_LOCATION;

typedef struct
{
    PROP_LOCATION   prop_position[1];  // there used to be more in there.
} PROP_INDEX;


typedef struct
{
    int             filenum;
    docProperties  *docProperties;  /* list of document props in memory */
    PROP_INDEX     *prop_index;     /* pointers to properties on disk */
} FileRec;


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
    char   *real_path;          /* path/URL to indexed file - may be modified by ReplaceRules */
    char   *orig_path;          /* original path provided to swish */
    char   *work_path;          /* path to file to index (may be tmpfile or real_path) */
    char   *real_filename;      /* basename() of real_path  */
    long    fsize;              /* size of the original file (not filtered) */
    long    bytes_read;         /* Number of bytes read from the stream - important for sw->truncateDocSize and -S prog */
    int     done;               /* flag to read no more from this stream (truncate) */
    int     external_program;   /* Flag to only read fsize bytes from stream */
    time_t  mtime;              /* Date of last mod of or. file */
    int     doctype;            /* Type of document HTML, TXT, XML, ... */
    int     index_no_content;   /* Flag, index "filename/real_path" only! */
    struct StoreDescription *stordesc;   /* Null if no description/summary */
    struct FilterList *hasfilter;       /* NULL if no filter for this file */
}
FileProp;


typedef struct LOCATION
{
    struct LOCATION *next;
    int     metaID;
    int     filenum;
    int     frequency;
    int     posdata[1];
}
LOCATION;


/* 2002/01 jmruiz macros for accesing POSITION and structure */
#define SET_POSDATA(pos,str)  ((pos) << 8 | (str))
#define GET_POSITION(pos)      ((pos) >> 8)
#define GET_STRUCTURE(pos)     ((pos) & 0xff)

typedef struct ENTRY
{
    struct ENTRY *next;
    int     tfrequency;
       /* Chunk's LOCATIONs goes here */
    LOCATION *currentChunkLocationList;
    LOCATION *currentlocation;
       /* All locations goes here */
    LOCATION *allLocationList;

    /* this union is just for saving memory */
    struct
    {
        long    wordID;
        int     last_filenum;
    }
    u1;
#ifdef __VMS
    char    word[];     /* actual word starts here */
#else
    char    word[0];    /* actual word starts here */
#endif
}
ENTRY;

struct swline
{
    struct swline *next;
    struct swline *nodep;
    char   *line;
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

    /* vars for bump position chars */
    int     lenbumpposchars;
    char   *bumpposchars;

    /* vars for header values */
    char   *savedasheader;
    int     lensavedasheader;

    /* vars for numberchars */  /* Not yet stored in the header. */ 
    int     lennumberchars;     /* Probably don't need it for searching */
    char   *numberchars;
    int     numberchars_used_flag;
    

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

    /* Total files and words in index file */
    int     totalwords;
    int     totalfiles;

    /* var to specify how to ranking while indexing */
    int     ignoreTotalWordCountWhenRanking; /* added 11/24/98 - MG */

    int     *TotalWordsPerFile;
    int     TotalWordsPerFileMax;  /* max size of array - this isn't saved in the header */


    /* Lookup tables for fast access */
    int     wordcharslookuptable[256];
    int     begincharslookuptable[256];
    int     endcharslookuptable[256];
    int     ignorefirstcharlookuptable[256];
    int     ignorelastcharlookuptable[256];
    int     bumpposcharslookuptable[256];
    int     translatecharslookuptable[256]; /* $$$ rasc 2001-02-21 */
    int     numbercharslookuptable[256];    /* Dec 12, 2001 - moseley -- mostly for ignoring numbers */

    /* values for handling stopwords */
    struct swline *hashstoplist[HASHSIZE];
    char  **stopList;
    int     stopMaxSize;
    int     stopPos;

    /* This is an array of properties that are used */
    /* These should not be in the header, rather in indexf as they are not written to disk */
    int     *propIDX_to_metaID;
    int     *metaID_to_PropIDX;
    int     property_count;



    /* Buzzwords hash */
    int    buzzwords_used_flag; /* flag to indicate that buzzwords are being used */
    struct swline *hashbuzzwordlist[HASHSIZE];
    
    /* values for handling "use" words - > Unused in the search proccess */
    int     is_use_words_flag;
    struct swline *hashuselist[HASHSIZE];

	    /* Values for fields (metanames) */
    struct metaEntry **metaEntryArray;
    int     metaCounter;        /* Number of metanames */

}
INDEXDATAHEADER;

typedef struct IndexFILE
{
    struct IndexFILE *next;
    struct IndexFILE *nodep;

    char   *line;               /*Name of the index file */

    unsigned long total_bytes;  /* Just to show total size when indexing */
    unsigned long total_word_positions;


    /* DB handle */
    void   *DB;

    /* Header Info */
    INDEXDATAHEADER header;

    /* Pointer to cache the keywords */
    char   *keywords[256];


    /* props IDs */
    int    *propIDToDisplay;
    int    *propIDToSort;


    /* Support for merge */
    int     *meta_map;              // maps metas from this index to the output index
    int     *path_order;            // lists files in order of pathname
    int     current_file;           // current file pointer, used for merged reading
    struct  metaEntry *path_meta;   // meta entry for the path name
    struct  metaEntry *modified_meta;
    propEntry *cur_prop;            // last read pathname
    int     filenum;                // current filenumber to use
    

}
IndexFILE;


typedef struct RESULT_LIST
{
    struct RESULT *head;
    struct RESULT *tail;
    struct SWISH  *sw;
}
RESULT_LIST;

typedef struct RESULT
{
    struct RESULT *next;

    int     count;              /* result Entry-Counter */
    int     filenum;            /* there's an extra four bytes we don't need */
    FileRec fi;                 /* This is used to cache the properties and the seek index */
    int     rank;
    int     frequency;
    int     tfrequency;         /* Total frequency of result */

    /* file position where this document's properties are stored */
    char  **PropSort;
    int    *iPropSort;          /* Used for presorted data */
    IndexFILE *indexf;

    RESULT_LIST *reslist;

    int     posdata[0];
}
RESULT;

struct multiswline
{
    struct multiswline *next;
    struct swline *list;
};


typedef struct
{
    int     numWords;
    ENTRY **elist;     /* Sorted by word */
}
ENTRYARRAY;



struct url_info
{
    struct url_info *next;
    char   *url;
};

struct IndexContents
{
    struct IndexContents *next;
    int     DocType;
    struct swline *patt;
};

struct StoreDescription
{
    struct StoreDescription *next;
    int     DocType;
    char   *field;
    int     size;
};

/* These two structs are used for lookuptables in order to save memory */
/* Normally Metaname, frequency and structure are repetitive schemas */
/* and usually have also low values */
/* In this way three values can be fit in just one using a lookup table*/
/* Structure itself can use its own lookuptable */
struct int_st
{
    struct int_st *next;
    int     index;
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
    struct char_st *next;
    int     index;
    char   *val;
};

struct char_lookup_st
{
    int     n_entries;
    struct char_st *hash_entries[HASHSIZE];
    struct char_st *all_entries[1];
};


/* Place to store compiled regular expressions */

typedef struct regex_list
{
    struct regex_list *next;
    regex_t     re;
    char       *replace;
    int         replace_count;  /* number of pattern replacements - to estimate size of replacement string */
    int         replace_length; /* newstr_max = replace_length + ( replace_count * search_str_len ) */
    int         global;         /* /g flag to repeat sub */
    int         negate;         /* Flag for matches if the match should be negated */
    char       *pattern;        /* keep string pattern around for debugging */
} regex_list;

typedef struct path_extract_list
{
    struct path_extract_list    *next;
    struct metaEntry            *meta_entry;
    regex_list                  *regex;
} path_extract_list;



/* -- Property data types 
   -- Result handling structures, (types storage, values)
   -- Warnung! Changing types inflicts outpur routines, etc 
   -- 2001-01  rasc
  
   $$$ ToDO: data types are not yet fully supported by swish
   $$$ Future: to be part of module data_types.c/h
*/


typedef enum
{                               /* Property Datatypes */
    UNDEFINED = -1,
    UNKNOWN = 0,
    STRING,
    INTEGER,
    FLOAT,
    DATE, 
    ULONG
}
PropType;

/* For undefined meta names */
typedef enum
{
    UNDEF_META_DISABLE = 0, // Only for XMLAtrributes - don't even try with attributes
    UNDEF_META_INDEX,       // index as plain text
    UNDEF_META_AUTO,        // create metaname if doesn't exist
    UNDEF_META_ERROR,       // throw a nasty error
    UNDEF_META_IGNORE       // don't index
}
UndefMetaFlag;
    

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
    int      destroy;           /* flag to destroy (free) any pointer type */
}
PropValue;



/* --------------------------------------- */



/* Structure to hold all results per index */
struct DB_RESULTS
{
    struct DB_RESULTS *next;
    /* Values for handling results */
    RESULT_LIST *resultlist;
    RESULT *sortresultlist;
    RESULT *currentresult;
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
    struct MOD_PropLimit     *PropLimit;      /* For proplimit.c */


    /** General Purpose **/

    /* list of associated index files  */
    IndexFILE *indexlist;


    unsigned char            *Prop_IO_Buf;      /* For compressing and uncompressing properties (static-like buffer) */
    unsigned long             PropIO_allocated;// total size of the structure
    int                       PropCompressionLevel;


    /* Total words and files in all index files */
    int     TotalWords;
    int     TotalFiles;

    /* verbose flag */
    int     verbose;

    /* Error vars */
    int     commonerror;
    int     lasterror;
    char    *lasterrstr;


    /* 06/00 Jose Ruiz */
    int     isvowellookuptable[256];  //??? is this used any place?


    /********* Document Source info **********/

    /* structure for handling all the directories/files (IndexDIR) while indexing  */
    struct swline *dirlist;

    /* structure for handling IndexOnly config data while indexing */
    struct swline *suffixlist;




    /******** Structures for parsers **********/


    /* Limit indexing by a file date */
    time_t  mtime_limit;

    long    truncateDocSize;    /* size of doc, at which it will be truncated (2001-03-16 rasc) */


    /* structure for handling replace config data while searching */
    regex_list     *replaceRegexps;


    /* It's common to want to limit searches to areas of a file or web space */
    /* This allow extraction of a substring out of a file path, and indexed as a metaname */
    path_extract_list   *pathExtractList;

    

    /* structure for handling NoContents config data while searching */
    struct swline *nocontentslist;

    /* 08/00 Jose Ruiz Values for document type support */
    int     DefaultDocType;

    /* maps file endings to document types */
    struct IndexContents *indexcontents;


    /* Should comments be indexed */
    int     indexComments;



    /******** Variables used by the parsers *********/

    /* 12/00 Jose Ruiz Values for summary support */
    struct StoreDescription *storedescription;


    /* structure to handle Ignoremeta metanames */
    struct swline *ignoremetalist;


    /* Structure for handling metatags from DontBumpPositionOnMetaTags */
    struct swline *dontbumpstarttagslist;
    struct swline *dontbumpendtagslist;


    /* Undefined MetaName indexing options */
    UndefMetaFlag   UndefinedMetaTags;
    UndefMetaFlag   UndefinedXMLAttributes;  // What to do with attributes  libxml2 only



    /*** libxml2 additions ***/

    /* parser error warning level */
    int     parser_warn_level;

    int     obeyRobotsNoIndex;

    /* for extracting links into a metaEntry */
    struct metaEntry *links_meta;

    /* for extracting image hrefs into a metaEntry */
    struct metaEntry *images_meta;

    /* for converting relative links in href's and img src tags absoulte */
    int               AbsoluteLinks;


    /* structure to handle XMLClassAttributes - list of attributes to use content to make a metaname*/
    /* <foo class="bar"> => generates a metaname foo.bar */
    struct swline *XMLClassAttributes;

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



void    allocatedefaults(void);

SWISH  *SwishNew(void);
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
#define Min(a,b) ((a) < (b) ? (a) : (b))



/* C library prototypes */
SWISH  *SwishOpen(char *);
void    SwishClose(SWISH *);
void    SwishResetSearch(SWISH *);
RESULT *SwishNext(SWISH *);
int     SwishSearch(SWISH *, char *, int, char *, char *);
int     SwishSeek(SWISH * sw, int pos);


/* These are only checked in dump.c */
#define DEBUG_INDEX_HEADER		(1<<0)
#define DEBUG_INDEX_WORDS		(1<<1)
#define DEBUG_INDEX_WORDS_FULL	(1<<2)
#define DEBUG_INDEX_STOPWORDS	(1<<3)
#define DEBUG_INDEX_FILES		(1<<4)
#define DEBUG_INDEX_METANAMES	(1<<5)
#define DEBUG_INDEX_ALL			(1<<6)
#define DEBUG_INDEX_WORDS_ONLY	(1<<7)
#define DEBUG_INDEX_WORDS_META	(1<<8)

/* These are only checked while indexing */
#define DEBUG_WORDS				(1<<0)
#define DEBUG_PARSED_WORDS		(1<<1)
#define DEBUG_PROPERTIES		(1<<2)
#define DEBUG_REGEX 	    	(1<<3)
#define DEBUG_PARSED_TAGS    	(1<<4)
#define DEBUG_PARSED_TEXT 	   	(1<<5)

/* These are only checked while searching */

/* These are are checked everywhere (can't share bits) */


extern unsigned int DEBUG_MASK;
