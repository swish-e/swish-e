/*
** Copyright (C) 1995, 1996, 1997, 1998 Hewlett-Packard Company
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
** 2001-02-12 rasc   errormsg "print" changed...
**
*/

#define MAIN_FILE

#include "swish.h"

#include "error.h"
#include "list.h"
#include "search.h"
#include "index.h"
#include "string.h"
#include "file.h"
#include "http.h"
#include "merge.h"
#include "docprop.h"
#include "mem.h"
#include "metanames.h"
#include "parse_conffile.h"
#include "result_output.h"
#include <time.h>



/*
** This array has pointers to all the indexing data source
** structures
*/
extern struct _indexing_data_source_def *data_sources[];

int main(argc, argv)
int argc;
char **argv;
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
long offsetstart, hashstart, starttime, stoptime;
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
int keychar2;
char *keywords=NULL;
IndexFILE *tmpindexlist=NULL;
struct swline *tmpprops=NULL,*tmpsortprops=NULL;
clock_t search_time, run_time;


    run_time = clock();

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
				sw->dirlist = (struct swline *)
					addswline(sw->dirlist, (++argv)[0]);
				argc--;
			}
		}
		else if (c == 'w') 
		{
			while ((argv + 1)[0] != '\0' && *(argv + 1)[0] != '-') 
			{
				word = SafeStrCopy(word, (++argv)[0],&lenword);
				argc--;
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
		else if (c == 'f') 
		{
			while ((argv + 1)[0] != '\0' && *(argv + 1)[0] != '-') 
			{
				sw->indexlist = (IndexFILE *)
					addindexfile(sw->indexlist, (++argv)[0]);
				argc--;
			}
		}
		else if (c == 'c') 
		{
			index = 1;
			hasconf = 1;
			while ((argv + 1)[0] != '\0' && *(argv + 1)[0] != '-') {
				conflist = (struct swline *)
					addswline(conflist, (++argv)[0]);
				argc--;
			}
		}
		else if (c == 'C') {
			while ((argv + 1)[0] != '\0' && *(argv + 1)[0] != '-')
			{
				conflist = (struct swline *)
					addswline(conflist, (++argv)[0]);
				argc--;
			}
			if (conflist == NULL)
				progerr("Specifiy the configuration file.");
			else
				hasMetaName = 1;
		      }
		else if (c == 'l') {
			sw->followsymlinks = 1;
			argc--;
		}
		else if (c == 'b') {
			if ((argv + 1)[0] == '\0')
				sw->beginhits = 0;
			else {
				beginhitstr = SafeStrCopy(beginhitstr, (++argv)[0],&lenbeginhitstr);
				if (isdigit((int)beginhitstr[0]))
					sw->beginhits = atoi(beginhitstr);
				else
					sw->beginhits = 0;
				argc--;
			}
		}
		else if (c == 'm') {
			if ((argv + 1)[0] == '\0')
				sw->maxhits = -1;
			else {
				maxhitstr = SafeStrCopy(maxhitstr, (++argv)[0],&lenmaxhitstr);
				if (lstrstr(maxhitstr, "all"))
					sw->maxhits = -1;
				else if (isdigit((int)maxhitstr[0])) /* cast to int, 2/22/00 */
					sw->maxhits = atoi(maxhitstr);
				else
					sw->maxhits = -1;
				argc--;
			}
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
				sw->indexlist = (IndexFILE *)
					addindexfile(sw->indexlist, (++argv)[0]);
				argc--;
			}
		}
		else if (c == 'D') {
			decode = 1;
			while ((argv + 1)[0] != '\0' && *(argv + 1)[0] != '-') {
				sw->indexlist = (IndexFILE *)
					addindexfile(sw->indexlist, (++argv)[0]);
				argc--;
			}
		}
		else if (c == 'P')
		{
			/* Custom Phrase Delimiter - Jose Ruiz 01/00 */
			if (((argv + 1)[0] != '\0') && (*(argv + 1)[0] != '-'))
			{
				sw->PhraseDelimiter = (int) (++argv)[0][0];
				argc--;
			}
		}
		else if (c == 'd')
		{
			/* added 11/24/98 MG */
			if (((argv + 1)[0] != '\0') && (*(argv + 1)[0] != '-'))
			{
				sw->useCustomOutputDelimiter = 1;
				sw->customOutputDelimiter=SafeStrCopy(sw->customOutputDelimiter, (++argv)[0],&sw->lencustomOutputDelimiter);
				if (strcmp(sw->customOutputDelimiter, "dq") == 0)
					{ sw->customOutputDelimiter=SafeStrCopy(sw->customOutputDelimiter, "\"",&sw->lencustomOutputDelimiter); } /* double quote is cool */
				if (sw->customOutputDelimiter[0]=='\\')
				{
					switch(sw->customOutputDelimiter[1])
					{
						case 'f': sw->customOutputDelimiter[0]='\f'; break;
						case 'n': sw->customOutputDelimiter[0]='\n'; break;
						case 'r': sw->customOutputDelimiter[0]='\r'; break;
						case 't': sw->customOutputDelimiter[0]='\t'; break;
					}
					sw->customOutputDelimiter[1]='\0';
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
			   sw->opt.extendedformat = (s) ? s : *argv;
			   initPrintExtResult (sw, sw->opt.extendedformat);
			   argc--;
			}
			/* $$$ progerror todo... */
                }
                else if (c == 'X') {
                                        /* rasc 2001-02 */
                        sw->opt.extendedheader = 1;
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
	
	if (decode) {
		
		if (!hasindex)
			progerr("Specify the index file to decode.");
		
		while (sw->indexlist != NULL) {
			
			if ((sw->indexlist->fp = openIndexFILEForRead(sw->indexlist->line)) == NULL) {
				progerr("Couldn't open the index file \"%s\".", sw->indexlist->line);
			}
			if (!isokindexheader(sw->indexlist->fp)) {
				progerr("File \"%s\" has an unknown format.", sw->indexlist->line);
			}
			
			decompress(sw,sw->indexlist);
			putchar('\n');
			
			sw->indexlist = sw->indexlist->next;
		}
		exit(0);
		
	}
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
			sw->indexlist = (IndexFILE *) 
				addindexfile(sw->indexlist, INDEXFILE);

		if (!hasdir)
			progerr("Specify directories or files to index.");
		
		if (sw->verbose < 0)
			sw->verbose = 0;
		if (sw->verbose > 4)
			sw->verbose = 4;
		if (sw->verbose)
			starttime = getthetime();

			/* Update Swap mode */
		sw->swap_flag=swap_mode;

		while (sw->dirlist != NULL) {
			if (sw->verbose) {
				printf("Indexing %s..\n",sw->dirlist->line);
				fflush(stdout);
			}
			indexpath(sw,sw->dirlist->line);
			sw->dirlist = sw->dirlist->next;
		}

			/* Create an empty File */
		CreateEmptyFile(sw,sw->indexlist->line);
			/* Open it for Read/Write */
		sw->indexlist->fp = openIndexFILEForReadAndWrite(sw->indexlist->line);

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
				sw->indexlist->header.totalwords -= stopwords;
				if(sw->indexlist->header.totalwords<0) sw->indexlist->header.totalwords=0;
				printf("%d word%s removed.\n",
					stopwords, (stopwords == 1) ? "" : "s");
				printf("%d words removed not in common words array:\n", sw->indexlist->stopPos);
				for (pos = 0; pos < sw->indexlist->stopPos; pos++) 
					printf("%s, ", sw->indexlist->stopList[pos]);
				printf("\n");
			}
			else
				printf("no words removed.\n");
			printf("Writing main index...\n");
		}

		if (sw->verbose)
			printf("Writing header ...\n");
		fflush(stdout);
		printheader(&sw->indexlist->header,sw->indexlist->fp, sw->indexlist->line, sw->indexlist->header.totalwords, totalfiles, 0);
		fflush(stdout);
		
		offsetstart = ftell(sw->indexlist->fp);
		for (i = 0; i < MAXCHARS; i++)
			printlong(sw->indexlist->fp,(long)0);
		
		hashstart = ftell(sw->indexlist->fp);
		for (i = 0; i < SEARCHHASHSIZE; i++)
			printlong(sw->indexlist->fp,(long)0);

		if (sw->verbose)
			printf("Writing index entries ...\n");
		printindex(sw, sw->indexlist);
		if (sw->verbose)
			printf("Writing stopwords ...\n");
		printstopwords(sw->indexlist);

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
		printfilelist(sw,sw->indexlist);

		if(sw->verbose)
			printf("Writing file offsets ...\n");
		printfileoffsets(sw->indexlist);

		if(sw->verbose)
			printf("Writing MetaNames ...\n");
		printMetaNames(sw->indexlist);

		if(sw->verbose)
			printf("Writing Location lookup tables ...\n");
		printlocationlookuptables(sw->indexlist);
		printpathlookuptable(sw->indexlist);

		if(sw->verbose)
			printf("Writing offsets (2)...\n");
		fseek(sw->indexlist->fp, offsetstart, 0);
		for (i = 0; i < MAXCHARS; i++)
			printlong(sw->indexlist->fp,sw->indexlist->offsets[i]);

		fseek(sw->indexlist->fp, hashstart, 0);
		for (i = 0; i < SEARCHHASHSIZE; i++)
			printlong(sw->indexlist->fp,sw->indexlist->hashoffsets[i]);
		fclose(sw->indexlist->fp);
		sw->indexlist->fp=NULL;
	
		if (sw->verbose) 
		{
			if (totalfiles)
				printf("%d file%s indexed.\n", totalfiles,
				(totalfiles == 1) ? "" : "s");
			else
				printf("no files indexed.\n");
			
			stoptime = getthetime();
			printrunning(starttime, stoptime);
			printf("Indexing done!\n");
		}
		if (isfile(sw->swap_file_name))remove(sw->swap_file_name);
		if (isfile(sw->swap_location_name))remove(sw->swap_location_name);
#ifdef INDEXPERMS
		chmod(sw->indexlist->line, INDEXPERMS);
#endif

#ifdef DEBUGMEMORY
        checkmem();
#endif
		SwishClose(sw);
		exit(0);

	}
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
		
		if(sw->tmpdir && sw->tmpdir[0] && isdirectory(sw->tmpdir))
		{
			tmpindex1=SafeStrCopy(tmpindex1, tempnam(sw->tmpdir,"swme"),&lentmpindex1);
			tmpindex2=SafeStrCopy(tmpindex2, tempnam(sw->tmpdir,"swme"),&lentmpindex2);
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
#ifdef DEBUGMEMORY
        checkmem();
#endif
	
	}
	else if(keychar)
	{
		if (!hasindex)
			sw->indexlist = (IndexFILE *)
			addindexfile(sw->indexlist, INDEXFILE);
		rc=SwishAttach(sw,1);
		switch(rc) {
			case INDEX_FILE_NOT_FOUND:
				printf("# Name: unknown index\n");
				printf("err: could not open index file %s errno: %d\n.\n",sw->indexlist->line,errno);
				exit(-1);
				break;
			case UNKNOWN_INDEX_FILE_FORMAT:
				progerr("the index file format is unknown");
				break;
		}

		printf("%s\n", INDEXHEADER);
			/* print out "original" search words */
		for(tmpindexlist=sw->indexlist;tmpindexlist;tmpindexlist=tmpindexlist->next)
		{
			printf("%s:",tmpindexlist->line);
			if(keychar=='*')
			{
				for(keychar2=1;keychar2<256;keychar2++)
				{
					keywords=getfilewords(sw,(unsigned char )keychar2,tmpindexlist);
					for(;keywords && keywords[0];keywords+=strlen(keywords)+1) printf(" %s",keywords);
				}
			} else {
				keywords=getfilewords(sw,keychar,tmpindexlist);
				for(;keywords && keywords[0];keywords+=strlen(keywords)+1) printf(" %s",keywords);
			}
			printf("\n");
		}
		SwishClose(sw);
	}
	else 
	{
		if (!hasindex)
			sw->indexlist = (IndexFILE *)
			addindexfile(sw->indexlist, INDEXFILE);
		
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
		
		if (sw->maxhits <= 0)
			sw->maxhits = -1;
		if (hasMetaName)
			while (conflist != NULL) {
				getdefaults(sw,conflist->line, &hasdir, &hasindex, hasverbose);
				conflist = conflist->next;
			}
		rc=SwishAttach(sw,1);
		switch(rc) {
			case INDEX_FILE_NOT_FOUND:
				printf("# Name: unknown index\n");
				printf("err: could not open index file %s errno: %d\n.\n",sw->indexlist->line,errno);
				exit(-1);
				break;
			case UNKNOWN_INDEX_FILE_FORMAT:
				progerr("the index file format is unknown");
				break;
		}

		printf("%s\n", INDEXHEADER);
			/* print out "original" search words */
		printf("# Search words: %s\n#\n",wordlist);

        search_time = clock();

		rc=search(sw,wordlist, structure);

		switch(rc) {
			case INDEX_FILE_NOT_FOUND:
				printf("# Name: unknown index\n");
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
		if(rc>0) {
                	printf("# Number of hits: %d\n",rc);

                	printf("# Search time: %0.3f seconds\n",
                	     ((float)clock()-search_time)/CLOCKS_PER_SEC );
                	printf("# Run time: %0.3f seconds\n",
                	     ((float)clock()-run_time)/CLOCKS_PER_SEC );

                	printSortedResults(sw);
			printf(".\n");
		} else if(!rc) {
			printf("err: no results\n.\n");
		}
			/* Free conflist */
		freeswline(conflist);
		if(lenword) efree(word);
		if(lenwordlist) efree(wordlist);
		if(lenmaxhitstr) efree(maxhitstr);
		if(lenstructstr) efree(structstr);
		if(lenbeginhitstr) efree(beginhitstr);
		SwishClose(sw);
	}

#ifdef DEBUGMEMORY
	checkmem();
#endif	
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
	printf("          [-m num] [-t str] [-d delim] [-X] [-x output_format]\n");
	printf("    swish -k (char|*) [-f file1 file2 ...]\n");
	printf("    swish -M index1 index2 ... outputfile\n");
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
	printf("              The default value is: \"%s\"\n", defaultIndexingSystem);

	printf("         -i : create an index from the specified files\n");
	printf("         -w : search for words \"word1 word2 ...\"\n");
	printf("         -t : tags to search in - specify as a string\n");
	printf("              \"HBthec\" - in head, body, title, header,\n");
	printf("              emphasized, comments\n");
	printf("         -f : index file to create or search from [%s]\n", INDEXFILE);
	printf("         -c : configuration file to use for indexing\n");
	printf("         -v : verbosity level (0 to 3) [%d]\n", VERBOSE);
	printf("         -l : follow symbolic links when indexing\n");
	printf("         -b : begin results at this number\n");
	printf("         -m : the maximum number of results to return [defaults to all results]\n");
	printf("         -M : merges index files\n");
	printf("         -D : decodes an index file\n");
	printf("         -p : include these document properties in the output \"prop1 prop2 ...\"\n");
	printf("         -s : sort by these document properties in the output \"prop1 prop2 ...\"\n");
	printf("         -d : next param is delimiter. use \"-d dq\" to use a double quote\n");
	printf("         -P : next param is Phrase delimiter.\n");
	printf("         -V : prints the current version\n");
	printf("         -e : \"Economic Mode\": The index proccess uses less RAM.\n");
	printf("         -x : \"Extended Output Format\": Specify the output format.\n");
	printf("         -X : \"Extended Search Header\": The search proccess gives more info.\n");
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



