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
** 2001-01-01  rasc    space for change history
**
*/

#include <string.h>
...
#include "module_example.h"


/* private module prototypes */

....



/* 
  -- init structures for #modulename#
*/

void initModule_#modulename# (SWISH  *sw)

{
   struct MOD_#modulename# *md;

      md = (struct MOD_#modulename# *) emalloc(sizeof(struct MOD_#modulename#));
      sw->#modulename# = md;

      md->myparameter = ...
      ...

}


/* 
  -- release structures for #modulename#
  -- release all wired memory
*/

void freeModule_#modulename# (SWISH *sw)

{
   struct MOD_#modulename# *md = sw->#modulename#;

      md->....
      ...


      efree (sw->#modulename#);
      sw->#modulename# = NULL;
}




/*
 -- Config Directives
 -- Configuration directives for this Module
 -- return: 0/1 = none/config applied
*/

int configModule_#modulename# (SWISH *sw, StringList *sl)

{
  struct MOD_#modulename# *md = sw->#modulename#;
  char *w0;
  int  retval;


  w0 = sl->word[0];
  retval = 1;

  if (strcasecmp(w0, "....xxxx....")==0) {
      if (sl->n==2) {
          ... example
             progerr("%s: errormessage ",w0);
          }
      } else progerr("%s: requires one value",w0);
  }
  else if (strcasecmp(w0, ".......xyyyyy...")==0) {
      if (sl->n==3 || sl->n==4) {
        ... example
      } else progerr("%s: requires .....",w0);
  }
  ....... more config directives
  else {
      retval = 0;	            /* not a #modulename# directive */
  }

  return retval;
}





/*
 -- CommandLine options
 -- Set param from cmdline for this Module
*/

int cmdlineModule_#modulename# (SWISH *sw, .....)

{
.......

}





...... start module code here ...


