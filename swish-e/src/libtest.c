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
*   See the perl/SWISHE.xs file for more detail
*
*/


#include <stdio.h>
#include "swish.h"
#include "error.h"
#include "mem.h"   // for mem_summary only

#define DISPLAY_COUNT 10  // max to display


void show_results( SWISH *swish_handle, int );

int     main(int argc, char **argv)
{
    SWISH  *swish_handle;
    int     num_results;
    int     structure   = IN_FILE;
    char   *properties  = NULL;
    char   *sortspec    = NULL;
    char    input_buf[200];   




    /* Connect to the indexes specified */
   
    swish_handle = SwishInit( argv[1] && *(argv[1]) ? argv[1] : "index.swish-e");

    /* Now, let's send all warnings and error messages to stderr instead of stdout (the default) */
    SwishErrorsToStderr();  /* Global.  Must be set after calling SwishInit() */



    /* how to check for errors */

    if ( SwishError( swish_handle ) )
    {
        /* Now, there's a few ways to do this */

        SwishAbortLastError( swish_handle ); // The easy way.  Format, print message and aborts.

        /* Or for more control */

        {
            int   number  = SwishError( swish_handle );
            char *message = SwishErrorString( swish_handle );
            char *comment = SwishLastErrorMsg( swish_handle );

            fprintf(stderr, "err: Number [%d], Type [%s],  Optional String [%s]\n", number, message, comment );
        }

        /* Now if you want to exit */
        exit(1);

        /* Otherwise, to continue you need to clean up memory */
        SwishClose( swish_handle );
        // return;  // for example
    }


    while ( 1 )
    {
        printf("Enter search words: ");
        if ( !fgets( input_buf, 200, stdin ) )
            break;


        num_results = SwishSearch(swish_handle, input_buf,structure,properties,sortspec);

        /* SwishSearch return:
        *    Number of hits if positive
        *    Zero on no results
        *    A negative number (the error number) on fail.
        */

        if ( num_results >= 0 )
        {
            printf("Total Results: %d\n", num_results );

            if ( num_results > 0 )
                show_results( swish_handle, DISPLAY_COUNT );
        }


        /* Deal with errors */
        
        if ( num_results < 0 && SwishError( swish_handle ) )
        {
            if ( SwishCriticalError( swish_handle ) )
                SwishAbortLastError( swish_handle );
            else
                printf("Error: %s %s\n", SwishErrorString( swish_handle ), SwishLastErrorMsg( swish_handle ) );
        }

        Mem_Summary("At end of loop", 0);

    }

    Mem_Summary("Before Free", 0);
    SwishClose( swish_handle );

    /* Look for memory leaks -- must change settings in mem.h and recompile swish */
    Mem_Summary("At end of program", 1);

    return 0;
    
}

/* Display some standard properties -- see perl/SWISHE.xs for how to get at the data */

void show_results( SWISH *swish_handle, int max_display )
{
    RESULT *result;


    while ( max_display-- && (result = SwishNext( swish_handle )) )
    {

        /* This SwishResultPropertyStr() will work for all types of props */
        /* But SwishResultPropertyULong() can be used to return numeric types */
        /* Should probably check for errors after every call  */
        /* SwishResultPropertyULong will return ULONG_MAX if the value cannot be returned */
        /* that could mean an error, or just that there was not a property assigned (which is not an error) */

        printf("%lu %s '%s' %lu %s\n",
            SwishResultPropertyULong (swish_handle, result, "swishrank" ),
            SwishResultPropertyStr (swish_handle, result, "swishdocpath" ),
            SwishResultPropertyStr (swish_handle, result, "swishtitle"),
            SwishResultPropertyULong (swish_handle, result, "swishdocsize" ),
            SwishResultPropertyStr (swish_handle, result, "swishlastmodified" )
        );
    }

    
}

            

     
    

