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
** Added support for METADATA
** G. Hill ghill@library.berkeley.edu 3/18/97
**
** Added printing of common words
** G.Hill 4/7/97 ghill@library.berkeley.edu
**
** change sprintf to snprintf to avoid corruption
** added safestrcpy() macro to avoid corruption from strcpy overflow
** added INDEX_READ_ONLY option to make "safe" exe that can only READ index files
**   (INDEX_READ_ONLY may be defined in the Makefile or by uncommenting define)
** SRE 11/17/99
**
** fixed cast to int and unused variable problems pointed out by "gcc -Wall"
** fixed variable declarations for INDEX_READ_ONLY to avoid unused variables
** SRE 2/22/00
**
** 2001-02-12 rasc    errormsg "print" changed...
** 2001-03-13 rasc    result header output routine  -X?  (changed -H <n>)
**
*/

#define MAIN_FILE

#include "swish.h"

#include "string.h"
#include "error.h"
#include "list.h"
#include "search.h"
#include "index.h"
#include "file.h"
#include "http.h"
#include "merge.h"
#include "docprop.h"
#include "mem.h"
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



/* vvvvvvvvvv example only vvvvvvvvvvvvv*/

unsigned int Dump_Index = 0;

unsigned int DEBUG_MASK = 0;

typedef struct
{
    char   *name;
    unsigned int bit;
    char   *description;
} DEBUG_MAP;

static DEBUG_MAP debug_map[] = {
    /* These dump data from the index file */
    { "INDEX_HEADER", DEBUG_INDEX_HEADER, "Show the headers from the index" },
    { "INDEX_WORDS", DEBUG_INDEX_WORDS, "List words stored in index" },
    { "INDEX_WORDS_ONLY", DEBUG_INDEX_WORDS_ONLY, "List only words, one per line, stored in index" },
    { "INDEX_WORDS_META", DEBUG_INDEX_WORDS_META, "List only words and associated metaID separated by a tab" },
    { "INDEX_WORDS_FULL", DEBUG_INDEX_WORDS_FULL, "List words stored in index (more verbose)" },
    { "INDEX_STOPWORDS", DEBUG_INDEX_STOPWORDS, "List stopwords stored in index" },
    { "INDEX_FILES", DEBUG_INDEX_FILES, "List file data stored in index" },
    { "INDEX_METANAMES", DEBUG_INDEX_METANAMES, "List metaname table stored in index" },
    { "INDEX_ALL", DEBUG_INDEX_ALL, "Dump data ALL above data from index file" },

    /* These trace indexing */
    { "INDEXED_WORDS", DEBUG_WORDS, "Display words as they are indexed" },
    { "PARSED_WORDS", DEBUG_PARSED_WORDS, "Display words as they are parsed from source" },
    { "PROPERTIES", DEBUG_PROPERTIES, "Display properties associted with each file as they are indexed" },
};

unsigned int isDebugWord( char *word )
{
    int i,help;

    help = strcasecmp( word, "help" ) == 0;

    if ( help )
        printf("\nAvailable debugging options for swish-e:\n");
    
    for (i = 0; i < sizeof(debug_map)/sizeof(debug_map[0]); i++)
        if ( help )
            printf("  %20s => %s\n", debug_map[i].name, debug_map[i].description );
        else
            if ( strcasecmp( debug_map[i].name, word ) == 0 )
            {
                if ( strncasecmp( word, "INDEX_", 6 ) == 0 )
                    Dump_Index++;
                    
                return debug_map[i].bit;
            }

    if ( help )
        exit(1);

    return 0;
}



/* ^^^^^^^^^^ example only ^^^^^^^^^^^^^*/



