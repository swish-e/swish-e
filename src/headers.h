#ifndef HEADERS_H
#define HEADERS_H 1

#ifdef __cplusplus
extern "C" {
#endif


/*************************************************************************
*  Index File Header access interface
*
*   Notes:
*       Must match up with the public library interface headers.
*
*
**************************************************************************/


typedef enum {
    SWISH_NUMBER,
    SWISH_STRING,
    SWISH_LIST,
    SWISH_BOOL,
    SWISH_WORD_HASH,
    SWISH_OTHER_DATA,
    SWISH_HEADER_ERROR, /* must check error in this case */
} SWISH_HEADER_TYPE;

typedef union
{
    const char           *string;
    const char          **string_list;
          unsigned long   number;
          int             boolean;
} SWISH_HEADER_VALUE;


void print_index_headers( IndexFILE *indexf );



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* !HEADERS_H */


