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
** ** ** PATCHED 5/13/96, CJC
** Added MatchAndChange for regex in replace rule G.Hill 2/10/98
**
** change sprintf to snprintf to avoid corruption
** added safestrcpy() macro to avoid corruption from strcpy overflow
** SRE 11/17/99
**
** fixed cast to int problems pointed out by "gcc -Wall"
** SRE 2/22/00
**
**
*/

#include "swish.h"
#include "string.h"
#include "mem.h"

/* My own case-insensitive strstr().
*/

char *lstrstr(s, t)
char *s;
char *t;
{
	int i, j, k, l;
	
	for (i = 0; s[i]; i++) {
		for (j = 0, l = k = i; s[k] && t[j] &&
			tolower(s[k]) == tolower(t[j]); j++, k++)
			;
		if (t[j] == '\0')
			return s + l;
	}
	return NULL;
}

/* Gets the next word in a line. If the word's in quotes,
** include blank spaces in the word or phrase.
*/

char *getword(line, skiplen)
char *line;
int *skiplen;
{
int i, inquotes;
char *start,*p;
int lenword;
char *word;
	start = line;
	if (!(*line)) return "\0";

	while (isspace((int)((unsigned char)*line))) line++;

	if (!(*line)) return "\0";

	if (*line == '\"') {
		inquotes = 1;
		line++;
	}
	else
		inquotes = 0;

	word = (char *)emalloc((lenword=MAXWORDLEN) + 1);

	for (i = 0; *line && ((inquotes) ? (*line != '\"') : (!isspace((int)((unsigned char)*line)))); line++) {
		if(i==lenword) {
			lenword *=2;
			word = (char *)erealloc(word,lenword+1);
		}
		word[i++] = *line;
	}
	word[i] = '\0';

	if (!(*line) && i) if ((p=strpbrk(word,"\r\n"))) *p='\0';

	if (*line == '\"') line++;
	
	*skiplen = line - start;

		/* Do not wastw memory !! Resize word */	
	p=word;
	word=estrdup(p);
	efree(p);
	return word;
}

/* Gets the value of a variable in a line of the configuration file.
** Basically, anything in quotes or an argument to a variable.
*/

char *getconfvalue(line, var)
char *line;
char *var;
{
int i;
char *c;
int lentmpvalue;
char *tmpvalue,*p;
	if ((c = (char *) lstrstr(line, var)) != NULL) {
		if (c != line) return NULL;
		c += strlen(var);
		while (isspace((int)((unsigned char)*c)) || *c == '\"')
			c++;
		if (*c == '\0')
			return NULL;
		tmpvalue = (char *) emalloc((lentmpvalue=MAXSTRLEN) + 1);
		for (i = 0; *c != '\0' && *c != '\"' && *c != '\n' && *c!= '\r' ; c++) {
			if(i==lentmpvalue) {
				lentmpvalue *=2;
				tmpvalue= (char *) erealloc(tmpvalue,lentmpvalue +1);
			}
			tmpvalue[i++] = *c;
		}
		tmpvalue[i] = '\0';
		/* Do not wastw memory !! Resize word */	
		p=tmpvalue;
		tmpvalue=estrdup(p);
		efree(p);
		return tmpvalue;
	}
	else
		return NULL;
}

/* Extracts anything in <title> tags from an HTML file and returns it.
** Otherwise, only the file name without its path is returned.
*/

