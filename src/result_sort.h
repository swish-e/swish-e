

int compResultsByNonSortedProps(const void *,const void *);
int compResultsBySortedProps(const void *,const void *);




char **getResultSortProperties(RESULT *);

int initSortResultProperties (SWISH *);

void sortFileProperties(IndexFILE *indexf);

RESULT *addsortresult(SWISH *, RESULT *sp, RESULT *);
int sortresults (SWISH *, int );


