#ifndef __HasSeenModule_PropLimit
#define __HasSeenModule_PropLimit  1

void initModule_PropLimit (SWISH  *sw);
void ClearLimitParameter (SWISH *sw);
void freeModule_PropLimit (SWISH *sw);

int Prepare_PropLookup(SWISH *sw );
int SetLimitParameter(SWISH *sw, char *propertyname, char *low, char *hi);
int LimitByProperty( SWISH *sw, IndexFILE *indexf, int filenum );
int is_prop_limit_used( SWISH *sw );

#endif