char *parsetitle(filename, alttitle)
char *filename;
char *alttitle;
{
register int c, d;
register char *p,*q;
char *tag;
int lentitle;
char *title;
char *shorttitle;
int i, j, lines, status, tagbuflen, totaltaglen, curlentitle;
FILE *fp;
	
	title = (char *) emalloc((lentitle=MAXTITLELEN) +1);
	tag = (char *) emalloc(1);
	tag[0] = '\0';
	lines = status = 0;
	p = title;
	*p = '\0';
	
	if ((q=strrchr(alttitle, '/'))) 
		q++;
	else
		q=alttitle;
	shorttitle = estrdup(q);
	
	fp = fopen(filename, "r");
	if (fp == NULL) {
		efree(tag);
		efree(title);
		return shorttitle;
	}
	for (; lines < TITLETOPLINES ; ) {
		c = getc(fp);
		if (c == '\n')
			lines++;
		if (feof(fp)) {
			fclose(fp);
			efree(tag);
			efree(title);
			return shorttitle;
		}
		switch(c) {
		case '<':
			tag = (char *) erealloc(tag,(tagbuflen=MAXSTRLEN)+1);
			totaltaglen = 0;
			tag[totaltaglen++] = '<';
			status = TI_OPEN;
			
			while (1) {
				d = getc(fp);
				if (d == EOF) {
					fclose(fp);
					efree(tag);
					efree(title);
					return shorttitle;
				}
				if(totaltaglen==tagbuflen) {
					tagbuflen+=200;
					tag=erealloc(tag,tagbuflen+1);
				}
				tag[totaltaglen++] = d;
				if (d == '>') {
					tag[totaltaglen]='\0';	
					break;
				}
			}
			
			if (lstrstr(tag, "</title>")) {
				status = TI_CLOSE;
				*p = '\0';
				fclose(fp);
				for (i = 0; title[i]; i++)
					if (title[i] == '\n')
					title[i] = ' ';
				for (i = 0; isspace((int)((unsigned char)title[i])) ||
					title[i] == '\"'; i++)
					;
				for (j = 0; title[i]; j++)
					title[j] = title[i++];
				title[j] = '\0';
				for (j = strlen(title) - 1;
				(j && isspace((int)((unsigned char)title[j])))
					|| title[j] == '\0' || title[j] == '\"'; j--)
					title[j] = '\0';
				for (j = 0; title[j]; j++)
					if (title[j] == '\"')
					title[j] = '\'';
				efree(tag);
				if (*title) {
					efree(shorttitle);
					return(title);
				} else {
					efree(title);
					return(shorttitle);
				}
			}
			else {
				if (lstrstr(tag, "<title>"))
					status = TI_FOUND;
			}
			break;
		default:
			if (status == TI_FOUND) {
				curlentitle=p-title;
				if(curlentitle==lentitle) {
					lentitle +=200;
					title = (char *)erealloc(title,lentitle +1);
					p = title + curlentitle;
				}
				*p = c;
				p++;
			}
			else {
				if (status == TI_CLOSE) {
					fclose(fp);
					efree(tag);
					efree(title);
					return shorttitle;
				}
			}
		}
	}
	fclose(fp);
	efree(tag);
	efree(title);
	return shorttitle;
}

/* In a string, replaces all occurrences of "oldpiece" with "newpiece".
** This is not really bulletproof yet.
*/
/* 05/00 Jose Ruiz 
** Totally rewritten
*/
char *replace(string, oldpiece, newpiece)
char *string;
char *oldpiece;
char *newpiece;
{
int limit, curpos, lennewpiece, lenoldpiece, curnewlen;
char *c, *p, *q;
int lennewstring;
char *newstring;
	newstring = (char *) emalloc((lennewstring=strlen(string)*2) + 1);
	lennewpiece = strlen(newpiece);
	lenoldpiece = strlen(oldpiece);
	c = string;
	q = newstring;
	curnewlen = 0;
	while ((p = (char *) strstr(c, oldpiece))) {
		limit = p - c;
		curnewlen += (limit + lennewpiece);
		if(curnewlen > lennewstring) {
			curpos = q - newstring;
			lennewstring = curnewlen + 200;
			newstring = (char *) erealloc(newstring,lennewstring+1);
			q=newstring+curpos;
		}
		memcpy(q,c,limit);
		q+=limit;
		memcpy(q,newpiece,lennewpiece);
		q+=lennewpiece;
		c=p+lenoldpiece;
	}
	curnewlen +=strlen(c);
	if(curnewlen > lennewstring) {
		curpos = q - newstring;
		lennewstring = curnewlen + 200;
		newstring = (char *) erealloc(newstring,lennewstring+1);
		q=newstring+curpos;
	}
	strcpy(q,c);
	efree(string);
	return newstring;
}

