/*
$Id$
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
** 2001-02-xx  rasc  makeItLow, strtolower  optimized/new
**			   iso handling, minor bugfixes
**
** 2001-02-xx  jruiz, rasc:  -- IMPORTANT NOTE --
**                   due to ISO charsset tolower,isspace, strcmp, etc.
**                   have to be (unsigned char)!!
**                   otherwise some chars may fail.
**
** 2001-03-08 rasc   rewritten and enhanced suffix routines
** 2001-04-10 rasc   str_dirname, str_basename, changed char_decode_C_ESC
**
*/

#include <ctype.h>
#include "swish.h"
#include "string.h"
#include "mem.h"


/* Case-insensitive strstr(). */
/* Jose Ruiz 02/2001 Faster one */
char *lstrstr(char *s, char *t)

{
int lens;
int lent;
int first=tolower((unsigned char)*t);
	lent=strlen(t);
	lens=strlen(s);
	for (; lens && lent<=lens; lens--,s++) {
		if(tolower((int)((unsigned char)*s))==first)
		{
			if(lent==1) return s;
			if(strncasecmp(s+1,t+1,lent-1)==0) return s;
		}
	}
	return NULL;
}

/* Gets the next word in a line. If the word's in quotes,
** include blank spaces in the word or phrase.
   -- 2001-02-11 rasc  totally rewritten, respect escapes like \"
*/

char *getword(char *s, int *skiplen)

{
 unsigned char quotechar;
 unsigned char uc;
 char *start;
 char *wstart;
 char *word;
 int  len;


	start = s;
	quotechar = '\0';

      s = str_skip_ws (s);
	if (! *s) return "\0";
	if (*s == '\"') quotechar = *s++;

	/* find end of "more words" or word */

      wstart = s;
      while (*s) {
         uc = (unsigned char) *s;
         if (uc == '\\') {                      /* Escape \     */
            s++;
            continue;
         }
         if (! quotechar) {
            if (isspace ((int)uc)) break;		/* end of word  */
         } else {
            if (uc == quotechar) break;		/* end of quote */
		if (uc == '\n') break;			/* EOL          */
		if (uc == '\r') break;			/* EOL (WIN)    */
         }

         s++;
      }

	/* make a save copy of "word" */

      len = s - wstart;
      word = (char *)emalloc(len + 1);
      strncpy (word,wstart,len);
      *(word+len) = '\0';

      if (*s && *s == quotechar) s++;
      *skiplen = s - start;

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





/*----------------------------------------------------*/


/*
  -- Check if a file with a particular suffix should be indexed
  -- according to the settings in the configuration file.
  -- 2001-03-08 rasc   rewritten (optimize and match also
  --                   e.g. ".htm.de" or ".html.gz")
*/

int isoksuffix(char *filename, struct swline *rulelist)
{
 char *s, *fe;


   if (!rulelist) return 1;      /* no suffixlist */
 
   /* basically do a right to left compare */
   fe = (filename + strlen(filename));
   while (rulelist) {
      s = fe - strlen (rulelist->line);
      if (s >= filename) {   /* no negative overflow! */
         if (! strcasecmp(rulelist->line, s)) {
             return 1;
         }
      }
      rulelist = rulelist->next;
   }

   return 0;
}




/* 05/00 Jose Ruiz
** Function to copy strings 
** Reallocate memory if needed
** Returns the string copied
** [see als estrredup() and estrdup()]
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
                *initialsize=len + 200;   /* 200 extra chars!!! */
                if(oldlen)
                        efree(dest);
                dest = (char *) emalloc(*initialsize + 1);
        }
        memcpy(dest,orig,len);
        *(dest+len)='\0';
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
** 02/2001 Jose Ruiz - Added extra NULL at the end
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
	sl->word=(char **)emalloc((maxsize=2)*sizeof(char *));
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
		/* Add an extra NULL */
	if(cursize==maxsize)
		sl->word=(char **)erealloc(sl->word,(maxsize+=1)*sizeof(char *));
	sl->word[cursize]=NULL;
	
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
register char *p,*r;
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
	for(p=s;p;) if( (p=strchr(p,'\n')) ) *p++=' ';
	for(p=s;p;) if( (p=strchr(p,'\r')) ) *p++=' ';
}

