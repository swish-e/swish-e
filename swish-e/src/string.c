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

#define NO_TAG 0
#define TAG_CLOSE 1
#define TAG_FOUND 2

/* Gets the content between "<parsetag>" and "</parsetag>" from buffer
limiting the scan to the first max_lines lines (0 means all lines) */
char *parsetag(char *parsetag, char *buffer, int max_lines, int case_sensitive)
{
register int c, d;
register char *p,*q,*r;
char *tag;
int lencontent;
char *content;
int i, j, lines, status, tagbuflen, totaltaglen, curlencontent;
char *begintag;
char *endtag;
char *(*f_strstr)();

	if(case_sensitive)
		f_strstr=strstr;
	else
		f_strstr=lstrstr;

	lencontent=strlen(parsetag);
	begintag=emalloc(lencontent+3);
	endtag=emalloc(lencontent+4);
	sprintf(begintag,"<%s>",parsetag);
	sprintf(endtag,"</%s>",parsetag);

	tag = (char *) emalloc(1);
	tag[0] = '\0';

	content = (char *) emalloc((lencontent=MAXSTRLEN) +1);
	lines = 0;
	status= NO_TAG;
	p = content;
	*p = '\0';

	for (r=buffer; ; ) {
		c = *r++;
		if (c == '\n')
		{
			lines++;
			if(max_lines && lines==max_lines) break;
		}
		if (!c) {
			efree(tag); efree(content); efree(endtag); efree(begintag);
			return NULL;
		}
		switch(c) {
		case '<':
			tag = (char *) erealloc(tag,(tagbuflen=MAXSTRLEN)+1);
			totaltaglen = 0;
			tag[totaltaglen++] = '<';
			
			while (1) {
				d = *r++;
				if (!d) {
					efree(tag); efree(content); efree(endtag); efree(begintag);
					return NULL;
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
			
			if (f_strstr(tag, endtag)) {
				status = TAG_CLOSE;
				*p = '\0';
				for (i = 0; content[i]; i++)
					if (content[i] == '\n')
					content[i] = ' ';
				for (i = 0; isspace((int)((unsigned char)content[i])) ||
					content[i] == '\"'; i++)
					;
				for (j = 0; content[i]; j++)
					content[j] = content[i++];
				content[j] = '\0';
				for (j = strlen(content) - 1;
				(j && isspace((int)((unsigned char)content[j])))
					|| content[j] == '\0' || content[j] == '\"'; j--)
					content[j] = '\0';
				for (j = 0; content[j]; j++)
					if (content[j] == '\"')
					content[j] = '\'';
				efree(tag); efree(endtag); efree(begintag); 
				if (*content) {
					return(content);
				} else {
					efree(content);
					return NULL;
				}
			}
			else if (f_strstr(tag, begintag))
			{
				status = TAG_FOUND;
			}
			break;
		default:
			if (status == TAG_FOUND) {
				curlencontent=p-content;
				if(curlencontent==lencontent) {
					lencontent +=200;
					content = (char *)erealloc(content,lencontent +1);
					p = content + curlencontent;
				}
				*p = c;
				p++;
			}
		}
	}
	efree(tag); efree(content); efree(endtag); efree(begintag);
	return NULL;
}

/* Routine to check if a string contains only numbers */
int isnumstring(unsigned char *s)
{
	if(!s || !*s) return 0;
	for(;*s;s++)
		if(!isdigit((int)(*s))) break;
	if(*s) return 0;
	return 1;
}

void remove_newlines(char *s)
{
char *p;
	if(!s || !*s) return;
	for(p=s;p;) if(p=strchr(p,'\n')) *p++=' ';
	for(p=s;p;) if(p=strchr(p,'\r')) *p++=' ';
}


void remove_tags(char *s)
{
int intag;
char *p,*q;

	if(!s || !*s) return;
	for(p=q=s,intag=0;*q;q++)
	{
		switch(*q)
		{
			case '<':
				intag=1;
				break;
			case '>':
				intag=0;
				break;
			default:
				if(!intag)
				{
					*p++=*q;
				}
				break;
		}
	}
	*p='\0';
}

/* #### Function to conver binary data of length len to a string */
unsigned char *bin2string(unsigned char *data,int len)
{
unsigned char *s=NULL;
	if(data && len)
	{
		s=emalloc(len+1);
		memcpy(s,data,len);
		s[len]='\0';
	}
	return(s);
}
/* #### */