/* Just for A.P. and K.H. 2/5/98 by G.Hill - not really used now */
char* replaceWild (char* fileName, char* pattern, char* subs)
{
	int i;
	for (i = 0; pattern[i] != '*' && fileName[i] != '\0'; i++) 
    {
		if (fileName[i] != pattern[i])
			return fileName;
    }
	return subs;
}

/* Like strcmp(), but the order of sorting the first char is
** determined by the order of the characters in the wordchars array.
*/

/* Obsolete and slow
int wordcompare(s1, s2)
     char *s1;
     char *s2;
{
	register int i, j;
	
	if (s1[0] != s2[0]) {
		for (i = 0; wordchars[i] != '\0'; i++)
			if (s1[0] == wordchars[i])
			break;
		for (j = 0; wordchars[j] != '\0'; j++)
		{
			if (s2[0] == wordchars[j])
				break;
		}
		if (i < j)
			return -1;
		else
			return 1;
	}
	else
		return strcmp(s1, s2);
}
*/

/* This converts HTML numbered entities (such as &#169;)
** to strings (like &copy;). Much is this function is
** simply adding semicolons in the right places.
** This and the functions it calls are not very fast
** and could be made faster.
*/

char *convertentities(s)
char *s;
{
int lens, skip;
char *ent;
char *newword;
char *p,*q;

	if (!(p=strchr(s, '&'))) return s;
	lens = (int)strlen(s);

		/* Allocate enough memory */
	newword = (char *) emalloc(lens*2+1);

	q=newword;
	memcpy(q,s,p-s);
	q +=(p-s);

	for (s=p; *s != '\0'; ) {
		if (*s == '&') {
			ent = getent(s, &skip);
			if (ent[0] == '\0') {
				*q++=*s++;			
			} else {
				s += skip;
				if (*s == ';') s++;
				memcpy(q,ent,skip);
				q+=skip;
				*q++=';';
			}
			efree(ent);
		}
		else
			*q++=*s++;
	}
	*q='\0';
	if (ASCIIENTITIES) { 
			/* Jose Ruiz 06/00 Do not call to converttonamed
			** here. convertoascii do all the work 
			*/
		newword = (char *) converttoascii(newword);
	} else {
		newword = (char *) converttonamed(newword);
	}
	return newword;
}

/* Returns a matching entity that matches the beginning of a string, if any.
*/

char *getent(s, skip)
char *s;
int *skip;
{
int i;
int lenent;
char *ent;
int lentestent;
char *testent=NULL;

	ent = (char *)emalloc((lenent=MAXENTLEN) +1);

	*skip = 0;
	ent = SafeStrCopy(ent,s,&lenent);
	if (ent[1] == '#') {
		if (isdigit((int)ent[5])) {
			ent[0]='\0';
			return ent;
		}
		for (i = 2; ent[i] != '\0' && isdigit((int)ent[i]); i++);

		while (ent[i] != '\0' && !isdigit((int)ent[i])) ent[i++] = '\0';

		*skip = strlen(ent);
		return ent;
	}
	else {
		testent = (char *)emalloc((lentestent=MAXENTLEN) +1);
		for (i = 0; entities[i] != NULL; i += 3) {
			testent = SafeStrCopy(testent, entities[i],&lentestent);
			if (testent[0] != '\0') {
				if (!strncmp(testent, ent, strlen(testent))) {
					ent = SafeStrCopy(ent, testent,&lenent);
					*skip = strlen(ent);
					efree(testent);
					return ent;
				}
			}
		}
		efree(testent);
	}
	
	ent = SafeStrCopy(ent,"\0",&lenent);
	return ent;
}

/* This is the real function called by convertentities() that
** changes numbered to named entities.
*/

