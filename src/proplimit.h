void initModule_PropLimit (SWISH  *sw);
void freeModule_PropLimit (SWISH *sw);
int Prepare_PropLookup(SWISH *sw );
void SetLimitParameter(SWISH *sw, char *propertyname, char *low, char *hi);
int LimitByProperty( IndexFILE *indexf, int filenum );



