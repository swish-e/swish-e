/*
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

/* WIDE AREA INFORMATION SERVER SOFTWARE:
   No guarantees or restrictions.  See the readme file for the full standard
   disclaimer.

   francois@welchgate.welch.jhu.edu
*/

/* Copyright (c) CNIDR (see ../COPYRIGHT) */


/* 
 * stems a word.
 * 
 */

/* the main functions are:
 *   stemmer
 *
 */

#include "swish.h"
#include "error.h"
#include "soundex.h"
#include "swstring.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>


#include "stemmer.h"
#include "mem.h"
#include "search.h"  /* for stemming via a result */

#define FALSE	0
#define TRUE	1

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
#include "snowball/api.h"

static FUZZY_WORD *no_stem( FUZZY_OBJECT *fi, const char *inword);
static FUZZY_WORD *Stem( FUZZY_OBJECT *fi, const char *inword);
static FUZZY_WORD *Stem_snowball( FUZZY_OBJECT *fi, const char *inword);
static FUZZY_WORD *double_metaphone( FUZZY_OBJECT *fi, const char *inword);





static FUZZY_OPTS fuzzy_opts[] = {

    /* fuzzy_mode               *name               *routine  *init *free *lang_stem */

    { FUZZY_NONE,               "None",             no_stem, NULL, NULL, NULL },
    { FUZZY_STEMMING_EN,        "Stemming_en",      Stem, NULL, NULL, NULL },
    { FUZZY_STEMMING_EN,        "Stem",             Stem, NULL, NULL, NULL },
    { FUZZY_SOUNDEX,            "Soundex",          soundex, NULL, NULL, NULL },
    { FUZZY_METAPHONE,          "Metaphone",        double_metaphone, NULL, NULL, NULL },
    { FUZZY_DOUBLE_METAPHONE,   "DoubleMetaphone",  double_metaphone, NULL, NULL, NULL },
    { FUZZY_STEMMING_ES,        "Stemming_es",      Stem_snowball, spanish_create_env, spanish_close_env, spanish_stem },
    { FUZZY_STEMMING_FR,        "Stemming_fr",      Stem_snowball, french_create_env, french_close_env, french_stem },
    { FUZZY_STEMMING_IT,        "Stemming_it",      Stem_snowball, italian_create_env, italian_close_env, italian_stem },
    { FUZZY_STEMMING_PT,        "Stemming_pt",      Stem_snowball, portuguese_create_env, portuguese_close_env, portuguese_stem },
    { FUZZY_STEMMING_DE,        "Stemming_de",      Stem_snowball, german_create_env, german_close_env, german_stem },
    { FUZZY_STEMMING_NL,        "Stemming_nl",      Stem_snowball, dutch_create_env, dutch_close_env, dutch_stem },
    { FUZZY_STEMMING_EN1,       "Stemming_en1",     Stem_snowball, porter_create_env, porter_close_env, porter_stem },
    { FUZZY_STEMMING_EN2,       "Stemming_en2",     Stem_snowball, english_create_env, english_close_env, english_stem },
    { FUZZY_STEMMING_NO,        "Stemming_no",      Stem_snowball, norwegian_create_env, norwegian_close_env, norwegian_stem },
    { FUZZY_STEMMING_SE,        "Stemming_se",      Stem_snowball, swedish_create_env, swedish_close_env, swedish_stem },
    { FUZZY_STEMMING_DK,        "Stemming_dk",      Stem_snowball, danish_create_env, danish_close_env, danish_stem },
    { FUZZY_STEMMING_RU,        "Stemming_ru",      Stem_snowball, russian_create_env, russian_close_env, russian_stem },
    { FUZZY_STEMMING_FI,        "Stemming_fi",      Stem_snowball, finnish_create_env, finnish_close_env, finnish_stem }
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

    SN_set_current(snowball,strlen(inword),inword); /* Set Word to Stem */

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



/* Here's the old porter stemmer */


/*******************************   stem.c   ***********************************

   Purpose:    Implementation of the Porter stemming algorithm documented 
               in: Porter, M.F., "An Algorithm For Suffix Stripping," 
               Program 14 (3), July 1980, pp. 130-137.

   Provenance: Written by B. Frakes and C. Cox, 1986.
               Changed by C. Fox, 1990.
                  - made measure function a DFA
                  - restructured structs
                  - renamed functions and variables
                  - restricted function and variable scopes
               Changed by C. Fox, July, 1991.
                  - added ANSI C declarations 
                  - branch tested to 90% coverage

   Notes:      This code will make little sense without the the Porter
               article.  The stemming function converts its input to
               lower case.
**/

/************************   Standard Include Files   *************************/

/*****************************************************************************/
/*****************   Private Defines and Data Structures   *******************/

#define IsVowel(c)        ('a'==(c)||'e'==(c)||'i'==(c)||'o'==(c)||'u'==(c))

typedef struct
{
    int     id;                 /* returned if rule fired */
    char   *old_end;            /* suffix replaced */
    char   *new_end;            /* suffix replacement */
    int     old_offset;         /* from end of word to start of suffix */
    int     new_offset;         /* from beginning to end of new suffix */
    int     min_root_size;      /* min root word size for replacement */
    int     (*condition) ();    /* the replacement test function */
}
RuleList;

#define LAMBDA ""               /* the constant empty string */

/*****************************************************************************/
/********************   Private Function Declarations   **********************/

#ifdef __STDC__

int     WordSize(char *word);
int     ContainsVowel(char *word);
int     EndsWithCVC(char *word);
int     AddAnE(char *word);
int     RemoveAnE(char *word);
int     ReplaceEnd(char *word, RuleList * rule);

#else

int     WordSize( /* word */ );
int     ContainsVowel( /* word */ );
int     EndsWithCVC( /* word */ );
int     AddAnE( /* word */ );
int     RemoveAnE( /* word */ );
int     ReplaceEnd( /* word, rule */ );

#endif

/******************************************************************************/
/*****************   Initialized Private Data Structures   ********************/
/* 2/22/00 - added braces around each line */

static RuleList step1a_rules[] = {
    {101, "sses", "ss", 3, 1, -1, NULL,},
    {102, "ies", "i", 2, 0, -1, NULL,},
    {103, "ss", "ss", 1, 1, -1, NULL,},
    {104, "s", LAMBDA, 0, -1, -1, NULL,},
    {000, NULL, NULL, 0, 0, 0, NULL,}
};

static RuleList step1b_rules[] = {
    {105, "eed", "ee", 2, 1, 0, NULL,},
    {106, "ed", LAMBDA, 1, -1, -1, ContainsVowel,},
    {107, "ing", LAMBDA, 2, -1, -1, ContainsVowel,},
    {000, NULL, NULL, 0, 0, 0, NULL,}
};

static RuleList step1b1_rules[] = {
    {108, "at", "ate", 1, 2, -1, NULL,},
    {109, "bl", "ble", 1, 2, -1, NULL,},
    {110, "iz", "ize", 1, 2, -1, NULL,},
    {111, "bb", "b", 1, 0, -1, NULL,},
    {112, "dd", "d", 1, 0, -1, NULL,},
    {113, "ff", "f", 1, 0, -1, NULL,},
    {114, "gg", "g", 1, 0, -1, NULL,},
    {115, "mm", "m", 1, 0, -1, NULL,},
    {116, "nn", "n", 1, 0, -1, NULL,},
    {117, "pp", "p", 1, 0, -1, NULL,},
    {118, "rr", "r", 1, 0, -1, NULL,},
    {119, "tt", "t", 1, 0, -1, NULL,},
    {120, "ww", "w", 1, 0, -1, NULL,},
    {121, "xx", "x", 1, 0, -1, NULL,},
    {122, LAMBDA, "e", -1, 0, -1, AddAnE,},
    {000, NULL, NULL, 0, 0, 0, NULL,}
};

static RuleList step1c_rules[] = {
    {123, "y", "i", 0, 0, -1, ContainsVowel,},
    {000, NULL, NULL, 0, 0, 0, NULL,}
};

static RuleList step2_rules[] = {
    {203, "ational", "ate", 6, 2, 0, NULL,},
    {204, "tional", "tion", 5, 3, 0, NULL,},
    {205, "enci", "ence", 3, 3, 0, NULL,},
    {206, "anci", "ance", 3, 3, 0, NULL,},
    {207, "izer", "ize", 3, 2, 0, NULL,},
    {208, "abli", "able", 3, 3, 0, NULL,},
    {209, "alli", "al", 3, 1, 0, NULL,},
    {210, "entli", "ent", 4, 2, 0, NULL,},
    {211, "eli", "e", 2, 0, 0, NULL,},
    {213, "ousli", "ous", 4, 2, 0, NULL,},
    {214, "ization", "ize", 6, 2, 0, NULL,},
    {215, "ation", "ate", 4, 2, 0, NULL,},
    {216, "ator", "ate", 3, 2, 0, NULL,},
    {217, "alism", "al", 4, 1, 0, NULL,},
    {218, "iveness", "ive", 6, 2, 0, NULL,},
    {219, "fulnes", "ful", 5, 2, 0, NULL,},
    {220, "ousness", "ous", 6, 2, 0, NULL,},
    {221, "aliti", "al", 4, 1, 0, NULL,},
    {222, "iviti", "ive", 4, 2, 0, NULL,},
    {223, "biliti", "ble", 5, 2, 0, NULL,},
    {000, NULL, NULL, 0, 0, 0, NULL,}
};

static RuleList step3_rules[] = {
    {301, "icate", "ic", 4, 1, 0, NULL,},
    {302, "ative", LAMBDA, 4, -1, 0, NULL,},
    {303, "alize", "al", 4, 1, 0, NULL,},
    {304, "iciti", "ic", 4, 1, 0, NULL,},
    {305, "ical", "ic", 3, 1, 0, NULL,},
    {308, "ful", LAMBDA, 2, -1, 0, NULL,},
    {309, "ness", LAMBDA, 3, -1, 0, NULL,},
    {000, NULL, NULL, 0, 0, 0, NULL,}
};

static RuleList step4_rules[] = {
    {401, "al", LAMBDA, 1, -1, 1, NULL,},
    {402, "ance", LAMBDA, 3, -1, 1, NULL,},
    {403, "ence", LAMBDA, 3, -1, 1, NULL,},
    {405, "er", LAMBDA, 1, -1, 1, NULL,},
    {406, "ic", LAMBDA, 1, -1, 1, NULL,},
    {407, "able", LAMBDA, 3, -1, 1, NULL,},
    {408, "ible", LAMBDA, 3, -1, 1, NULL,},
    {409, "ant", LAMBDA, 2, -1, 1, NULL,},
    {410, "ement", LAMBDA, 4, -1, 1, NULL,},
    {411, "ment", LAMBDA, 3, -1, 1, NULL,},
    {412, "ent", LAMBDA, 2, -1, 1, NULL,},
    {423, "sion", "s", 3, 0, 1, NULL,},
    {424, "tion", "t", 3, 0, 1, NULL,},
    {415, "ou", LAMBDA, 1, -1, 1, NULL,},
    {416, "ism", LAMBDA, 2, -1, 1, NULL,},
    {417, "ate", LAMBDA, 2, -1, 1, NULL,},
    {418, "iti", LAMBDA, 2, -1, 1, NULL,},
    {419, "ous", LAMBDA, 2, -1, 1, NULL,},
    {420, "ive", LAMBDA, 2, -1, 1, NULL,},
    {421, "ize", LAMBDA, 2, -1, 1, NULL,},
    {000, NULL, NULL, 0, 0, 0, NULL,}
};

static RuleList step5a_rules[] = {
    {501, "e", LAMBDA, 0, -1, 1, NULL,},
    {502, "e", LAMBDA, 0, -1, -1, RemoveAnE,},
    {000, NULL, NULL, 0, 0, 0, NULL,}
};

static RuleList step5b_rules[] = {
    {503, "ll", "l", 1, 0, 1, NULL,},
    {000, NULL, NULL, 0, 0, 0, NULL,}
};


static RuleList *all_steps[] = {
    step1a_rules,
    step1b_rules,
    /* step1b1_rules, -- conditionaly called below */
    step1c_rules,
    step2_rules,
    step3_rules,
    step4_rules,
    step5a_rules,
    step5b_rules,
};




/*****************************************************************************/
/********************   Private Function Declarations   **********************/

/*FN***************************************************************************

       WordSize( word )

   Returns: int -- a weird count of word size in adjusted syllables

   Purpose: Count syllables in a special way:  count the number 
            vowel-consonant pairs in a word, disregarding initial 
            consonants and final vowels.  The letter "y" counts as a
            consonant at the beginning of a word and when it has a vowel
            in front of it; otherwise (when it follows a consonant) it
            is treated as a vowel.  For example, the WordSize of "cat" 
            is 1, of "any" is 1, of "amount" is 2, of "anything" is 3.

   Plan:    Run a DFA to compute the word size

   Notes:   The easiest and fastest way to compute this funny measure is
            with a finite state machine.  The initial state 0 checks
            the first letter.  If it is a vowel, then the machine changes
            to state 1, which is the "last letter was a vowel" state.
            If the first letter is a consonant or y, then it changes
            to state 2, the "last letter was a consonant state".  In
            state 1, a y is treated as a consonant (since it follows
            a vowel), but in state 2, y is treated as a vowel (since
            it follows a consonant.  The result counter is incremented
            on the transition from state 1 to state 2, since this
            transition only occurs after a vowel-consonant pair, which
            is what we are counting.
**/

int     WordSize(char *word)
{
    register int result;        /* WordSize of the word */
    register int state;         /* current state in machine */

    result = 0;
    state = 0;

    /* Run a DFA to compute the word size */
    while ( *word )
    {
        switch (state)
        {
        case 0:
            state = (IsVowel(*word)) ? 1 : 2;
            break;
        case 1:
            state = (IsVowel(*word)) ? 1 : 2;
            if (2 == state)
                result++;
            break;
        case 2:
            state = (IsVowel(*word) || ('y' == *word)) ? 1 : 2;
            break;
        }
        word++;
    }

    return (result);

}                               /* WordSize */

/*FN**************************************************************************

       ContainsVowel( word, end )

   Returns: int -- TRUE (1) if the word parameter contains a vowel,
            FALSE (0) otherwise.

   Purpose: Some of the rewrite rules apply only to a root containing
            a vowel, where a vowel is one of "aeiou" or y with a
            consonant in front of it.

   Plan:    Obviously, under the definition of a vowel, a word contains
            a vowel iff either its first letter is one of "aeiou", or
            any of its other letters are "aeiouy".  The plan is to
            test this condition.

   Notes:   None
**/

int     ContainsVowel(char *word)
{

    /* This isn't needed, right? */
    if ( !*word )
        return FALSE;

    return (IsVowel(*word) || (NULL != strpbrk(word + 1, "aeiouy")));


}                               /* ContainsVowel */

/*FN**************************************************************************

       EndsWithCVC( word )

   Returns: int -- TRUE (1) if the current word ends with a
            consonant-vowel-consonant combination, and the second
            consonant is not w, x, or y, FALSE (0) otherwise.

   Purpose: Some of the rewrite rules apply only to a root with
            this characteristic.

   Plan:    Look at the last three characters.

   Notes:   None
**/

int     EndsWithCVC(char *word)
{
    int     length;             /* for finding the last three characters */

    if ((length = (int) strlen(word)) < 3) /* This was < 2 in original - Moseley 10/19/99 */
        return (FALSE);


    return (
        (NULL == strchr("aeiouwxy", (int)word[--length])) /* consonant */
        && (NULL != strchr("aeiouy", (int)word[--length])) /* vowel */
        && (NULL == strchr("aeiou", (int)word[--length]))
    ); /* consonant */

}                               /* EndsWithCVC */

/*FN**************************************************************************

       AddAnE( word )

   Returns: int -- TRUE (1) if the current word meets special conditions
            for adding an e.

   Purpose: Rule 122 applies only to a root with this characteristic.

   Plan:    Check for size of 1 and a consonant-vowel-consonant ending.

   Notes:   None
**/

int     AddAnE(char *word)
{

    return ((1 == WordSize(word)) && EndsWithCVC(word));

}                               /* AddAnE */

/*FN**************************************************************************

       RemoveAnE( word )

   Returns: int -- TRUE (1) if the current word meets special conditions
            for removing an e.

   Purpose: Rule 502 applies only to a root with this characteristic.

   Plan:    Check for size of 1 and no consonant-vowel-consonant ending.

   Notes:   None
**/

int     RemoveAnE(char *word)
{

    return ((1 == WordSize(word)) && !EndsWithCVC(word));

}                               /* RemoveAnE */

/*FN**************************************************************************

       ReplaceEnd( word, rule, end)

   Returns: int -- the id for the rule fired, 0 is none is fired

   Purpose: Apply a set of rules to replace the suffix of a word

   Plan:    Loop through the rule set until a match meeting all conditions
            is found.  If a rule fires, return its id, otherwise return 0.
            Connditions on the length of the root are checked as part of this
            function's processing because this check is so often made.

   Notes:   This is the main routine driving the stemmer.  It goes through
            a set of suffix replacement rules looking for a match on the
            current suffix.  When it finds one, if the root of the word
            is long enough, and it meets whatever other conditions are
            required, then the suffix is replaced, and the function returns.
**/

int     ReplaceEnd(char *word, RuleList *rule)
{
    char   *ending;             /* set to start of possible stemmed suffix */
    char    tmp_ch;             /* save replaced character when testing */
    char   *end;                /* pointer to last char of string */

    end = word + strlen( word ) - 1;

    while ( rule->id )
    {
        /* point ending to the start of the test sufix */
        ending = end - rule->old_offset;

        /* is word long enough to contain suffix, and does it exist? */
        if ( (word <= ending ) && (0 == strcmp(ending, rule->old_end)) )
        {
            tmp_ch = *ending;  /* in case we change our mind */
            *ending = '\0';

            if ( rule->min_root_size < WordSize(word) )
                if (!rule->condition || (*rule->condition) ( word ))
                {
                    /* replace the ending */
                    if ( (strlen( word ) + rule->new_offset + 1 ) >= MAXWORDLEN )
                        return STEM_WORD_TOO_BIG;
                    strcat( word, rule->new_end );

                    return rule->id;
                }
            *ending = tmp_ch;  /* nope, put it back */
        }

        rule++;
    }

    return STEM_OK;

}                               /* ReplaceEnd */

/*****************************************************************************/
/*********************   Public Function Declarations   **********************/

/*FN***************************************************************************

       Stem( word )

   Returns: a FUZZY_WORD pointer

   Purpose: Stem a word

   Plan:    Part 1: Check to ensure the word is all alphabetic
            Part 2: Run through the Porter algorithm
            Part 3: Return an indication of successful stemming

   Notes:   This function implements the Porter stemming algorithm, with
            a few additions here and there.  See:

               Porter, M.F., "An Algorithm For Suffix Stripping,"
               Program 14 (3), July 1980, pp. 130-137.

            Porter's algorithm is an ad hoc set of rewrite rules with
            various conditions on rule firing.  The terminology of
            "step 1a" and so on, is taken directly from Porter's
            article, which unfortunately gives almost no justification
            for the various steps.  Thus this function more or less
            faithfully refects the opaque presentation in the article.
            Changes from the article amount to a few additions to the
            rewrite rules;  these are marked in the RuleList data
            structures with comments.
**/


static FUZZY_WORD *Stem( FUZZY_OBJECT *fi, const char *inword)
{
    char   *end;                /* pointer to the end of the word */
    char    word[MAXWORDLEN+1];
    int     length;
    int     rule_result;        /* which rule is fired in replacing an end */
    int     i;

    FUZZY_WORD *fw = create_fuzzy_word( inword, 1 );

    /* Make sure the word is not too large from the start. */
    if ( strlen( inword ) >= MAXWORDLEN )
    {
        fw->error = STEM_WORD_TOO_BIG;
        return fw;
    }


    /* make working copy */
    strcpy( word, inword );


    /* Part 1: Check to ensure the word is all alphabetic */
    /* no longer converts to lower case -- word should be lower before calling */

    for ( end = word; *end; end++ )
        if ( !isalpha( (unsigned int) *end ) )
        {
            fw->error = STEM_NOT_ALPHA;
            return fw;
        }



    /*  Part 2: Run through the Porter algorithm */


    for (i = 0; i < (int)(sizeof(all_steps)/sizeof(all_steps[0])); i++)
    {
        rule_result = ReplaceEnd(word, all_steps[i]);

        if ((rule_result == 106) || (rule_result == 107))
            rule_result = ReplaceEnd(word, step1b1_rules);

        if ( rule_result == STEM_WORD_TOO_BIG )
        {
            fw->error = rule_result;
            return fw;
        }
    }



    length = strlen( word );

    /* Stem must be two chars or more in length */
    if ( length <= 1 )
    {
        fw->error = STEM_TO_NOTHING;
        return fw;
    }


    if ( length >= MAXWORDLEN )
    {
        fw->error = STEM_WORD_TOO_BIG;
        return fw;
    }


    fw->free_strings = 1;
    fw->string_list[0] = estrdup( word );

    return fw;
}










