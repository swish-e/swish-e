/*
$Id$
**
** Copyright (C) 1995, 1996, 1997, 1998 Hewlett-Packard Company
**
** Originally by Kevin Hughes, kev@kevcom.com, 3/11/94
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
*/


#include <stdio.h>
#include "swish.h"
#include "error.h"
#include "mem.h"

int     main(int argc, char **argv)
{
    SWISH  *swish_handle;
    int     num_results;
    int     structure   = IN_FILE;
    char   *properties  = NULL;
    char   *sortspec    = NULL;

    if ( argc < 2 )
    {
        fprintf(stderr, "err: Enter a search word\n");
        exit(1);
    }

    /* This would have likely been a fatal error and not return */
    if ( !(swish_handle = SwishOpen("index.swish-e index.swish-e")) )
    {
        fprintf(stderr, "err: Failed to create swish handle\n");
        exit(1);
    }

    num_results = SwishSearch(swish_handle, argv[1],structure,properties,sortspec);


    /* Or would it be better to call abort_last_error() */
    /* These require error.h */
    
    if ( num_results <= 0 )
    {
        int     error_num = SwishError( swish_handle );
        char   *error_str = SwishErrorString( error_num );
        char   *last_error = SwishLastError( swish_handle );

        fprintf(stderr, "err: (%d) %s: %s\n", error_num, error_str, last_error);
        exit(1);

        /* or optionally - but doesn't print the error number */
        abort_last_error( swish_handle );
    }

    printf("Total Results: %d\n", num_results );

    SwishClose( swish_handle );

    Mem_Summary("At end of program", 1);

    return 0;
    
}

