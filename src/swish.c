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


#include <limits.h>     // for ULONG_MAX
#include "swish.h"
#include "mem.h"
#include "string.h"
#include "error.h"
#include "list.h"
#include "search.h"
#include "index.h"
#include "file.h"
#include "http.h"
#include "merge.h"
#include "docprop.h"
#include "metanames.h"
#include "parse_conffile.h"
#include "result_output.h"
#include "result_sort.h"
#include "keychar_out.h"
#include "date_time.h"
#include "db.h"
#include "fs.h"
#include "dump.h"

#include "proplimit.h"


/*
** This array has pointers to all the indexing data source
** structures
*/
extern struct _indexing_data_source_def *data_sources[];





typedef struct
{
    char   *name;
    unsigned int bit;
    char   *description;
}
DEBUG_MAP;

static DEBUG_MAP debug_map[] = {
    /* These dump data from the index file */
    {"INDEX_HEADER", DEBUG_INDEX_HEADER, "Show the headers from the index"},
    {"INDEX_WORDS", DEBUG_INDEX_WORDS, "List words stored in index"},
    {"INDEX_WORDS_ONLY", DEBUG_INDEX_WORDS_ONLY, "List only words, one per line, stored in index"},
    {"INDEX_WORDS_META", DEBUG_INDEX_WORDS_META, "List only words and associated metaID separated by a tab"},
    {"INDEX_WORDS_FULL", DEBUG_INDEX_WORDS_FULL, "List words stored in index (more verbose)"},
    {"INDEX_STOPWORDS", DEBUG_INDEX_STOPWORDS, "List stopwords stored in index"},
    {"INDEX_FILES", DEBUG_INDEX_FILES, "List file data stored in index"},
    {"INDEX_METANAMES", DEBUG_INDEX_METANAMES, "List metaname table stored in index"},
    {"INDEX_ALL", DEBUG_INDEX_ALL, "Dump data ALL above data from index file\n\n-- indexing --\n"},

    /* These trace indexing */
    {"INDEXED_WORDS", DEBUG_WORDS, "Display words as they are indexed"},
    {"PARSED_WORDS", DEBUG_PARSED_WORDS, "Display words as they are parsed from source"},
    {"PROPERTIES", DEBUG_PROPERTIES, "Display properties associted with each file as they are indexed"},
    {"REGEX", DEBUG_REGEX, "Debug regular expression processing"},
    {"PARSED_TAGS", DEBUG_PARSED_TAGS, "Show meta tags as they are found"},
    {"PARSED_TEXT", DEBUG_PARSED_TEXT, "Show text as it's parsed"},
};


/* Possible run modes */
typedef enum {
    MODE_SEARCH,
	MODE_INDEX,
	MODE_DUMP,
    MODE_WORDS,
	MODE_MERGE,
    MODE_UPDATE
}
CMD_MODE;


/* Parameters read from the command line, that are not stored in *SWISH */
typedef struct
{
    CMD_MODE    run_mode;           /* selected run mode */
    char       *wordlist;           /* list of -w words */
    char        keychar;            /* for dumping words */

    struct swline *tmpsortprops;    /* sort properties */
    struct swline *conflist;        /* Configuration file list */

    int         hasverbose;         /* flag if -v was used */

    int         index_read_only;    /* flag to not allow indexing or merging */
    int         swap_mode;
    int         structure;          /* where in file to search */

    char       *merge_out_file;     /* the output file for merge */

}
CMDPARAMS;


/************* TOC ***************************************/
static CMDPARAMS *new_swish_params();
static void printTime(double time);
static void get_command_line_params(SWISH *sw, char **argv, CMDPARAMS *params );
static void free_command_line_params( CMDPARAMS *params );
static unsigned int isDebugWord(char *word, CMDPARAMS *params );
static void printversion();
static void usage();
static int check_readonly_mode( char * );

static void cmd_dump( SWISH *sw, CMDPARAMS *params );
static void cmd_index( SWISH *sw, CMDPARAMS *params );
static void cmd_merge( SWISH *sw, CMDPARAMS *params );
static void cmd_search( SWISH *sw, CMDPARAMS *params );
static void cmd_keywords( SWISH *sw, CMDPARAMS *params );
static void write_index_file( SWISH *sw, int process_stopwords, double elapsedStart, double cpuStart, int merge, int is_update);
/************* TOC ***************************************/


