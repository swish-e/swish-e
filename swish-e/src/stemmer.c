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
#include <ctype.h>
#include <string.h>
#include <stdio.h>


#include "stemmer.h"
#include "mem.h"

#define FALSE	0
#define TRUE	1

/* Includes for using SNOWBALL stemmer */
#ifdef SNOWBALL
#include "snowball/stem_es.h"
#include "snowball/stem_fr.h"
#include "snowball/stem_it.h"
#include "snowball/stem_pt.h"
#include "snowball/stem_de.h"
#include "snowball/stem_nl.h"
#include "snowball/stem_en1.h"
#include "snowball/stem_en2.h"
#include "snowball/api.h"
#endif


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

    return 0;

}                               /* ReplaceEnd */

/*****************************************************************************/
/*********************   Public Function Declarations   **********************/

/*FN***************************************************************************

       Stem( word )

   Returns: int -- FALSE (0) if the word contains non-alphabetic characters
            and hence is not stemmed, TRUE (1) otherwise

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

#ifdef SNOWBALL
int     Stem(char **inword, int *lenword, struct SN_env  *snowball, int (*lang_stem)(struct SN_env *))
#else
int     Stem(char **inword, int *lenword)
#endif
{
    char   *end;                /* pointer to the end of the word */
    char    word[MAXWORDLEN+1];
    int     length;
    int     rule_result;        /* which rule is fired in replacing an end */
//    RuleList **rule;
    int     i;
    


    /* Make sure the word is not too large from the start. */
    if ( strlen( *inword ) >= MAXWORDLEN )
        return STEM_WORD_TOO_BIG;


    /* make working copy */
    strcpy( word, *inword );

    

    /* Part 1: Check to ensure the word is all alphabetic */
    /* no longer converts to lower case -- word should be lower before calling */

    for ( end = word; *end; end++ )
        if ( !isalpha( (unsigned int) *end ) )
            return STEM_NOT_ALPHA;



    /*  Part 2: Run through the Porter algorithm */


    for (i = 0; i < sizeof(all_steps)/sizeof(all_steps[0]); i++)
    {
        rule_result = ReplaceEnd(word, all_steps[i]);

        if ((rule_result == 106) || (rule_result == 107))
            rule_result = ReplaceEnd(word, step1b1_rules);

        if ( rule_result == STEM_WORD_TOO_BIG )
            return rule_result;
            
    }



    length = strlen( word );

    /* Stem must be two chars or more in length */
    if ( length <= 1 )
        return STEM_TO_NOTHING;

    /* reallocate memory if need more room */

    if ( length >= *lenword )
    {
        efree( *inword );

        *lenword = length;
        *inword = emalloc( *lenword + 1 );
    }

    strcpy( *inword, word );
    return STEM_OK;
}        













typedef struct
{
    FuzzyIndexType  fuzzy_mode;
    char            *name;
#ifdef SNOWBALL
    int             (*routine) (char **, int *, struct SN_env  *, int (*lang_stem)(struct SN_env *));
    struct SN_env  *(*init) (void);
    void           (*free) (struct SN_env *);
    int            (*lang_stem)(struct SN_env *);
#else
    int             (*routine) (char **, int *);
    void           *dummy1;
    void           *dummy2;
    int            *dummy3;
#endif
}
FUZZY_OPTS;

static FUZZY_OPTS fuzzy_opts[] = {

    { FUZZY_NONE, "None", NULL, NULL, NULL, NULL },
    { FUZZY_STEMMING_EN, "Stemming_en", Stem, NULL, NULL, NULL },
    { FUZZY_STEMMING_EN, "Stem", Stem, NULL, NULL, NULL },
    { FUZZY_SOUNDEX, "Soundex", NULL, NULL, NULL, NULL },
    { FUZZY_METAPHONE, "Metaphone", NULL, NULL, NULL, NULL },
    { FUZZY_DOUBLE_METAPHONE, "DoubleMetaphone", NULL, NULL, NULL, NULL }
#ifdef SNOWBALL
    ,{ FUZZY_STEMMING_ES, "Stemming_es", Stem_snowball, spanish_create_env, spanish_close_env, spanish_stem },
    { FUZZY_STEMMING_FR, "Stemming_fr", Stem_snowball, french_create_env, french_close_env, french_stem },
    { FUZZY_STEMMING_IT, "Stemming_it", Stem_snowball, italian_create_env, italian_close_env, italian_stem },
    { FUZZY_STEMMING_PT, "Stemming_pt", Stem_snowball, portuguese_create_env, portuguese_close_env, portuguese_stem },
    { FUZZY_STEMMING_DE, "Stemming_de", Stem_snowball, german_create_env, german_close_env, german_stem },
    { FUZZY_STEMMING_NL, "Stemming_nl", Stem_snowball, dutch_create_env, dutch_close_env, dutch_stem },
    { FUZZY_STEMMING_EN1, "Stemming_en1", Stem_snowball, porter_create_env, porter_close_env, porter_stem },
    { FUZZY_STEMMING_EN2, "Stemming_en2", Stem_snowball, english_create_env, english_close_env, english_stem }
#endif
};

