/*
$Id$

*/

/*
   seems to be a very old module of swish
   some serious work to do in html.c and html.h!! 
   but anyway it seems to work...
 */


/* Just the prototypes */

int countwords_HTML(SWISH *, FileProp *, char *buffer);
int parsecomment (SWISH *, char *, int, int, int, int *);
char   *parsetitle(char *buffer);
