/*
 * This program connects to the http server and perform a fetch
 * of the URL.
 *
 * Usage:
 *      $ fetch_http <URL> [outputfile] [-b][-s][-q][-h][-Hheader]
 *                                      [-a username:password]
 *                                      [-p proxygateway[:port]]
 *                                      [-f inputfile]
 *
 *
 * Arguments:
 *      URL             URL to fetch.
 *      outputfile      File to store result in.
 *
 * Options:
 *	-b		If specified, use 'binary' mode to save file.
 *	-s		If specified, use HTTP simplerequest format for query.
 *	-q		If specified, just return response header.
 *	-H		If specified, take rest of line to send as additional
 *                      header to remote site prior to fetching file.
 *	-h		If specified, print usage message.
 *
 * Author:      David Jones
 * Revised:     13-JAN-1995 GJC@VILLAGE.COM. Add file output argument.
 * Revised:      9-MAR-1995 Fixed bug in non-binary output to file.
 * Revised:     28-AUG-1996 Tim Adye <T.J.Adye@rl.ac.uk>. Add -h and -q opts.
 * Revised:     30-AUG-1996 levitte@lp.se. Add header switch.
 * Revised:      5-SEP-1996 Michael Lemke <ai26@a400.sternwarte.uni-erlangen.de>
 *               Merged last two independent revisions into one file.
 * Revised:	15-APR-1999 Richard Levitte <levitte@lp.se>
 *			    Merged three versions together.
 * Archived:    http://www.stacken.kth.se/cgi-bin/cvsweb.cgi/osu/http_server/base_code/fetch_http.c?cvsroot=RemoteRepository
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef VMS
#include <unixio.h>
#include <types.h>
#include <socket.h>
#include <in.h>
#else
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#include <errno.h>
#include <netdb.h>

static char six2pr[64] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};

static int BASE64_encode(unsigned char *bufin, unsigned int nbytes,
			 char *bufcoded)
{
/* ENC is the basic 1 character encoding function to make a char printing */
#define ENC(c) six2pr[c]

    register char *outptr = bufcoded;
    unsigned int i;

    for (i=0; i<nbytes; i += 3) {
	*(outptr++) = ENC(*bufin >> 2);            /* c1 */
	*(outptr++) = ENC(((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)); /*c2*/
	*(outptr++) = ENC(((bufin[1] << 2) & 074) | ((bufin[2] >> 6) & 03));/*c3*/
	*(outptr++) = ENC(bufin[2] & 077);         /* c4 */

	bufin += 3;
    }

    /* If nbytes was not a multiple of 3, then we have encoded too
     * many characters.  Adjust appropriately.
     */
    if(i == nbytes+1) {
	/* There were only 2 bytes in that last group */
	outptr[-1] = '=';
    } else if(i == nbytes+2) {
	/* There was only 1 byte in that last group */
	outptr[-1] = '=';
	outptr[-2] = '=';
    }
    *outptr = '\0';
    return(outptr - bufcoded);
}

void usage ()
{
    printf("\
This program connects to the http server and perform a fetch\n\
of the file specified by the URL.\n\
\n\
Usage:\n\
     $ fetch_http URL [outputfile] [-b][-s][-q][-h][-Hheader]\n\
                                   [-a username:password]\n\
                                   [-p proxygateway[:port]]\n\
                                   [-f inputfile]\n\
\n\
Arguments:\n\
     URL             URL to fetch.\n\
     outputfile      File to store result in.\n\
\n\
Options:\n\
     -b              If specified, use 'binary' mode to save file.\n\
     -s              If specified, use HTTP simplerequest format for query.\n\
     -q              If specified, just return response header.\n\
     -H              If specified, take rest of line to send as additional\n\
                     header to remote site prior to fetching file.\n\
     -h              If specified, print usage message.");
}

/***************************************************************************/

static char *arg_proxy = 0;

