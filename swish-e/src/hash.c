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
**---------------------------------------------------------
** Added addStopList to support printing of common words
** G. Hill 4/7/97  ghill@library.berkeley.edu
**
** change sprintf to snprintf to avoid corruption
** SRE 11/17/99
**
** 04/00 - Jose Ruiz
** change hash for bighash in mergeresultlists for better performance
** when big searchs (a* or b* or c*)
**
*/

#include "swish.h"
#include "string.h"
#include "hash.h"
#include "search.h"
#include "mem.h"

/* Hashes a string.
*/

unsigned hash(s)
     char   *s;
{
    unsigned hashval;

    for (hashval = 0; *s != '\0'; s++)
        hashval = (int) ((unsigned char) *s) + 31 * hashval;
    return hashval % HASHSIZE;
}

/* Hashes a string for a larger hash table.
*/

unsigned bighash(s)
     char   *s;
{
    unsigned hashval;

    for (hashval = 0; *s != '\0'; s++)
        hashval = (int) ((unsigned char) *s) + 31 * hashval;
    return hashval % BIGHASHSIZE;
}

/* Hashes a int.
*/

unsigned numhash(i)
     int     i;
{
    return i % HASHSIZE;
}

/* Hashes a int for a larger hash table.
*/

unsigned bignumhash(i)
     int     i;
{
    return i % BIGHASHSIZE;
}

/* Hashes a string for a larger hash table (for search).
*/

unsigned searchhash(s)
     char   *s;
{
    unsigned hashval;

    for (hashval = 0; *s != '\0'; s++)
        hashval = (int) ((unsigned char) *s) + 31 * hashval;
    return hashval % SEARCHHASHSIZE;
}

/* Reads the internal list of default stopwords.
*/

void    readdefaultstopwords(INDEXDATAHEADER *header)
{
    int     i;

    for (i = 0; defaultstopwords[i] != NULL; i++)
        addstophash(header, defaultstopwords[i]);
}

/* Adds a stop word to the list of removed common words */
void    addStopList(INDEXDATAHEADER *header, char *word)
{
    char   *arrayWord;

    if (isstopword(header, word))
        return;

    /* Another BUG!!  Jose Ruiz 04/00
       The dimension of the array was not checked 
       Fixed */
    if (header->stopPos == header->stopMaxSize)
    {
        header->stopMaxSize += 100;
        if (!header->stopList)
            header->stopList = (char **) emalloc(header->stopMaxSize * sizeof(char *));

        else
            header->stopList = (char **) erealloc(header->stopList, header->stopMaxSize * sizeof(char *));
    }
    arrayWord = (char *) estrdup(word);
    header->stopList[header->stopPos++] = arrayWord;
}


/* Adds a stop word to a hash table.
*/

void    addstophash(INDEXDATAHEADER *header, char *word)
{
    unsigned hashval;
    struct swline *sp;

    if (isstopword(header, word))
        return;

    sp = (struct swline *) emalloc(sizeof(struct swline));

    sp->line = (char *) estrdup(word);

    hashval = hash(word);
    sp->next = header->hashstoplist[hashval];
    header->hashstoplist[hashval] = sp;
}

/* Sees if a word is a stop word by looking it up in the hash table.
*/

int     isstopword(INDEXDATAHEADER *header, char *word)
{
    unsigned hashval;
    struct swline *sp;

    hashval = hash(word);
    sp = header->hashstoplist[hashval];

    while (sp != NULL)
    {
        if (!strcmp(sp->line, word))
            return 1;
        sp = sp->next;
    }
    return 0;
}



/* Adds a buzzword to a hash table.*/

void    addbuzzwordhash(INDEXDATAHEADER *header, char *word)
{
    unsigned hashval;
    struct swline *sp;

    if (isbuzzword(header, word))
        return;

    header->buzzwords_used_flag++;

    sp = (struct swline *) emalloc(sizeof(struct swline));

    sp->line = (char *) estrdup(word);

    hashval = hash(word);
    sp->next = header->hashbuzzwordlist[hashval];
    header->hashbuzzwordlist[hashval] = sp;
}

void    freebuzzwordhash(INDEXDATAHEADER *header)
{
    int     i;
    struct swline *sp,
           *tmp;

    for (i = 0; i < HASHSIZE; i++)
        if (header->hashbuzzwordlist[i])
        {
            sp = (struct swline *) header->hashbuzzwordlist[i];
            while (sp)
            {
                tmp = sp->next;
                efree(sp->line);
                efree(sp);
                sp = tmp;
            }
            header->hashbuzzwordlist[i] = NULL;
        }
}


/* Sees if a word is a buzzword by looking it up in the hash table. */

int     isbuzzword(INDEXDATAHEADER *header, char *word)
{
    unsigned hashval;
    struct swline *sp;

    hashval = hash(word);
    sp = header->hashbuzzwordlist[hashval];

    while (sp != NULL)
    {
        if (!strcmp(sp->line, word))
            return 1;
        sp = sp->next;
    }
    return 0;
}



void    freestophash(INDEXDATAHEADER *header)
{
    int     i;
    struct swline *sp,
           *tmp;

    for (i = 0; i < HASHSIZE; i++)
        if (header->hashstoplist[i])
        {
            sp = (struct swline *) header->hashstoplist[i];
            while (sp)
            {
                tmp = sp->next;
                efree(sp->line);
                efree(sp);
                sp = tmp;
            }
            header->hashstoplist[i] = NULL;
        }
}

void    freeStopList(INDEXDATAHEADER *header)
{
    int     i;

    for (i = 0; i < header->stopPos; i++)
        efree(header->stopList[i]);
    if (header->stopList)
        efree(header->stopList);
    header->stopList = NULL;
    header->stopPos = header->stopMaxSize = 0;
}

/* Adds a "use" word to a hash table.
*/

void    addusehash(INDEXDATAHEADER *header, char *word)
{
    unsigned hashval;
    struct swline *sp;

    if (isuseword(header, word))
        return;

    sp = (struct swline *) emalloc(sizeof(struct swline));

    sp->line = (char *) estrdup(word);

    hashval = hash(word);
    sp->next = header->hashuselist[hashval];
    header->hashuselist[hashval] = sp;
}

/* Sees if a word is a "use" word by looking it up in the hash table.
*/

int     isuseword(INDEXDATAHEADER *header, char *word)
{
    unsigned hashval;
    struct swline *sp;

    hashval = hash(word);
    sp = header->hashuselist[hashval];

    while (sp != NULL)
    {
        if (!strcmp(sp->line, word))
            return 1;
        sp = sp->next;
    }
    return 0;
}