char *converttonamed(str)
char *str;
{
int i, hasnumbered, ilen;
int lentestent;
char *testent;
int lennewent;
char *newent;
char *newword;
	
	newword = (char *) estrdup(str);
	efree(str);
	testent = (char *) emalloc((lentestent=MAXENTLEN) + 1);
	newent = (char *) emalloc((lennewent=MAXENTLEN) + 1);
	
	do {
	   for (i = 0, hasnumbered = 0; entities[i] != NULL; i += 3) {
		ilen=strlen(entities[i+1]);
		if((ilen+1)>=lentestent) {
			lentestent=ilen+1+100;
			testent=erealloc(testent,lentestent+1);
		}
		memcpy(testent, entities[i + 1],ilen);
		testent[ilen]=';';
		testent[ilen+1]='\0';
		if (strstr(newword, testent) != NULL &&
			(entities[i])[0] != '\0') {
			hasnumbered=1;
			ilen=strlen(entities[i]);
			if((ilen+1)>=lennewent) {
				lennewent=ilen+1+100;
				newent=erealloc(newent,lennewent+1);
			}
			memcpy(newent,entities[i],ilen);
			newent[ilen]=';';
			newent[ilen+1]='\0';
			newword = (char *) replace(newword, testent, newent);
		}
	   }
	} while (hasnumbered);
	efree(testent);
	efree(newent);
	return newword;
}

/* This function converts all convertable named and numbered
** entities to their ASCII equivalents, if they exist.
*/

char *converttoascii(str)
char *str;
{
int i, hasnonascii,ilen;
char *c, *d;
int lenwrdent;
char *wrdent;
int lennument;
char *nument;
char *newword;

	newword = (char *) estrdup(str);
	efree(str);
	wrdent = (char *) emalloc((lenwrdent=MAXENTLEN) + 1);
	nument = (char *) emalloc((lennument=MAXENTLEN) + 1);
	
	do {
	   for (i = 0, hasnonascii = 0; entities[i] != NULL; i += 3) {
		ilen=strlen(entities[i]);
		if((ilen+1)>=lenwrdent) {
			lenwrdent=ilen+1+200;
			wrdent=erealloc(wrdent,lenwrdent+1);
		}
		memcpy(wrdent,entities[i],ilen);
		wrdent[ilen]=';';
		wrdent[ilen+1]='\0';
		ilen=strlen(entities[i+1]);
		if((ilen+1)>=lennument) {
			lennument=ilen+1+200;
			nument=erealloc(nument,lennument+1);
		}
		memcpy(nument,entities[i+1],ilen);
		nument[ilen]=';';
		nument[ilen+1]='\0';
		
		c = d = NULL;
		if ((entities[i])[0] != '\0')
			c = (char *) strstr(newword, wrdent);
		if ((entities[i + 1])[0] != '\0')
			d = (char *) strstr(newword, nument);
		if ((entities[i + 2])[0] != '\0' && (c!=NULL || d!=NULL)) {
			hasnonascii=1;
			if (c != NULL && d==NULL) { 
				newword = (char *) replace(newword, wrdent, entities[i + 2]); 
			} else if (d != NULL && c==NULL) { 
				newword = (char *) replace(newword, nument, entities[i + 2]); 
			} else {
				newword = (char *) replace(newword, wrdent, entities[i + 2]); 
				newword = (char *) replace(newword, nument, entities[i + 2]); 
			}
		}
	   }
	} while (hasnonascii);
	efree(wrdent);
	efree(nument);
	return newword;
}