int     main(int argc, char **argv)
{
    SWISH  *sw;
    CMDPARAMS *params;



    setlocale(LC_CTYPE, "");



    /* Start a session */
    sw = SwishNew();            /* Get swish handle */



    /* By default we are set up to use the first data source in the list */
    /* I don't like this.   modules.c would fix this */
    IndexingDataSource = data_sources[0];

    


    params = new_swish_params();
    get_command_line_params(sw, argv, params );

    switch( params->run_mode )
    {
        case MODE_DUMP:
            cmd_dump( sw, params );  /* first so will override */
            break;

        case MODE_MERGE:
            cmd_merge( sw, params );
            break;

        case MODE_INDEX:
        case MODE_UPDATE:
            cmd_index( sw, params );
            break;

        case MODE_SEARCH:
            cmd_search( sw, params );
            break;

        case MODE_WORDS:
            cmd_keywords( sw ,params );  /* -k setting */
            break;

            
        default:
            progerr("Invalid operation mode '%d'", (int)params->run_mode);
    }

    free_command_line_params( params );

    SwishClose(sw);

    Mem_Summary("At end of program", 1);

    exit(0);

    return 0;
}

/* Prints the running time (the time it took for indexing).
*/

static void printTime(double time)
{
    int     hh,
            mm,
            ss;
    int     delta;

    delta = (int) (time + 0.5);

    ss = delta % 60;
    delta /= 60;
    hh = delta / 60;
    mm = delta % 60;

    printf("%02d:%02d:%02d", hh, mm, ss);
}

/* Prints the SWISH usage.
*/

static void    usage()
{
    const char *defaultIndexingSystem = "";

    printf(" usage:\n");
    printf("    swish [-e] [-i dir file ... ] [-S system] [-c file] [-f file] [-l] [-v (num)]\n");
    printf("    swish -w word1 word2 ... [-f file1 file2 ...] \\\n");
    printf("          [-P phrase_delimiter] [-p prop1 ...] [-s sortprop1 [asc|desc] ...] \\\n");
    printf("          [-m num] [-t str] [-d delim] [-H (num)] [-x output_format]\n");
    printf("    swish -k (char|*) [-f file1 file2 ...]\n");
    printf("    swish -M index1 index2 ... outputfile\n");
    printf("    swish -N /path/to/compare/file\n");
    printf("    swish -V\n");
    putchar('\n');
    printf("options: defaults are in brackets\n");


    printf("         -S : specify which indexing system to use.\n");
    printf("              Valid options are:\n");
#ifdef ALLOW_FILESYSTEM_INDEXING_DATA_SOURCE
    printf("              \"fs\" - index local files in your File System\n");
    if (!*defaultIndexingSystem)
        defaultIndexingSystem = "fs";
#endif

#ifdef ALLOW_HTTP_INDEXING_DATA_SOURCE
    printf("              \"http\" - index web site files using a web crawler\n");
    if (!*defaultIndexingSystem)
        defaultIndexingSystem = "http";
#endif

#ifdef ALLOW_EXTERNAL_PROGRAM_DATA_SOURCE
    printf("              \"prog\" - index files supplied by an external program\n");

    if (!*defaultIndexingSystem)
        defaultIndexingSystem = "http";
#endif

    printf("              The default value is: \"%s\"\n", defaultIndexingSystem);

    printf("         -i : create an index from the specified files\n");
    printf("         -w : search for words \"word1 word2 ...\"\n");
    printf("         -t : tags to search in - specify as a string\n");
    printf("              \"HBthec\" - in Head|Body|title|header|emphasized|comments\n");
    printf("         -f : index file to create or file(s) to search from [%s]\n", INDEXFILE);
    printf("         -c : configuration file(s) to use for indexing\n");
    printf("         -v : verbosity level (0 to 3) [-v %d]\n", VERBOSE);
    printf("         -l : follow symbolic links when indexing\n");
    printf("         -b : begin results at this number\n");
    printf("         -m : the maximum number of results to return [defaults to all results]\n");
    printf("         -M : merges index files\n");
    printf("         -N : index only files with a modification date newer than path supplied\n");
    printf("         -p : include these document properties in the output \"prop1 prop2 ...\"\n");
    printf("         -s : sort by these document properties in the output \"prop1 prop2 ...\"\n");
    printf("         -d : next param is delimiter.\n");
    printf("         -P : next param is Phrase delimiter.\n");
    printf("         -V : prints the current version\n");
    printf("         -e : \"Economic Mode\": The index proccess uses less RAM.\n");
    printf("         -x : \"Extended Output Format\": Specify the output format.\n");
    printf("         -H : \"Result Header Output\": verbosity (0 to 9)  [1].\n");
    printf("         -k : Print words starting with a given char.\n");
    printf("         -T : Trace options\n\n");
    printf("\nversion: %s\n", SWISH_VERSION);
    printf("   docs: http://swish-e.org/\n");
    exit(1);
}

