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
**
** fixed non-int subscripting pointed out by "gcc -Wall"
** SRE 2/22/00
**
** 2001-03-08 rasc   rewritten and enhanced suffix routines
**
*/

#include "swish.h"
#include "check.h"
#include "hash.h"
#include "string.h"
#include "mem.h"

/* Check if a file with a particular suffix should be indexed
** according to the settings in the configuration file.
*/

/* Should a word be indexed? Consults the stopword hash list
** and checks if the word is of a reasonable length...
** If you have any good rules that can work with most languages,
** please let me know...
*/

int isokword(sw, word, indexf)
SWISH *sw;
char *word;
IndexFILE *indexf;
{
	int i, same, hasnumber, hasvowel, hascons,
		numberrow, vowelrow, consrow, wordlen;
	char lastchar;
	
	if (word[0] == '\0')
		return 0;
	
	if (isstopword(indexf,word))
		return 0;
	wordlen = strlen(word);
	if ((wordlen < indexf->header.minwordlimit) || (wordlen > indexf->header.maxwordlimit))
		return 0;
	
	lastchar = '\0';
	same = 0;
	hasnumber = hasvowel = hascons = 0;
	numberrow = vowelrow = consrow = 0;
	for (i = 0; word[i] != '\0'; i++) {
		if (word[i] == lastchar) {
			same++;
			if (same > IGNORESAME)
				return 0;
		}
		else
			same = 0;
		if (isdigit((int)word[i])) {
			hasnumber = 1;
			numberrow++;
			if (numberrow > IGNOREROWN)
				return 0;
			vowelrow = 0;
			consrow = 0;
		}
		else if (isvowel(sw,word[i])) {
			hasvowel = 1;
			vowelrow++;
			if (vowelrow > IGNOREROWV)
				return 0;
			numberrow = 0;
			consrow = 0;
		}
		else if (!ispunct((int)word[i])) {
			hascons = 1;
			consrow++;
			if (consrow > IGNOREROWC)
				return 0;
			numberrow = 0;
			vowelrow = 0;
		}
		lastchar = word[i];
	}
	
	if (IGNOREALLV)
		if (hasvowel && !hascons)
		return 0;
	if (IGNOREALLC)
		if (hascons && !hasvowel)
		return 0;
	if (IGNOREALLN)
		if (hasnumber && !hasvowel && !hascons)
		return 0;
	
	return 1;
}

/* Does a word have valid characters?
*/

int hasokchars(indexf, word)
IndexFILE *indexf;
char *word;
{
int i;
	if(!indexf->header.begincharslookuptable[(int)((unsigned char)word[0])]) return 0;

	if(!indexf->header.endcharslookuptable[(int)((unsigned char)word[strlen(word)-1])]) return 0;

	for (i = 0; word[i] != '\0'; i++)
		if(iswordchar(indexf->header,word[i]))
		return 1;

	return 0;
}


//$$$$$$$$ to be deleted if new routine works >>>>>>>>>>>>>>>

int OLD__getdoctype(char *filename, struct IndexContents *indexcontents)
{
char *c, *checksuffix;
int  lchecksuffix;
struct IndexContents *ic;
struct swline *swl;
	if(!indexcontents)
		return NODOCTYPE;
	if (! (c = (char *)strrchr(filename, '.')) ) return NODOCTYPE;
   
	lchecksuffix = strlen(c);
	checksuffix=(char *) emalloc(lchecksuffix + 1);
	strcpy(checksuffix, c);

	for(ic=indexcontents;ic;ic=ic->next) 
	{
		for(swl=ic->patt;swl;swl=swl->next)
		{
			if (lstrstr(swl->line, checksuffix)
			   && ((int)strlen(swl->line)) == lchecksuffix) {
				efree(checksuffix);
				return ic->DocType;
			}
		}
	}
	efree(checksuffix);
	return NODOCTYPE;
}
//$$$$$$ to be deleted <<<<<<<<<<<<<<<<<


/*
  -- Determine document type by checking the file extension
  -- of the filename
  -- Return: doctype
  -- 2001-03-08 rasc   rewritten (optimize and match also
  --                   e.g. ".htm", ".htm.de" or ".html.gz")
*/

int getdoctype(char *filename, struct IndexContents *indexcontents)
{
 struct swline *swl;
 char *s, *fe;


   if(!indexcontents) return NODOCTYPE;

   /* basically do a right to left compare */
   fe = (filename + strlen(filename));
   while (indexcontents) {
	swl=indexcontents->patt;

	while (swl) {
	   s = fe - strlen (swl->line);
	   if (s >= filename) {   /* no negative overflow! */
		if (! strcasecmp(swl->line, s)) {
			return indexcontents->DocType;;
		}
	   }
	   swl = swl->next;
	}

	indexcontents = indexcontents->next;
   }

   return NODOCTYPE;
}





struct StoreDescription *hasdescription(int doctype, struct StoreDescription *sd)
{
	while(sd)
	{
		if(sd->DocType==doctype)
			return sd;
		sd=sd->next;
	}
	return NULL;
}
