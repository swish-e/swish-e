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
    
    Swish-e includes a library for searching with a well-defined API. The library
    is named libswish-e.
    
    Linking libswish-e statically or dynamically with other modules is making a
    combined work based on Swish-e.  Thus, the terms and conditions of the GNU
    General Public License cover the whole combination.

    As a special exception, the copyright holders of Swish-e give you
    permission to link Swish-e with independent modules that communicate with
    Swish-e solely through the libswish-e API interface, regardless of the license
    terms of these independent modules, and to copy and distribute the
    resulting combined work under terms of your choice, provided that
    every copy of the combined work is accompanied by a complete copy of
    the source code of Swish-e (the version of Swish-e used to produce the
    combined work), being distributed under the terms of the GNU General
    Public License plus this exception.  An independent module is a module
    which is not derived from or based on Swish-e.

    Note that people who make modified versions of Swish-e are not obligated
    to grant this special exception for their modified versions; it is
    their choice whether to do so.  The GNU General Public License gives
    permission to release a modified version without this exception; this
    exception also makes it possible to release a modified version which
    carries forward this exception.


Tue May 10 08:00:20 CDT 2005
added GPL and removed OpenSA copyright per David Norris, author
*********************************************************************************
**
 * Reference: Adapted from Knuth, D.E. (1973) The art of computer programming;
 *    Volume 3: Sorting and searching.  Addison-Wesley Publishing Company:
 *    Reading, Mass. Page 392.
 *
 * 1. Retain the first letter of the name, and drop all occurrences of
 *    a, e, h, i, o, u, w, y in other positions.
 *
 * 2. Assign the following numbers to the remaining letters after the first:
 *      b, f, p, v -> 1                         l -> 4
 *      c, g, j, k, q, s, x, z -> 2             m, n -> 5
 *      d, t -> 3                               r -> 6
 *
 * 3. If two or more letters with the same code were adjacent in the original
 *    name (before step 1), omit all but the first.
 *
 * 4. Convert to the form ``letter, digit, digit, digit'' by adding trailing
 *    zeros (if there are less than three digits), or by dropping rightmost
 *    digits (if there are more than three).
 *
 * The examples given in the book are:
 *
 *      Euler, Ellery           E460
 *      Gauss, Ghosh            G200
 *      Hilbert, Heilbronn      H416
 *      Knuth, Kant             K530
 *      Lloyd, Ladd             L300
 *      Lukasiewicz, Lissajous  L222
 *
 * Most algorithms fail in two ways:
 *  1. they omit adjacent letters with the same code AFTER step 1, not before.
 *  2. they do not omit adjacent letters with the same code at the beginning
 *     of the name.
 *
 */

#include "swish.h"
#include "stemmer.h"  /* For constants */
#include "swstring.h" /* for estrdup */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "soundex.h"

FUZZY_WORD *soundex( FUZZY_OBJECT *fi, const char *inword)
   {
        FUZZY_WORD *fw = create_fuzzy_word( inword, 1 ); /* create place to store stemmed word */
        char word[MAXWORDLEN+1];
	/* Misc Stuff  */
	char u, l ;
	int i, j, n;
	/* Resultant Sound Code  */
	char soundCode[5] = "0000\0";
	/* Group Number Lookup Table  */
	static char soundTable[26] =
	{0,						/* A  */
	 '1',					/* B  */
	 '2',					/* C  */
	 '3',					/* D  */
	 0,						/* E  */
	 '1',					/* F  */
	 '2',					/* G  */
	 0,						/* H  */
	 0,						/* I  */
	 '2',					/* J  */
	 '2',					/* K  */
	 '4',					/* L  */
	 '5',					/* M  */
	 '5',					/* N  */
	 0,						/* O  */
	 '1',					/* P  */
	 '2',					/* Q  */
	 '6',					/* R  */
	 '2',					/* S  */
	 '3',					/* T  */
	 0,						/* U  */
	 '1',					/* V  */
	 0,						/* W  */
	 '2',					/* X  */
	 0,						/* Y  */
	 '2'};					/* Z  */

    /* Make sure the word is not too large from the start. */
    if ( strlen( inword ) >= MAXWORDLEN )
    {
        fw->error =  STEM_WORD_TOO_BIG;
        return fw;
    }


    /* make working copy */
    strcpy( word, inword );

  

#ifdef _DEBUG
	/* Debug to console  */
	printf("# %15s: %s ", "soundex.c", word);
#endif

	/* Make sure it actually starts with a letter  */
	if(!isalpha((int)((unsigned char)word[0]))) 
        {
            fw->error = STEM_NOT_ALPHA;
            return fw;
        }

#ifdef _DEBUG
	/* Debug to console  */
	printf("isalpha, ");
#endif
	
	/* Get string length and make sure its at least 3 characters  */
	if((n = (int)strlen(word)) < 3) 
        {
            fw->error = STEM_TOO_SMALL;
            return fw;
        }
#ifdef _DEBUG
	/* Debug to console  */
	printf("=>3, ");
#endif

        /* If looks like a 4 digit soundex code we don't want to touch it. */

        /* Humm.  Just because it looks like a duck, doesn't mean it is one
         * The source is suppose to not be soundex, so this doesn't make a lot of sense.  - moseely */
#ifdef skip_section
        
        if((n = (int)strlen(word)) == 4){
                if( isdigit( (int)(unsigned char)word[1] ) 
                 && isdigit( (int)(unsigned char)word[2] ) 
                 && isdigit( (int)(unsigned char)word[3] ) )
                       return STEM_OK;  /* Hum, probably not right */
        }
#endif

	/* Convert chars to lower case and strip non-letter chars  */
	j = 0;
	for (i = 0; i < n; i++) {
		u = tolower((unsigned char)word[i]);
		if ((u > 96) && (u < 123)) {
			 word[j] = u;
			j++;
		}
	}

	/* terminate string  */
	 word[j] = 0;

	/* String length again  */
	n = strlen(word);

	soundCode[0] = word[0];

	/* remember first char  */
	l = soundTable[((word[0]) - 97)];

	j = 1;

	/* build soundex string  */
	for (i = 1; i < n && j < 4; i++) {
		u = soundTable[((word[i]) - 97)];

		if (u != l) {
			if (u != 0) {
				soundCode[(int) j++] = u;
			}
			l = u;
		}
	}


    fw->free_strings = 1; /* flag that we are creating a string */
    fw->string_list[0] = estrdup( soundCode );
    return fw;

}
