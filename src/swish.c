/*
$Id$
**
** Swish Originally by Kevin Hughes, kev@kevcom.com, 3/11/94
**

** Mon May  9 11:07:38 CDT 2005
** like swish2.c -- how much of this is really Kevin's original work?
**


    This file is part of Swish-e.

    Swish-e is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Swish-e is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along  with Swish-e; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    
    See the COPYING file that accompanies the Swish-e distribution for details
    of the GNU GPL and the special exception available for linking against
    the Swish-e library.
    
Mon May  9 10:57:22 CDT 2005 -- added GPL notice


*/



#include <limits.h>     // for ULONG_MAX
#include "swish.h"
#include "swstring.h"
#include "mem.h"
#include "error.h"
#include "list.h"
#include "search.h"
#include "index.h"
#include "file.h"
#include "http.h"
#include "merge.h"
#include "docprop.h"
#include "hash.h"
#include "entities.h"
#include "filter.h"
/* #include "search_alt.h" */
#include "result_output.h"
#include "result_sort.h"
#include "db.h"
#include "fs.h"
#include "swish_words.h"
#include "extprog.h"
#include "metanames.h"
#include "proplimit.h"
#include "parse_conffile.h"
#include "date_time.h"
#include "dump.h"
#include "keychar_out.h"
#ifdef HAVE_ZLIB
#include <zlib.h>
#endif
#include "headers.h"
#include "stemmer.h"
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
    {"INDEX_WORD_COUNT", DEBUG_INDEX_WORD_COUNT, "List number of words in all files"},
    {"INDEX_METANAMES", DEBUG_INDEX_METANAMES, "List metaname table stored in index"},
    {"INDEX_ALL", DEBUG_INDEX_ALL, "Dump data ALL above data from index file"},
    {"LIST_FUZZY_MODES", DEBUG_LIST_FUZZY, "List fuzzy options available\n\n-- indexing --\n"},

    /* These trace indexing */
    {"INDEXED_WORDS", DEBUG_WORDS, "Display words as they are indexed"},
    {"PARSED_WORDS", DEBUG_PARSED_WORDS, "Display words as they are parsed from source"},
    {"PROPERTIES", DEBUG_PROPERTIES, "Display properties associted with each file as they are indexed"},
    {"REGEX", DEBUG_REGEX, "Debug regular expression processing"},
    {"PARSED_TAGS", DEBUG_PARSED_TAGS, "Show meta tags as they are found"},
    {"PARSED_TEXT", DEBUG_PARSED_TEXT, "Show text as it's parsed"},
};



/* Parameters read from the command line, that are not stored in *SWISH */
typedef struct
{
    CMD_MODE    run_mode;           /* selected run mode.  Default is MODE_SEARCH */
    char        keychar;            /* for dumping words */


    /* Search related params */
    char        *query;             /* Query string */
    int          PhraseDelimiter;   /* Phrase delimiter char */
    int          structure;         /* Structure for limiting to HTML tags */
    struct swline *sort_params;     /* sort properties */
    LIMIT_PARAMS *limit_params;     /* for storing -L command line settings */

    int         query_len;          /* length of buffer */

    struct swline *disp_props;      /* extra display props */
    int         beginhits;          /* starting hit number */
    int         maxhits;            /* total hits to display */


    struct swline *conflist;        /* Configuration file list */

    int         hasverbose;         /* flag if -v was used */

    int         index_read_only;    /* flag to not allow indexing or merging */
    int         swap_mode;

    char       *merge_out_file;     /* the output file for merge */
}
CMDPARAMS;




/************* TOC ***************************************/
static CMDPARAMS *new_swish_params(void);
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
static void write_index_file( SWISH *sw, int process_stopwords, double elapsedStart, double cpuStart, int merge);

static char **fetch_search_params(SWISH *sw, char **argv, CMDPARAMS *params, char switch_char );
static char **fetch_indexing_params(SWISH *sw, char **argv, CMDPARAMS *params, char switch_char );
static void display_result_headers( RESULTS_OBJECT *results );
static void swline_header_out( SWISH *sw, int v, char *desc, struct swline *sl );