static void    printversion()
{
    printf("SWISH-E %s\n", SWISH_VERSION );
    exit(0);
}


/*************************************************************************
*   Deal with -T debug options
*
*
**************************************************************************/

static unsigned int isDebugWord(char *word, CMDPARAMS *params)
{
    int     i,
            help;

    help = strcasecmp(word, "help") == 0;

    if (help)
        printf("\nAvailable debugging options for swish-e:\n");

    for (i = 0; i < sizeof(debug_map) / sizeof(debug_map[0]); i++)
        if (help)
            printf("  %20s => %s\n", debug_map[i].name, debug_map[i].description);
        else if (strcasecmp(debug_map[i].name, word) == 0)
        {
            if (strncasecmp(word, "INDEX_", 6) == 0)
                params->run_mode = MODE_DUMP;

            return debug_map[i].bit;
        }

    if (help)
        exit(1);

    return 0;
}

/*************************************************************************
*   Initialize the swish command parameters
*
*   Call with:
*       void
*
*   Returns:
*       pointer to CMDPARAMS
*
*   To Do:
*       The swish parameters probably should be groupped by switches and
*       by config file (and maybe someday also by directory or path or
*       content-type) and then merged.
*
**************************************************************************/

static CMDPARAMS *new_swish_params()
{
    CMDPARAMS *params = (CMDPARAMS *)emalloc( sizeof( CMDPARAMS ) );
    memset( params, 0, sizeof( CMDPARAMS ) );

    params->run_mode = MODE_SEARCH; /* default run mode */
    params->structure = IN_FILE;    /* look in the file, by default */

    return params;
}



/*************************************************************************
*   Free the swish command parameters
*
*   Call with:
*       *CMDPARAMS
*
*   Returns:
*       void
*
*   To Do:
*       The swish parameters probably should be groupped by switches and
*       by config file (and maybe someday also by directory or path or
*       content-type) and then merged.
*
**************************************************************************/

static void free_command_line_params( CMDPARAMS *params )
{
    if ( params->wordlist )
        efree( params->wordlist );

    if ( params->tmpsortprops )
        freeswline( params->tmpsortprops );

    if ( params->conflist )
        freeswline( params->conflist );

    efree( params );
}


/*************************************************************************
*  Just checks if there is a next word
*  Three helper fuctions - to be replaced by better command parsing soon...
**************************************************************************/

static char *is_another_param( char **argv )
{
    return ( *(argv + 1) && *(argv + 1)[0] != '-' )
            ? *(argv + 1)
            : NULL;
}
    
static char *next_param( char ***argv )
{
    char *c;
    
    if ( ( c = is_another_param( *argv ) ) )
    {
        (*argv)++;
        return c;
    }

    return NULL;
}


static int get_param_number(char ***argv, char c )
{
    char *badchar;
    long  num;
    char *string = next_param( argv );

    if ( !string )
        progerr(" '-%c' requires a positive integer.", c );

    num = strtol( string, &badchar, 10 ); // would base zero be more flexible?
    
    if ( num == LONG_MAX || num == LONG_MIN )
        progerrno("Failed to convert '-%c %s' to a number: ", c, string );

    if ( *badchar )
        progerr("Invalid char '%c' found in argument to '-%c %s'", badchar[0], c, string);


    return (int) num;
}




