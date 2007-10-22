/*
** Copyright (C) 1995, 1996, 1997, 1998 Hewlett-Packard Company
** Originally by Kevin Hughes, kev@kevcom.com, 3/11/94
**

$Id$

    This file is part of Swish-e.

    Swish-e is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Swish-e is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along  with Swish-e; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    
    See the COPYING file that accompanies the Swish-e distribution for details
    of the GNU GPL and the special exception available for linking against
    the Swish-e library.
    
** Mon May  9 18:19:34 CDT 2005
** added GPL


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


#ifdef __VMS
#define PROPFILE_EXTENSION "_prop"
#define WORDDATA_EXTENSION "_wdata"
#define PRESORTED_EXTENSION "_psort"
#define BTREE_EXTENSION "_btree"
#define ARRAY_EXTENSION "_array"
#define HASHFILE_EXTENSION "_file"
#else
#define PROPFILE_EXTENSION ".prop"
#define WORDDATA_EXTENSION ".wdata"
#define PRESORTED_EXTENSION ".psort"
#define BTREE_EXTENSION ".btree"
#define ARRAY_EXTENSION ".array"
#define HASHFILE_EXTENSION ".file"
#endif

/* MIN_PROP_COMPRESS_SIZE sets the limit for which properties are compressed 
 * must be compiled with zlib.
 *
 * NEAR WORD feature might benefit from setting these next 2 higher
 */
#define MIN_PROP_COMPRESS_SIZE 100
/* Same for worddata */
#define MIN_WORDDATA_COMPRESS_SIZE 100

/* This is the character used to replace UTF-8 characters that cannot be
 * converted to 8859-1 Latin-1 character
 */
#define ENCODE_ERROR_CHAR ' '

/* Defines the file extension to use on the property file.
*/

#define MAX_SORT_STRING_LEN 100

/* MAX_SORT_STRING_LEN defines the max string length to use
*  for sorting properties.  Should be long enough to sort ALL
*  file paths or URLs.  Useful if using StoreDescription to store
*  a large amount of text.
*/

#define USE_DOCPATH_AS_TITLE 1

/* If USE_DOCPATH_AS_TITLE is defined then documents that do not have
*  a title defined (xml and txt, and HTML documents without a title)
*  will display the document path as the title in results.
*  Documents without a title will sort as a blank title, and not
*  by the document path regardless of this setting.  This is a change
*  from versions previous to 2.2.
*/

#ifdef __VMS
#define USE_TEMPFILE_EXTENSION "_temp"
#else
#define USE_TEMPFILE_EXTENSION ".temp"
#endif

/* If USE_TMPFILE_EXTENSION is defined then swish will append the supplied
*  extension onto the index files during indexing, and when indexing is
*  complete will remove the extension by renaming the files.
*  This has two important uses when an index file already exists (and is in use):
*    1) the old index can be used while indexing is running
*    2) a failure during indexing will not destroy the existing index
*
*   Note: This is used instead of a normal temporary file because possible limitation
*   in renaming across file systems.  Therefore, the temporary index files are
*   stored in the same directory as the final index files.
*/

#define TEMP_FILE_PREFIX "swtmp"

/* TEMP_FILE_PREFIX is prepended to all temporary files.  Makes them
*  easier to find.
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

#define DEFAULT_HTTP_DELAY 5

/* DEFAULT_HTTP_DELAY is the default delay when using swishspider -S http */


#define DATE_FORMAT_STRING "%Y-%m-%d %H:%M:%S %Z"
/* default format string for dates */


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

#define VERBOSE 1

/* You can define VERBOSE to be a number from 0 to 4. 0 is totally
** silent operation.  The default before swish 2.2 was 3
*/

#define _NEAR_WORD  "near"
#define _AND_WORD   "and"
#define _OR_WORD    "or"
#define _NOT_WORD   "not"

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
** attempt to find a <TITLE> tag.  This has no effect when using the libxml2 parser.
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
/* Dec 6, 2001 - Grabbed "letters" from /usr/local/share/aspell/iso8859-1.dat (http://aspell.sf.net) - moseley */
#define WORDCHARS "0123456789abcdefghijklmnopqrstuvwxyz™µ∫¿¡¬√ƒ≈∆«»… ÀÃÕŒœ–—“”‘’÷ÿŸ⁄€‹›ﬁﬂ‡·‚„‰ÂÊÁËÈÍÎÏÌÓÔÒÚÛÙıˆ¯˘˙˚¸˝˛ˇ"

/*
#define WORDCHARS "abcdefghijklmnopqrstuvwxyz¡¬√»˝ ÀÃ–›ﬁÕŒœ“”‘’ÿŸ€ÓË„ıöúùÄﬂÉäåçéè¯ü£‹ûêÌ¿0123456789"
*/

/* WORDCHARS is a string of characters which SWISH permits to
** be in words.  Words are defined by these characters.
**
** Also note that if you specify the backslash character (\) or
** double quote (") you need to type a backslash before them to
** make the compiler understand them.
*/

