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
**--------------------------------------------------------------------------
** Config file edited by Roy Tennant 2/20/96
** Config file edited by Giulia Hill 2/27/97 to increase lenght of
**        words that are indexed
** Added IGNORELASTCHAR
**        G. Hill 3/12/97 ghill@library.berkeley.edu
**
** Added OKNOMETA to allow no failing in case the META name is
** not listed in the config.h
**        G. Hill 4/15/97 ghill@library.berkeley.edu
**
** Added IGNOREFIRSTCHAR
**        G.Hill 10/16/97 ghill@library.berkeley.edu
**-----------------------------------------------------------------------
** The following are user-definable options that you can change
** to fine-tune SWISH's default options.
**
** 2001-03-13 rasc   moved search boolean words from swish.h
**
** 2001-05-23 wsm    added ranking weights
**
*/

#define ALLOW_HTTP_INDEXING_DATA_SOURCE		1
#define ALLOW_FILESYSTEM_INDEXING_DATA_SOURCE	1
#define ALLOW_EXTERNAL_PROGRAM_DATA_SOURCE	1

/* These symbols allow compile-time elimination of indexing
** data sources. Any Data Source that is allowed by these
** symbols can be selected for indexing from the command line.
** Comment out any options you do not want to support, but
** be sure to leave at least one option.
*/

#define INDEXPERMS 0644

/* After SWISH generates an index file, it changes the permissions
** of the file to this mode. Change to the mode you like
** (note that it must be an octal number). If you don't want
** permissions to be changed for you, comment out this line.
*/

#define NO_PLIMIT 101

#define PLIMIT NO_PLIMIT
#define FLIMIT 10000

/* SWISH uses these parameters to automatically mark words as
** being too common while indexing. For instance, if I defined PLIMIT
** as 80 and FLIMIT as 256, SWISH would define a common word as
** a word that occurs in over 80% of all indexed files and over
** 256 files. Making these numbers lower will most likely make your
** index files smaller. Making PLIMIT and FLIMIT small will also
** ensure that searching consumes only so much CPU resources.
*/

#define VERBOSE 3

/* You can define VERBOSE to be a number from 0 to 4. 0 is totally
** silent operation; 4 is very verbose.
*/

#define _AND_WORD "and"
#define _OR_WORD "or"
#define _NOT_WORD "not"

/* 
 ** these are the default boolean operator words used by swish search
*/

#define DEFAULT_RULE AND_RULE

/* If a list of search words is specified without booleans,
** SWISH will assume they are connected by a default rule.
** This can be AND_RULE or OR_RULE.
*/

#define TITLETOPLINES 12

/* This is how many lines deep SWISH will look into an HTML file to
** attempt to find a <TITLE> tag.
*/


#define MINWORDLIMIT 1

/* This is the minimum length of a word. Anything shorter will not
** be indexed.
** Do not change it here. Use MinWordLimit in config file
*/

#define MAXWORDLIMIT 40

/* This is the maximum length of a word. Anything longer will not
** be indexed.
** Do not change it here. Use MaxWordLimit in config file
*/

#define CONVERTHTMLENTITIES 1

/* If defined as 1, all entities in indexed
** words will be converted to an ASCII equivalent. For instance,
** with this feature you can index the word "resum&eacute;" or
** "resum&#233;" and it will be indexed as the word "resume".
** 2001-01 Do not change it here. Use ConvertHTMLEtities Yes/No in 
** config file
*/

#define IGNOREALLV 0
#define IGNOREALLC 0
#define IGNOREALLN 0

/* If IGNOREALLV is 1, words containing all vowels won't be indexed.
** If IGNOREALLC is 1, words containing all consonants won't be indexed.
** If IGNOREALLN is 1, words containing all digits won't be indexed.
** Define as 0 to allow words with consistent characters.
** Vowels are defined as "aeiou", digits are "0123456789".
*/

#define IGNOREROWV 60
#define IGNOREROWC 60
#define IGNOREROWN 60

/* IGNOREROWV is the maximum number of consecutive vowels a word can have.
** IGNOREROWC is the maximum number of consecutive consonants a word can have.
** IGNOREROWN is the maximum number of consecutive digits a word can have.
** Vowels are defined as "aeiou", digits are "0123456789".
*/

#define IGNORESAME 100

/* IGNORESAME is the maximum times a character can repeat in a word.
*/

#define WORDCHARS "abcdefghijklmnopqrstuvwxyz¡¬√»˝ ÀÃ–›ﬁÕŒœ“”‘’ÿŸ€ÓË„ıöúùÄﬂÉäåçéè¯ü£‹ûêÌ¿0123456789"

/* WORDCHARS is a string of characters which SWISH permits to
** be in words.  Words are defined by these characters.
**
** Also note that if you specify the backslash character (\) or
** double quote (") you need to type a backslash before them to
** make the compiler understand them.
*/

#define BEGINCHARS "abcdefghijklmnopqrstuvwxyz¡¬√»˝ ÀÃ–›ﬁÕŒœ“”‘’ÿŸ€ÓË„ıöúùÄﬂÉäåçéè¯ü£‹ûêÌ¿0123456789"

/* Of the characters that you decide can go into words, this is
** a list of characters that words can begin with. It should be
** a subset of (or equal to) WORDCHARS.
*/

