struct swline *tokenize_query_string( SWISH *sw, char *words, INDEXDATAHEADER *header );
char *isBooleanOperatorWord( char * word );

void initModule_Swish_Words (SWISH  *sw);
void freeModule_Swish_Words (SWISH *sw);