static SWISH  *swish_new();
static void    swish_close(SWISH * sw);


struct _indexing_data_source_def *IndexingDataSource;

/************* TOC ***************************************/


int     main(int argc, char **argv)
{
    SWISH  *sw;
    CMDPARAMS *params;

    setlocale(LC_ALL, "");


    /* Start a session */
    sw = swish_new();            /* Get swish handle */


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
        case MODE_REMOVE:
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


    swish_close(sw);

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
    printf("          [-m num] [-t str] [-d delim] [-H (num)] [-x output_format] \\\n");
    printf("          [-R rank_scheme] [-L prop low high]\n");
    printf("    swish -k (char|*) [-f file1 file2 ...]\n");
    printf("    swish -M index1 index2 ... outputfile\n");
    printf("    swish -N /path/to/compare/file\n");
    printf("    swish -V\n");
    putchar('\n');
    printf("options: defaults are in brackets\n");

    printf("         -b : begin results at this number\n");
    printf("         -c : configuration file(s) to use for indexing\n");
    printf("         -d : next param is delimiter.\n");
    printf("         -E : Append errors to file specified, or stderr if file not specified.\n");
    printf("         -e : \"Economic Mode\": The index proccess uses less RAM.\n");
    printf("         -f : index file to create or file(s) to search from [%s]\n", INDEXFILE);
    printf("         -H : \"Result Header Output\": verbosity (0 to 9)  [1].\n");
    printf("         -i : create an index from the specified files\n");

#ifdef ALLOW_FILESYSTEM_INDEXING_DATA_SOURCE
    printf("              for \"-S fs\" - specify a list of files or directories\n");
#endif

#ifdef ALLOW_HTTP_INDEXING_DATA_SOURCE
    printf("              for \"-S http\" - specify a list of URLs\n");
#endif

#ifdef ALLOW_EXTERNAL_PROGRAM_DATA_SOURCE
    printf("              for \"-S prog\" - specify a list of programs or the string \"stdin\"\n");
#endif

    printf("         -k : Print words starting with a given char.\n");
    printf("         -l : follow symbolic links when indexing\n");
    printf("         -L : Limit results to a range of property values\n");
    printf("         -M : merges index files\n");
    printf("         -m : the maximum number of results to return [defaults to all results]\n");
    printf("         -N : index only files with a modification date newer than path supplied\n");
    printf("         -P : next param is Phrase delimiter.\n");
    printf("         -p : include these document properties in the output \"prop1 prop2 ...\"\n");
    printf("         -R : next param is Rank Scheme number (0 to 1)  [0].\n");
#ifdef USE_BTREE
        printf("         -r : remove: remove files from index\n");
#endif

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


    printf("         -s : sort by these document properties in the output \"prop1 prop2 ...\"\n");
    printf("         -T : Trace options ('-T help' for info)\n");
    printf("         -t : tags to search in - specify as a string\n");
    printf("              \"HBthec\" - in Head|Body|title|header|emphasized|comments\n");
#ifdef USE_BTREE
    printf("         -u : update: adds files to existing index\n");
#endif
    printf("         -V : prints the current version\n");
    printf("         -v : indexing verbosity level (0 to 3) [-v %d]\n", VERBOSE);
    printf("         -w : search for words \"word1 word2 ...\"\n");
    printf("         -W : next param is ParserWarnLevel [-W 2]\n");
    printf("         -x : \"Extended Output Format\": Specify the output format.\n");
    printf("\n");
    printf("version: %s\n docs: http://swish-e.org\n Scripts and Modules at: (libexecdir) = %s\n", VERSION, get_libexec());
    exit(1);
}

static void    printversion()
{
    printf("SWISH-E %s\n", VERSION );
    exit(0);
}




/*
  -- init swish structure
*/

/*************************************************************************
* swish_new -- create a general purpose swish structure
*
*  Note that initModule_* code is called even when it's not going to be used
*  (e.g. initModule_HTTP is called when searching).
*
*
**************************************************************************/

