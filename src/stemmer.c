/*

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
    along with Swish-e; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    
    See the COPYING file that accompanies the Swish-e distribution for details
    of the GNU GPL and the special exception available for linking against
    the Swish-e library.

** Tue May 10 08:19:25 CDT 2005
** removed original Porter stemmer code, Stem() function, and all related copyrights
** everything else is GPL by wmoseley
****************************************************************************************


** NOTE: This implementation was originally part of the WAIS system
** The main function, Stem(), was incorporated into Swish-E 1.1
** to provide a stemming function.
**   11/24/98 Mark Gaulin
**
** Stem returns original word if words stems to empty string
** Bill Moseley 10/11/99
**
** Repeats stemming until word will stem no more
** Bill Moseley 10/17/99
**
** function: EndsWithCVC patched a bug. see below.  Moseley 10/19/99
**
** Added word length arg to ReplaceEnd and Stem to avoid strcat overflow
** 11/17/99 - SRE
**
** fixed int cast, missing return value, braces around initializations: problems pointed out by "gcc -Wall"
** SRE 2/22/00
**
** Jose Ruiz 18/10/00
** Remove static word from end var and make the code thread safe
**
** Bill Moseley 20/05/01
** Rewrote to simplify.  No more need to repeat stem (expandstar gone from search.c)
** got rid of most of the reallocation of memory.
*
*
* Dec 11, 2003 - refactored ;)
*
* Now all stemmers are accessed the same way.  get_fuzzy_mode/set_fuzzy_mode both
* create a FUZZY_OBJECT for the selected fuzzy mode.  This action also calls the init
* function for the stemmer (only for snowball at this time).
* fuzzy_convert() is passed the object and the word to stem and a FUZZY_WORD is returned.
* The FUZZY_WORD is a structure and will always contain a word which may be the unchanged
* input word or a stemmed word or words.  The FUZZY_WORD can be checked to see if
* the status of the stemming (in some cases anyway).  The idea is you can use the
* returned word list regardless of if it stemmed or not.
* After stemming call fuzzy_free_word() to free the memory use in the conversion.
* When done with the stemmer call free_fuzzy_mode() to free up the stemmer.
* Look at index.c for an example.
*/


#include "swish.h"
#include "error.h"
#include "soundex.h"
#include "swstring.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "headers.h"
#include "stemmer.h"
#include "mem.h"
#include "search.h"  /* for stemming via a result */

#define FALSE	0
#define TRUE	1

#include "double_metaphone.h"

/* Includes for using SNOWBALL stemmer */
#include "snowball/stem_es.h"
#include "snowball/stem_fr.h"
#include "snowball/stem_it.h"
#include "snowball/stem_pt.h"
#include "snowball/stem_de.h"
#include "snowball/stem_nl.h"
#include "snowball/stem_en1.h"
#include "snowball/stem_en2.h"
#include "snowball/stem_no.h"
#include "snowball/stem_se.h"
#include "snowball/stem_dk.h"
#include "snowball/stem_ru.h"
#include "snowball/stem_fi.h"
#include "snowball/stem_ro.h"
#include "snowball/stem_hu.h"
#include "snowball/api.h"

static FUZZY_WORD *no_stem( FUZZY_OBJECT *fi, const char *inword);
static FUZZY_WORD *Stem_snowball( FUZZY_OBJECT *fi, const char *inword);
static FUZZY_WORD *double_metaphone( FUZZY_OBJECT *fi, const char *inword);