/*************************************************************************
*   Gets the command line parameters, if any, and set values in the CMDPARMAS structure
*
*
*   Returns:
*       void (changes *sw and *params)
*
*   To Do:
*       This code is horrific.  Get a structure to define the parameters, and messages!
*       Move this into its own module!
*
*       Also, mixes two structres for parameters, SWISH and CMDPARAMS.  Not a great setup.
*
*
*       I'd like to see a centeral routine for processing switches, and a way for
*       modules to "register" what config options to parse out by the central routine.
*
**************************************************************************/
static void get_command_line_params(SWISH *sw, char **argv, CMDPARAMS *params )
{
    char c;
    int  lenwordlist = 0;
    char *w;

    /* not excited about this */
    params->wordlist = (char *) emalloc((lenwordlist = MAXSTRLEN) + 1);
    params->wordlist[0] = '\0';
    


    params->index_read_only = check_readonly_mode( *argv );



    if ( !*(argv + 1 ) )
        progerr("Missing parameter.  Use -h for options.", *argv);

    while ( *++argv )
    {

        if ((*argv)[0] != '-')  // every parameter starts with a dash
            progerr("Missing switch character at '%s'.  Use -h for options.", *argv);

        if ( !(c = (*argv)[1] ) )         // get single switch char
            progerr("Missing switch character at '%s'.  Use -h for options.", *argv);

        /* allow joined arguments */
        if ( (*argv)[2] )
        {
            *argv += 2;
            argv--;
        }


        /* files to index */
        if (c == 'i')
        {
            if ( !is_another_param( argv ) )
                progerr(" '-i' requires a list of index files.");

            if(params->run_mode != MODE_UPDATE)   /* Preserve update mode */
                params->run_mode = MODE_INDEX;

            while ( (w = next_param( &argv )) )
                sw->dirlist = addswline(sw->dirlist, w );

            continue;
        }


        /* search words */

        if (c == 'w')
        {
            if ( !is_another_param( argv ) )
                progerr(" '-w' requires list of search words.");
                
            while ( (w = next_param( &argv )) )
            {
                /* don't add blank words */
                if (w[0] == '\0')
                    continue;

                if ((int)( strlen(params->wordlist) + strlen(" ") + strlen(w) ) >= lenwordlist)
                {
                    lenwordlist = strlen(params->wordlist) + strlen(" ") + strlen(w) + 200;
                    params->wordlist = (char *) erealloc(params->wordlist, lenwordlist + 1);
                }

                params->run_mode = MODE_SEARCH;
                sprintf(params->wordlist, "%s%s%s", params->wordlist, (params->wordlist[0] == '\0') ? "" : " ", w);
            }

            continue;
        }




        /* words to dump from index */

        if (c == 'k')
        {
            if ( !(w = next_param( &argv )) )
                progerr(" '-k' requires a character (or '*').");

            if ( strlen( w ) != 1 )                
                progerr(" '-k' requires a character (or '*').");


            params->run_mode = MODE_WORDS;
            params->keychar = w[0];

            continue;
        }



        /* Data source */

        else if (c == 'S')
        {
            struct _indexing_data_source_def **data_source;

            if ( !(w = next_param( &argv )) )
                progerr(" '-S' requires a valid data source.");

            for (data_source = data_sources; *data_source != 0; data_source++)
                if (strcmp(w, (*data_source)->IndexingDataSourceId) == 0)
                    break;
                    

            if (!*data_source)
                progerr("Unknown -S option \"%s\"", w);
            else
                IndexingDataSource = *data_source;

            continue;
        }



        /* sort properties */

        if (c == 's')
        {
            if ( !is_another_param( argv ) )
                progerr(" '-s' requires list of sort properties.");
                
            while ( (w = next_param( &argv )) )
                params->tmpsortprops = addswline(params->tmpsortprops, w);

            continue;
        }


        /* display properties */

        if (c == 'p')
        {
            if ( !is_another_param( argv ) )
                progerr(" '-p' requires list of properties.");
                
            while ( (w = next_param( &argv )) )
                addSearchResultDisplayProperty(sw, w);

            continue;
        }



        /* Set limit values */

        if (c == 'L')
        {
            if ( !( is_another_param( argv ) && is_another_param( argv + 1  ) && is_another_param( argv + 2 )) )
                progerr("-L requires three parameters <propname> <lorange> <highrange>");

            SetLimitParameter(sw, argv[1], argv[2], argv[3]);
            argv += 3;

            continue;
        }



        /* Index file(s) selection */
        
        if (c == 'f')
        {
            if ( !is_another_param( argv ) )
                progerr(" '-f' requires list of index files.");
                
            while ( (w = next_param( &argv )) )
                sw->indexlist = addindexfile(sw->indexlist, w);

            continue;
        }


        /* config file list */

        if (c == 'c')
        {
            if ( !is_another_param( argv ) )
                progerr(" '-c' requires one or more configuration files.");

            if(params->run_mode != MODE_UPDATE)   /* Preserve update mode */     
                params->run_mode = MODE_INDEX;

            while ( (w = next_param( &argv )) )
                params->conflist = addswline(params->conflist, w);

            continue;
        }



        /* Follow symbolic links */
        
        if (c == 'l')
        {
            sw->FS->followsymlinks = 1;
            continue;
        }


        /* Set begin hit location */

        if (c == 'b')
        {
            sw->Search->beginhits = get_param_number( &argv, c );
            continue;
        }


        /* Set max hits */

        if (c == 'm')
        {
            sw->Search->maxhits = get_param_number( &argv, c );
            continue;
        }
        


        /* Save the time for limiting indexing by a file date */

        if (c == 'N')
        {
            struct stat stat_buf;

            if ( !(w = next_param( &argv )) )
                progerr("-N requires a path to a local file");

            if (stat( w, &stat_buf))
                progerrno("Bad path '%s' specified with -N: ", w );

            sw->mtime_limit = stat_buf.st_mtime;

            continue;
        }


        /* limit by structure */

        if (c == 't')
        {
            char * c;
            
            if ( !(w = next_param( &argv )) )
                progerr("Specify tag fields (HBtheca).");


            params->structure = 0;  /* reset to none */

            for ( c = w; *c; c++ )
                switch ( *c )
                {
                case 'H':
                    params->structure |= IN_HEAD;
                    break;
                case 'B':
                    params->structure |= IN_BODY;
                    break;
                case 't':
                    params->structure |= IN_TITLE;
                    break;
                case 'h':
                    params->structure |= IN_HEADER;
                    break;
                case 'e':
                    params->structure |= IN_EMPHASIZED;
                    break;
                case 'c':
                    params->structure |= IN_COMMENTS;
                    break;
                case 'a':
                    params->structure |= IN_ALL;
                    break;
                default:
                    progerr("-t must only include HBthec.  Found '%c'", *c );
                }
            continue;
        }





        /* verbose while indexing */
        
        if (c == 'v')
        {
            params->hasverbose = 1;
            sw->verbose = get_param_number( &argv, c );
            continue;
        }



        /* print the version number */
        
        if (c == 'V')
            printversion();



        /* "z" Huh? */

        if (c == 'z' || c == 'h' || c == '?')
            usage();



        /* Merge settings */

        if (c == 'M')
        {
            if ( !is_another_param( argv )  )
                progerr(" '-M' requires an output file name.");
                
            params->run_mode = MODE_MERGE;

            while ( (w = next_param( &argv )) )
            {
                /* Last one listed is the output file */
                if ( is_another_param( argv  ) )
                    sw->indexlist = addindexfile(sw->indexlist, w);
                else
                    params->merge_out_file = estrdup( w );
            }

            continue;
        }



        /* Debugging options */

        if (c == 'T')
        {
            while ( (w = next_param( &argv )) )
            {
                unsigned int bit;

                if ((bit = isDebugWord( w, params) ))
                    DEBUG_MASK |= bit;
                else
                    progerr("Invalid debugging option '%s'.  Use '-T help' for help.", w);

            }
            continue;
        }



       
        /* Custom Phrase Delimiter - Jose Ruiz 01/00 */

        if (c == 'P')
        {
            if ( !(w = next_param( &argv )) )
                progerr("'-P' requires a phrase delimiter.");

            sw->Search->PhraseDelimiter = (int) w[0];
            continue;
        }



        /* Set the custom delimiter */
        if (c == 'd')
        {
            if ( !(w = next_param( &argv )) )
                progerr("'-d' requires an output delimiter.");

            sw->ResultOutput->stdResultFieldDelimiter = estrredup(sw->ResultOutput->stdResultFieldDelimiter, w );

            /* This really doesn't work as is probably expected since it's a delimiter and not quoting the fields */
            if (strcmp(sw->ResultOutput->stdResultFieldDelimiter, "dq") == 0)
                strcpy( sw->ResultOutput->stdResultFieldDelimiter, "\"" );
            else
            {
                int     i,j;
                int     backslash = 0;

                for ( j=0, i=0; i < strlen( w ); i++ )
                {
                    if ( !backslash )
                    {
                        if ( w[i] == '\\' )
                        {
                            backslash++;
                            continue;
                        }
                        else
                        {
                            sw->ResultOutput->stdResultFieldDelimiter[j++] = w[i];
                            continue;
                        }
                    }
                        

                    switch ( w[i] )
                    {
                    case 'f':
                        sw->ResultOutput->stdResultFieldDelimiter[j++] = '\f';
                        break;
                    case 'n':
                        sw->ResultOutput->stdResultFieldDelimiter[j++] = '\n';
                        break;
                    case 'r':
                        sw->ResultOutput->stdResultFieldDelimiter[j++] = '\r';
                        break;
                    case 't':
                        sw->ResultOutput->stdResultFieldDelimiter[j++] = '\t';
                        break;
                    case '\\':
                        sw->ResultOutput->stdResultFieldDelimiter[j++] = '\\';
                        sw->ResultOutput->stdResultFieldDelimiter[j++] = '\\';
                        break;
                    default:
                        progerr("Unknown escape sequence '\\%c'.  Must be one of \\f \\n \\r \\t \\\\", w[i]);
                    }
                    backslash = 0;
                }
                sw->ResultOutput->stdResultFieldDelimiter[j] = '\0';
            }
            continue;
        }



        /* Econ mode */

        if (c == 'e')
        {
            /* Jose Ruiz 09/00 */
            params->swap_mode = 1;      /* "Economic mode": Uses less RAM */
            /* The proccess is slower: Part of */
            /* info is preserved in temporal */
            /* files */

            continue;
        }


        /* $$$ These need better error reporting */

        /* Extended format */
        
        if (c == 'x')
        {
            /* Jose Ruiz 09/00 */
            /* Search proc will show more info */
            /* rasc 2001-02  extended  -x fmtstr */

            if ( !(w = next_param( &argv )) )
                progerr("'-x' requires an output format string.");

            {
                char   *s;
                s = hasResultExtFmtStr(sw, w);
                sw->ResultOutput->extendedformat = (s) ? s : w;
                initPrintExtResult(sw, sw->ResultOutput->extendedformat);
            }

            continue;
        }



        /* Search header output control */
        if (c == 'H')
        {
            sw->ResultOutput->headerOutVerbose = get_param_number( &argv, c );
            continue;
        }


        /* Ignore sorted indexes */

        if (c == 'o')
        {
            sw->ResultSort->isPreSorted = 0;
            continue;
        }

        /* Update mode jmruiz 2002/03 */
        
        if (c == 'u')
        {
            params->run_mode = MODE_UPDATE;
            continue;
        }

        progerr("Unknown switch '-%c'.  Use -h for options.", c );
    }
}


