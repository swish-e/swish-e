#ifndef SEARCHSWISH_H
#define SEARCHSWISH_H 1

#ifdef __cplusplus
extern "C" {
#endif

typedef void * SW_HANDLE;
typedef void * SW_SEARCH;
typedef void * SW_RESULTS;
typedef void * SW_RESULT;


#ifdef DISPLAY_HEADERS


typedef enum {
    SWISH_LONG,
    SWISH_STRING,
    SWISH_LIST,
    SWISH_BOOL
} SWISH_HEADER_TYPE;

union
{
    const char           *string;
    const char          **string_list;
    const unsigned long   number;
    const int             boolean;
} s_SWISH_HEADER_VALUE;

typedef s_SWISH_HEADER_VALUE SWISH_HEADER_VALUE;

const char **SwishHeaderNames(void);  /* fetch the list of available header names */
const char **SwishIndexNames( SW_HANDLE );

SWISH_HEADER_VALUE SwishResultIndexValue( SW_RESULT, const *char name, int *type );
SWISH_HEADER_VLLUE SwishHeaderValue( const char *index_name, const  char *cur_header, int *type );


#endif




/* Limit searches by structure */


#define IN_TITLE_BIT    1
#define IN_HEAD_BIT     2
#define IN_BODY_BIT     3
#define IN_COMMENTS_BIT 4
#define IN_HEADER_BIT   5
#define IN_EMPHASIZED_BIT   6
#define IN_META_BIT     7
#define STRUCTURE_END 7


#define IN_FILE         (1<<IN_FILE_BIT)
#define IN_TITLE        (1<<IN_TITLE_BIT)
#define IN_HEAD         (1<<IN_HEAD_BIT)
#define IN_BODY         (1<<IN_BODY_BIT)
#define IN_COMMENTS     (1<<IN_COMMENTS_BIT)
#define IN_HEADER       (1<<IN_HEADER_BIT)
#define IN_EMPHASIZED (1<<IN_EMPHASIZED_BIT)
#define IN_META         (1<<IN_META_BIT)
#define IN_ALL (IN_FILE|IN_TITLE|IN_HEAD|IN_BODY|IN_COMMENTS|IN_HEADER|IN_EMPHASIZED|IN_META)



SW_HANDLE  SwishInit(char *);
SW_RESULTS SwishQuery(SW_HANDLE, char *words );

SW_SEARCH New_Search_Object( SW_HANDLE, char *query );
void SwishSetStructure( SW_SEARCH srch, int structure );
void SwishPhraseDelimiter( SW_SEARCH srch, char delimiter );
void SwishSetSort( SW_SEARCH srch, char *sort );

int SwishSetSearchLimit( SW_SEARCH srch, char *propertyname, char *low, char *hi);
void SwishResetSearchLimit( SW_SEARCH srch );

SW_RESULTS SwishExecute( SW_SEARCH, char *optional_query );

int SwishHits( SW_RESULTS );

int SwishSeekResult( SW_RESULTS, int position );
SW_RESULT SwishNextResult( SW_RESULTS );

char *SwishResultPropertyStr(SW_RESULT, char *propertyname);
unsigned long SwishResultPropertyULong(SW_RESULT, char *propertyname);

void Free_Search_Object( SW_SEARCH srch );
void Free_Results_Object( SW_RESULTS results );


void SwishClose( SW_HANDLE );


int  SwishError( SW_HANDLE );           /* test if error state - returns error number */
int  SwishCriticalError( SW_HANDLE );   /* true if show stopping error */
void SwishAbortLastError( SW_HANDLE );  /* format and abort the error message */

char *SwishErrorString( SW_HANDLE );    /* string for the error number */
char *SwishLastErrorMsg( SW_HANDLE );   /* more specific message about the error */

void set_error_handle( FILE *where );
void SwishErrorsToStderr( void );





#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* !SEARCHSWISH_H */




