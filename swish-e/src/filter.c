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
** 2001-04-09 rasc    options for filters  (%f, etc)
**
*/

#include <string.h>
#include "swish.h"
#include "file.h"
#include "mem.h"
#include "string.h"
#include "error.h"
#include "filter.h"


#ifdef _WIN32
#define pclose _pclose
#define popen _popen
#endif


/* private module prototypes */

static struct FilterList *addfilter (struct FilterList *rp, char *FilterSuffix, char *FilterProg, char *options, char *FilterDir);
static char *filterCallCmdOptParam2 (char *str, char param, FileProp *fp);
static char *filterCallCmdOptStr (char *opt_mask, FileProp *fprop);




/* 
  -- init structures for filters
*/

void initModule_Filter (SWISH  *sw)

{
   struct MOD_Filter *md;

      md = (struct MOD_Filter *) emalloc(sizeof(struct MOD_Filter));
      sw->Filter = md;

      md->filterdir = NULL;    /* init FilterDir  */
      md->filterlist = NULL;   /* init FileFilter */
   return;
}


/* 
  -- release structures for filters
  -- release all wired memory
  -- 2001-04-09 rasc
*/

void freeModule_Filter (SWISH *sw)

{
  struct MOD_Filter *md = sw->Filter;
  struct FilterList *f, *fn;


   efree(md->filterdir);	/* free FilterDir */

   f = md->filterlist;
   while (f) {			/* free FileFilter List */
      efree (f->suffix);
      efree (f->options);
      efree (f->prog);
      fn = f->next;
      efree (f);
      f = fn;
   }
   md->filterlist = NULL;

   efree (sw->Filter);   /* free modul data structure */
   sw->Filter = NULL;
   return;
}




/*
 -- Config Directives
 -- Configuration directives for this Module
 -- return: 0/1 = none/config applied
*/

int configModule_Filter  (SWISH *sw, StringList *sl)

{
  struct MOD_Filter *md = sw->Filter;
  char *w0;
  int  retval;


  w0 = sl->word[0];
  retval = 1;

  if (strcasecmp(w0, "FilterDir")==0) {      /* 1999-05-05 rasc */
      if (sl->n==2) {
          md->filterdir = estrredup(md->filterdir,sl->word[1]);
          if (!isdirectory(md->filterdir)) {
             progerr("%s: %s is not a directory",w0,md->filterdir);
          }
      } else progerr("%s: requires one value",w0);
  }
  else if (strcasecmp(w0, "FileFilter")==0) {  /* 1999-05-05 rasc */
                               /* FileFilter fileextension  filterprog  [options] */
      if (sl->n==3 || sl->n==4) {
          md->filterlist = (struct FilterList *) addfilter(md->filterlist,sl->word[1],sl->word[2],sl->word[3],md->filterdir);
      } else progerr("%s: requires \"extension\" \"filter\" \"[options]\"",w0);
  }
  else {
      retval = 0;	            /* not a filter directive */
  }

  return retval;
}




/*
 -- Add a filter to the filterlist (file ext -> filterprog [cmd-options])
 -- (filterdir may be NULL)
 -- 1999-08-07 rasc
 -- 2001-02-28 rasc
 -- 2001-04-09 rasc   options, maybe NULL  
*/


static struct FilterList *addfilter(struct FilterList *rp,
			 char *suffix, char *prog, char *options, char *filterdir)

