/* 
fs.h
*/

void indexadir(SWISH *, char *);
void indexafile(SWISH *, char *);
void printfile(SWISH *, DOCENTRY *);
void printfiles(SWISH *, DOCENTRYARRAY *);
void printdirs(SWISH *, DOCENTRYARRAY *);
int ishtml(SWISH *,char *);
int isoktitle(SWISH *,char *);

