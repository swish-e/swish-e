/* 
added getrankIDF and getrankDEF to allow multiple ranking schemes
karman Mon Aug 30 07:01:31 CDT 2004
*/
#ifndef RANK_H
#define RANK_H 1

/* #define DEBUG_RANK 1 */

#include "mem.h"
#include "search.h"

int getrank( RESULT *r );
int getrankDEF( RESULT *r );
int getrankIDF( RESULT *r );
int scale_word_score( int score );

#endif
