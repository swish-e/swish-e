
/*
   -- This module does result output for swish...
   -- License: GPL

   -- 2001-01  R. Scherg  (rasc)   initial coding

*/



void printsortedresults (SWISH *sw);
void print_ext_resultentry (SWISH *sw, FILE *f, char *fmt, RESULT *r);
char *printResultControlChar (FILE *f, char *s);
char *printTagAbbrevControl (SWISH *sw, FILE *f, char *s, RESULT *r);
char *parsePropertyResultControl (char *s, char **propertyname, char **subfmt);
char *skip_ws (char *s);
void printPropertyResultControl (SWISH *sw, FILE *f, char *propname,
				 char *subfmt, RESULT *r);

