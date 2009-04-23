/*
$Id$
**


    This file is part of Swish-e.

    Swish-e is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Swish-e is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along  with Swish-e; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    
    See the COPYING file that accompanies the Swish-e distribution for details
    of the GNU GPL and the special exception available for linking against
    the Swish-e library.
    
** Mon May  9 18:19:34 CDT 2005
** added GPL


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
    SWINT_T     defaultrule;        /* missing op == this rule */
}
LOGICAL_OP;



/* Global module data */

struct MOD_SearchAlt {
   /* public:  */
    SWINT_T     enableAltSearchSyntax;         /* Alternate search strings 0/1  */
    LOGICAL_OP srch_op;                    /* search operator words         */

   /* private: don't use outside this module! */
   /* none */
};






/* exported Prototypes */

void initModule_SearchAlt   (SWISH *sw);
void freeModule_SearchAlt   (SWISH *sw);

SWINT_T     u_SelectDefaultRulenum(SWISH * sw, char *word);

#endif

