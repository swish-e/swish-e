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
**  2001-02-15   rasc   estrredup, type corrections
**
*/

#include "swish.h"
#include "mem.h"
#include "error.h"

/* Error-checking malloc()...
*/

#ifdef DEBUGMEMORY

struct memory {
	void *p;
	int sz;
};

#define MAXMEMARRAY 1000000
int firsttime=1;
struct memory mem[MAXMEMARRAY];

void *emalloc(i)
int i;
{
int j;
void *p;
if (firsttime) { firsttime = 0; for(j=0;j<MAXMEMARRAY;j++) mem[j].p=NULL; }

for(j=0;j<MAXMEMARRAY;j++) if (!mem[j].p) break;
if (j==MAXMEMARRAY) progerr("Out memory in Debug Array");	 

        if ((p = (void *) malloc(i)) == NULL)
                progerr("Ran out of memory (could not allocate enough)!");
mem[j].p = (void *)p;
mem[j].sz = i;
        return p;
}

void *erealloc(ptr, i)
void *ptr;
int i;
{
void *p;
int j;
if (firsttime) progerr("Memory error. Calling erealloc without emalloc");
for(j=0;j<MAXMEMARRAY;j++) if (mem[j].p == ptr) break;
if (j==MAXMEMARRAY) progerr("Error reallocating memory. Original pointer not found");	 
        if ((p = (void *) realloc(ptr, i)) == NULL)
                progerr("Ran out of memory (could not reallocate enough)!");
mem[j].p = (void *)p;
mem[j].sz = i;
        return p;
}

char *estrdup(str)
char *str;
{
char *p;
int j;
if (firsttime) { firsttime = 0; for(j=0;j<MAXMEMARRAY;j++) mem[j].p=NULL; }
for(j=0;j<MAXMEMARRAY;j++) if (!mem[j].p) break;
if (j==MAXMEMARRAY) progerr("Out memory in Debug Array");

        if ((p = strdup(str)) == NULL)
                progerr("Ran out of memory (could not allocate enough)!");
mem[j].p = (void *)p;
mem[j].sz = strlen(str) + 1;
        return p;
}

void efree(ptr)
void *ptr;
{
int j;
if (firsttime) progerr("Memory error. Calling efree without emalloc or estrdup");
for(j=0;j<MAXMEMARRAY;j++) if (mem[j].p == ptr) break;
if (j==MAXMEMARRAY) progerr("Error freeing memory. Pointer not found");
        free(ptr);
mem[j].p = NULL;
}

void checkmem(void)
{
int j,k,l;
char *p;
printf("Unallocated Memory:\n");
for(j=0;j<MAXMEMARRAY;j++) 
	if (mem[j].p) {
		printf("Pointer: %p Size: %d Contents:",mem[j].p,mem[j].sz);
		if(mem[j].sz>50)l=50;
		else l=mem[j].sz;
		for(k=0,p=mem[j].p;k<l;k++,p++) if(isalnum((int)((unsigned char)p[0])) || isspace((int)((unsigned char)p[0]))) putchar((int)p[0]);
		putchar('\n');
	}
}

#else

void checkmem(void)
{
	return;
}

void *emalloc(size_t i)
{
	void *p;
	
	if ((p = (void *) malloc(i)) == NULL)
		progerr("Ran out of memory (could not allocate enough)!");
	return p;
}

void *erealloc(void *ptr, size_t i)
{
	void *p;
	
	if ((p = (void *) realloc(ptr, i)) == NULL)
		progerr("Ran out of memory (could not reallocate enough)!");
	return p;
}

char *estrdup(char *str)
{
	char *p;
	
	if ((p = strdup(str)) == NULL)
		progerr("Ran out of memory (could not reallocate enough)!");
	return p;
}

void efree(void *ptr)
{
	free(ptr);
}

#endif

void freeswline(struct swline *tmplist)
{
struct swline *tmplist2;
       while (tmplist) {
              tmplist2 = tmplist->next;
              efree(tmplist->line);
              efree(tmplist);
              tmplist = tmplist2;
       }
}

void freeindexfile(IndexFILE *tmplist)
{
IndexFILE *tmplist2;
       while (tmplist) {
              tmplist2 = tmplist->next;
              efree(tmplist->line);
              efree(tmplist);
              tmplist = tmplist2;
       }
}

/* Be careful when freeing an entry because location info
** cannot be in memory (economic mode or swap_flag enabled) */
void freeentry(ENTRY *e)
{
int i;
	if (!e) return;
	for(i=0;i<e->currentlocation;i++) efree(e->locationarray[i]);
	efree(e->locationarray);
	efree(e->word);
	efree(e);
}

char *estrndup(char *s, size_t n)
{
size_t lens=strlen(s);
size_t newlen;
char *news;
	if(lens<n)
		newlen=lens;
	else
		newlen=n;
	news=emalloc(newlen+1);
	memcpy(news,s,newlen);
	news[newlen]='\0';
	return news;
}



/*
   -- estrredup
   -- do free on s1 and make copy of s2
   -- this is used, when s1 is replaced by s2
   -- 2001-02-15 rasc

*/

char *estrredup (char *s1, char *s2)
{
   if (s1) efree (s1);
   return estrdup (s2);
}

