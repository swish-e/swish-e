/*
$Id$
**
** This program and library is free software; you can redistribute it and/or
** modify it under the terms of the GNU (Library) General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU (Library) General Public License for more details.
**
** You should have received a copy of the GNU (Library) General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
**---------------------------------------------------------
*
*   Example program for interfacing a C program with the Swish-e C library.
*
*   ./libtest [optional index file]
*
*   use quotes for more than one file
*       ./libtest index.swish-e
*       ./libtest 'index1 index2 index3'
*
*   See the perl/API.xs file for more detail
*
*/


#include <stdio.h>
#include "swish-e.h"  /* use locally for testing */



#define MEM_TEST 1

#ifdef MEM_TEST
#include "mem.h"   // for mem_summary only
#endif



#define DISPLAY_COUNT 10  // max to display


static void display_results( SW_HANDLE, SW_RESULTS );
static void print_error_or_abort( SW_HANDLE swish_handle );

static void print_index_headers( SW_HANDLE swish_handle, SW_RESULTS results );
static void print_header_value( SW_HANDLE swish_handle, const char *name, SWISH_HEADER_VALUE head_value, SWISH_HEADER_TYPE head_type );
static void demo_stemming( SW_RESULTS results );
static void stem_it( SW_RESULT r, char *word );


int     main(int argc, char **argv)
{
    SW_HANDLE   swish_handle = NULL;    /* Database handle */
    SW_SEARCH   search = NULL;          /* search handle -- holds search parameters */
    SW_RESULTS  results = NULL;         /* results handle -- holds list of results */

    char    input_buf[200];
    char   *index_file_list;



    SwishErrorsToStderr();      /* Send any errors or warnings to stderr (default is stdout) */

    /* Connect to the indexes specified */

    index_file_list = argv[1] && *(argv[1]) ? argv[1] : "index.swish-e";

    swish_handle = SwishInit( index_file_list );


    /* Check for errors after every call */

    if ( SwishError( swish_handle ) )
        print_error_or_abort( swish_handle );  /* print an error or abort -- see below */


    /* Here's a short-cut to searching that creates a search object and searches at the same time */

    results = SwishQuery( swish_handle, "foo OR bar" );

    if ( SwishError( swish_handle ) )
        print_error_or_abort( swish_handle );  /* print an error or abort -- see below */
    else
    {
        display_results( swish_handle, results );

        printf( "Testing SW_ResultsToSW_HANDLE() = '%s'\n",
            SW_ResultsToSW_HANDLE( results ) == swish_handle ? "OK" : "Not OK" );

        demo_stemming( results );

        Free_Results_Object( results );
    }

    /* This may change since it only supports 8-bit chars */
    {
        const char *words = SwishWordsByLetter( swish_handle, "index.swish-e", 'f' );
        char *tmp = (char *)words;
        printf("Words that begin with 'f': ");
        for(;tmp && tmp[0]; tmp += strlen(tmp)+1 )
            printf("%s \n", tmp);

        printf("\n");
    }

    /* 
     * Stem a word -- this method is somewhat depreciated.
     * It stores the stemmed word in a single location in the SW_OBJECT
     */

    {
        char *stemmed = SwishStemWord( swish_handle, "running" );
        printf("SwishStemWord 'running' => '%s'\n\n", stemmed ? stemmed : "Failed to stem" );
    }


    /* Typical use of the library is to create a search object */
    /* and use the search object to make multiple queries */

    /* Create a search object for searching - the query string is optional */
    /* Remember to free the search object when done */

    search = New_Search_Object( swish_handle, "foo" );




    /* Adjust some of the search parameters if different than the defaults */
    SwishSetSort( search, "swishtitle" );

    // SwishSetStructure( search, IN_TITLE );  /* limit to title only */

    /* Set Limit parameters like */

    /*****

    SwishSetSearchLimit( search, "swishtitle", "a", "z" );
    SwishSetSearchLimit( search, "age", "18", "65" );

    if ( SwishError( swish_handle ) )  // e.g. can't define two limits for same prop name
        print_error_or_abort( swish_handle );

    // use SwishResetLimit() if wish to change the parameters on a active search object        

    *****/


    /* Now we are ready to search  */


    while ( 1 )
    {
        printf("Enter search words: ");
        if ( !fgets( input_buf, 200, stdin ) )
            break;


        results = SwishExecute( search, input_buf );

        /* check for errors */

        if ( SwishError( swish_handle ) )
        {
            print_error_or_abort( swish_handle );

            if ( results ) /* probably always true */
                Free_Results_Object( results );

            continue;
        }

        display_results( swish_handle, results );
        Free_Results_Object( results );

#ifdef MEM_TEST
        /* It's expected to see some memory used here since a swish_handle exists */
        Mem_Summary("End of loop", 1);
#endif

    }

    Free_Search_Object( search );
    SwishClose( swish_handle );


    /* Look for memory leaks -- configure swish-e with --enable-memtrace to use */
#ifdef MEM_TEST
    Mem_Summary("At end of program", 1);
#endif

    return 0;
}