/*************************************************************************
*   Returns true if we think the program is called swish-search
*
**************************************************************************/
static int check_readonly_mode( char *prog )
{
    char   *tmp;
    

    /* If argv is swish-search then you cannot index and/or merge */
    if ( (tmp = strrchr( prog, DIRDELIMITER)) )
        tmp++;  // bump past delimiter
    else
        tmp = prog;

    /* We must ignore case for WIN 32 */
    if (strcasecmp(tmp, "swish-search") == 0)
        return 1;

    return 0;
}


/*************************************************************************
*   Dumps the index file(s) 
*
**************************************************************************/
static void cmd_dump( SWISH *sw, CMDPARAMS *params )
{

    /* Set the default index file */
    if ( sw->indexlist == NULL )
        sw->indexlist = addindexfile(sw->indexlist, INDEXFILE);

    while ( sw->indexlist != NULL )
    {

        DB_decompress(sw, sw->indexlist);
        putchar('\n');

        sw->indexlist = sw->indexlist->next;
    }
}
/*************************************************************************
*   This run the indexing code
*
**************************************************************************/

static void cmd_index( SWISH *sw, CMDPARAMS *params )
{
    int     hasdir = (sw->dirlist == NULL) ? 0 : 1;
    int     hasindex = (sw->indexlist == NULL) ? 0 : 1;
    double  elapsedStart = TimeElapsed();
    double  cpuStart = TimeCPU();
    struct swline *tmpswline;

    if ( params->index_read_only )
        progerr("Sorry, this program is in readonly mode");


    /* Read configuration files */
    while (params->conflist != NULL)
    {
        getdefaults(sw, params->conflist->line, &hasdir, &hasindex, params->hasverbose);
        params->conflist = params->conflist->next;
    }


    /* Default index file */
    if ( sw->indexlist == NULL )
        sw->indexlist = addindexfile(sw->indexlist, INDEXFILE);


    if (!hasdir)
        progerr("Specify directories or files to index.");


    /* What's the point of this? */
    if (sw->verbose < 0)
        sw->verbose = 0;
    if (sw->verbose > 4)
        sw->verbose = 4;

    /* Update Economic mode */
    sw->Index->swap_locdata = params->swap_mode;


    /* Check for UPDATE_MODE jmruiz 2002/03 */
    if(params->run_mode == MODE_UPDATE)
    {
        /* Open the index file for read/write */
        sw->indexlist->DB = (void *) DB_Open(sw, sw->indexlist->line,DB_READWRITE);

        /* Read the header and overwrite the '-c' option  and feault values - In other 
        ** words, the header values are the good ones */
        read_header(sw, &sw->indexlist->header, sw->indexlist->DB);
        sw->TotalWords = sw->indexlist->header.totalwords;
        sw->TotalFiles = sw->indexlist->header.totalfiles;

        /* Adjust filenum to totalfiles */
        sw->Index->filenum = sw->TotalFiles;

        progerr("Invalid operation mode '%d': Update mode is not yet ", (int)params->run_mode);

    }
    else
    {
        /* Create an empty File - before indexing to make sure can write to the index */
        sw->indexlist->DB = (void *) DB_Create(sw, sw->indexlist->line);
    }


    /* This should be printed by the module that's reading the source */
    printf("Indexing Data Source: \"%s\"\n", IndexingDataSource->IndexingDataSourceName);

    tmpswline = sw->dirlist;
    while (tmpswline != NULL)
    {
        if (sw->verbose)
        {
            printf("Indexing \"%s\"\n", tmpswline->line);
            fflush(stdout);
        }
        indexpath(sw, tmpswline->line);
        tmpswline = tmpswline->next;
    }


    Mem_Summary("After indexing", 0);


    if (sw->verbose > 1)
        putchar('\n');


    if (sw->verbose)
        printf("Removing very common words...\n");

    fflush(stdout);

    write_index_file( sw, 1, elapsedStart, cpuStart, 0, params->run_mode == MODE_UPDATE?1:0);
}