#define BEGINCHARS "0123456789abcdefghijklmnopqrstuvwxyz™µ∫¿¡¬√ƒ≈∆«»… ÀÃÕŒœ–—“”‘’÷ÿŸ⁄€‹›ﬁﬂ‡·‚„‰ÂÊÁËÈÍÎÏÌÓÔÒÚÛÙıˆ¯˘˙˚¸˝˛ˇ"

/* Of the characters that you decide can go into words, this is
** a list of characters that words can begin with. It should be
** a subset of (or equal to) WORDCHARS.
*/

#define ENDCHARS "0123456789abcdefghijklmnopqrstuvwxyz™µ∫¿¡¬√ƒ≈∆«»… ÀÃÕŒœ–—“”‘’÷ÿŸ⁄€‹›ﬁﬂ‡·‚„‰ÂÊÁËÈÍÎÏÌÓÔÒÚÛÙıˆ¯˘˙˚¸˝˛ˇ"

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

// #define BLANK_PROP_VALUE " *BLANK*"

/* This effects how blank properties are stored 
** Normally, blank properties are treated as if they were not even contained int
** the document.  That is:
**           <meta name="author" content="">
** is ignored, and no "author" property is stored for that docment.
** If BLANK_PROP_VALUE is set, then blank properties will be stored
** but using the string provided as the property value.
** If you use a leading space, then these properties will sort
** before other properties (since leading whitespace is removed from
** properties), and after documents that do not include the property
*/

#define RANK_TITLE		7
#define RANK_HEADER		5
#define RANK_META		3
#define RANK_COMMENTS	1
#define RANK_EMPHASIZED 0

/* This symbols affect the weights applied during ranking. Note that they are added
** together and added to a base rank of 1.0 -- thus defining a rank with a value of
** 2.0 really means it is ranked (1.0 + 2.0) times greater than normal. 
** A value of 0.0 applies no additional ranking boost. Note that RANK_COMMENTS only
** applies if you are indexing comment. Be sure you understand how these interact
** in getrank; don't just go changing these values!
*/

#define SWAP_LOC_DEFAULT 0

/* 2001/08 jmruiz -- Default chunk size - Index will work with blocks of files. This number specifies when to coalesce locations to save memory */
#define INDEX_DEFAULT_CHUNK_SIZE 10000

/* 2001/08 jmruiz -- Default optimal zone size for temporal storage of locations */
/* 1<<23  is 8 MB */
#define INDEX_DEFAULT_OPTIMAL_CHUNK_ZONE_SIZE_FOR_LOCATIONS 1<<23

/* 2002/06 Number of swap loc files (-e) */
#define MAX_LOC_SWAP_FILES 377

/* 2001/08 jmruiz -- To avoid emalloc/erealloc in some routines some stack arrays have been added. This is their default size */
#define MAX_STACK_POSITIONS 1024

/* 2001/08 jmruiz -- Do not change this (it must be a unsigned number) */
/* This is the maximum size of a block of coalesced locations */
#define COALESCE_BUFFER_MAX_SIZE 1<<18  /* (256 KB) */

/* 2003/06 jmruiz -- Snowball's Stemmers activation */
#define SNOWBALL 1

/* 2003/08 jmruiz -- Use cache for stemming */
#define STEMCACHE 1

/* 2001/08 jmruiz -- File System sort flag - 0 means that filenames
** will not be indexed - 1 means that filenames will be indexed */
#define SORT_FILENAMES 0

/* 2001/10 jmruiz -- Added BTREE schema to store words */

//#define USE_BTREE  /* use --enable-incremental at configure time */

/* If USE_BTREE then enable the ARRAY code for the pre-sorted indexes */

#define sw_fopen fopen
#define sw_fclose fclose
#define sw_fwrite fwrite
#define sw_fread fread
#define sw_fputc fputc
#define sw_fgetc fgetc

/* 64 bit LFS support */
#ifdef _LARGEFILE_SOURCE
#define sw_off_t off_t
#define sw_fseek fseeko
#define sw_ftell ftello
#else
#define sw_off_t long
#define sw_fseek fseek
#define sw_ftell ftell
#endif

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
#define NO_SYMBOLIC_FILE_LINKS          /* Win32 has no symbolic links */
#endif

#ifdef __VMS
#define NO_SYMBOLIC_FILE_LINKS          /* VMS has no symbolic links */
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
#define F_READ_BINARY           "rb"
#define F_WRITE_BINARY          "wb"
#define F_READWRITE_BINARY      "rb+"

#define F_READ_TEXT             "r"
#define F_WRITE_TEXT            "w"
#define F_READWRITE_TEXT        "r+"



/* #define NEXTSTEP */

/* You may need to define this if compiling on a NeXTstep machine.
*/

/* --- END PORTING-RELATED SYMBOLS --- */