static FUZZY_OPTS fuzzy_opts[] = {

    /* fuzzy_mode               *name               *routine  *init *free *lang_stem */
    { FUZZY_NONE,               "None",             no_stem, NULL, NULL, NULL },
    { FUZZY_SOUNDEX,            "Soundex",          soundex, NULL, NULL, NULL },
    { FUZZY_METAPHONE,          "Metaphone",        double_metaphone, NULL, NULL, NULL },
    { FUZZY_DOUBLE_METAPHONE,   "DoubleMetaphone",  double_metaphone, NULL, NULL, NULL },
    { FUZZY_STEMMING_ES,        "Stemming_es",      Stem_snowball, spanish_ISO_8859_1_create_env, spanish_ISO_8859_1_close_env, spanish_ISO_8859_1_stem },
    { FUZZY_STEMMING_FR,        "Stemming_fr",      Stem_snowball, french_ISO_8859_1_create_env, french_ISO_8859_1_close_env, french_ISO_8859_1_stem },
    { FUZZY_STEMMING_IT,        "Stemming_it",      Stem_snowball, italian_ISO_8859_1_create_env, italian_ISO_8859_1_close_env, italian_ISO_8859_1_stem },
    { FUZZY_STEMMING_PT,        "Stemming_pt",      Stem_snowball, portuguese_ISO_8859_1_create_env, portuguese_ISO_8859_1_close_env, portuguese_ISO_8859_1_stem },
    { FUZZY_STEMMING_DE,        "Stemming_de",      Stem_snowball, german_ISO_8859_1_create_env, german_ISO_8859_1_close_env, german_ISO_8859_1_stem },
    { FUZZY_STEMMING_NL,        "Stemming_nl",      Stem_snowball, dutch_ISO_8859_1_create_env, dutch_ISO_8859_1_close_env, dutch_ISO_8859_1_stem },
    { FUZZY_STEMMING_EN1,       "Stemming_en1",     Stem_snowball, porter_ISO_8859_1_create_env, porter_ISO_8859_1_close_env, porter_ISO_8859_1_stem },
    { FUZZY_STEMMING_EN2,       "Stemming_en2",     Stem_snowball, english_ISO_8859_1_create_env, english_ISO_8859_1_close_env, english_ISO_8859_1_stem },
    { FUZZY_STEMMING_NO,        "Stemming_no",      Stem_snowball, norwegian_ISO_8859_1_create_env, norwegian_ISO_8859_1_close_env, norwegian_ISO_8859_1_stem },
    { FUZZY_STEMMING_SE,        "Stemming_se",      Stem_snowball, swedish_ISO_8859_1_create_env, swedish_ISO_8859_1_close_env, swedish_ISO_8859_1_stem },
    { FUZZY_STEMMING_DK,        "Stemming_dk",      Stem_snowball, danish_ISO_8859_1_create_env, danish_ISO_8859_1_close_env, danish_ISO_8859_1_stem },
    { FUZZY_STEMMING_RU,        "Stemming_ru",		Stem_snowball, russian_KOI8_R_create_env, russian_KOI8_R_close_env, russian_KOI8_R_stem },
    { FUZZY_STEMMING_FI,        "Stemming_fi",      Stem_snowball, finnish_ISO_8859_1_create_env, finnish_ISO_8859_1_close_env, finnish_ISO_8859_1_stem },
    { FUZZY_STEMMING_RO,        "Stemming_ro",		Stem_snowball, romanian_ISO_8859_2_create_env, romanian_ISO_8859_2_close_env, romanian_ISO_8859_2_stem },
    { FUZZY_STEMMING_HU,        "Stemming_hu",		Stem_snowball, hungarian_ISO_8859_1_create_env, hungarian_ISO_8859_1_close_env, hungarian_ISO_8859_1_stem },
    /* these next two are deprecated and are identical to Stemming_en1 */
    { FUZZY_STEMMING_EN1,       "Stemming_en",      Stem_snowball, porter_ISO_8859_1_create_env, porter_ISO_8859_1_close_env, porter_ISO_8859_1_stem },
    { FUZZY_STEMMING_EN1,       "Stem",             Stem_snowball, porter_ISO_8859_1_create_env, porter_ISO_8859_1_close_env, porter_ISO_8859_1_stem }


};


/*
 * This function calls the individual stemmer based on the stemmer selected in the fuzzy_index
 * Returns pointer to a FUZZY_WORD which contains a char** of stemmed words -- typically the
 * list is a single word followed by a null pointer (null terminated list).  Double-metaphone
 * may return two strings.  The string is initially set to the incoming word so can normally
 * just use the list without checking for errors (if you don't care if the word stems or not).
 * Otherwise, need to check fw->error to see if there was a problem in stemming.
 */


FUZZY_WORD *fuzzy_convert( FUZZY_OBJECT *fi, const char *inword )
{
    if ( !fi )
        progerr("called fuzzy_convert with NULL FUZZY_OBJECT");

    return fi->stemmer->routine( fi, inword );  /* call the specific stemmer */
}

/* This is a dummy stemmer that returns nothing and just eats cpu */

static FUZZY_WORD *no_stem( FUZZY_OBJECT *fi, const char *inword)
{
    return create_fuzzy_word( inword, 1 );
}



/* Frees up a fuzzy data structure for a given word.  Frees memory of strings, if needed */

void fuzzy_free_word( FUZZY_WORD *fw )
{
    if ( !fw )
        progerr("called fuzzy_free_data with null value");

    if ( fw->free_strings )
    {
        char **word = fw->word_list;
        while ( *word )
        {
            efree( *word );
            word++;
        }
    }
    efree( fw );
}

/* 
 * creates a FUZZY_WORD structure for "word_count" words + a null at the end
 * called by the individual stemming routines.
 */