/*************************************************************************
*   MERGE: prepare index files for merging, and call merge.c
*
*   Most of this should probably be in merge.c
*
**************************************************************************/
static void cmd_merge( SWISH *sw_input, CMDPARAMS *params )
{
    SWISH *sw_out;
    double  elapsedStart = TimeElapsed();
    double  cpuStart = TimeCPU();

    if ( params->index_read_only )
        progerr("Sorry, this program is in readonly mode");


    if (!sw_input->indexlist)
        progerr("Failed to list any input files for merging");


    /* Open all the index files for reading */
    SwishAttach(sw_input, 1);


    /* Check output file */
    if ( !params->merge_out_file )
        progerr("Failed to provide merge output file");

    if ( isfile(params->merge_out_file) )
        progerr("Merge output file '%s' already exists.  Won't overwrite.\n", params->merge_out_file);

    /* create output */
    sw_out = SwishNew();

    sw_out->indexlist = addindexfile(sw_out->indexlist, params->merge_out_file);

        
    /* Update Economic mode */
    sw_out->Index->swap_locdata = params->swap_mode;


    /* Create an empty File - before indexing to make sure can write to the index */
    sw_out->indexlist->DB = (void *) DB_Create(sw_out, params->merge_out_file);


    merge_indexes( sw_input, sw_out );

    write_index_file( sw_out, 0, elapsedStart, cpuStart, 1, 0);

    SwishClose( sw_out );

    efree( params->merge_out_file );
}