/* Display some standard properties -- see perl/SWISHE.xs for how to get at the data */

static void display_results( SW_HANDLE swish_handle, SW_RESULTS results )
{
    SW_RESULT result;
    int       hits;
    int       first = 1;

    if ( !results )  /* better safe than sorry */
        return;



    /* Display the set of headers for the index(es) */
    print_index_headers( swish_handle, results );


    hits = SwishHits( results );

    if ( 0 == hits )
    {
        printf("no results!\n");
        return;
    }


    printf("# Total Results: %d\n", hits );




    if ( SwishSeekResult(results, 0 ) < 0 )  // how to seek to a page of results
    {
        print_error_or_abort( swish_handle );  /* seek past end of file */
        return;
    }

   

    while ( (result = SwishNextResult( results )) )
    {

        /* This SwishResultPropertyStr() will work for all types of props */
        /* But SwishResultPropertyULong() can be used to return numeric types */
        /* Should probably check for errors after every call  */
        /* SwishResultPropertyULong will return ULONG_MAX if the value cannot be returned */
        /* that could mean an error, or just that there was not a property assigned (which is not an error) */

        printf("Path: %s\n  Rank: %lu\n  Size: %lu\n  Title: %s\n  Index: %s\n  Modified: %s\n  Record #: %lu\n  File   #: %lu\n\n",
            SwishResultPropertyStr   ( result, "swishdocpath" ),
            SwishResultPropertyULong ( result, "swishrank" ),
            SwishResultPropertyULong ( result, "swishdocsize" ),
            SwishResultPropertyStr   ( result, "swishtitle"),
            SwishResultPropertyStr   ( result, "swishdbfile" ),
            SwishResultPropertyStr   ( result, "swishlastmodified" ),
            SwishResultPropertyULong ( result, "swishreccount" ),  /* can figure this out in loop, of course */
            SwishResultPropertyULong ( result, "swishfilenum" )
        );



        /* Generally not useful, but also can lookup Index header data via the current result */
        {
            SWISH_HEADER_VALUE header_value;
            SWISH_HEADER_TYPE  header_type;
            const char *example = "WordCharacters";
            
            header_value = SwishResultIndexValue( result, example, &header_type );
            print_header_value( swish_handle, example, header_value, header_type );
        }

        if ( first )
        {
            printf( "Testing SW_ResultToSW_HANDLE() = '%s'\n",
                SW_ResultToSW_HANDLE( result ) == swish_handle ? "OK" : "Not OK" );

            first = 0;
        }
            
    }

    
}



/**********************************************************************
* print_index_headers
*
*   This displays the standard headers associated with an index
*
*   Pass in:
*       swish_handle -- for standard headers
*
*   Note:
*       The SWISH_HEADER value, and the data it points to, is only
*       valid during the current call.
*
*
***********************************************************************/

