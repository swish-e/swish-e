/*
$ID  $

   -- This module does result output for swish...
   -- License: GPL

   -- 2001-01  R. Scherg  (rasc)   initial coding

*/



void initPrintExtResult (SWISH *sw, char *fmt);
void printResultOutput (SWISH *sw);
void printSortedResults (SWISH *sw);
void printExtResultEntry (SWISH *sw, FILE *f, char *fmt, RESULT *r);
char *printResultControlChar (FILE *f, char *s);
char *printTagAbbrevControl (SWISH *sw, FILE *f, char *s, RESULT *r);
char *parsePropertyResultControl (char *s, char **propertyname, char **subfmt);
void printPropertyResultControl (SWISH *sw, FILE *f, char *propname,
				 char *subfmt, RESULT *r);

struct ResultExtFmtStrList *addResultExtFormatStr (
             struct ResultExtFmtStrList *rp, char *name, char *fmtstr);
char *hasResultExtFmtStr (SWISH *sw, char *name);

int resultHeaderOut (SWISH *sw, int min_verbose, char *prtfmt, ...);
void resultPrintHeader (SWISH *sw, int min_verbose, INDEXDATAHEADER *h, 
				char *pathname, int merged);