/*************************************************************************
*   Displays all the words staring with params->keychar
*
**************************************************************************/
static void cmd_keywords( SWISH *sw, CMDPARAMS *params )
{
    if (!sw->indexlist)
        sw->indexlist = addindexfile(sw->indexlist, INDEXFILE);

    OutputKeyChar(sw, (int) (unsigned char) params->keychar);
}    


/*************************************************************************
*   Runs a swish query
*
**************************************************************************/
static void cmd_search( SWISH *sw, CMDPARAMS *params )
{
    int     rc = 0;
    double  elapsedStart = TimeElapsed();
    double  elapsedSearchStart;
    double  elapsedEnd;


    /* Set default index file, if none specified */
    if (!sw->indexlist)
        sw->indexlist = addindexfile(sw->indexlist, INDEXFILE);


    /* Set the result sort order */

    if ( params->tmpsortprops )
    {
        int sortmode = -1;      /* Ascendind by default */
        struct swline *tmplist;
        char   *field;

        for (tmplist = params->tmpsortprops; tmplist; tmplist = tmplist->next)
        {
            field = tmplist->line;
            if (tmplist->next)
            {
                if (!strcasecmp(tmplist->next->line, "asc"))
                {
                    sortmode = -1; /* asc sort */
                    tmplist = tmplist->next;
                }
                else if (!strcasecmp(tmplist->next->line, "desc"))
                {
                    sortmode = 1; /* desc sort */
                    tmplist = tmplist->next;
                }
            }
            addSearchResultSortProperty(sw, field, sortmode);
        }
    }


    if (sw->Search->maxhits <= 0)
        sw->Search->maxhits = -1;

    SwishAttach(sw, 1);


    resultHeaderOut(sw, 1, "%s\n", INDEXHEADER);

    /* print out "original" search words */
    resultHeaderOut(sw, 1, "# Search words: %s\n", params->wordlist);



    /* Get starting time */
    elapsedSearchStart = TimeElapsed();

    rc = search(sw, params->wordlist, params->structure);

    if ( rc < 0 )
        progerr( "%s %s", getErrorString( rc ), sw->lasterrorstr );
        
    

    resultHeaderOut(sw, 2, "#\n");

    if (rc > 0)
    {
        resultHeaderOut(sw, 1, "# Number of hits: %d\n", rc);

        elapsedEnd = TimeElapsed();
        resultHeaderOut(sw, 1, "# Search time: %0.3f seconds\n", elapsedEnd - elapsedSearchStart);
        resultHeaderOut(sw, 1, "# Run time: %0.3f seconds\n", elapsedEnd - elapsedStart);
        printSortedResults(sw);
        resultHeaderOut(sw, 1, ".\n");
    }
    else if (!rc )
        resultHeaderOut(sw, 1, "err: no results\n.\n");


}