{
 struct FilterList *newnode;
 char *buf;
 char *f_dir;
 int ilen1,ilen2;

	newnode = (struct FilterList *) emalloc(sizeof(struct FilterList));
	newnode->suffix= (char *) estrdup(suffix);
	newnode->options = (options) ? (char *) estrdup(options) : NULL;

      /* absolute path and filterdir check  */
	f_dir = (filterdir && (*prog != DIRDELIMITER)) ? filterdir : "";


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
 -- 3002-05-04 rasc  adapted to new module design
*/

struct FilterList *hasfilter (SWISH *sw, char *filename)
{
  struct MOD_Filter *md = sw->Filter;
  struct FilterList *fl;
  char *s, *fe;


   fl = md->filterlist;
   if (! fl) return (struct FilterList *)NULL;
   fe = (filename + strlen(filename));

   while (fl != NULL) {
      s = fe - strlen (fl->suffix);
      if (s >= filename) {   /* no negative overflow! */
         if (! strcasecmp(fl->suffix, s)) {
             return fl;
         }
      }
      fl = fl->next;
   }

   return (struct FilterList *)NULL;
}



/*
  -- open filter (in: file,  out: FILE *)
  -- params are in (FileProp *) - but should be adapted later
  -- Return: FILE *  (filter stream) or NULL
*/

FILE *FilterOpen (FileProp *fprop)
{
  char   *filtercmd;
  struct FilterList *fi;
  char   *cmd_opts, *opt_mask;
  FILE   *fp;
  int    len;

   /*
      -- simple filter call "filter 'work_file' 'real_path_url'"
      -- or call filter "filter  user_param"
      --    > (decoded output =stdout)
   */

   fi = fprop->hasfilter;

// old code (rasc, leave it for checks and speed benchmarks)
//      len =  strlen(fi->prog)+strlen(fprop->work_path)+strlen(fprop->real_path);
//
//      filtercmd=emalloc(len + 6 +1);
//      sprintf(filtercmd, "%s \'%s\' \'%s\'", fi->prog,
//                 fprop->work_path, fprop->real_path);


   /* if no filter cmd param given, use default */

   opt_mask = (fi->options) ? fi->options : "'%p' '%P'";
   cmd_opts = filterCallCmdOptStr (opt_mask, fprop);

   len = strlen (fi->prog) + strlen (cmd_opts);
   filtercmd=emalloc(len + 2);
   sprintf(filtercmd, "%s %s", fi->prog, cmd_opts);

   fp = popen (filtercmd,"r");  /* Open stream */

   efree (filtercmd);
   efree (cmd_opts);

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





/*
  -- process Filter cmdoption parameters
  -- Replace variables with the file/document names.
  -- Variables: please see code below
  -- return: (char *) new string
  -- 2001-04-10 rasc
*/

static char *filterCallCmdOptStr (char *opt_mask, FileProp *fprop)

{
  char *cmdopt, *co, *om;


  cmdopt = (char *)emalloc (MAXSTRLEN * 3);
  *cmdopt = '\0';

  co = cmdopt;
  om = opt_mask;
  while (*om) {

    switch (*om) {

      case '\\':
         *(co++) = charDecode_C_Escape (om, &om);
	   break;

      case '%':
	   ++om;
         co = filterCallCmdOptParam2 (co, *om, fprop);
         if (*om) om++;
         break;       

      default:
         *(co++) = *(om++);
         break;
    }

  }

  *co = '\0';
  return cmdopt;
}


static char *filterCallCmdOptParam2 (char *str, char param, FileProp *fprop)

{
  char *x;


   switch (param) {

      case 'P':              /* Full Doc Path/URL */
           strcpy (str,fprop->real_path);    
           break;

      case 'p':              /* Full Path TMP/Work path */
           strcpy (str,fprop->work_path);    
           break;

      case 'F':              /* Basename Doc Path/URL */
           strcat (str,fprop->real_filename);
           break;

      case 'f':              /* basename Path TMP/Work path */
           strcpy (str,str_basename (fprop->work_path));    
           break;

      case 'D':              /* Dirname Doc Path/URL */
           x = cstr_dirname (fprop->real_path);   
           strcpy (str,x);
           efree (x);
           break;

      case 'd':              /* Dirname TMP/Work Path */
           x = cstr_dirname (fprop->work_path);   
           strcpy (str,x);
           efree (x);
           break;
 
      case '%':              /* %% == print %    */
           *str = param;
           *(str+1) = '\0';
           break;

      default:               /* unknown, print  % and char */
           *str = '%';
           *(str+1) = param;   
           *(str+2) = '\0';
           break;
   }

   while (*str) str++;   /* Pos to end of string */
   return str;
}