static SWISH  *swish_new()

{
    SWISH  *sw = SwishNew();


    /* Additional modules needed for indexin (which we are not sure about yet... */
    initModule_ResultSort(sw);
    initModule_Filter(sw);
    initModule_Entities(sw);  /* used only by the old HTML parser -- not long to live */
    initModule_Index(sw);
    initModule_FS(sw);
    initModule_HTTP(sw);
    initModule_Prog(sw);

    return (sw);
}


/*************************************************************************
* swish_close -- free up a general purpose swish structure
*
*  NOTE: ANY CHANGES HERE SHOULD ALSO BE MADE IN swish2.c:SwishClose()
*
*  SwishClose is search related only
*
**************************************************************************/


static void    swish_close(SWISH * sw)
{

    if (!sw)
        return;

    free_swish_memory(sw);


    /* Free specific data related to indexing */

    freeModule_Filter(sw);
    freeModule_Entities(sw);
    freeModule_Index(sw);
    freeModule_ResultSort(sw);
    freeModule_FS(sw);
    freeModule_HTTP(sw);
    freeModule_Prog(sw);


    /* Free ReplaceRules regular expressions */
    free_regex_list(&sw->replaceRegexps);

    /* Free ExtractPath list */
    free_Extracted_Path(sw);

    /* FileRules?? $$$ */

    /* meta name for ALT tags */
    if ( sw->IndexAltTagMeta )
    {
        efree( sw->IndexAltTagMeta );
        sw->IndexAltTagMeta = NULL;
    }


    freeSwishConfigOptions( sw );  // should be freeConfigOptions( sw->config )

    efree(sw);
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

    for (i = 0; i < (int)(sizeof(debug_map) / sizeof(debug_map[0])); i++)
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

    params->PhraseDelimiter = PHRASE_DELIMITER_CHAR;
    params->structure = IN_FILE;


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
    if ( params->disp_props )
        freeswline( params->disp_props );

    if ( params->conflist )
        freeswline( params->conflist );

    if ( params->query )
        efree( params->query );

    if ( params->sort_params )
        freeswline( params->sort_params );

    if ( params->limit_params )
        ClearLimitParams( params->limit_params );


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
    char *w;




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

        switch (c)
        {

            /* Search related options */

            case 'w':  /* query string */
            case 'L':  /* Limit range */
            case 'P':  /* phrase char */
            case 't':  /* struture match */
            case 's':  /* sort */
            case 'b':  /* begin location */
            case 'm':  /* max hits */
            case 'H':  /* Header display control */
            case 'x':  /* extended format */
            case 'p':  /* old-style display properties */
            case 'd':  /* old-style custom delimiter */
            case 'o':  /* don't use pre-sorted indexes */
            case 'R':  /* Ranking Scheme -- default is 1 */
                argv = fetch_search_params( sw, argv, params, c );
                break;

            /* Indexing options */

            case 'i':  /* input files for indexing */
            case 'S':  /* data Source */
            case 'c':  /* config file */
            case 'v':  /* verbose indexing - not really limited to indexing */
            case 'W':  /* ParserWarnLevel - also configurable in conf file */
            case 'N':  /* limit by date */
            case 'l':  /* follow symbolic links */
            case 'e':  /* economy indexing mode (also for merge) */
                argv = fetch_indexing_params( sw, argv, params, c );
                break;



            /* Index file(s) selection */

            case 'f':
            {
                if ( !is_another_param( argv ) )
                    progerr(" '-f' requires list of index files.");

                while ( (w = next_param( &argv )) )
                    addindexfile(sw, w);

                break;
            }


            /* words to dump from index */

            case 'k':
            {
                if ( !(w = next_param( &argv )) )
                    progerr(" '-k' requires a character (or '*').");

                if ( strlen( w ) != 1 )
                    progerr(" '-k' requires a character (or '*').");


                params->run_mode = MODE_WORDS;
                params->keychar = w[0];

                return;  /* nothing else to look for */
            }


            /* print the version number */

            case 'V':
                printversion();



            case 'h':
            case '?':
                usage();



            /* Merge settings */

            case 'M':
            {
                if ( !is_another_param( argv )  )
                    progerr(" '-M' requires an output file name.");

                params->run_mode = MODE_MERGE;

                while ( (w = next_param( &argv )) )
                {
                    /* Last one listed is the output file */
                    if ( is_another_param( argv  ) )
                        addindexfile(sw, w);
                    else
                        params->merge_out_file = estrdup( w );
                }

                break;
            }



            /* Debugging options */

            case 'T':
            {
                while ( (w = next_param( &argv )) )
                {
                    unsigned int bit;

                    if ((bit = isDebugWord( w, params) ))
                        DEBUG_MASK |= bit;
                    else
                        progerr("Invalid debugging option '%s'.  Use '-T help' for help.", w);

                }

#if defined(_WIN32) || defined(__CYGWIN__)
		/* 
		 * DEBUG_MASK: can't sum constants imported from a DLL
		 * (see: "man ld" under --enable-auto-import)
		 * 2005-05-12 - David L Norris 
		 */
		volatile unsigned int DEBUG_MASK_HACK = DEBUG_MASK;
		if ( DEBUG_MASK_HACK & DEBUG_LIST_FUZZY )
#else
                if ( DEBUG_MASK & DEBUG_LIST_FUZZY )
#endif
                {
                    dump_fuzzy_list();
                    exit(1);
                }
                break;
            }



            /* Set where errors go */

            case 'E':
            {
                if ( !is_another_param( argv ) )
                    set_error_handle( stderr );  // -E alone goes to stderr

                else
                {
                    FILE *f;
                    w = next_param( &argv );
                    f = fopen( w, "a" );
                    if ( !f )
                        progerrno("Failed to open Error file '%s' for appending: ", w );

                    set_error_handle( f );
                }

                break;
            }

            case 'u':
            case 'r':

#ifndef USE_BTREE
                progerr("Must compile swish-e with --enable-incremental to use -%c option",c);
#else
            {
                int mode = ( 'u' == c )
                        ? MODE_UPDATE
                        : MODE_REMOVE;


                /* Make sure not trying to mix modes at same time */

                if ( MODE_UPDATE == params->run_mode || MODE_REMOVE == params->run_mode )
                    if ( mode != params->run_mode )
                        progerr("Cannot mix -u (update) and -r (remove) indexing modes.");


                params->run_mode = mode;

                if ( is_another_param( argv ) )
                    progerr("Option -%c does not take a parameter -- use -i to list paths", c );

                break;
            }
#endif


            default:
                progerr("Unknown switch '-%c'.  Use -h for options.", c );
        }
    }
}