int main(int argc, char **argv)
{
char c;
char *word=NULL, *wordlist=NULL, *maxhitstr=NULL, *structstr=NULL, *beginhitstr=NULL;
int lenword=0, lenwordlist=0, lenmaxhitstr=0, lenstructstr=0, lenbeginhitstr=0;
int i, hasindex, hasdir, hasconf, hasverbose, structure, stopwords, index, decode, merge, hasMetaName;
struct swline *conflist, *tmplist;
IndexFILE *tmplistindex;
SWISH *sw;
int rc=0,sortmode;
char *field;
int totalfiles, pos, j;
long starttime, stoptime;
int lentmpindex1=0;
char *tmpindex1=NULL;
int lentmpindex2=0;
char *tmpindex2=NULL;
int lenindex1=0;
char *index1=NULL;
int lenindex2=0;
char *index2=NULL;
int lenindex3=0;
char *index3=NULL;
int lenindex4=0;
char *index4=NULL;
int INDEX_READ_ONLY;
char *tmp;
int swap_mode=0; /* No swap */
char keychar=0;
struct swline *tmpprops=NULL,*tmpsortprops=NULL;
double search_starttime, run_starttime, endtime;
struct stat stat_buf;



    run_starttime = TimeHiRes();

	starttime=0L;

	if(!lenindex1)index1=emalloc((lenindex1=MAXSTRLEN)+1);index1[0]='\0';
	if(!lenindex2)index2=emalloc((lenindex2=MAXSTRLEN)+1);index2[0]='\0';
	if(!lenindex3)index3=emalloc((lenindex3=MAXSTRLEN)+1);index3[0]='\0';
	if(!lenindex4)index4=emalloc((lenindex4=MAXSTRLEN)+1);index4[0]='\0';
	if(!lentmpindex1)tmpindex1=emalloc((lentmpindex1=MAXSTRLEN)+1);tmpindex1[0]='\0';
	if(!lentmpindex2)tmpindex2=emalloc((lentmpindex2=MAXSTRLEN)+1);tmpindex2[0]='\0';
	
        /* If argv[0] is swish-search then you cannot index and/or merge */
        if(!(tmp=strrchr(argv[0],DIRDELIMITER)))
                tmp=argv[0];
        else tmp++;
                /* We must ignore case for WIN 32 */
        if(strcasecmp(tmp,"swish-search")==0)
                INDEX_READ_ONLY=1;
        else
                INDEX_READ_ONLY=0;

	index = decode = merge = 0;
	hasindex = hasdir = hasconf = hasverbose = hasMetaName = 0;
	stopwords = 0;
	conflist = tmplist = NULL;
	structure = 1;
	if(!lenwordlist)wordlist=(char *)emalloc((lenwordlist=MAXSTRLEN) + 1);
	wordlist[0] = '\0';
	if(!lenstructstr)structstr=(char *)emalloc((lenstructstr=MAXSTRLEN)+1);
	structstr[0] = '\0';
	setlocale(LC_CTYPE,"");


	/* init cmd options, set default values   */
	sw=SwishNew();				/* Get swish handle */
	

	/* By default we are set up to use the first data source in the list */
 	IndexingDataSource = data_sources[0];
	

	if (argc == 1)
		usage();


	while (--argc > 0) 
	{
		++argv;
		if ((*argv)[0] != '-')
			usage();

		c = (*argv)[1];
		
		if ((*argv)[2] != '\0' && isalpha((int)((unsigned char)(*argv)[2]))) /* cast to int, 2/22/00 */
			usage();

		if (c == 'i') 
		{
			index = 1;
			while ((argv + 1)[0] != '\0' && *(argv + 1)[0] != '-') {
				sw->dirlist = addswline(sw->dirlist, (++argv)[0]);
				argc--;
			}
		}
		else if (c == 'w') 
		{

			while (*(argv + 1) && *(argv + 1)[0] != '-') 
			{
				word = SafeStrCopy(word, (++argv)[0],&lenword);
				argc--;

                /* don't add blank words */
				if ( word[0] == '\0' )
                    continue;

				if((int)(strlen(wordlist) +  strlen(" ") + strlen(word)) >= lenwordlist) {
					lenwordlist =strlen(wordlist) +  strlen(" ") + strlen(word) + 200;
					wordlist = (char *) erealloc(wordlist,lenwordlist + 1);
				}
				sprintf(wordlist,"%s%s%s", wordlist, (wordlist[0] == '\0') ? "" : " ", word);
			}
		}
		else if (c == 'k') 
		{
			if ((argv + 1)[0] == '\0')
				keychar = 0;
			else {
				argv++;
				if(!argv || !argv[0])
					progerr("Specify the starting char or '*' for all words");
				else if(strlen(argv[0])>1)
					progerr("Specify the starting char or '*' for all words");
				keychar = argv[0][0];
				argc--;
			}
		}
		else if (c == 'S')
		{
		  struct _indexing_data_source_def **data_source;
		  char* opt = (++argv)[0];
		  argc--;
		  for (data_source = data_sources; *data_source != 0; data_source++ )

		    {
		      if (strcmp(opt, (*data_source)->IndexingDataSourceId) == 0)
			{
			  break;
			}
		    }
		  
		  if (!*data_source) {
		      progerr("Unknown -S option \"%s\"", opt);
		    }
		  else
		    {
		      IndexingDataSource = *data_source;
		    }  
		}
		else if (c == 's') {
		  /* -s <property_to_sort> [asc|desc] [<property_to_sort> asc|desc]* */
		  while ((argv + 1)[0] != '\0' && *(argv + 1)[0] != '-') 
			{
				tmpsortprops=addswline(tmpsortprops,(++argv)[0]);
				argc--;
			}
		}

		else if (c == 'p') {
		  /* -p <property_to_display> [<property_to_display>]* */
		  while ((argv + 1)[0] != '\0' && *(argv + 1)[0] != '-') 
			{
				tmpprops=addswline(tmpprops,(++argv)[0]);
				argc--;
			}
		}


        /*** Fix this $$$ ***/
		else if (c == 'L') {
		  /* -L property_to_limit lowrange highrange  */
		  if ( argc < 4 || *(argv + 1)[0] == '-' || *(argv + 2)[0] == '-')
		    progerr("-L requires three parameters <propname> <lorange> <highrange>");

		  SetLimitParameter(sw, argv[1], argv[2], argv[3] );
		  argv += 3;
		  argc -= 3;
		}
		

		
		else if (c == 'f') 
		{
			while ((argv + 1)[0] != '\0' && *(argv + 1)[0] != '-') 
			{
				sw->indexlist = addindexfile(sw->indexlist, (++argv)[0]);
				argc--;
			}
		}
		else if (c == 'c') 
		{
			index = 1;
			hasconf = 1;
			while ((argv + 1)[0] != '\0' && *(argv + 1)[0] != '-') {
				conflist = addswline(conflist, (++argv)[0]);
				argc--;
			}
		}
		else if (c == 'C') {
			while ((argv + 1)[0] != '\0' && *(argv + 1)[0] != '-')
			{
				conflist = addswline(conflist, (++argv)[0]);
				argc--;
			}
			if (conflist == NULL)
				progerr("Specifiy the configuration file.");
			else
				hasMetaName = 1;
		      }
		else if (c == 'l') {
			sw->FS->followsymlinks = 1;
			argc--;
		}
		else if (c == 'b') {
			if ((argv + 1)[0] == '\0')
				sw->Search->beginhits = 0;
			else {
				beginhitstr = SafeStrCopy(beginhitstr, (++argv)[0],&lenbeginhitstr);
				if (isdigit((int)beginhitstr[0]))
					sw->Search->beginhits = atoi(beginhitstr);
				else
					sw->Search->beginhits = 0;
				argc--;
			}
		}
		else if (c == 'm') {
			if ((argv + 1)[0] == '\0')
				sw->Search->maxhits = -1;
			else {
				maxhitstr = SafeStrCopy(maxhitstr, (++argv)[0],&lenmaxhitstr);
				if (lstrstr(maxhitstr, "all"))
					sw->Search->maxhits = -1;
				else if (isdigit((int)maxhitstr[0])) /* cast to int, 2/22/00 */
					sw->Search->maxhits = atoi(maxhitstr);
				else
					sw->Search->maxhits = -1;
				argc--;
			}
		}

        /* Save the time for limiting indexing by a file date */
        else if (c == 'N') {
            if ( (argv + 1)[0] == '\0' || *(argv + 1)[0] == '-')
                progerr("-N requires a path to a local file");

            if ( stat( (++argv)[0], &stat_buf ) )
                progerr("Bad path specified with -N");

            sw->mtime_limit = stat_buf.st_mtime;
            argc--;
        }
		
		else if (c == 't') {
			if ((argv + 1)[0] == '\0')
				progerr("Specify tag fields (HBtheca).");
			else {
				structure = 0;
				structstr = SafeStrCopy(structstr,(++argv)[0],&lenstructstr);
				argc--;
			}
		}
		else if (c == 'v') {
			hasverbose = 1;
			if ((argv + 1)[0] == '\0') {
				sw->verbose = 3;
				break;
			}
			else if (!isdigit((int)(argv + 1)[0][0])) /* cast to int, 2/22/00 */
				sw->verbose = 3;
			else
				sw->verbose = atoi((++argv)[0]);
			argc--;
		}
		else if (c == 'V')
			printversion();
		else if (c == 'z' || c == 'h' || c == '?')
			usage();
		else if (c == 'M') {
			merge = 1;
			while ((argv + 1)[0] != '\0' && *(argv + 1)[0] != '-') {
				sw->indexlist = addindexfile(sw->indexlist, (++argv)[0]);
				argc--;
			}
		}

		/* for demo only */
		else if (c == 'T') { 
			while ((argv + 1)[0] != '\0' && *(argv + 1)[0] != '-')
			{
			    unsigned int bit;

			    if ( (bit = isDebugWord( (++argv)[0] )) )
			    {
			        DEBUG_MASK |= bit;
      				argc--;
                }
			    else
			        progerr("Invalid debugging option '%s'.  Use '-T help' for help.", *argv );

			}
		}
		else if (c == 'D') {

			while ((argv + 1)[0] != '\0' && *(argv + 1)[0] != '-') {
				sw->indexlist = addindexfile(sw->indexlist, (++argv)[0]);
				argc--;
			}
		}
		else if (c == 'P')
		{
			/* Custom Phrase Delimiter - Jose Ruiz 01/00 */
			if (((argv + 1)[0] != '\0') && (*(argv + 1)[0] != '-'))
			{
				sw->Search->PhraseDelimiter = (int) (++argv)[0][0];
				argc--;
			}
		}
		else if (c == 'd')
		{
 			/* added 11/24/98 MG */
			if (((argv + 1)[0] != '\0') && (*(argv + 1)[0] != '-'))
			{

				sw->ResultOutput->stdResultFieldDelimiter=estrredup(sw->ResultOutput->stdResultFieldDelimiter,(++argv)[0]);
				if (strcmp(sw->ResultOutput->stdResultFieldDelimiter, "dq") == 0)
					{ sw->ResultOutput->stdResultFieldDelimiter=estrredup(sw->ResultOutput->stdResultFieldDelimiter, "\""); } /* double quote is cool */
				if (sw->ResultOutput->stdResultFieldDelimiter[0]=='\\')
				{
					switch(sw->ResultOutput->stdResultFieldDelimiter[1])
					{
						case 'f': sw->ResultOutput->stdResultFieldDelimiter[0]='\f'; break;
						case 'n': sw->ResultOutput->stdResultFieldDelimiter[0]='\n'; break;
						case 'r': sw->ResultOutput->stdResultFieldDelimiter[0]='\r'; break;
						case 't': sw->ResultOutput->stdResultFieldDelimiter[0]='\t'; break;
					}
					sw->ResultOutput->stdResultFieldDelimiter[1]='\0';
				}
				argc--;
			}
		}
		else if (c == 'e')
		{
					/* Jose Ruiz 09/00 */
			swap_mode=1;    /* "Economic mode": Uses less RAM*/
					/* The proccess is slower: Part of */
					/* info is preserved in temporal */
					/* files */
		}
		else if (c == 'x') {
					/* Jose Ruiz 09/00 */
					/* Search proc will show more info */
					/* rasc 2001-02  extended  -x fmtstr */

			if (*(argv+1)) {
			   char *s;
			   argv++;
			   s = hasResultExtFmtStr (sw, *argv);
			   sw->ResultOutput->extendedformat = (s) ? s : *argv;
			   initPrintExtResult (sw, sw->ResultOutput->extendedformat);
			   argc--;
			} else {
			   usage();
			}
		}
		else if (c == 'H') {
			  /* rasc 2001-02, 2001-03 */
			if ( *(argv+1) && isdigit ((int) **(argv+1)) ) {
			   sw->ResultOutput->headerOutVerbose = atoi(*(++argv));
			   argc--;
			} else {
			   usage();
			}
		}    
		else if (c == 'o')
		{
			    /* Ignore sorted indexes */
			sw->ResultSort->isPreSorted = 0;
		}
		else
			usage();
		if (argc == 0)
			break;
	}

	hasdir = (sw->dirlist == NULL) ? 0 : 1;
	hasindex = (sw->indexlist == NULL) ? 0 : 1;
	
	if (index && merge)
		index = 0;


	/* -D => give a dump of the contents of index files */
	// if ( decode )
	if ( Dump_Index )
	{
		if (!hasindex)
			sw->indexlist = addindexfile(sw->indexlist, INDEXFILE);
		
		while (sw->indexlist != NULL) {
			
			DB_decompress(sw,sw->indexlist);
			putchar('\n');
			
			sw->indexlist = sw->indexlist->next;
		}
	}


	/* -i or -c => Index the files */
	else if (index) 
	{
		if(INDEX_READ_ONLY) {
			/* new "safe" mode, added 11/99 to optionally build exe that cannot write index files */
			printf("CANNOT INDEX Data Source: \"%s\"\nREAD-ONLY MODE\n", IndexingDataSource->IndexingDataSourceName);
			exit(1);
		}
		printf("Indexing Data Source: \"%s\"\n", IndexingDataSource->IndexingDataSourceName);

		if (hasconf)
		{
			while (conflist != NULL) {
				getdefaults(sw,conflist->line, &hasdir, &hasindex, hasverbose);
				conflist = conflist->next;
			}
		}

		if (!hasindex)
			sw->indexlist = addindexfile(sw->indexlist, INDEXFILE);

		if (!hasdir)
			progerr("Specify directories or files to index.");
		
		if (sw->verbose < 0)
			sw->verbose = 0;
		if (sw->verbose > 4)
			sw->verbose = 4;
		if (sw->verbose)
			starttime = getTheTime();

			/* Update Economic mode */
		sw->Index->economic_flag=swap_mode;

		while (sw->dirlist != NULL) {
			if (sw->verbose) {
				printf("Indexing %s..\n",sw->dirlist->line);
				fflush(stdout);
			}
			indexpath(sw,sw->dirlist->line);
			sw->dirlist = sw->dirlist->next;
		}

		

		checkmem();
			/* Create an empty File */
		sw->indexlist->DB = (void *) DB_Create(sw, sw->indexlist->line);
		
		if (sw->verbose > 1)
			putchar('\n');
		if (sw->verbose)
			printf("Removing very common words...\n");
		fflush(stdout);

		totalfiles = getfilecount(sw->indexlist);

		stopwords = removestops(sw);
		
		if (sw->verbose) {
			if (stopwords) {
				/* 05/00 Jose Ruiz
				** Adjust totalwords
				*/
				/* sw->indexlist->header.totalwords -= stopwords; */
				if(sw->indexlist->header.totalwords<0) sw->indexlist->header.totalwords=0;
				printf("%d word%s removed.\n",
					stopwords, (stopwords == 1) ? "" : "s");
				printf("%d words removed not in common words array:\n", sw->indexlist->header.stopPos);
				for (pos = 0; pos < sw->indexlist->header.stopPos; pos++) 
					printf("%s, ", sw->indexlist->header.stopList[pos]);
				printf("\n");
			}
			else
				printf("no words removed.\n");
			printf("Writing main index...\n");
		}




		if (sw->verbose)
			printf("Sorting words ...\n");
		sort_words(sw, sw->indexlist);

		if (sw->verbose)
			printf("Writing header ...\n");
		fflush(stdout);
		write_header(sw, &sw->indexlist->header,sw->indexlist->DB, sw->indexlist->line, sw->indexlist->header.totalwords, totalfiles, 0);
		fflush(stdout);
		
		if (sw->verbose)
			printf("Writing index entries ...\n");


		write_index(sw, sw->indexlist);


		if (sw->verbose) {
			if (sw->indexlist->header.totalwords)
				printf("%d unique word%s indexed.\n",
				sw->indexlist->header.totalwords, (sw->indexlist->header.totalwords == 1) ? "" : "s");
			else
				printf("no unique words indexed.\n");
			printf("Writing file index...\n");
		}
		
		if(sw->verbose)
			printf("Writing file list ...\n");
		write_file_list(sw,sw->indexlist);

		if(sw->verbose)
			printf("Writing sorted index ...\n");
		write_sorted_index(sw,sw->indexlist);

		if (sw->verbose) 
		{
			if (totalfiles)
				printf("%d file%s indexed.\n", totalfiles,
				(totalfiles == 1) ? "" : "s");
			else
				printf("no files indexed.\n");
			
			stoptime = getTheTime();
			printrunning(starttime, stoptime);
			printf("Indexing done!\n");
		}
#ifdef INDEXPERMS
		chmod(sw->indexlist->line, INDEXPERMS);
#endif
	}


	/* -M => Merge indexes together */
	else if (merge) 
	{
		if(INDEX_READ_ONLY) {
			/* new "safe" mode, added 11/99 to optionally build exe that cannot write index files */
			printf("CANNOT MERGE Data Source: \"%s\"\nREAD-ONLY MODE\n", IndexingDataSource->IndexingDataSourceName);
			exit(1);
		}
		
		if (sw->indexlist == NULL)
			progerr("Specify index files and an output file.");
		if (hasconf)
		{
			while (conflist != NULL) 
			{
				getdefaults(sw,conflist->line, &hasdir, &hasindex, hasverbose);
				conflist = conflist->next;
			}
		}
		
		tmplistindex = sw->indexlist;
		for (i = 0; tmplistindex != NULL; i++) {
			index4=SafeStrCopy(index4, tmplistindex->line, &lenindex4);
			tmplistindex = tmplistindex->next;
		}
		j = i - 2;
		if (i < 3)
			progerr("Specify index files and an output file.");
		
		if(sw->Index->tmpdir && sw->Index->tmpdir[0] && isdirectory(sw->Index->tmpdir))
		{
			tmpindex1=SafeStrCopy(tmpindex1, tempnam(sw->Index->tmpdir,"swme"),&lentmpindex1);
			tmpindex2=SafeStrCopy(tmpindex2, tempnam(sw->Index->tmpdir,"swme"),&lentmpindex2);
		} else {
			tmpindex1=SafeStrCopy(tmpindex1, tempnam(NULL,"swme"),&lentmpindex1);
			tmpindex2=SafeStrCopy(tmpindex2, tempnam(NULL,"swme"),&lentmpindex2);
		}

		i = 1;
		index1=SafeStrCopy(index1, sw->indexlist->line,&lenindex1);
		sw->indexlist = sw->indexlist->next;
		while (i <= j) {
			index2=SafeStrCopy(index2, sw->indexlist->line,&lenindex2);
			if (i % 2) {
				if (i != 1)
					{ index1=SafeStrCopy(index1, tmpindex2, &lenindex1); }
				index3=SafeStrCopy(index3,tmpindex1,&lenindex3);
			}
			else {
				index1=SafeStrCopy(index1,tmpindex1,&lenindex1);
				index3=SafeStrCopy(index3,tmpindex2,&lenindex3);
			}
			if (i == j)
				{index3=SafeStrCopy(index3,index4,&lenindex3);}
			readmerge(index1, index2, index3, sw->verbose);
			sw->indexlist = sw->indexlist->next;
			i++;
		}
#ifdef INDEXPERMS
		chmod(index3, INDEXPERMS);
#endif
		if (isfile(tmpindex1)) remove(tmpindex1);
		if (isfile(tmpindex2)) remove(tmpindex2);
	
	}


	/* -k => Output all words in index that start with specified letter */
	else if(keychar)
	{
		if (!hasindex) {
			sw->indexlist = addindexfile(sw->indexlist, INDEXFILE);
		}
		OutputKeyChar (sw, (int)(unsigned char)keychar);
	}


	/* anything else => Search for words in the index */
	else 
	{
		if (!hasindex)
			sw->indexlist = addindexfile(sw->indexlist, INDEXFILE);
		
		if(tmpprops)
		{
			for(tmplist=tmpprops;tmplist;tmplist=tmplist->next)
			{
				addSearchResultDisplayProperty(sw,tmplist->line);
			}
			freeswline(tmpprops);
		}

		if(tmpsortprops)
		{

			sortmode=-1; /* Ascendind by default */
			for(tmplist=tmpsortprops;tmplist;tmplist=tmplist->next)
			{
				field=tmplist->line;
				if (tmplist->next) {
					if(!strcasecmp(tmplist->next->line,"asc")) {
						sortmode=-1;  /* asc sort */
						tmplist=tmplist->next;		
					} else if(!strcasecmp(tmplist->next->line,"desc")){
						sortmode=1; /* desc sort */
						tmplist=tmplist->next;		
					}
				}
				addSearchResultSortProperty(sw,field,sortmode);
			}
			freeswline(tmpsortprops);
		}

		for (i = 0; structstr[i] != '\0'; i++)
		{
			switch (structstr[i]) 
			{
			case 'H':
				structure |= IN_HEAD;
				break;
			case 'B':
				structure |= IN_BODY;
				break;
			case 't':
				structure |= IN_TITLE;
				break;
			case 'h':
				structure |= IN_HEADER;
				break;
			case 'e':
				structure |= IN_EMPHASIZED;
				break;
			case 'c':
				structure |= IN_COMMENTS;
				break;
			default:
				structure |= IN_FILE;
				break;
			}
		}
		
		if (sw->Search->maxhits <= 0)
			sw->Search->maxhits = -1;
		if (hasMetaName)
			while (conflist != NULL) {
				getdefaults(sw,conflist->line, &hasdir, &hasindex, hasverbose);
				conflist = conflist->next;
			}
		rc=SwishAttach(sw,1);
		switch(rc) {
			case INDEX_FILE_NOT_FOUND:
				resultHeaderOut(sw,1, "# Name: unknown index\n");
				progerrno("could not open index file: ");
				break;
			case UNKNOWN_INDEX_FILE_FORMAT:
				progerr("the index file format is unknown");
				break;
		}

		resultHeaderOut(sw,1, "%s\n", INDEXHEADER);
			/* print out "original" search words */
		resultHeaderOut(sw,1, "# Search words: %s\n",wordlist);

        search_starttime = TimeHiRes();

		rc=search(sw,wordlist, structure);

		switch(rc) {
			case INDEX_FILE_NOT_FOUND:
				resultHeaderOut(sw,1, "# Name: unknown index\n");
				progerr("could not open index file");
				break;
			case UNKNOWN_INDEX_FILE_FORMAT:
				progerr("the index file format is unknown");
				break;
			case NO_WORDS_IN_SEARCH:
				progerr("no search words specified");
				break;
			case WORDS_TOO_COMMON:
				progerr("all search words too common to be useful");
				break;
			case INDEX_FILE_IS_EMPTY:
				progerr("the index file(s) is empty");
				break;
			case UNKNOWN_PROPERTY_NAME_IN_SEARCH_DISPLAY:
			case UNKNOWN_PROPERTY_NAME_IN_SEARCH_SORT:
				/* error msg already printed */
				break;
		}
		resultHeaderOut(sw,2, "#\n");
		if(rc>0) {
			resultHeaderOut(sw,1, "# Number of hits: %d\n",rc);

			endtime = TimeHiRes();
			resultHeaderOut(sw,1, "# Search time: %0.3f seconds\n", endtime - search_starttime );
			resultHeaderOut(sw,1, "# Run time: %0.3f seconds\n", endtime - run_starttime );
			printSortedResults(sw);
			resultHeaderOut(sw,1, ".\n");
		} else if(!rc) {
			resultHeaderOut(sw,1, "err: no results\n.\n");
		}
			/* Free conflist */
		if(lenword) efree(word);
		if(lenmaxhitstr) efree(maxhitstr);
		if(lenbeginhitstr) efree(beginhitstr);
	}


/* common exit for all functions */

	if (sw->dirlist) freeswline(sw->dirlist);

	SwishClose(sw);

	if (conflist) freeswline(conflist);

	if(index1) efree(index1);
	if(index2) efree(index2);
	if(index3) efree(index3);
	if(index4) efree(index4);
	if(tmpindex1) efree(tmpindex1);
	if(tmpindex2) efree(tmpindex2);
	if(wordlist) efree(wordlist);
	if(structstr) efree(structstr);

	checkmem();
	
	exit(0);

	return 0;
}

