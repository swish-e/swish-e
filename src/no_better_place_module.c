/*
$Id$
**
** Swish code is going to be reorganized...
** Better module design, better structure and better code,
** better routine names and more comments (hopefully).
**
**
** This module is a  TMP MODULE   for code reorganization.
**
**
** Routines which were wrongly in other modules or are currently
** not yet assigned to a module will be tmp. moved here.
**
**
**
**
**  == when moveing a module her, shortly do a remark:
**  ==  -- why here
**  ==  -- from where 
**  ==  -- what are the plans with this routine
**  ==  -- who did it
**
**
**  05/2001  rasc
 */


#include "swish.h"

#include "swstring.h"
#include "mem.h"
#include "merge.h"
#include "metanames.h"
#include "search.h"
#include "docprop.h"
#include "result_output.h"



/*---------------------------------*/



  // rasc (2001-05-14):
  // From: result_output.c
  // Why:  totally wrong there...
  // this routine has to be moved to a new module.
  // because resultoutput has nothing to do with
  // "how to get" the index translate information.
  // (keep the new module concept tidy and clean!)

  // --> maybe there will be a new module word_rewrite.c

  // $$$ routine description missing (who ever did it)


void	translatecharHeaderOut (SWISH *sw,  int v, INDEXDATAHEADER *h )
{
    int   from_pos = 0;
    int   to_pos = 0;
    int   len,
          i;
    char *from,
         *to;

    len  = sizeof(h->translatecharslookuptable) / sizeof(int);
    from = (char *) emalloc(len+1);
    to   = (char *) emalloc(len+1);

    for ( i = 0; i < len; i++ )
    {
        if ( h->translatecharslookuptable[i] == i )
            continue;

        /* what escaping should be done - and how to (re)allocate memory */
        from[from_pos++] = (unsigned char) i;
        to[to_pos++]     = (unsigned char) h->translatecharslookuptable[i];
    }

    from[from_pos] = '\0';
    to[from_pos] = '\0';

    if ( *from )
        resultHeaderOut (sw,v, "%s %s -> %s\n", TRANSLATECHARTABLEHEADER, from, to );

    efree( from );
    efree( to );
            
}        










