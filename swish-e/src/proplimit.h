#ifndef __HasSeenModule_PropLimit
#define __HasSeenModule_PropLimit  1

void ClearLimitParameter (LIMIT_PARAMS *params);

int Prepare_PropLookup(SEARCH_OBJECT * srch );
int SetLimitParameter(SEARCH_OBJECT *srch, char *propertyname, char *low, char *hi);
int LimitByProperty( IndexFILE *indexf, int filenum );

#endif