FUZZY_WORD *create_fuzzy_word( const char *input_word, int word_count )
{
    size_t bytes;
    FUZZY_WORD *fw;

    if ( word_count < 1 )
        word_count = 1;

    bytes = sizeof(FUZZY_WORD) + ( word_count * sizeof(char *) );

    fw = (FUZZY_WORD *)emalloc( bytes );
    memset( fw, 0, bytes );

    fw->error = STEM_OK;                    /* default to OK */
    fw->orig_word = input_word;             /* original string */
    fw->string_list[0] = (char *)input_word;/* so we have an output word */
    fw->list_size = 1;                      /* count of words in list */
    fw->word_list = &fw->string_list[0];    /* so we have a **char */
    return fw;
}


void dump_fuzzy_list( void )
{
    int i;

    printf("Options available for FuzzyIndexingMode:\n");

    for (i = 0; i < (int)(sizeof(fuzzy_opts) / sizeof(fuzzy_opts[0])); i++)
        printf("    %s\n", fuzzy_opts[i].name );
}


/*
 * sets the fuzzy indexing mode once the option has been selected.
 */


FUZZY_OBJECT *create_fuzzy_struct( FUZZY_OBJECT *fi, FUZZY_OPTS *fuzzy_opts )
{
    FUZZY_OBJECT *f_new = (FUZZY_OBJECT *)emalloc( sizeof( FUZZY_OBJECT) ); 

    free_fuzzy_mode( fi );  /* tidy up previous mode, if one */

    f_new->stemmer = fuzzy_opts;  /* save a reference to the stemmer data */

    /* Call the init function if there is one */
    if ( fuzzy_opts->init )
        f_new->snowball_options = fuzzy_opts->init();      /* initialize the stemmer */

    return f_new;
}

/*
 * Free a stemmer object -- calling the free() function if one is defined.
 */

void free_fuzzy_mode( FUZZY_OBJECT *fi )
{
    if ( !fi )
        return;

    if ( fi->stemmer->stemmer_free )
        fi->stemmer->stemmer_free( fi->snowball_options );

    efree( fi );
}



/*
 * Selects the fuzzy mode by passing in a string describing the fuzzy mode.
 * Returns a pointer to a structure, or null if can't find a valid stemmer.
 */


FUZZY_OBJECT *set_fuzzy_mode(FUZZY_OBJECT *fi, char *param )
{
    int     i;

    for (i = 0; i < (int)(sizeof(fuzzy_opts) / sizeof(fuzzy_opts[0])); i++)
        if ( 0 == strcasecmp(fuzzy_opts[i].name, param ) )
        {
            if ( fuzzy_opts[i].name == "Stem" || fuzzy_opts[i].name == "Stemming_en" )
            {
                fprintf(stderr, "*************\n");
                fprintf(stderr, "  Old stemmer '%s' is no longer supported -- using Stemming_en1 instead.\n", fuzzy_opts[i].name);
                fprintf(stderr, "  Please update your config file.\n*************\n");
            }
            
            return create_fuzzy_struct( fi, &fuzzy_opts[i] );
        }

    return NULL;
}

/* Sets FUZZY_OBJECT struc (fi) based on the integer mode passed in */
/* Used to set the fuzzy data structure based on the value stored in the index header */
/* This one will fail badly (abort) since it's getting the value from the index file */


FUZZY_OBJECT *get_fuzzy_mode( FUZZY_OBJECT *fi, int fuzzy )
{
    int i;

    for (i = 0; i < (int)(sizeof(fuzzy_opts) / sizeof(fuzzy_opts[0])); i++)
        if ( (FuzzyIndexType)fuzzy == fuzzy_opts[i].fuzzy_mode )
        {
            return create_fuzzy_struct( fi, &fuzzy_opts[i] );
        }

    progerr("Invalid FuzzyIndexingMode '%d' in index file", fuzzy);
    return NULL;
}



/*
 * Converts a fuzzy mode to a string by looking up the string in the table.
 */

const char *fuzzy_string( FUZZY_OBJECT *fi )
{
    if ( !fi )
        return "Unknown FuzzyIndexingMode";

    return fi->stemmer->name;
}

FuzzyIndexType fuzzy_mode_value( FUZZY_OBJECT *fi )
{
    if ( !fi )
        return FUZZY_NONE;
    return fi->stemmer->fuzzy_mode;
}


int stemmer_applied( FUZZY_OBJECT *fi )
{
     return (FUZZY_NONE != fi->stemmer->fuzzy_mode ) ? 1 : 0;
}



/* 06/2003 Jose Ruiz - Interface to snowball's spanish stemmer */