/*************************************************************************
*   Set config options for the indexing switches
*
**************************************************************************/

static char **fetch_indexing_params(SWISH *sw, char **argv, CMDPARAMS *params, char switch_char )
{
    char *w;

    switch (switch_char)
    {

        /* files to index */
        case 'i':
        {
            if ( !is_another_param( argv ) )
                progerr(" '-i' requires a list of things to index.");

            /* Set run_mode to index, unless in update/remove mode */

            if ( MODE_UPDATE != params->run_mode && MODE_REMOVE != params->run_mode )
                params->run_mode = MODE_INDEX;

            while ( (w = next_param( &argv )) )
                sw->dirlist = addswline(sw->dirlist, w );

            break;
        }


        /* Data source */

        case 'S':
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

            break;
        }




        /* config file list */

        case 'c':
        {
            if ( !is_another_param( argv ) )
                progerr(" '-c' requires one or more configuration files.");


            /* Set one of the indexing modes when specifying -c */

            if ( MODE_UPDATE != params->run_mode && MODE_REMOVE != params->run_mode )
                params->run_mode = MODE_INDEX;

            while ( (w = next_param( &argv )) )
                params->conflist = addswline(params->conflist, w);
            break;
        }



        /* Follow symbolic links */

        case 'l':
            sw->FS->followsymlinks = 1;
            break;



        /* Save the time for limiting indexing by a file date */

        case 'N':
        {
            struct stat stat_buf;

            if ( !(w = next_param( &argv )) )
                progerr("-N requires a path to a local file");

            if (stat( w, &stat_buf))
                progerrno("Bad path '%s' specified with -N: ", w );

            sw->mtime_limit = stat_buf.st_mtime;
            break;
        }


        /* Econ mode */

        case 'e':
            params->swap_mode = 1;      /* "Economic mode": Uses less RAM */
            break;



        /* verbose while indexing */

        case 'v':
        {
            params->hasverbose = 1;
            sw->verbose = get_param_number( &argv, switch_char );
            break;
        }
        
        
        /* ParserWarnLevel */
        case 'W':
            sw->parser_warn_level = get_param_number( &argv, switch_char );
            break;



        default:
            progerr("Invalid index switch option '%s'", switch_char );
    }

    return argv;
}