static void print_index_headers( SW_HANDLE swish_handle, SW_RESULTS results )
{
    const char **header_names = SwishHeaderNames(swish_handle);  /* fetch the list of available header names */
    const char **index_name = SwishIndexNames( swish_handle );
    SWISH_HEADER_VALUE header_value;
    SWISH_HEADER_TYPE  header_type;

    /* display for each index */

    while ( *index_name )
    {
        const char **cur_header = header_names;

        while ( *cur_header )
        {
            header_value = SwishHeaderValue( swish_handle, *index_name, *cur_header, &header_type );
            print_header_value( swish_handle, *cur_header, header_value, header_type );


            cur_header++;  /* move to next header name */
        }


        /* Now print out results-specific data */

        header_value = SwishParsedWords( results, *index_name );
        print_header_value( swish_handle, "Parsed Words", header_value, SWISH_LIST );

        header_value = SwishRemovedStopwords( results, *index_name );
        print_header_value( swish_handle, "Removed Stopwords", header_value, SWISH_LIST );


        index_name++;  /* move to next index file */
    }
}

static void print_header_value( SW_HANDLE swish_handle, const char *name, SWISH_HEADER_VALUE head_value, SWISH_HEADER_TYPE head_type )
{
    const char **string_list;
    
    printf("# %s:", name );

    switch ( head_type )
    {
        case SWISH_STRING:
            printf(" %s\n", head_value.string ? head_value.string : "" );
            return;

        case SWISH_NUMBER:
            printf(" %lu\n", head_value.number );
            return;

        case SWISH_BOOL:
            printf(" %s\n", head_value.boolean ? "Yes" : "No" );
            return;

        case SWISH_LIST:
            string_list = head_value.string_list;
            
            while ( *string_list )
            {
                printf(" %s", *string_list );
                string_list++;
            }
            printf("\n");
            return;

        case SWISH_HEADER_ERROR:
            print_error_or_abort( swish_handle );
            return;

        default:
            printf(" Unknown header type '%d'\n", (int)head_type );
            return;
    }
}




/*************************************************************
*  print_error_or_abort -- display an error message / abort
*
*   This displays the error message, and aborts if it's a critical
*   error.  This is overkill -- normally a critical error means
*   that the you should call SwishClose() and start over.
*
*   On searches means that the search could not be completed
*
*
**************************************************************/

static void print_error_or_abort( SW_HANDLE swish_handle )
{
    if ( !SwishError( swish_handle ) )
        return;

    /* On critical errors simply exit -- normally you would SwishClose() and loop */

    if ( SwishCriticalError( swish_handle ) )
       SwishAbortLastError( swish_handle );   /* prints message and exits */


    /* print a message */        
    fprintf(stderr,
        "err: Number [%d], Type [%s],  Optional Message: [%s]\n",
        SwishError( swish_handle ),
        SwishErrorString( swish_handle ),
        SwishLastErrorMsg( swish_handle )
    );
}

/*
 * This shows how to use the stemmer based on a result.
 * It's done this way because a result is related to a
 * specific index (where a result list may contain results
 * from many indexes).
 * Typically, the stemmer is used at search time to highlight words
 * so it would be based on a given result.
 */

static void demo_stemming( SW_RESULTS results )
{
    SW_RESULT r;

    printf("\n-- Stemmer Test --\n");


    if ( !SwishHits( results ) )
    {
        printf("Couldn't test stemming because search returned no results\n");
        return;
    }

    if (SwishSeekResult( results, 0) )
    {
        printf("Failed to seek to result 0\n");
        return;
    }
    r = SwishNextResult( results );

    if ( !r )
    {
        printf("Failed to get first result\n");
        return;
    }

    printf("Fuzzy Mode: %s\n", SwishFuzzyMode( r ) );

    stem_it( r, "running" );
    stem_it( r, "runs" );
    stem_it( r, "12345" );
    stem_it( r, "abc3def" );
    stem_it( r, "");
    stem_it( r, "sugar" );  /* produces two metaphones */
}

static void stem_it( SW_RESULT r, char *word )
{
    const char **word_list;
    SW_FUZZYWORD fw;

    printf(" [%s] : ", word );
    
    fw = SwishFuzzyWord( r, word );
    printf(" Status: %d", SwishFuzzyWordError(fw) );
    printf(" Word Count: %d\n", SwishFuzzyWordCount(fw) );

    printf("   words:");
    word_list = SwishFuzzyWordList( fw );
    while ( *word_list )
    {
        printf(" %s", *word_list );
        word_list++;
    }
    
    printf("\n");

    SwishFuzzyWordFree( fw );
}