/* Prints the running time (the time it took for indexing).
*/

void printrunning(starttime, stoptime)
     long starttime;
     long stoptime;
{
	int minutes, seconds;
	
	minutes = (stoptime - starttime) / SECSPERMIN;
	seconds = (stoptime - starttime) % SECSPERMIN;
	printf("Running time: ");
	if (minutes)
		printf("%d minute%s", minutes, (minutes == 1) ? "" : "s");
	if (minutes && seconds)
		printf(", ");
	if (seconds)
		printf("%d second%s", seconds, (seconds == 1) ? "" : "s");
	if (!minutes && !seconds)
		printf("Less than a second");
	printf(".\n");
}

/* Prints the SWISH usage.
*/

void usage()
{
	const char* defaultIndexingSystem = "";

	printf(" usage:\n");
	printf("    swish [-e] [-i dir file ... ] [-S system] [-c file] [-f file] [-l] [-v (num)]\n");
	printf("    swish -w word1 word2 ... [-f file1 file2 ...] \\\n");
	printf("          [-P phrase_delimiter] [-p prop1 ...] [-s sortprop1 [asc|desc] ...] \\\n");
	printf("          [-m num] [-t str] [-d delim] [-H (num)] [-x output_format]\n");
	printf("    swish -k (char|*) [-f file1 file2 ...]\n");
	printf("    swish -M index1 index2 ... outputfile\n");
	printf("    swish -N /path/to/compare/file\n");
	printf("    swish -D [-v 4] -f indexfile\n");
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
	printf("              \"HBthec\" - in head, body, title, header,\n");
	printf("              emphasized, comments\n");
	printf("         -f : index file to create or search from [%s]\n", INDEXFILE);
	printf("         -c : configuration file to use for indexing\n");
	printf("         -v : verbosity level (0 to 3) [-v %d]\n", VERBOSE);
	printf("         -l : follow symbolic links when indexing\n");
	printf("         -b : begin results at this number\n");
	printf("         -m : the maximum number of results to return [defaults to all results]\n");
	printf("         -M : merges index files\n");
	printf("         -N : index only files with a modification date newer than path supplied\n");
	printf("         -D : decodes an index file\n");
	printf("         -p : include these document properties in the output \"prop1 prop2 ...\"\n");
	printf("         -s : sort by these document properties in the output \"prop1 prop2 ...\"\n");
	printf("         -d : next param is delimiter.\n");
	printf("         -P : next param is Phrase delimiter.\n");
	printf("         -V : prints the current version\n");
	printf("         -e : \"Economic Mode\": The index proccess uses less RAM.\n");
	printf("         -x : \"Extended Output Format\": Specify the output format.\n");
	printf("         -H : \"Result Header Output\": verbosity (0 to 9)  [1].\n");
	printf("         -k : Print words starting with a given char.\n\n");
	printf("version: %s\n", SWISH_VERSION);
	printf("   docs: http://sunsite.berkeley.edu/SWISH-E/\n");
	exit(1);
}

void printversion()
{
	printf("SWISH-E %s\n", SWISH_VERSION);
	exit(0);
}