/*************************************************************************
*   Set config options for the search switches
*
**************************************************************************/


static char **fetch_search_params(SWISH *sw, char **argv, CMDPARAMS *params, char switch_char )
{
    char *w;

    /*** Display Properties Setup ***/
    if ( !sw->ResultOutput )
        initModule_ResultOutput(sw);



    switch (switch_char)
    {
        /* search words */
        case 'w':
        {
            if ( !is_another_param( argv ) )
                progerr(" '-w' requires list of search words.");

            if ( !params->query )
            {
                params->query_len = 200;
                params->query = (char *)emalloc( params->query_len + 1 );
                params->query[0] = '\0';
            }


            while ( (w = next_param( &argv )) )
            {
                /* don't add blank words */
                if (w[0] == '\0')
                    continue;

                if ((int)( strlen(params->query) + strlen(" ") + strlen(w) ) >= params->query_len)
                {
                    params->query_len = strlen(params->query) + strlen(" ") + strlen(w) + 200;
                    params->query = (char *) erealloc(params->query, params->query_len + 1);
                }

                params->run_mode = MODE_SEARCH;
                sprintf(params->query, "%s%s%s", params->query, (params->query[0] == '\0') ? "" : " ", w);
            }
            break;
        }


        /* Set limit values */

        case 'L':
        {
            if ( !( is_another_param( argv ) && is_another_param( argv + 1  ) && is_another_param( argv + 2 )) )
                progerr("-L requires three parameters <propname> <lorange> <highrange>");


            params->limit_params = setlimit_params(sw, params->limit_params, argv[1], argv[2], argv[3]);
            if ( sw->lasterror )
                SwishAbortLastError( sw );

            argv += 3;
            break;
        }


        /* Custom Phrase Delimiter - Jose Ruiz 01/00 */

        case 'P':
        {
            if ( !(w = next_param( &argv )) )
                progerr("'-P' requires a phrase delimiter.");

            params->PhraseDelimiter = (int) w[0];
            break;
        }


        /* limit by structure */

        case 't':
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
            break;
        }


        /* sort properties */

        case 's':
        {
            if ( !is_another_param( argv ) )
                progerr(" '-s' requires list of sort properties.");

            while ( (w = next_param( &argv )) )
                params->sort_params = addswline(params->sort_params, w);

            break;
        }


        /* Set begin hit location */

        case 'b':
            params->beginhits = get_param_number( &argv, switch_char );
            break;


        /* Set max hits */

        case 'm':
            params->maxhits = get_param_number( &argv, switch_char );
            break;



        /* $$$ These need better error reporting */

        /* Extended format */

        case 'x':
        {
            /* Jose Ruiz 09/00 */
            /* Search proc will show more info */
            /* rasc 2001-02  extended  -x fmtstr */

            if ( !(w = next_param( &argv )) )
                progerr("'-x' requires an output format string.");

            {
                char   *s;
                /* check if name is a predefined format - not implemented */
                s = hasResultExtFmtStr(sw, w);
                sw->ResultOutput->extendedformat = (s) ? s : w;
                initPrintExtResult(sw, sw->ResultOutput->extendedformat);
            }
            break;
        }



        /* Search header output control */
        case 'H':
            sw->headerOutVerbose = get_param_number( &argv, switch_char );
            break;



        /* display properties */

        case 'p':
        {
            if ( !is_another_param( argv ) )
                progerr(" '-p' requires list of properties.");

            while ( (w = next_param( &argv )) )
                params->disp_props = addswline( params->disp_props, w);

            break;
        }



        /* Set the output custom delimiter */

        case 'd':
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

                for ( j=0, i=0; i < (int)strlen( w ); i++ )
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
            break;
        }


        /* Ranking Scheme */
        case 'R':
            sw->RankScheme = get_param_number( &argv, switch_char );
            break;
            

        /* Ignore sorted indexes */

        case 'o':
            sw->ResultSort->isPreSorted = 0;
            break;


        default:
            progerr("Invalid search switch option '%c'\n", switch_char );
            break;

    }
    return argv;
}



