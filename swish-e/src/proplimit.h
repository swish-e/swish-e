#ifndef __HasSeenModule_PropLimit
#define __HasSeenModule_PropLimit  1

/* internal use */
void ClearLimitParams( LIMIT_PARAMS *params );
LIMIT_PARAMS *setlimit_params( SWISH *sw, LIMIT_PARAMS *params, char *propertyname, char *low, char *hi );
void ResetLimitParameters( SEARCH_OBJECT *srch );

int Prepare_PropLookup(SEARCH_OBJECT * srch );
int SetLimitParameter(SEARCH_OBJECT *srch, char *propertyname, char *low, char *hi);

int LimitByProperty( IndexFILE *indexf, PROP_LIMITS *prop_limits, int filenum );


#endif



