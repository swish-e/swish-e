#ifndef SWISH_WORDS_H
#define  SWISH_WORDS_H 1

#ifdef __cplusplus
extern "C" {
#endif

char *isBooleanOperatorWord( char * word );
struct swline *parse_swish_query( DB_RESULTS *db_results );

void initModule_Swish_Words (SWISH *sw);
void freeModule_Swish_Words (SWISH *sw);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

