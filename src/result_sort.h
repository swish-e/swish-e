

int compResultsByNonSortedProps(const void *,const void *);
int compResultsBySortedProps(const void *,const void *);




char **getResultSortProperties(RESULT *);

int initSortResultProperties (SWISH *);

void sortFileProperties(IndexFILE *indexf);

RESULT *addsortresult(SWISH *, RESULT *sp, RESULT *);
int sortresults (SWISH *, int );

int sw_strcasecmp(unsigned char *,unsigned char *, int *);
int sw_strcmp(unsigned char *,unsigned char *, int *);
void initModule_ResultSort (SWISH *);
void freeModule_ResultSort (SWISH *);