/*************************************************************************
*   Returns true if we think the program is called swish-search
*   offers no real security
*
**************************************************************************/
static int check_readonly_mode( char *prog )
{
    char   *tmp = prog + strlen(prog) - strlen("swish-search");

    if ( tmp < prog )
        return 0;

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
        addindexfile(sw, INDEXFILE);

    while ( sw->indexlist != NULL )
    {

        DB_decompress(sw, sw->indexlist, params->beginhits, params->maxhits);
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
    {
        struct swline *tmp = params->conflist;
        while ( tmp != NULL)
        {
            getdefaults(sw, tmp->line, &hasdir, &hasindex, params->hasverbose);
            tmp = tmp->next;
        }
    }


    /* Default index file */
    if ( sw->indexlist == NULL )
        addindexfile(sw, INDEXFILE);


    if (!hasdir)
        progerr("Specify directories or files to %s.", MODE_INDEX == params->run_mode ? "index" : MODE_UPDATE == params->run_mode ? "update" : "remove" );


    if (sw->verbose < 0)
        sw->verbose = 0;

    /* Update Economic mode */
    sw->Index->swap_locdata = params->swap_mode;


    /* Check for UPDATE_MODE jmruiz 2002/03 */
    if ( MODE_UPDATE == params->run_mode || MODE_REMOVE == params->run_mode )
#ifndef USE_BTREE
        progerr("Invalid operation mode '%d': Update mode only supported with USE_BTREE feature", (int)params->run_mode);
#else
    {
        /* Set update_mode */
        sw->Index->update_mode = params->run_mode;

        if ( !open_single_index( sw, sw->indexlist, DB_READWRITE ) )
            SwishAbortLastError( sw );

        /* Adjust file number to start after the last file number in the index */
        sw->Index->filenum = sw->indexlist->header.totalfiles;
    }
#endif


    else
    {
        /* Create an empty File - before indexing to make sure can write to the index */
        sw->indexlist->DB = (void *) DB_Create(sw, sw->indexlist->line);
        if ( sw->lasterror )
            SwishAbortLastError( sw );
    }


    /* This should be printed by the module that's reading the source */
    if (sw->verbose >= 1)
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

    write_index_file( sw, 1, elapsedStart, cpuStart, 0);
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
    if ( !SwishAttach(sw_input) )
        SwishAbortLastError( sw_input );


    /* Check output file */
    if ( !params->merge_out_file )
        progerr("Failed to provide merge output file");

    if ( isfile(params->merge_out_file) )
        progerr("Merge output file '%s' already exists.  Won't overwrite.\n", params->merge_out_file);

    /* create output */
    sw_out = swish_new();
    sw_out->verbose = sw_input->verbose;
    sw_out->headerOutVerbose = sw_input->headerOutVerbose;


    addindexfile(sw_out, params->merge_out_file);


    /* Update Economic mode */
    sw_out->Index->swap_locdata = params->swap_mode;


    /* Create an empty File - before indexing to make sure can write to the index */
    sw_out->indexlist->DB = (void *) DB_Create(sw_out, params->merge_out_file);
    if ( sw_out->lasterror )
        SwishAbortLastError( sw_out );


    merge_indexes( sw_input, sw_out );


    write_index_file( sw_out, 0, elapsedStart, cpuStart, 1);

    swish_close( sw_out );

    efree( params->merge_out_file );
}


/*************************************************************************
*   Displays all the words staring with params->keychar
*
**************************************************************************/
static void cmd_keywords( SWISH *sw, CMDPARAMS *params )
{
    if (!sw->indexlist)
        addindexfile(sw, INDEXFILE);

    OutputKeyChar(sw, (int) (unsigned char) params->keychar);
}


/*************************************************************************
*   Runs a swish query
*
**************************************************************************/
static void cmd_search( SWISH *sw, CMDPARAMS *params )
{
    double  elapsedStart = TimeElapsed();
    double  elapsedSearchStart;
    double  elapsedEnd;
    SEARCH_OBJECT *srch;
    RESULTS_OBJECT *results;


    /* Set default index file, if none specified */
    if (!sw->indexlist)
        addindexfile(sw, INDEXFILE);


    /* Open index files */

    if ( !SwishAttach(sw) )
        SwishAbortLastError( sw );


    srch = New_Search_Object( sw, params->query );
    if ( sw->lasterror )
        SwishAbortLastError( sw );


    srch->PhraseDelimiter = params->PhraseDelimiter;
    srch->structure       = params->structure;


    if ( params->sort_params )
    {
        srch->sort_params = params->sort_params;
        params->sort_params = NULL;
    }

    if ( params->limit_params )
    {
        srch->limit_params =  params->limit_params;
         params->limit_params = NULL;
    }



    /* Set up for -p printing */
    if ( params->disp_props )
    {
        struct swline *tmp = params->disp_props;
        while ( tmp )
        {
            addSearchResultDisplayProperty(sw, tmp->line);
            tmp = tmp->next;
        }

        initSearchResultProperties(sw);
    }




    /* Get starting time */
    elapsedSearchStart = TimeElapsed();



    /* Run the query */

    results = SwishExecute( srch, NULL );
    display_result_headers(results);

    if ( sw->lasterror )
        SwishAbortLastError( sw );



    if (results->total_results > 0)
    {
        resultHeaderOut(sw, 1, "# Number of hits: %d\n", results->total_results);

        elapsedEnd = TimeElapsed();
        resultHeaderOut(sw, 1, "# Search time: %0.3f seconds\n", elapsedEnd - elapsedSearchStart);
        resultHeaderOut(sw, 1, "# Run time: %0.3f seconds\n", elapsedEnd - elapsedStart);

        /* Display the results */
        printSortedResults(results, params->beginhits, params->maxhits);

        resultHeaderOut(sw, 1, ".\n");
    }
    else
        resultHeaderOut(sw, 1, "err: no results\n.\n");


    Free_Results_Object( results );
    Free_Search_Object( srch );



    freeModule_ResultOutput(sw);

}



/*************************************************************************
*   write_index_file -- used for both merge and for indexing
*
**************************************************************************/

static void write_index_file( SWISH *sw, int process_stopwords, double elapsedStart, double cpuStart, int merge)
{
    int totalfiles = getfilecount(sw->indexlist) - sw->indexlist->header.removedfiles;  /* just for display */
    int stopwords = 0;
    struct swline *cur_line;

        /* Coalesce all remaining locations */
    coalesce_all_word_locations(sw, sw->indexlist);

    if ( process_stopwords )
    {

        /* Proccess IgnoreLimit option */
        getPositionsFromIgnoreLimitWords(sw);

        stopwords = getNumberOfIgnoreLimitWords(sw);


        if (sw->verbose )
        {
            if (stopwords)
            {
                /* 05/00 Jose Ruiz  Adjust totalwords for IgnoreLimit ONLY  */
                /* 2002-07 jmruiz
                **This is already done in getPositionsFromIgnoreLimitWords
                ** sw->indexlist->header.totalwords -= stopwords;
                */

                if (sw->indexlist->header.totalwords < 0)
                    sw->indexlist->header.totalwords = 0;

                /* Same as "stopwords" */
                printf("%d words removed by IgnoreLimit:\n", stopwords);

                for (cur_line = sw->Index->IgnoreLimitWords; cur_line; cur_line = cur_line->next )
                    printf("%s, ", cur_line->line);

                printf("\n");

                freeswline( sw->Index->IgnoreLimitWords );
            }
            else
                printf("no words removed.\n");

        }
    }

    if (sw->verbose)
        printf("Writing main index...\n");

    if ( !sw->indexlist->header.totalwords )
    {
        /* Would be better to flag so db_native would know not to rename the (empty) index file */
        // printf("No unique words indexed!\n");

        progerr("No unique words indexed!");
    }




    if (sw->verbose)
        printf("Sorting words ...\n");

    sort_words(sw);



    if (sw->verbose)
        printf("Writing header ...\n");

    fflush(stdout);

    write_header( sw, merge );

    fflush(stdout);

    if (sw->verbose)
        printf("Writing index entries ...\n");


    write_index(sw, sw->indexlist);


    if (sw->verbose)
    {
        int totalwords = sw->indexlist->header.totalwords;
        printf("%s unique word%s indexed.\n", comma_long( totalwords ), (totalwords == 1) ? "" : "s");
    }


    /* Sort properties -> Better search performance */

    /* First reopen the property file in read only mode for seek speed */

    DB_Reopen_PropertiesForRead( sw, sw->indexlist->DB  );
    if ( sw->lasterror )
        SwishAbortLastError( sw );



    /* This sorts all the properties */
    sortFileProperties(sw,sw->indexlist);




    if (sw->verbose)
    {
        if (totalfiles)
        {
            printf("%s file%s indexed.  ",
            comma_long( totalfiles ), (totalfiles == 1) ? "" : "s"); /* common_long is not thread safe -- shares memory */

            printf("%s total bytes.  ", comma_long(sw->indexlist->total_bytes) );

            printf("%s total words.\n", comma_long(sw->indexlist->total_word_positions_cur_run) );
        }

        else
            printf("no files indexed.\n");

        printf("Elapsed time: ");
        printTime(TimeElapsed() - elapsedStart);
        printf(" CPU time: ");
        printTime(TimeCPU() - cpuStart);
        printf("\n");

        printf("Indexing done!\n");
    }

}




/*****************************************************************
* dispaly_result_headers -- prints the result header list
*
******************************************************************/

static void display_result_headers( RESULTS_OBJECT *results )
{
    SWISH       *sw = results->sw;
    DB_RESULTS  *db_results = results->db_results;


    resultHeaderOut(sw, 1, "%s\n", INDEXHEADER);  /* print # SWISH format: <version> */

    /* print out "original" search words */
    resultHeaderOut(sw, 1, "# Search words: %s\n", results->query);

    while ( db_results )
    {
        IndexFILE *indexf = db_results->indexf;

        resultHeaderOut(sw, 2, "#\n# Index File: %s\n", indexf->line);


        /* Print all the headers */
        print_index_headers( indexf );


        /* $$$ move to headers.c and fix as it's corrupting memory */
       // translatecharHeaderOut(sw, 2, &indexf->header);



        /* Show parsed words */
        resultHeaderOut(sw, 2, "# Search words: %s\n", results->query);
        swline_header_out( sw, 2, "# Parsed Words: ", db_results->parsed_words );
        swline_header_out( sw, 1, "# Removed stopwords: ", db_results->removed_stopwords );

        db_results = db_results->next;
    }

    resultHeaderOut(sw, 2, "#\n");
}



static void swline_header_out( SWISH *sw, int v, char *desc, struct swline *sl )
{
    resultHeaderOut(sw, v, desc);

    while (sl)
    {
        resultHeaderOut(sw, v, "%s ", sl->line);
        sl = sl->next;
    }

    resultHeaderOut(sw, v, "\n");
}


