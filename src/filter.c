/*
$Id$
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
**
** 1998-07-04 rasc    original filter code
** 1999-08-07 rasc    
** 2001-02-28 rasc    own module started for filters
**                    some functions rewritten and enhanced...
**
*/

#include "swish.h"
#include "file.h"
#include "filter.h"
#include "mem.h"
#include "string.h"
#include <string.h>


#ifdef _WIN32
#define pclose _pclose
#define popen _popen
#endif




/*
 -- Add a filter to the filterlist (file ext -> filter prog)
 -- (filterdir may be NULL)
 -- 1999-08-07 rasc
 -- 2001-02-28 rasc  
*/


struct filter *addfilter(struct filter *rp, char *suffix, char *prog, char *filterdir)

{
 struct filter *newnode;
 char *buf;
 char *f_dir;
 int ilen1,ilen2;

	newnode = (struct filter *) emalloc(sizeof(struct filter));
	newnode->suffix= (char *) estrdup(suffix);

	f_dir = (filterdir) ? filterdir : "";


	ilen1=strlen(f_dir);
	ilen2=strlen(prog);
	buf = (char *)emalloc(ilen1+ilen2+2);
	memcpy(buf,f_dir,ilen1);

		/* If filterdir exists and not ends in DIRDELIMITER 
		** (/ in Unix or \\ in WIN32) add it
		*/
	if(ilen1 && buf[ilen1-1]!=DIRDELIMITER) buf[ilen1++]=DIRDELIMITER;
	memcpy(buf+ilen1,prog,ilen2);
	buf[ilen1+ilen2]='\0';

	newnode->prog= buf;
	newnode->next = NULL;

	if (rp == NULL)
		rp = newnode;
	else
		rp->nodep->next = newnode;

	rp->nodep = newnode;
	return rp;
}





/*
 -- Check, if a filter is needed to retrieve information from a file
 -- Returns NULL or path to filter prog according conf file.
 -- 1999-08-07 rasc
 -- 2001-02-28 rasc  rewritten, now possible: search for ".pdf.gz", etc.
*/

char *hasfilter (char *filename, struct filter *filterlist)
{
struct filter *fl;
char *s, *fe;


   fl = filterlist;
   if (! fl) return (char *)NULL;

   fe = (filename + strlen(filename));

   while (fl != NULL) {
      s = fe - strlen (fl->suffix);
      if (s >= filename) {   /* no negative overflow! */
         if (! strcasecmp(fl->suffix, s)) {
             return fl->prog;
         }
      }
      fl = fl->next;
   }

   return (char *)NULL;
}



/*
  -- open filter (in: file,  out: FILE *)
  -- params are in (FileProp *) - but should be adapted later
  -- Return: FILE *  (filter stream) or NULL
*/

FILE *FilterOpen (FileProp *fprop)
{
  char *filtercmd;
  FILE *fp;

   /*
      -- simple filter call "filter 'work_file' 'real_path_url'"
      --    > (decoded output =stdout)
   */

   filtercmd=emalloc(strlen(fprop->filterprog)
              +strlen(fprop->work_path)+strlen(fprop->real_path)+6+1);
   sprintf(filtercmd, "%s \'%s\' \'%s\'", fprop->filterprog,
              fprop->work_path, fprop->real_path);

   fp = popen (filtercmd,"r");  /* Open stream */
   efree (filtercmd);

   return fp;
}


/* 
  -- Close filter stream
  -- return: errcode
*/

int FilterClose (FILE *fp)
{
  return pclose (fp);
}






