/*
** Copyright (C) 1995, 1996, 1997, 1998 Hewlett-Packard Company
** Originally by Kevin Hughes, kev@kevcom.com, 3/11/94
**
*

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
    
** Mon May  9 15:51:39 CDT 2005
** added GPL
** if it was re-written by rasc, is it still copyright HP?


**
** fixed non-SWINT_T subscripting pointed out by "gcc -Wall"
** SRE 2/22/00
**
** 2001-03-08 rasc   rewritten and enhanced suffix routines
**
*/

#include "swish.h"
#include "check.h"
#include "hash.h"
#include "swstring.h"
#include "mem.h"

/* Check if a file with a particular suffix should be indexed
** according to the settings in the configuration file.
*/

/* Should a word be indexed? Consults the stopword hash list
** and checks if the word is of a reasonable length...
** If you have any good rules that can work with most languages,
** please let me know...
*/

SWINT_T     isokword(sw, word, indexf)
     SWISH  *sw;
     char   *word;
     IndexFILE *indexf;
{
    SWINT_T     i,
            same,
            hasnumber,
            hasvowel,
            hascons,
            numberrow,
            vowelrow,
            consrow,
            wordlen;
    char    lastchar;

    if (word[0] == '\0')
        return 0;

    if ( is_word_in_hash_table( indexf->header.hashstoplist, word ) )
        return 0;

    wordlen = strlen(word);
    if ((wordlen < indexf->header.minwordlimit) || (wordlen > indexf->header.maxwordlimit))
        return 0;

    lastchar = '\0';
    same = 0;
    hasnumber = hasvowel = hascons = 0;
    numberrow = vowelrow = consrow = 0;

    for (i = 0; word[i] != '\0'; i++)
    {
        /* Max number of times a char can repeat in a word */
        if (word[i] == lastchar)
        {
            same++;
            if (same > IGNORESAME)
                return 0;
        }
        else
            same = 0;

        /* Max number of consecutive digits */
        if (isdigit((SWINT_T) ( (unsigned char) word[i])))
        {
            hasnumber = 1;
            numberrow++;
            if (numberrow > IGNOREROWN)
                return 0;
            vowelrow = 0;
            consrow = 0;
        }

        /* maximum number of consecutive vowels a word can have */
        else if (isvowel(sw, word[i]))
        {
            hasvowel = 1;
            vowelrow++;
            if (vowelrow > IGNOREROWV)
                return 0;
            numberrow = 0;
            consrow = 0;
        }

        /* maximum number of consecutive consonants a word can have */
        else if (!ispunct((SWINT_T) ( (unsigned char) word[i])))
        {
            hascons = 1;
            consrow++;
            if (consrow > IGNOREROWC)
                return 0;
            numberrow = 0;
            vowelrow = 0;
        }
        lastchar = word[i];
    }

    /* If IGNOREALLV is 1, words containing all vowels won't be indexed. */
    if (IGNOREALLV)
        if (hasvowel && !hascons)
            return 0;

    /* If IGNOREALLC is 1, words containing all consonants won't be indexed */
    if (IGNOREALLC)
        if (hascons && !hasvowel)
            return 0;

    /* If IGNOREALLN is 1, words containing all digits won't be indexed */
    if (IGNOREALLN)
        if (hasnumber && !hasvowel && !hascons)
            return 0;

    return 1;
}


/*
  -- Determine document type by checking the file extension
  -- of the filename
  -- Return: doctype
  -- 2001-03-08 rasc   rewritten (optimize and match also
  --                   e.g. ".htm", ".htm.de" or ".html.gz")
*/

SWINT_T     getdoctype(char *filename, struct IndexContents *indexcontents)
{
    struct swline *swl;
    char   *s,
           *fe;


    if (!indexcontents)
        return NODOCTYPE;

    /* basically do a right to left compare */
    fe = (filename + strlen(filename));
    while (indexcontents)
    {
        swl = indexcontents->patt;

        while (swl)
        {
            s = fe - strlen(swl->line);
            if (s >= filename)
            {                   /* no negative overflow! */
                if (!strcasecmp(swl->line, s))
                {
                    return indexcontents->DocType;;
                }
            }
            swl = swl->next;
        }

        indexcontents = indexcontents->next;
    }

    return NODOCTYPE;
}





struct StoreDescription *hasdescription(SWINT_T doctype, struct StoreDescription *sd)
{
    while (sd)
    {
        if (sd->DocType == doctype)
            return sd;
        sd = sd->next;
    }
    return NULL;
}
