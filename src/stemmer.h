/*
** stemmer.h

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


*/

#ifndef STEMMER_H
#define STEMMER_H 1

#include "swishtypes.h"
#include "snowball/api.h"  /* For snoball's SN_env */

/* 
 * Warning: 
 * Don't change the order of these as it will break existing indexes.
 * The index stores this enum number.
 */

typedef enum {
    FUZZY_NONE = 0,
    FUZZY_STEMMING_EN,
    FUZZY_SOUNDEX,
    FUZZY_METAPHONE,
    FUZZY_DOUBLE_METAPHONE,
    FUZZY_STEMMING_ES,
    FUZZY_STEMMING_FR,
    FUZZY_STEMMING_IT,
    FUZZY_STEMMING_PT,
    FUZZY_STEMMING_DE,
    FUZZY_STEMMING_NL,
    FUZZY_STEMMING_EN1,
    FUZZY_STEMMING_EN2,
    FUZZY_STEMMING_NO,
    FUZZY_STEMMING_SE,
    FUZZY_STEMMING_DK,
    FUZZY_STEMMING_RU,
    FUZZY_STEMMING_FI,
    FUZZY_STEMMING_RO,
    FUZZY_STEMMING_HU
} FuzzyIndexType;


typedef enum {
    STEM_OK,
    STEM_NOT_ALPHA,     /* not all alpha */
    STEM_TOO_SMALL,     /* word too small to be stemmed */
    STEM_WORD_TOO_BIG,  /* word it too large to stem, would would be too large */
    STEM_TO_NOTHING    /* word stemmed to the null string */
} STEM_RETURNS;



/*
 * This structure manages the results from a stemming operation.
 */


typedef struct {
    STEM_RETURNS error;         /* return value from stemmer */
    const char *orig_word;      /* address of input string */
    SWINT_T list_size;              /* number of entries in the string list */
    char **word_list;           /* pointer to list of stemmed words */
    SWINT_T free_strings;           /* flag if true means free individual strings in string list */
    char *string_list[1];       /* null terminated array of string pointers */
} FUZZY_WORD;


/* FUZZY_OBJECT and FUZZY_OPTS reference each other, so pre-define */

typedef struct s_FUZZY_OBJECT FUZZY_OBJECT;
typedef struct s_FUZZY_OPTS   FUZZY_OPTS;



/*
 * This structure defines the layout of fuzzy options table.
 * The table lists the available stemmers.
 */


struct s_FUZZY_OPTS {
    FuzzyIndexType  fuzzy_mode;
    char            *name;
    FUZZY_WORD      *(*routine) ( FUZZY_OBJECT *fi, const char *inword );
    struct SN_env   *(*init) (void);
    void            (*stemmer_free) (struct SN_env *);
    SWINT_T             (*lang_stem)(struct SN_env *);
};


/*
 * This structure caches what stemmer to use.
 */

struct s_FUZZY_OBJECT {
    FUZZY_OPTS *stemmer;                /* stemmer options to use */
    struct SN_env *snowball_options;    /* snowball's data */
};




FUZZY_WORD *fuzzy_convert( FUZZY_OBJECT *fi, const char *inword );
void fuzzy_free_word( FUZZY_WORD *fd );
FUZZY_WORD *create_fuzzy_word( const char *input_word, SWINT_T word_count );


FuzzyIndexType fuzzy_mode_value( FUZZY_OBJECT *fi );
const char *fuzzy_string( FUZZY_OBJECT *fi );

FUZZY_OBJECT *set_fuzzy_mode( FUZZY_OBJECT *fi, char *param );
FUZZY_OBJECT *get_fuzzy_mode( FUZZY_OBJECT *fi, SWINT_T fuzzy );
void free_fuzzy_mode( FUZZY_OBJECT *fi );
SWINT_T stemmer_applied( FUZZY_OBJECT *fi );
void dump_fuzzy_list( void );


#endif