/*************************************************************************
*   write_index_file -- used for both merge and for indexing
*
**************************************************************************/

static void write_index_file( SWISH *sw, int process_stopwords, double elapsedStart, double cpuStart, int merge, int is_update)
{
    int totalfiles = getfilecount(sw->indexlist);
    int stopwords = 0;

    if ( process_stopwords )
    {

        /* Proccess IgnoreLimit option */
        getPositionsFromIgnoreLimitWords(sw);

        stopwords = getNumberOfIgnoreLimitWords(sw);


        if (sw->verbose )
        {
            if (stopwords)
            {
                int pos;
        
                /* 05/00 Jose Ruiz  Adjust totalwords for IgnoreLimit ONLY  */
                sw->indexlist->header.totalwords -= stopwords;

                if (sw->indexlist->header.totalwords < 0)
                    sw->indexlist->header.totalwords = 0;

                /* Same as "stopwords" */
                printf("%d words removed by IgnoreLimit:\n", sw->indexlist->header.stopPos);

                for (pos = 0; pos < sw->indexlist->header.stopPos; pos++)
                    printf("%s, ", sw->indexlist->header.stopList[pos]);

                printf("\n");
            }
            else
                printf("no words removed.\n");

        }
    }

    if (sw->verbose)
        printf("Writing main index...\n");

    if ( !sw->indexlist->header.totalwords )
        /* Would be better to flag so db_native would know not to rename the (empty) index file */
        // printf("No unique words indexed!\n");
        progerr("No unique words indexed!");

    else
    {
    

        if (sw->verbose)
            printf("Sorting words ...\n");

        sort_words(sw, sw->indexlist);



        if (sw->verbose)
            printf("Writing header ...\n");
        fflush(stdout);

        if(is_update)
            update_header(sw,sw->indexlist->DB,sw->indexlist->header.totalwords, totalfiles);
        else
            write_header(sw, &sw->indexlist->header, sw->indexlist->DB, sw->indexlist->line, sw->indexlist->header.totalwords, totalfiles, merge);

        fflush(stdout);

        if (sw->verbose)
            printf("Writing index entries ...\n");


        write_index(sw, sw->indexlist);


        if (sw->verbose)
            printf("%d unique word%s indexed.\n", sw->indexlist->header.totalwords, (sw->indexlist->header.totalwords == 1) ? "" : "s");


        /* Sort properties -> Better search performance */

        /* First reopen the property file in read only mode for seek speed */
        DB_Reopen_PropertiesForRead( sw, sw->indexlist->DB  );

        /* This does it all */
        sortFileProperties(sw,sw->indexlist);
    }




    if (sw->verbose)
    {
        if (totalfiles)
            printf("%d file%s indexed.  %lu total bytes.  %lu total words.\n",
                totalfiles, (totalfiles == 1) ? "" : "s", sw->indexlist->total_bytes, sw->indexlist->total_word_positions);
        else
            printf("no files indexed.\n");

        printf("Elapsed time: ");
        printTime(TimeElapsed() - elapsedStart);
        printf(" CPU time: ");
        printTime(TimeCPU() - cpuStart);
        printf("\n");
    }

    printf("Indexing done!\n");


#ifdef INDEXPERMS
    chmod(sw->indexlist->line, INDEXPERMS);
#endif
}