void remove_controls(char *s)
{
unsigned char *p,*q;

	if (s && *s) {
	   for(p=s,q=s; *p; p++) {
		if(!iscntrl((int)(*p)))
			*q++=*p;
	   }
	   *q='\0';
	}
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
					/* jmruiz 02/2001 change <tag> by a space */
				*p++=' ';
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

/* #### Function to convert binary data of length len to a string */
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






/* ------------------------------------------------------------ */




/*
  -- Skip white spaces...
  -- position to non space character
  -- return: ptr. to non space char or \0
  -- 2001-01-30  rasc
*/

char *str_skip_ws (char *s)

{
   while (*s && isspace((int)(unsigned char)*s)) s++;
   return s;
}







/*
  -- character decode excape sequence
  -- input: ptr to \... escape sequence (C-escapes)
  -- return: character code
             se:  string ptr to char after control sequence.
                 (ignore, if NULL ptr)
  -- 2001-02-04  rasc
  -- 2001-04-10  rasc   handle  '\''\0'  (safty!)
*/


char charDecode_C_Escape (char *s, char **se) 

{
  char c, *se2;

  if (*s != '\\') {
                                       /* no escape   */		  
     c = *s;                           /* return char */

  } else {

    switch (*(++s)) {			   /* can be optimized ... */
	case 'a':   c = '\a';  break;
	case 'b':   c = '\b';  break;
	case 'f':   c = '\f';  break;
	case 'n':   c = '\n';  break;
	case 'r':   c = '\r';  break;
	case 't':   c = '\t';  break;
	case 'v':   c = '\v';  break;

	case 'x':				  /* Hex  \xff  */
		c = (char) strtoul (++s, &se2, 16);
		s = --se2;
		break;

	case '0':				/* Oct  \0,  \012 */
		c = (char) strtoul (s, &se2, 8);
		s = --se2;
		break;

	case '\0':				/* outch!! null after \ */
		s--;				/* it's a "\"	   */
	default: 
		c =  *s;		/* print the escaped character */
		break;
   }

  }

 if (se) *se = s+1;
 return c;
}





/* 
   -- strtolower (make this string to lowercase)
   -- The string itself will be converted
   -- Return: ptr to string
   -- 2001-02-09  rasc:  former makeItLow() been a little optimized

   !!!  most tolower() don't map umlauts, etc. 
   !!!  you have to use the right cast! (unsigned char)
   !!!  or an ISO mapping...
*/

char *strtolower (char *s)
{
  unsigned char *p = s;

  while (*p) {
     *p = tolower((unsigned char)*p);
     p++;
  }
  return s; 
}






/* ---------------------------------------------------------- */
/* ISO characters conversion/mapping  handling                */



/*
  -- character map to normalize chars for search and store
  -- characters are all mapped to lowercase
  -- umlauts and special characters are mapped to ascii7 chars
  -- control chars/ special chars are mapped to " "
  -- 2001-02-10 rasc
*/


static const unsigned char iso8859_to_ascii7_lower_map[] = {
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',   /*  0 */
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',   /*  8 */
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',   /* 16 */
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', '!', '"', '#', '$', '%', '&', '\'',  /* 32 */
	'(', ')', '*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7',   /* 48 */
	'8', '9', ':', ';', '<', '=', '>', '?',
	'@', 'a', 'b', 'c', 'd', 'e', 'f', 'g',   /* 64 */
	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
	'p', 'q', 'r', 's', 't', 'u', 'v', 'q',   /* 80 */
	'x', 'y', 'z', '[', '\\', ']', '^', '_',
	'`', 'a', 'b', 'c', 'd', 'e', 'f', 'g',   /* 96 */
	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
	'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
	'x', 'y', 'z', '{', '|', '}', '~', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',	/* 128 */
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',   /* 144 */
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', '!', 'c', 'l', 'o', 'y', '|', '§',   /* 160 */
	'\"', 'c', ' ', '\"', ' ', '-', 'r', ' ',
	' ', ' ', '2', '3', '\'', 'u', ' ', '.',  /* 176 */
	' ', '1', ' ', '"', ' ', ' ', ' ', '?',
	'a', 'a', 'a', 'a', 'a', 'a', 'e', 'c',	/* 192 */
	'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i',
	'd', 'n', 'o', 'o', 'o', 'o', 'o', ' ',   /* 208 */
	'o', 'u', 'u', 'u', 'u', 'y', ' ', 's',
	'a', 'a', 'a', 'a', 'a', 'a', 'e', 'c',   /* 224 */
	'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i',
	'd', 'n', 'o', 'o', 'o', 'o', 'o', ' ',   /* 240 */
	'o', 'u', 'u', 'u', 'u', 'y', ' ', 'y'
};



/*
  -- "normalize" ISO character for store and search
  -- operations. This means convert it to ascii7 lower case.
  -- Return: char
  -- 2001-02-11  rasc
*/

unsigned char char_ISO_normalize (unsigned char c)
{  
  return iso8859_to_ascii7_lower_map[c];
}




/*
  -- "normalize" ISO character for store and search
  -- operations. This means convert it to ascii7 lower case.
  -- Return: char
  -- 2001-02-11  rasc
*/


char *str_ISO_normalize (char *s)

{
  unsigned char *p;

  p = (unsigned char *)s;
  while (*p) {
    *p = iso8859_to_ascii7_lower_map[*p];
    p++;
  }
  return s;
}


/* 02/2001 Jmruiz - Builds a string from a Stringlist starting at the
n element */
unsigned char *StringListToString(StringList *sl,int n)
{
int i,j;
char *s;
int len_s,len_w;
	s=emalloc((len_s=256)+1);
	/* compute required string size */
	for(i=n,j=0;i<sl->n;i++) 
	{
		len_w=strlen(sl->word[i]);
		if(len_s<(j+len_w+1))
			s=erealloc(s,(len_s+=len_w+1)+1);
		if(i!=n)
		{
			*(s+j)=' ';
			j++;
		}
		memcpy(s+j,sl->word[i],len_w);
		j+=len_w;
	}
	*(s+j)='\0';
	return s;
}




/* ---------------------------------------------------------- */



/* 
  -- translate chars 
  -- rewrite string itself via an character translation table
  -- translation table is a int[256] 
  -- return: ptr to string itself
*/

unsigned char *TranslateChars (int trlookup[], unsigned char *s)
{
  unsigned char *p;

  p = s;
  while (*p) {
    *p = (unsigned char) trlookup[(int)*p];
    p++;
  }
  return s;
}



/*
   -- Build a character translation table
   -- characters "from" will be converted in "to"
   -- result is stored in a lookuptable fixed size
   -- does also special translation rules like :ascii7:
   -- return: 0/1 param fail/ok
*/

int BuildTranslateChars (int trlookup[], unsigned char *from, unsigned char *to)

{
  int i;

  /* default init = 1:1 translation */
  for (i=0; i<256; i++)
     trlookup[i] = i;

  if (! from) return 0; 	/* No param! */

  /* special cases, one param  */
  if (! strcmp (from,":ascii7:") ){
     for (i=0; i<256; i++)  
        trlookup[i] = (int) char_ISO_normalize (i);
     return 1;
  }

  if (! to) return 0;		/* missing second param */

  /* alter table for "non 1:1" translation... */ 
  while (*from && *to)
     trlookup[(int)*from++] = (int) *to++;
  if (*to || *from) return 0; /* length the same? no? -> err */

  return 1;
}




/* ---------------------------------------------------------- */


/*
  -- cstr_basename
  -- return basename of a document path
  -- return: (char *) copy of filename
*/

char *cstr_basename (char *path)
{
  return (char *) estrdup (str_basename (path));
}


/*
  -- str_basename
  -- return basename of a document path
  -- return: (char *) ptr into(!) path string
*/

char *str_basename (char *path)
{
  char *s;

  s = strrchr (path,DIRDELIMITER);
  return (s) ? s+1 : path;  
}


/*
  -- cstr_dirname (copy)
  -- return dirname of a document path
  -- return: (char *) ptr on copy(!) of path
*/

char *cstr_dirname (char *path)
{
  char *s;
  char *dir;
  int  len;

  s = strrchr (path,DIRDELIMITER);
  
  if (!s) {
    dir = (char *)estrdup (" ");
    *dir = (*path==DIRDELIMITER) ? DIRDELIMITER : '.'; 
  } else {
    len = s-path;
    dir = emalloc (len+1);
    strncpy (dir, path, len);
    *(dir+len) = '\0';
  }
 
  return dir;  
}