void set_fuzzy_mode( FUZZY_INDEX *fi, char *param )
{
    int     i;

    for (i = 0; i < sizeof(fuzzy_opts) / sizeof(fuzzy_opts[0]); i++)
        if ( 0 == strcasecmp(fuzzy_opts[i].name, param ) )
        {
            fi->fuzzy_mode = fuzzy_opts[i].fuzzy_mode;
            fi->fuzzy_routine = fuzzy_opts[i].routine;
#ifdef SNOWBALL
            if(fuzzy_opts[i].lang_stem)
                fi->lang_stem = fuzzy_opts[i].lang_stem;

            if(fuzzy_opts[i].init)
                fi->snowball = fuzzy_opts[i].init();
#endif
            return;
        }

    fi->fuzzy_mode = FUZZY_NONE;
    fi->fuzzy_routine = NULL;
#ifdef SNOWBALL
    fi->snowball = NULL;
#endif

    progerr("Invalid FuzzyIndexingMode '%s' in configuation file", param);
}

void get_fuzzy_mode( FUZZY_INDEX *fi, int fuzzy )
{
    int     i;

    for (i = 0; i < sizeof(fuzzy_opts) / sizeof(fuzzy_opts[0]); i++)
        if ( fuzzy == fuzzy_opts[i].fuzzy_mode ) 
        {
            fi->fuzzy_mode = fuzzy_opts[i].fuzzy_mode;
            fi->fuzzy_routine = fuzzy_opts[i].routine;
#ifdef SNOWBALL
            if(fuzzy_opts[i].lang_stem)
                fi->lang_stem = fuzzy_opts[i].lang_stem;

            if(fuzzy_opts[i].init)
                fi->snowball = fuzzy_opts[i].init();
#endif
            return;
        }

    fi->fuzzy_mode = FUZZY_NONE;
    fi->fuzzy_routine = NULL;
#ifdef SNOWBALL
    fi->snowball = NULL;
#endif

    progerr("Invalid FuzzyIndexingMode '%d' in index file", fuzzy);
}

void free_fuzzy_mode( FUZZY_INDEX *fi )
{
    int     i;

    for (i = 0; i < sizeof(fuzzy_opts) / sizeof(fuzzy_opts[0]); i++)
        if ( fi->fuzzy_mode == fuzzy_opts[i].fuzzy_mode )
        {
            fi->fuzzy_mode = FUZZY_NONE;
            fi->fuzzy_routine = NULL;
#ifdef SNOWBALL
            if(fuzzy_opts[i].free)
                fuzzy_opts[i].free(fi->snowball);

            fi->lang_stem = NULL;
            fi->snowball = NULL;
#endif
            return;
        }

    fi->fuzzy_mode = FUZZY_NONE;
    fi->fuzzy_routine = NULL;
#ifdef SNOWBALL
    fi->lang_stem = NULL;
    fi->snowball = NULL;
#endif
}

char *fuzzy_mode_to_string( FuzzyIndexType mode )
{
    int     i;
    for (i = 0; i < sizeof(fuzzy_opts) / sizeof(fuzzy_opts[0]); i++)
        if ( mode == fuzzy_opts[i].fuzzy_mode )
            return fuzzy_opts[i].name;

    return "Unknown FuzzyIndexingMode";
}

int stemmer_applied(INDEXDATAHEADER *header)
{
     return (FUZZY_STEMMING_EN == header->fuzzy_data.fuzzy_mode
#ifdef SNOWBALL
          || FUZZY_STEMMING_ES == header->fuzzy_data.fuzzy_mode ||
             FUZZY_STEMMING_FR == header->fuzzy_data.fuzzy_mode ||
             FUZZY_STEMMING_PT == header->fuzzy_data.fuzzy_mode ||
             FUZZY_STEMMING_IT == header->fuzzy_data.fuzzy_mode ||
             FUZZY_STEMMING_DE == header->fuzzy_data.fuzzy_mode ||
             FUZZY_STEMMING_NL == header->fuzzy_data.fuzzy_mode ||
             FUZZY_STEMMING_EN1 == header->fuzzy_data.fuzzy_mode ||
             FUZZY_STEMMING_EN2 == header->fuzzy_data.fuzzy_mode
#endif
                               ) ? 1 : 0;
}

#ifdef SNOWBALL
/* 06/2003 Jose Ruiz - Interface to snowball's stemmer */
int     Stem_snowball(char **inword, int *lenword, struct SN_env *snowball, int (*lang_stem)(struct SN_env *))
{
    int new_lenword;

    SN_set_current(snowball,strlen(*inword),*inword); /* Set Word to Stem */
    lang_stem(snowball);

    if((*lenword) < snowball->l)
    {
        efree(*inword);
        *inword = emalloc(snowball->l + 1);
        *lenword = snowball->l;
    }
    memcpy(*inword, snowball->p, snowball->l);
    (*inword)[snowball->l] = '\0';
}

#endif
