#ifndef SWISH_WORDS_H
#define  SWISH_WORDS_H 1

#ifdef __cplusplus
extern "C" {
#endif


/* internal representation,  may not be changed */
/* BTW, keep strlen(MAGIC_NOT_WORD) >= strlen(NOT_WORD) to avoid string reallocation in the code */
#define AND_WORD       "<and>" 
#define OR_WORD        "<or>"
#define NOT_WORD       "<not>"
#define MAGIC_NOT_WORD "<__not__>"
#define PHRASE_WORD    "<precd>"
#define AND_NOT_WORD   "<andnot>"

/* internal search rule numbers */
#define NO_RULE 0
#define AND_RULE 1
#define OR_RULE 2
#define NOT_RULE 3
#define PHRASE_RULE 4
#define AND_NOT_RULE 5



char *isBooleanOperatorWord( char * word );
struct swline *parse_swish_query( DB_RESULTS *db_results );

void initModule_Swish_Words (SWISH *sw);
void freeModule_Swish_Words (SWISH *sw);


void    stripIgnoreFirstChars(INDEXDATAHEADER *, char *);
void    stripIgnoreLastChars(INDEXDATAHEADER *, char *);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

