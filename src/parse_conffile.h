/*
$Id$
**
**
*/


void grabCmdOptions(StringList *sl, int start, struct swline **listOfWords);
void getdefaults(SWISH *sw, char *conffile, int *hasdir, int *hasindex, int hasverbose);
int getYesNoOrAbort (StringList *sl, int n, int islast);
int getDocTypeOrAbort (StringList *sl, int n);

void readstopwordsfile(SWISH *, IndexFILE *, char *);
void readusewordsfile(SWISH *, IndexFILE *, char *);
int parseconfline(SWISH *, StringList *);
void readbuzzwordsfile(SWISH *, IndexFILE *, char *);

void free_Extracted_Path( SWISH *sw );
void free_regex_list( regex_list **reg_list );