static FUZZY_WORD *Stem_snowball( FUZZY_OBJECT *fi, const char *inword)
{
    char *out_word;
    struct SN_env *snowball = fi->snowball_options;
    FUZZY_WORD *fw = create_fuzzy_word( inword, 1 ); /* create place to store stemmed word */

    SN_set_current(snowball,strlen(inword),(const symbol *)inword); /* Set Word to Stem */

    fi->stemmer->lang_stem(snowball); /* Stem the word */


    if ( 0 == snowball->l )
    {
        fw->error = STEM_TO_NOTHING;
        return fw;
    }

    fw->free_strings = 1; /* flag that malloc is used */

    out_word = emalloc(snowball->l + 1);
    memcpy(out_word, snowball->p, snowball->l);
    out_word[snowball->l] = '\0';
    fw->string_list[0] = out_word;

    return fw;
}

static FUZZY_WORD *double_metaphone( FUZZY_OBJECT *fi, const char *inword)
{
    FUZZY_WORD *fw = create_fuzzy_word( inword, 2 ); /* create place to store stemmed word */
    char *codes[2];

    DoubleMetaphone( inword, codes );

    if ( !(*codes[0]) )  /* was there at least one conversion? */
    {
        efree( codes[0] );
        efree( codes[1] );
        return fw;
    }

    fw->free_strings = 1;
    fw->string_list[0] = codes[0];


    /* Is double metaphone enabled? */

    if ( FUZZY_DOUBLE_METAPHONE != fi->stemmer->fuzzy_mode )
        return fw;

    /* Is there a second metaphone that is different from the first? */

    if ( *codes[1] && strcmp(codes[0], codes[1]) )
    {
        fw->list_size++;
        fw->string_list[1] = codes[1];
    }
    else
    {
        efree( codes[1] );
    }
    return fw;
}

/*************************************************************************
 *
 * These routines are the API interface for stemming.
 *
 *************************************************************************/



/*************************************************************************
* SwishStemWord -- utility function to stem a word
*
* This stores the stemmed word locally so it can be freed
# *Depreciated* because this only calls the original stemmer.
*
**************************************************************************/

char *SwishStemWord( SWISH *sw, char *word )
{
    FUZZY_OBJECT *fo = NULL;
    FUZZY_WORD *fw = NULL;

    if ( sw->stemmed_word )
    {
        efree( sw->stemmed_word );
        sw->stemmed_word = NULL;
    }

    fo = set_fuzzy_mode( fo, "Stem" );
    if ( !fo )
        return sw->stemmed_word;

    fw = fuzzy_convert( fo, word );
    sw->stemmed_word = estrdup( fw->string_list[0] );
    fuzzy_free_word( fw );

    free_fuzzy_mode( fo );
    return sw->stemmed_word;

}

/************************************************************************
* SwishFuzzyWord -- utility function to stem a word based on the current result
* (that is, the stemming mode of the current index.
*
* This is really a method call on a RESULT and returns a new object
* Currently this requires calling SwishFreeFuzzyWord to free memory.
*
**************************************************************************/

FUZZY_WORD *SwishFuzzyWord( RESULT *r, char *word )
{
    if ( !r )
        return NULL;

    return fuzzy_convert( r->db_results->indexf->header.fuzzy_data, word );
}

const char *SwishFuzzyMode( RESULT *r )
{
    return fuzzy_string(  r->db_results->indexf->header.fuzzy_data );
}


/*
 * These are accessors for the FUZZY_WORD
 */

/* Returns the list of words */

const char **SwishFuzzyWordList( FUZZY_WORD *fw )
{
    if ( !fw )
        return NULL;

    return (const char **)fw->word_list;
}

/* Returns the number of words in the list */

int SwishFuzzyWordCount( FUZZY_WORD *fw )
{
    if ( !fw )
        return 0;

    return fw->list_size;
}


/* Returns the integer value of the error */

int SwishFuzzyWordError( FUZZY_WORD *fw )
{
    if ( !fw )
        return -1;

    return (int)fw->error;
}

/* Frees the word */

void SwishFuzzyWordFree( FUZZY_WORD *fw )
{
    fuzzy_free_word( fw );
}



/*******************************************************************************

Stemmer access for SWISH object. idea is to be able to stem a word if an index
is named specifically, rather than waiting to have a RESULT object.

this is for API, to allow access to stemming functions without searching.

karman - Wed Oct 27 10:51:03 CDT 2004

*********************************************************************************/


FUZZY_WORD *SwishFuzzify( SWISH *sw, const char *index_name, char *word )
{
    
/* create FUZZY object like SwishFuzzyWord does, but with named index */

    IndexFILE *indexf = indexf_by_name( sw, index_name );
    
    if ( !sw )
      progerr("SwishFuzzify requires a valid swish handle");

    if ( !indexf ) {
      set_progerr( HEADER_READ_ERROR, sw, "Index file '%s' is not an active index file", index_name );
      return( NULL );
    }
    
    if ( !word )
        return NULL;
	

    return fuzzy_convert( indexf->header.fuzzy_data, word );
}


/* end SWISH stemmer function *************************************************/
