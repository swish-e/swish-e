/*
** Copyright (C) 1999 OpenSA Project
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "soundex.h"

int soundex(word)
   char *word;  /* in/out: Target word  */
   {
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
#ifdef _DEBUG
	/* Debug to console  */
	printf("# %15s: %s ", "soundex.c", word);
#endif

	/* Make sure it actually starts with a letter  */
	if(!isalpha((int)((unsigned char)word[0]))) return soundXit();
#ifdef _DEBUG
	/* Debug to console  */
	printf("isalpha, ");
#endif
	
	/* Get string length and make sure its at least 3 characters  */
	if((n = (int)strlen(word)) < 3) return soundXit();
#ifdef _DEBUG
	/* Debug to console  */
	printf("=>3, ");
#endif

        /* If looks like a 4 digit soundex code we don't want to touch it. */
        if((n = (int)strlen(word)) == 4){
                if( isdigit( (int)(unsigned char)word[1] ) 
                 && isdigit( (int)(unsigned char)word[2] ) 
                 && isdigit( (int)(unsigned char)word[3] ) )
                       return soundXit();
        }

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
	strcpy(word, soundCode);
#ifdef _DEBUG
	/* Debug to console  */
	printf("-> \"%s\"\n", word);
#endif

	return(1);
}

int soundXit(void)
{
#ifdef _DEBUG
	printf("was left as is...\n");
#endif
	return(1);
}