int main ( int argc, char **argv )
{
    int status, sd, i, j, timeout, in_hdr, rem_port, http_parse_url();
    int swish = 0;
    int binary_mode, simple_request, query;
    struct header_item {
        char *header;
        struct header_item *next;
    } *headers = 0;
    struct sockaddr local, remote;
    struct hostent *hostinfo;
    char *arg_url, *arg_ofile, *service, *node, *ident, *arg, *remote_host;
    char *arg_auth, *arg_file;
    int	auth_flag = 0;
    int	file_flag = 0;
    int	proxy_flag = 0;
    char url_buf[1024];
    char basic_buf[80];
    char data_buf[32000];
    FILE *trcf;
    int On = 1;
    /*
     * Extract arguments from command line.
     */
    arg_url = arg_ofile = arg_auth = arg_file = NULL;
    binary_mode = simple_request = query = 0;
    for ( i = 1; i < argc; i++ ) {
        if ( argv[i][0] == '-' ) {
            for ( j = 1; argv[i][j]; j++ ) {
                if ( argv[i][j] == 'b' ) binary_mode = 1;
                else if ( argv[i][j] == 's' ) simple_request = 1;
		else if ( argv[i][j] == 'q' ) query = 1;
                else if ( argv[i][j] == 'H' ) {
                    struct header_item *h = malloc (sizeof (struct header_item));
                    h->next = headers;
                    headers = h;
                    h->header = &argv[i][j+1];
		    if (*h->header == '\0') h->header = argv[++i];
                    break;
                } else if ( argv[i][j] == 'h' ) {
		    usage ();
		    exit ( EXIT_FAILURE );
		} else if ( argv[i][j] == 'a' ) {
		    auth_flag = 1;
		    arg_auth = &argv[i][j+1];
		    if (*arg_auth == '\0') arg_auth = argv[++i];
		    break;
		} else if ( argv[i][j] == 'p' ) {
		    proxy_flag = 1;
		    arg_proxy = &argv[i][j+1];
		    if (*arg_proxy == '\0') arg_proxy = argv[++i];
		    break;
		} else if ( argv[i][j] == 'i' ) {
		    swish = 1;
		} else if ( argv[i][j] == 'f' ) {
		    FILE *data;
		    arg_file = &argv[i][j+1];
		    if (*arg_file == '\0') arg_file = argv[++i];

		    data = fopen( arg_file, "r" );
		    if( !data ) {
			fprintf( stderr, "Error opening %s\n", arg_file );
			arg_file = NULL;
		    } else {
			int data_len = 0;
			while( fgets( data_buf+data_len,
				     sizeof(data_buf)-data_len, data ) ) {
			    data_len = strlen( data_buf ) - 1;
			    data_buf[data_len] = '\0';

			    if( data_len )
				if( data_buf[data_len-1] == '\\' )
				    data_buf[--data_len] = '\0';

			    if( data_len >= sizeof(data_buf) )
				break;
			}
			fclose( data );
		    }
		    break;
                } else fprintf(stderr,"Unknown option ignored\n");
            }
        } else if ( arg_url == NULL ) arg_url = argv[i];
        else if ( arg_ofile == NULL ) arg_ofile = argv[i];
        else {
            fprintf(stderr,"extra arguments ignored\n");
        }
    }
    if ( !arg_url ) {
        usage();
        exit ( EXIT_FAILURE );
    }
    if ( query && simple_request ) {
	fprintf(stderr,"Options -q and -s are incompatible\n");
	exit ( EXIT_FAILURE );
    }

    trcf = fopen("fetch_http.log","a");
    
    /*
     * Parse target.
     */
    status = http_parse_url ( arg_url, url_buf,
			     &service, &remote_host, &ident, &arg );

    rem_port = 80;
    for ( j = 0; remote_host[j]; j++ ) if ( remote_host[j] == ':' ) {
        rem_port = atoi ( &remote_host[j+1] );
        remote_host[j] = '\0';
        break;
    }
    if ( !*remote_host ) remote_host = "/localhost";
    /*
     * Create socket on first available port.
     */

    sd = socket ( AF_INET, SOCK_STREAM, 0 );
    if ( sd < 0 ) {
	fprintf(stderr,"Error creating socket: %s\n", strerror(errno) );
	return EXIT_FAILURE;
    }

    local.sa_family = AF_INET;
    for ( j = 0; j < sizeof(local.sa_data); j++ ) local.sa_data[j] = '\0';

    setsockopt(sd,SOL_SOCKET,SO_KEEPALIVE,&On,sizeof(On));

    status = bind ( sd, &local, sizeof ( local ) );
    if ( status < 0 ) {
	fprintf(stderr,"Error binding socket: %s\n", strerror(errno) );
	return EXIT_FAILURE;
    }


    /*
     * Lookup host.
     */

    hostinfo = gethostbyname ( &remote_host[1] );
    if ( !hostinfo ) {
        fprintf(stderr, "Could not find host '%s'\n", remote_host );
        return EXIT_FAILURE;
    }

    remote.sa_family = hostinfo->h_addrtype;
    for ( j = 0; j < hostinfo->h_length; j++ ) {
	remote.sa_data[j+2] = hostinfo->h_addr_list[0][j];
    }
    remote.sa_data[0] = rem_port >> 8;
    remote.sa_data[1] = (rem_port&255);

    /*
     * Connect to remote host.
     */

    status = connect ( sd, &remote, sizeof(remote) );
    if ( status == 0 ) {
        int length;
        char response[16384];
	char buf[80];
	char *r;
	struct header_item *h;
        /*
         * Send request followed by newline.
         */

        in_hdr = !simple_request;
	if ( in_hdr ) {
	    if ( query )
		sprintf(response, "HEAD %s%s HTTP/1.0\r\n\r\n", ident, arg );
	    else if ( !arg_file )
		sprintf(response, "GET %s%s HTTP/1.0\r\n", ident, arg );
	    else {
		sprintf( response, "POST %s HTTP/1.0\r\n", ident );
		strcat( response, "Content-Encoding: 8bit\r\n" );
		sprintf( buf, "Content-Length: %d\r\n", strlen(data_buf ) );
		strcat( response, buf );			
		strcat( response, "Content-type: application/x-www-form-urlencoded\r\n" );
	    }

	    if( arg_auth ) {
		strcat( response, "Authorization: Basic " );
		BASE64_encode( (unsigned char *) arg_auth,
			      strlen( arg_auth ), basic_buf );
		strcat( response, basic_buf );
		strcat( response, "\r\n" );
	    }
	    strcat( response, "User-Agent: fetch_http/1.1 OSU/1.2\r\n" );
	    for (r = response, h = headers; h != 0; h = h->next) {
		r = r + strlen (r);
		sprintf (r, "%s\r\n", h->header);
	    }
	    strcat (r, "\r\n");

	    if( arg_file ) {
		strcat( response, data_buf );
		strcat( response, "\r\n\r\n" );
	    }
	}
        else
            sprintf(response, "GET %s%s\r\n", ident, arg );

	length = strlen(response);
	fwrite(response,1,length,trcf);

        status = send ( sd, response, strlen(response), 0 );
        if ( status < 0 )
	    fprintf(stderr,"status of write: %d %d\n", status, errno );
        /*
         * Read and echo response.
         */

        if ( status > 0 ) {
            int i, j, cr, hdr_state;
            char ch;
            FILE *outf, *cur_outf;
            cr = 0;
            if (arg_ofile) {
                if (!(outf = fopen(arg_ofile,binary_mode?"wb":"w"))) {
		    perror(arg_ofile);
		    exit(EXIT_FAILURE);
		}
            }
            else
		outf = stdout;
	    if (swish) cur_outf = outf;
            else cur_outf = (query || !in_hdr) ? outf : stdout;
            hdr_state = 0;
            while (0 < (length=recv(sd, response, sizeof(response)-1,0))) {
                if ( binary_mode && !in_hdr ) {
                    fwrite ( response, 1, length, cur_outf );
                } else {
                    for ( i = j = 0; i < length; i++ ) {
                        ch = response[i];
                        if ( response[i] == '\r' ) {
                            if ( cr == 1 ) response[j++] = '\r';
                            cr = 1;
                        } else if ( cr == 1 ) {
                            if ( ch != '\n' ) response[j++] = '\r';
                            response[j++] = ch;
                            cr = 0;
                        } else {
                            response[j++] = ch;
                            cr = 0;
                        }
                        if ( in_hdr && arg_ofile && !query &&
			    ch == '\n' && !cr ) {
                            if ( hdr_state ) {
                                in_hdr = 0;
                                ch = response[j];
                                response[j] = '\0';
                                if ( j > 0 ) fprintf (cur_outf,"%s",response);
                                response[j] = ch;
                                cur_outf = outf;
                                if ( binary_mode ) {
                                    i++;
                                    if ( i < length )
                                        fwrite (&response[i], 1, length-i,outf);
                                    break;
                                }
                                j = 0;
                            }
                            hdr_state = 1;
                        } else if ( !cr ) hdr_state = 0;
                    }
                    response[j] = '\0';
                    if ( !binary_mode ) fwrite(response, j, 1, cur_outf );
                }
            }
	    if ( outf != stdout ) {
		fprintf(outf,"\n");
		fclose(outf);
	    }
        }
    } else {
        fprintf(stderr, "error connecting to '%s': %d (%s)\n",
		remote_host, status, strerror(errno) );
    }

    close ( sd );
    return EXIT_SUCCESS;
}
/***************************************************************************/
int http_parse_url(char *url,	/* locator to parse */
		   char *info,	/* Scratch area for result pts*/
		   char **service, /* Protocol (e.g. http) indicator */
		   char **node,	/* Node name. */
		   char **ident, /* File specification. */
		   char **arg )	/* Search argument */
{
    int i, state;
    char *last_slash, *p, c, arg_c;
    static char proxy_node[80];
    /*
     * Copy contents of url into info area.
     */
    *service = *node = *ident = *arg = last_slash = "";

    for ( state = i = 0; (info[i] = url[i]) != 0; ) {
        c = info[i];
        switch ( state ) {
	case 0:
	    if ( c == ':' ) {
		info[i] = '\0';     /* terminate string */
		*service = info;
		state = 1;
	    }
	case 1:
	    if ( c == '/' ) {
		*ident = last_slash = &info[i];
		state = 2;
	    }
	    break;
	case 2:
	    state = 4;
	    if ( c == '/' ) {       /* 2 slashes in a row */
		*node = *ident;
		state = 3;
	    }
	    else if ( (c == '#') || (c == '?') ) {
		arg_c = c;
		info[i] = '\0';
		*arg = &info[i+1];
		state = 5;
	    }
	    break;
	case 3:                     /* find end of host spec */
	    if ( c == '/' ) {
		state = 4;
		*ident = last_slash = &info[i];
		for ( p = *node; p < *ident; p++ ) p[0] = p[1];
		info[i-1] = '\0';   /* Terminate host string */
	    }
	    break;
	case 4:                     /* Find end of filename */
	    if ( c == '/' ) last_slash = &info[i];
	    else if ( (c == '#') || (c == '?') ) {
		arg_c = c;
		info[i] = '\0';
		*arg = &info[i+1];
		state = 5;
	    }
	case 5:
	    break;
        }
        i++;
    }
    /*
     * Insert arg delimiter back into string.
     */
    if ( **arg ) {
        char tmp;
        for ( p = (*arg); arg_c; p++ ) { tmp = *p; *p = arg_c; arg_c = tmp; }
        *p = '\0';
    }

    if( arg_proxy ) {
	proxy_node[0] = '/';
	strcpy( proxy_node+1, arg_proxy );
	*node = proxy_node;
	*ident = url;
	*arg = "";
    }

    return 1;
}