/* That regular expression matching and replacing thing */
char * matchAndChange (char *str, char *pattern, char *subs) 
{
int  status, lenSub, lenBeg, lenTmp;
regex_t re;
regmatch_t pmatch[MAXPAR];
char *tmpstr;
char begin[MAXSTRLEN];
int lennewstr;
char *newstr;

	newstr = (char *)emalloc((lennewstr=MAXSTRLEN) +1);
	
	status = regcomp(&re, pattern, REG_EXTENDED);
	if ( status != 0) {
		regfree(&re); /* Richard Beebe */
		efree(newstr);
		return str;
	}
	
	status = regexec(&re,str,(size_t)MAXPAR,pmatch,0);
	if (status != 0) {
		regfree(&re); /* Richard Beebe */
		efree(newstr);
		return str;
	}
	
	tmpstr = str;
	while (!status) {
		/* Stuff the new piece were needed */
		strncpy(begin,tmpstr,pmatch[0].rm_so); /* get the beginning */
		begin[pmatch[0].rm_so] = '\0';  /* Null terminate */
		
		lenBeg = strlen(begin);
		lenSub = strlen(subs);
		lenTmp = strlen(&(tmpstr[pmatch[0].rm_eo]));
		if ( (lenTmp + lenSub + lenBeg) >= lennewstr)
		{
			lennewstr = lenTmp + lenSub + lenBeg + 200;
			newstr = (char *)erealloc(newstr,lennewstr +1);
		}

		memcpy(newstr,begin,lenBeg);
		memcpy(newstr+lenBeg,subs,lenSub);
		memcpy(newstr+lenBeg+lenSub,&(tmpstr[pmatch[0].rm_eo]),lenTmp);
		newstr[lenBeg+lenSub+lenTmp]='\0';

		/* Copy the newstr into the tmpstr */

		safestrcpy(MAXSTRLEN,tmpstr,newstr); /* CAREFUL! tmpstr points to an arg of unknown length */
		/* Position the pointer to the end of the subs string */
		tmpstr = &(tmpstr[pmatch[0].rm_so+lenSub]);
		status = regexec(&re,tmpstr,(size_t)5,pmatch,0);
	}
	regfree(&re);
	efree(newstr);
	return str;
}
/*---------------------------------------------------------*/
/* Match a regex and a string */

int matchARegex( char *str, char *pattern) 
{
	int  status;
	regex_t re;
	
	status = regcomp(&re, pattern, REG_EXTENDED);
	if ( status != 0) {
		regfree(&re); /* Richard Beebe */
		return 0;
	}
	
	status = regexec(&re,str,(size_t)0,NULL,0);
	regfree(&re); /** Marc Perrin ## 18Jan99 **/
	if (status != 0)
		return 0;
	
	return 1;
}
/*-----------------------------------------------------*/
void makeItLow (char *str)
{
  int i;
  int len = strlen(str);
  for (i = 0; i < len; i++)
    str[i] = tolower(str[i]);
}
/*----------------------------------------------------*/

/* Check if a file with a particular suffix should be indexed
** according to the settings in the configuration file.
*/

int isoksuffix(filename, rulelist)
char *filename;
struct swline *rulelist;
{
int badfile;
char *c;
int lensuffix;
char *suffix;
int lenchecksuffix;
char *checksuffix;
struct swline *tmplist;
	tmplist = rulelist;
	if (tmplist == NULL)
		return 1; 

	if ((c = (char *) strrchr(filename, '.')) == NULL)
		return 0; 
	
	if ((int)strlen(c+1) >= MAXSUFFIXLEN)
	  return 0;

	suffix = (char *) emalloc((lensuffix=MAXSUFFIXLEN) + 1);
	checksuffix = (char *) emalloc((lenchecksuffix=MAXSUFFIXLEN) + 1);

	badfile = 1;
	checksuffix = SafeStrCopy(checksuffix, c + 1,&lenchecksuffix);
	while (tmplist != NULL) {
		if ((c = (char *) strrchr(tmplist->line, '.')) == NULL)
			suffix = SafeStrCopy(suffix, tmplist->line,&lensuffix); 
		else
			suffix = SafeStrCopy(suffix, c + 1,&lensuffix); 
		if (lstrstr(suffix, checksuffix) && strlen(suffix) == strlen(checksuffix))
			badfile = 0;
		tmplist = tmplist->next;
	}
	efree(checksuffix);
	efree(suffix);
	return !(badfile);
}