#define ENDCHARS "abcdefghijklmnopqrstuvwxyz¡¬√»˝ ÀÃ–›ﬁÕŒœ“”‘’ÿŸ€ÓË„ıöúùÄﬂÉäåçéè¯ü£‹ûêÌ¿0123456789"

/* This is the same as BEGINCHARS, except you're testing for
** valid characters at the ends of words.
*/

#define IGNORELASTCHAR ""

/* Array that contains the char that, if considered valid in the middle of 
** a word need to be disreguarded when at the end. It is important to also
** set the given char's in the ENDCHARS array, otherwise the word will not
** be indexed because considered invalid.
** If none just leave the empty list "". Do not erase the line.
*/

#define IGNOREFIRSTCHAR ""
 
/* Array that contains the char that, if considered valid in the middle of 
** a word need to be disreguarded when at the beginning. It is important to also
** set the given char's in the BEGINCHARS array, otherwise the word will not
** be indexed because considered invalid.
** If none just leave the empty list "". Do not erase the line.
*/

#define OKNOMETA 1
/* Switch that define if it is ok to fail in case the META name is not listed 
** in the METANAMES variable. Value of 1 will cause the word to be listed as a
** regular words with no metaName attached
** 2000/12 Jose Ruiz: do not change it here. Use OkNoMeta yes/no in config file
*/

#define REQMETANAME 0
/* Set to 1 to not index any Meta tag contents unless the tag name is
** listed in the MetaNames parameter. Set to 0 and with OKNOMETA set 1
** Swish will place META contents in index with no metaName attached.
** 10/11/99 Bill Moseley
** 2000/12 Jose Ruiz: do not change it here. Use ReqMetaName yes/no in config file
*/

#define IGNORE_STOPWORDS_IN_QUERY 1
 
/* Added JM 1/10/98.  Setting this to 0 (default) causes a stopword in
** an AND_RULE search to create an empty result.  Setting it to 1 simply
** ignores the stopwords and does a search on the remaining words.
*/

#define INDEXTAGS 0

/* Normally, all data in tags in HTML files (except for words in
** comments or meta tags) is ignored. If you want to index HTML files with the
** text within tags and all, define this to be 1 and not 0.
** NOTE: if you set it to 1 you will not be able to do context nor
** metaNames searches, as tags are just plain text with no specific
** meaning.
*/

#define RANK_TITLE		4.0
#define RANK_HEADER		3.0
#define RANK_META		3.0
#define RANK_COMMENTS	1.0
#define RANK_EMPHASIZED 0.0

/* This symbols affect the weights applied during ranking. Note that they are added
** together and added to a base rank of 1.0 -- thus defining a rank with a value of
** 2.0 really means it is ranked (1.0 + 2.0) times greater than normal. 
** A value of 0.0 applies no additional ranking boost. Note that RANK_COMMENTS only
** applies if you are indexing comment. Be sure you understand how these interact
** in getrank; don't just go changing these values!
*/

#define SPIDERDIRECTORY "./"

#define SWAP_DEFAULT 0

/* 09/00 Jose Ruiz. When set to 1 part of the info is swapped to disk
** to save memory in the index proccess
** Do not change it. You can activate this option through the command
** line (option -e)
*/

/* Set this to 1 if you are compiling under Win32
   define _WIN32 1
 */

/* --- BEGIN PORTING-RELATED SYMBOLS --- */

#ifdef _WIN32
#define NO_SYMBOLIC_FILE_LINKS		/* Win32 has no symbolic links */
#endif

#ifdef __VMS
#define NO_SYMBOLIC_FILE_LINKS          /* VMS has no symbolic links */
#endif

#ifdef _WIN32
#undef INDEXPERMS		/* Win32 version doesn't use chmod() */
#endif

#ifdef _WIN32
typedef int pid_t;		/* process ID */
#endif

#ifdef _WIN32
#define TMPDIR "c:\\windows\\temp"
#elif defined(__VMS)
#define TMPDIR "sys$scratch:"
#else
#define TMPDIR "/var/tmp"
#endif
			
	/* Directory delimiter */
#ifdef _WIN32
#define DIRDELIMITER '\\'
#else
#define DIRDELIMITER '/'
#endif

/* Default Delimiter of phrase search */
#define PHRASE_DELIMITER_CHAR '"'


/*
 * Binary files must be open with the "b" option under Win32, so all
 * fopen() calls to index files have to go through these routines to
 * keep the code portable.
 * Note: text files should be opened normally, without the "b" option,
 * otherwise end-of-line processing is not done correctly (on Win32).
 */
#ifdef _WIN32
#define FILEMODE_READ           "rb"
#define FILEMODE_WRITE          "wb"
#define FILEMODE_READWRITE      "rb+"
#elif defined(__VMS)
#define FILEMODE_READ           "rb"
#define FILEMODE_WRITE          "wb"
#define FILEMODE_READWRITE      "rb+"
#else
#define FILEMODE_READ           "r"
#define FILEMODE_WRITE          "w"
#define FILEMODE_READWRITE      "r+"
#endif



/* #define NEXTSTEP */

/* You may need to define this if compiling on a NeXTstep machine.
*/

/* --- END PORTING-RELATED SYMBOLS --- */

