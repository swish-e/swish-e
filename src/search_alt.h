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
** 2001-03-02 rasc   initial coding
*/


#ifndef __HasSeenModule_SearchAlt
#define __HasSeenModule_SearchAlt	1


/*
  -- internal logical/boolean search operator words 
  -- Defaults for user: see config.h
  --  or changed via config directives.
*/


typedef struct {                /* 2001-03-12 rasc */
    char   *and;                /* Logical Search  */
    char   *or;                 /* Operators (user) */
    char   *not;
    int     defaultrule;        /* missing op == this rule */
}
LOGICAL_OP;



/* Global module data */

struct MOD_SearchAlt {
   /* public:  */
    int     enableAltSearchSyntax;         /* Alternate search strings 0/1  */
    LOGICAL_OP srch_op;                    /* search operator words         */

   /* private: don't use outside this module! */
   /* none */
};



/* internal representation,  may not be changed */
#define AND_WORD "<and>"
#define OR_WORD "<or>"
#define NOT_WORD "<not>"
#define PHRASE_WORD "<precd>"
#define AND_NOT_WORD "<andnot>"

/* internal search rule numbers */
#define NO_RULE 0
#define AND_RULE 1
#define OR_RULE 2
#define NOT_RULE 3
#define PHRASE_RULE 4
#define AND_NOT_RULE 5





/* exported Prototypes */

void initModule_SearchAlt   (SWISH *sw);
void freeModule_SearchAlt   (SWISH *sw);
int  configModule_SearchAlt (SWISH *sw, StringList *sl);

char *convAltSearch2SwishStr (char *str);


#endif