/* 05/00 Jose Ruiz
** Function to copy strings 
** Reallocate memory if needed
** Returns the string copied
*/
char *SafeStrCopy(dest, orig, initialsize)
char *dest;
char *orig;
int *initialsize;
{
int len,oldlen;
	len=strlen(orig);
	oldlen=*initialsize;
	if(len > oldlen) {
		*initialsize=len + 200;
		if(oldlen)
			dest = (char *) erealloc(dest,*initialsize + 1);
		else
			dest = (char *) emalloc(*initialsize + 1);
	}
	strcpy(dest,orig);
	return(dest);
}


/* Comparison routine to sort a string - See sortstring */
int ccomp(const void *s1,const void *s2)
{
        return (*(unsigned char *)s1 - *(unsigned char *)s2);
}

/* Sort a string  removing dups */
void sortstring(char *s)
{
int i,j,len;
	len=strlen(s);
	qsort(s,len,1,&ccomp);
	for(i=1,j=1;i<len;i++) if(s[i]!=s[j-1]) s[j++]=s[i];
	s[j]='\0';

}

/* Merges two strings removing dups and ordering results */
char *mergestrings(char *s1, char *s2)
{
int i,j,ilen1,ilen2,ilent;
char *s,*p;
        ilen1=strlen(s1);
        ilen2=strlen(s2);
        ilent=ilen1+ilen2;
        s= emalloc(ilent+1);
        p= emalloc(ilent+1);
        if (ilen1) memcpy(s,s1,ilen1);
        if (ilen2) memcpy(s+ilen1,s2,ilen2);
        if (ilent) qsort(s,ilent,1,&ccomp);
        for(i=1,j=1,p[0]=s[0];i<ilent;i++) if(s[i]!=p[j-1]) p[j++]=s[i];
        p[j]='\0';
        efree(s);
        return(p);
}

void makelookuptable(char* s,int *l)
{
int i;
for(i=0;i<256;i++) l[i]=0;
for(;*s;s++)l[(int)((unsigned char)*s)]=1;
}

void makeallstringlookuptables(SWISH *sw)
{
        makelookuptable("aeiouAEIOU",sw->isvowellookuptable);
}

/* 06/00 Jose Ruiz- Parses a line into a StringList
*/
StringList *parse_line(char *line)
{
StringList *sl;
int cursize,skiplen,maxsize;
char *p;
	if(!line) return(NULL);
	if((p=strchr(line,'\n'))) 
		*p='\0';
	cursize=0;
	sl=(StringList *)emalloc(sizeof(StringList));
	sl->word=(char **)emalloc((maxsize=1)*sizeof(char *));
	p=line;
	skiplen=1;
	while(skiplen && *(p=(char *)getword(line,&skiplen))) 
	{
		if(cursize==maxsize) 
			sl->word=(char **)erealloc(sl->word,(maxsize*=2)*sizeof(char *));
		sl->word[cursize++]=(char *)p;
		line+=skiplen;
	}
	sl->n=cursize;
	return sl;
}

/* Frees memory used by a StringList
*/
void freeStringList(StringList *sl)
{
	if(sl) {
		while(sl->n)
			efree(sl->word[--sl->n]);
		efree(sl->word);
		efree(sl);
	}
}

/* 10/00 Jose Ruiz
** Function to copy len bytes from orig to dest+off_dest
** Reallocate memory if needed
** Returns the pointer to the new area
*/
unsigned char *SafeMemCopy(dest, orig, off_dest, sz_dest, len)
unsigned char *dest;
unsigned char *orig;
int off_dest;
int *sz_dest;
int len;
{
        if(len > (*sz_dest-off_dest)) {
                *sz_dest=len + off_dest;
                if(dest)
                        dest = (char *) erealloc(dest,*sz_dest);
                else
                        dest = (char *) emalloc(*sz_dest);
        }
        memcpy(dest+off_dest,orig,len);
        return(dest);
}

