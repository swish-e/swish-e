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
**  This file handles alternat search syntax handling
**   e.g.  "+word -word word"   (basic altavista, lycos, etc mode...)
**         word1  UND  word2    (language specific boolean operators)
**
**  
**
** 2001-03-02 rasc   initial coding
** 2001-04-12 rasc   Module init rewritten
**
*/


#include <string.h>
#include "swish.h"
#include "string.h"
#include "mem.h"
#include "error.h"
#include "parse_conffile.h"
#include "search.h"
#include "search_alt.h"





/* 
  -- init structures for this module
*/

void initModule_SearchAlt (SWISH  *sw)

{
   	sw->enableAltSearchSyntax = 0;	/* default: enable only Swish syntax */

	/* init default logical operator words (2001-03-12 rasc) */
	sw->srch_op.and = estrdup (_AND_WORD);
	sw->srch_op.or  = estrdup (_OR_WORD);
	sw->srch_op.not = estrdup (_NOT_WORD);
	sw->srch_op.defaultrule = AND_RULE;	

   return;
}


/* 
  -- release all wired memory for this module
*/

void freeModule_SearchAlt (SWISH *sw)

{
	/* free logical operator words   (2001-03-12 rasc) */
	efree (sw->srch_op.and);
	efree (sw->srch_op.or);
	efree (sw->srch_op.not);

  return;
}






/* ---------------------------------------------- */




/*
 -- Config Directives
 -- Configuration directives for this Module
 -- return: 0/1 = none/config applied
*/

int configModule_SearchAlt  (SWISH *sw, StringList *sl)

{
  char *w0    = sl->word[0];
  int  retval = 1;

                /* $$$ this will not work unless swish is reading the config file also for search ... */

  if (strcasecmp(w0, "EnableAltSearchSyntax")==0) {         /* rasc 2001-02 */
	sw->enableAltSearchSyntax = getYesNoOrAbort (sl, 1,1);
  } else if (strcasecmp(w0, "SwishSearchOperators")==0) {   /* rasc 2001-03 */
	if(sl->n == 4) {
	   sw->srch_op.and = sl->word[1];
	   sw->srch_op.or = sl->word[2];
	   sw->srch_op.not = sl->word[3];
	} else progerr("%s: requires 3 parameters (and-, or-, not-word)",w0);
  }
  else if (strcasecmp(w0, "SwishSearchDefaultRule")==0) {   /* rasc 2001-03 */
	if(sl->n == 2) {
	   sw->srch_op.defaultrule = u_SelectDefaultRulenum(sw,sl->word[1]);
	   if (sw->srch_op.defaultrule == NO_RULE) {
		progerr("%s: requires \"%s\" or \"%s\"",w0, sw->srch_op.and, sw->srch_op.or);
	   }
	} else progerr("%s: requires 1 parameter",w0);
  }
  else {
      retval = 0;	            /* not a module directive */
  }


  return retval;
}





/* ---------------------------------------------- */



/* 
  -- a simple routine to convert a alternative (+-) search string
  -- "word1 +word2 +word3  word4 -word5" 
  -- to a swish-e search string... 
  -- only a basic altavista/lycos/etc. syntax conversion is done
  -- Return: (char *) swish-search-string
*/

char *convAltSearch2SwishStr (char *str) 
{
  StringList *slst;
  char   *sr_new, *p, *tmp;
  char   *sr_or,*sr_and,*sr_not;
  char   **s_or, **s_and, **s_not;  
  int    n_or, n_and, n_not;
  int    i;


  sr_new = (char *)emalloc (2 * MAXSTRLEN);
  sr_or  = (char *)emalloc (2 * MAXSTRLEN);
  sr_and = (char *)emalloc (2 * MAXSTRLEN);
  sr_not = (char *)emalloc (2 * MAXSTRLEN);
  tmp    = (char *)emalloc (2 * MAXSTRLEN);
  *sr_new = *sr_or = *sr_and = *sr_not = '\0';


  slst = parse_line (str);     /* parse into words */

  i = (slst->n + 1) * sizeof (char *);
  s_or  = (char **)emalloc (i);
  s_and = (char **)emalloc (i);
  s_not = (char **)emalloc (i);
  *s_or = *s_and = *s_not = (char *)NULL;
  n_or = n_and = n_not = 0;


	/* parse string into and, not, or */

  for (i=0; i < slst->n; i++) {
     p = slst->word[i];
     switch (*p) {
       case '-':            /* Exclude, Not */
		*(s_not + (n_not++)) = p+1;
            break;

       case '+':            /* AND  */
		*(s_and + (n_and++)) = p+1;
            break;  

       default:             /* OR   */
		*(s_or + (n_or++)) = p;
            break;
     }
  }
  *(s_and +(n_and)) = NULL;
  *(s_or  +(n_or))  = NULL;
  *(s_not +(n_not)) = NULL;


  /* 
    -- process or-list  (word word) 
    -- if also "+str" are present include these into "or"-list
    -- this is necessary  ($$$ to be checked!)
  */

   if (*s_or) {
     for (i=0; i<n_or; i++) {
       if (*sr_or) {     /* not first word */
          sprintf (tmp," %s ",OR_WORD);
          strcat (sr_or,tmp);
       }
       strcat (sr_or, *(s_or + i));
     }
     /* -- include And-words (if any) into OR-list */ 
     for (i=0; i<n_and; i++) {
       sprintf (tmp," %s %s",OR_WORD,*(s_and+i));
       strcat (sr_or,tmp);
     }
   }


  /* 
    -- process and-list  (+word +word) 

  */

     for (i=0; i<n_and; i++) {
       if (*sr_and) {     /* not first word */
          sprintf (tmp," %s ",AND_WORD);
          strcat (sr_and,tmp);
       }
       strcat (sr_and, *(s_and + i));
     }


  /* 
    -- process not-list  (-word -word) 

  */

     for (i=0; i<n_not; i++) {
       if (*sr_not) {     /* not first word */
          sprintf (tmp," %s ",AND_WORD);
          strcat (sr_not,tmp);
       }
       strcat (sr_not,*(s_not + i));
     }


  /*
   -- now build one new searchstring 
   -- merge and, or, not strings with AND (if not empty)
   -- build something like:
   --    (orw <or> ... ) <AND> (andw <and> ...) not (notw <and> ...) 
  */

  if (*sr_or) {
    sprintf (sr_new,"(%s)",sr_or);
  }
  if (*sr_and) {
    if (*sr_new) {
       sprintf (tmp," %s %s (%s)",sr_new,AND_WORD,sr_and);
    } else {
       sprintf (tmp,"(%s)",sr_and);
    }
    strcpy (sr_new,tmp);
  }
  if (*sr_not) {
    if (*sr_new) {
       sprintf (tmp," %s %s (%s)",sr_new,NOT_WORD,sr_not);
    } else {
       sprintf (tmp,"%s (%s)",NOT_WORD,sr_not);
    }
    strcpy (sr_new,tmp);
  }



  efree (s_or);
  efree (s_and);
  efree (s_not);
  efree (tmp);
  freeStringList (slst);
  return sr_new;
}

