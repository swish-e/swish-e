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
**-------------------------------------------------------------
** Changed getdefaults to allow metaNames in the user
** configuration file
** G.Hill 4/16/97 ghill@library.berkeley.edu
**
** change sprintf to snprintf to avoid corruption, and use MAXSTRLEN from swish.h
** added safestrcpy() macro to avoid corruption from strcpy overflow
** SRE 11/17/99
**
** added buffer size arg to grabStringValue - core dumping from overrun
** fixed logical OR and other problems pointed out by "gcc -Wall"
** SRE 2/22/00
** 
** counter modulo 128 had parens typo
** SRE 2/23/00
**
** read stopwords from file
** Rainer Scherg (rasc)  2000-06-15
** 
** 2000-11-15 rasc
** file_properties retrieves last mod date, filesize, and evals some swish
** config flags for this file!
**
** 2001-02-12 rasc   errormsg "print" changed...
**
**
*/

#include "swish.h"
#include "mem.h"
#include "string.h"
#include "file.h"
#include "error.h"
#include "list.h"
#include "hash.h"
#include "check.h"
#include "index.h"
/* #### Added metanames.h */
#include "metanames.h"
/* #### */

/* Is a file a directory?
*/

int isdirectory(char *path)
{
	struct stat stbuf;
	
	if (stat(path, &stbuf))
		return 0;
	return ((stbuf.st_mode & S_IFMT) == S_IFDIR) ? 1 : 0;
}

/* Is a file a regular file?
*/

int isfile(char *path)
{
	struct stat stbuf;
	
	if (stat(path, &stbuf))
		return 0;
	return ((stbuf.st_mode & S_IFMT) == S_IFREG) ? 1 : 0;
}

/* Is a file a link?
*/

int islink(char *path)
{
#ifndef NO_SYMBOLIC_FILE_LINKS
	struct stat stbuf;
	
	if (lstat(path, &stbuf))
		return 0;
	return ((stbuf.st_mode & S_IFLNK) == S_IFLNK) ? 1 : 0;
#else
	return 0;
#endif
}

/* Get the size, in bytes, of a file.
** Return -1 if there's a problem.
*/

int getsize(char *path)
{
	struct stat stbuf;
	
	if (stat(path, &stbuf))
		return -1;
	return stbuf.st_size;
}


/* 02/2001 Rewritten Jmruiz */
void grabCmdOptions(StringList *sl, int start, struct swline **listOfWords)
{
int i;
	for(i=start;i<sl->n;i++)
		*listOfWords = (struct swline *) addswline(*listOfWords, sl->word[i]);
}


/* Reads the configuration file and puts all the right options
** in the right variables and structures.
*/

void getdefaults(SWISH *sw, char *conffile, int *hasdir, int *hasindex, int hasverbose)
{
int i, gotdir, gotindex;
char *c, line[MAXSTRLEN];
FILE *fp;
int linenumber = 0;
int baddirective = 0;
StringList *sl;
int DocType=0;
struct IndexContents *ic;
struct StoreDescription *sd;
IndexFILE *indexf=NULL;
unsigned char *StringValue=NULL;
	
	gotdir = gotindex = 0;
	
	if ((fp = fopen(conffile, "r")) == NULL  ||
		!isfile(conffile) ) {
		progerr("Couldn't open the configuration file \"%s\".", conffile);
	}

	/* Init default index file */
	indexf=sw->indexlist=(IndexFILE *) addindexfile(sw->indexlist, INDEXFILE);
	while (fgets(line, MAXSTRLEN, fp) != NULL) 
	{
		linenumber++;
			/* Parse line */
		if(!(sl=parse_line(line))) continue;
		
		if(!sl->n)
		{
			freeStringList(sl);
			continue;
		}
		
		if (sl->word[0][0] == '#') continue;

		if (strcasecmp(sl->word[0],"IndexDir")==0) {
			if(sl->n>1) {
				if(!*hasdir)
				{
					gotdir=1;
					grabCmdOptions(sl,1, &sw->dirlist);
				}
			} else progerr("IndexDir requires one value");
		}
		else if (strcasecmp(sl->word[0],"NoContents")==0) {
			if(sl->n>1) {
				grabCmdOptions(sl,1,&sw->nocontentslist);
			} else progerr("NoContents requires one value");
		}
		else if (strcasecmp(sl->word[0], "IndexFile")==0) {
			if(sl->n==2) {
				if(indexf->line) efree(indexf->line);
				indexf->line=estrdup(sl->word[1]);
			} else progerr("Indexfile requires one value");
		}
		else if (strcasecmp(sl->word[0],"IndexReport")==0){
			if(sl->n==2) {
				if(!hasverbose)
				{
					sw->verbose=atoi(sl->word[1]);
				}
			} else progerr("IndexReport requires one value");
		}
		else if (strcasecmp(sl->word[0], "MinWordLimit")==0){
			if(sl->n==2) {
				indexf->header.minwordlimit=atoi(sl->word[1]);
			} else progerr("MinWordLimit requires one value");
		}
		else if (strcasecmp(sl->word[0], "MaxWordLimit")==0){
			if(sl->n==2) {
				indexf->header.maxwordlimit=atoi(sl->word[1]);
			} else progerr("MaxWordLimit requires one value");
		}
		else if (strcasecmp(sl->word[0], "IndexComments")==0){
			if(sl->n==2) {
				sw->indexComments=atoi(sl->word[1]);
			} else progerr("IndexComments requires one value");
		}
		else if (strcasecmp(sl->word[0], "WordCharacters")==0)	{
			if(sl->n==2) {
				indexf->header.wordchars = SafeStrCopy(indexf->header.wordchars,sl->word[1],&indexf->header.lenwordchars);
				sortstring(indexf->header.wordchars);
				makelookuptable(indexf->header.wordchars,indexf->header.wordcharslookuptable);
			} else progerr("WordCharacters requires one value");
		}
		else if (strcasecmp(sl->word[0], "BeginCharacters")==0)	{
			if(sl->n==2) {
				indexf->header.beginchars = SafeStrCopy(indexf->header.beginchars,sl->word[1],&indexf->header.lenbeginchars);
				sortstring(indexf->header.beginchars);
				makelookuptable(indexf->header.beginchars,indexf->header.begincharslookuptable);
			} else progerr("BeginCharacters requires one value");
		}
		else if (strcasecmp(sl->word[0], "EndCharacters")==0)	{
			if(sl->n==2) {
				indexf->header.endchars = SafeStrCopy(indexf->header.endchars,sl->word[1],&indexf->header.lenendchars);
				sortstring(indexf->header.endchars);
				makelookuptable(indexf->header.endchars,indexf->header.endcharslookuptable);
			} else progerr("EndCharacters requires one value");
		}
		else if (strcasecmp(sl->word[0], "IgnoreLastChar")==0)	{
			if(sl->n==2) {
				indexf->header.ignorelastchar = SafeStrCopy(indexf->header.ignorelastchar,sl->word[1],&indexf->header.lenignorelastchar);
				sortstring(indexf->header.ignorelastchar);
				makelookuptable(indexf->header.ignorelastchar,indexf->header.ignorelastcharlookuptable);
			} else progerr("IgnoreLastChar requires one value");
		}
		else if (strcasecmp(sl->word[0], "IgnoreFirstChar")==0)	{
			if(sl->n==2) {
				indexf->header.ignorefirstchar = SafeStrCopy(indexf->header.ignorefirstchar,sl->word[1],&indexf->header.lenignorefirstchar);
				sortstring(indexf->header.ignorefirstchar);
				makelookuptable(indexf->header.ignorefirstchar,indexf->header.ignorefirstcharlookuptable);
			} else progerr("IgnoreFirstChar requires one value");
		}
		else if (strcasecmp(sl->word[0], "ReplaceRules")==0)	{
			if(sl->n>1) {
				grabCmdOptions(sl,1, &sw->replacelist);
				checkReplaceList(sw);
			} else progerr("ReplaceRules requires at least one value");
		}
		else if (strcasecmp(sl->word[0], "FollowSymLinks")==0)	{
			if(sl->n==2) {
				sw->followsymlinks = (lstrstr(sl->word[1], "yes")) ? 1 : 0;
			} else progerr("FollowSymLinks requires one value");
		}
		else if (strcasecmp(sl->word[0], "IndexName")==0)	{
			if(sl->n>1) {
				StringValue=StringListToString(sl,1);
				indexf->header.indexn = SafeStrCopy(indexf->header.indexn,StringValue,&indexf->header.lenindexn);
				efree(StringValue);
			} else progerr("IndexName requires a value");
		}
		else if (strcasecmp(sl->word[0], "IndexDescription")==0)	{
			if(sl->n>1) {
				StringValue=StringListToString(sl,1);
				indexf->header.indexd = SafeStrCopy(indexf->header.indexd,StringValue,&indexf->header.lenindexd);
				efree(StringValue);
			} else progerr("IndexDescription requires a value");
		}
		else if (strcasecmp(sl->word[0], "IndexPointer")==0)	{
			if(sl->n>1) {
				StringValue=StringListToString(sl,1);
				indexf->header.indexp = SafeStrCopy(indexf->header.indexp,StringValue,&indexf->header.lenindexp);
				efree(StringValue);
			} else progerr("IndexPointer requires a value");
		}
		else if (strcasecmp(sl->word[0], "IndexAdmin")==0)	{
			if(sl->n>1) {
				StringValue=StringListToString(sl,1);
				indexf->header.indexa = SafeStrCopy(indexf->header.indexa,StringValue,&indexf->header.lenindexa);
				efree(StringValue);
			} else progerr("IndexAdmin requires one value");
		}
		else if (strcasecmp(sl->word[0], "UseStemming")==0)	{
			if(sl->n==2) {
				indexf->header.applyStemmingRules = (lstrstr(sl->word[1], "yes")) ? 1 : 0;
			} else progerr("UseStemming requires one value");
		}
		else if (strcasecmp(sl->word[0], "IgnoreTotalWordCountWhenRanking")==0)	{
			if(sl->n==2) {
				indexf->header.ignoreTotalWordCountWhenRanking = (lstrstr(sl->word[1], "yes")) ? 1 : 0;
			} else progerr("IgnoreTotalWordCountWhenRanking requires one value");
		}
		else if (strcasecmp(sl->word[0], "UseSoundex")==0)	{
			if(sl->n==2) {
				indexf->header.applySoundexRules = (lstrstr(sl->word[1], "yes")) ? 1 : 0;
			} else progerr("UseSoundex requires one value");
		}
                else if (strcasecmp(sl->word[0], "FilterDir")==0)    {      /* 1999-05-05 rasc */
			if(sl->n==2) {
				sw->filterdir = SafeStrCopy(sw->filterdir,sl->word[1],&sw->lenfilterdir);
				if(!isdirectory(sw->filterdir)) {
					progerr("FilterDir. %s is not a directory",sw->filterdir);
				}
			} else progerr("FilterDir requires one value");
		}
                else if (strcasecmp(sl->word[0], "FileFilter")==0) {
        /* 1999-05-05 rasc */
                                     /* FileFilter fileextension  filerprog */
			if(sl->n==3) {
				sw->filterlist = (struct filter *) addfilter(sw->filterlist,sl->word[1],sl->word[2],sw->filterdir);
			} else progerr("FileFilter requires two values");
                }
		else if (strcasecmp(sl->word[0], "MetaNames")== 0) 
		{
			if(sl->n>1) {
				for(i=1;i<sl->n;i++)
					addMetaEntry(indexf,sl->word[i], META_INDEX, &sw->applyautomaticmetanames);
			} else progerr("MetaNames requires at least one value");
		}
		else if (strcasecmp(sl->word[0], "TranslateCharacters")== 0)  
		{
			if(sl->n==3) {
				indexf->header.translatechars1=SafeStrCopy(indexf->header.translatechars1,sl->word[1],&indexf->header.lentranslatechars1);
				indexf->header.translatechars2=SafeStrCopy(indexf->header.translatechars2,sl->word[2],&indexf->header.lentranslatechars2);
				if(strlen(indexf->header.translatechars1)!=strlen(indexf->header.translatechars2)) progerr("TranslateCharacters option requires two values of the same length");
				
			} else progerr("TranslateCharacters requires two values");
		}
		else if (strcasecmp(sl->word[0], "PropertyNames")== 0)
		{
			if(sl->n>1) {
				for(i=1;i<sl->n;i++)
					addMetaEntry(indexf,sl->word[i], META_PROP, &sw->applyautomaticmetanames);
			} else progerr("PropertyNames requires at least one value");
		}
		else if (strcasecmp(sl->word[0], "IgnoreWords")== 0) {
			if(sl->n>1) {
				if (lstrstr(sl->word[1], "SwishDefault")) {
						readdefaultstopwords(indexf);
				} else if (lstrstr(sl->word[1], "File:")) {  /* 2000-06-15 rasc */
				        if (sl->n == 3) 
					    readstopwordsfile(sw,indexf,sl->word[2]);	
					else progerr ("IgnoreWords File: requires path");
				} else for(i=1;i<sl->n;i++) {
					addstophash(indexf,sl->word[i]);
				}
			} else progerr("IgnoreWords requires at least one value");
		}
		else if (strcasecmp(sl->word[0], "UseWords")== 0) {  /* 11/00 Jmruiz */
			indexf->is_use_words_flag=1;
			if(sl->n>1) {
				if (lstrstr(sl->word[1], "File:")) {  /* 2000-06-15 rasc */
				        if (sl->n == 3) 
					    readusewordsfile(sw,indexf,sl->word[2]);	
					else progerr ("UseWords File: requires path");
				} else for(i=1;i<sl->n;i++) {
					addusehash(indexf,sl->word[i]);
				}
			} else progerr("UseWords requires at least one value");
		}
		else if (strcasecmp(sl->word[0], "IgnoreLimit")==0) {
			if(sl->n==3) {
				sw->plimit = atol(sl->word[1]);
				sw->flimit = atol(sl->word[2]);
			} else progerr("IgnoreLimit requires two values");
		}
		/* IndexVerbose is supported for backwards compatibility */
		else if (strcasecmp(sl->word[0], "IndexVerbose")==0) { 
			if(sl->n==2) {
				sw->verbose = (lstrstr(sl->word[1], "yes")) ? 3 : 0;
			} else progerr("IndexVerbose requires one value");
		}
		else if (strcasecmp(sl->word[0], "IndexOnly")==0)
		{
			if(sl->n>1) {
				grabCmdOptions(sl,1, &sw->suffixlist);
			} else progerr("IndexOnly requires at least one value");
		}
		else if (strcasecmp(sl->word[0],"IndexContents")==0) {
			if(sl->n>2) {
				if(strcasecmp(sl->word[1],"TXT")==0) {
					DocType=TXT;
				}
				else if(strcasecmp(sl->word[1],"HTML")==0) {
					DocType=HTML;
				}
				else if(strcasecmp(sl->word[1],"XML")==0) {
					DocType=XML;
				}
				else if(strcasecmp(sl->word[1],"MULTITXT")==0) {
					DocType=MULTITXT;
				}
				else if(strcasecmp(sl->word[1],"WML")==0) {
					DocType=WML;
				} else progerr("Unknown document type in IndexContents");
				ic=(struct IndexContents *)emalloc(sizeof(struct IndexContents));
				ic->DocType=DocType;
				ic->patt=NULL;
				for(i=2;i<sl->n;i++)
					ic->patt=addswline(ic->patt,sl->word[i]);
				if(sw->indexcontents)
					ic->next=sw->indexcontents;
				else
					ic->next=NULL;
				sw->indexcontents=ic;
			} else progerr("IndexContents requires at least two values");
		}
		else if (strcasecmp(sl->word[0],"StoreDescription")==0) {
			if(sl->n==3 || sl->n==4) {
				if(strcasecmp(sl->word[1],"TXT")==0) {
					DocType=TXT;
				}
				else if(strcasecmp(sl->word[1],"HTML")==0) {
					DocType=HTML;
				}
				else if(strcasecmp(sl->word[1],"XML")==0) {
					DocType=XML;
				}
				else if(strcasecmp(sl->word[1],"MULTITXT")==0) {
					DocType=MULTITXT;
				}
				else if(strcasecmp(sl->word[1],"WML")==0) {
					DocType=WML;
				} else progerr("Unknown document type in StoreDescription");
				sd=(struct StoreDescription *)emalloc(sizeof(struct StoreDescription));
				sd->DocType=DocType;
				sd->size=0;
				sd->field=NULL;
				i=2;

				if(sl->word[i][0]=='<' && sl->word[i][strlen(sl->word[i])-1]=='>')
				{
					sl->word[i][strlen(sl->word[i])-1]='\0';
					sd->field=estrdup(sl->word[i]+1);
					i++;
				} 
				if(i<sl->n && isnumstring(sl->word[i]))
				{
					sd->size=atoi(sl->word[i]);
				}
				if(sl->n==3 && !sd->field && !sd->size)
					progerr("Second parameter of StoreDescription must be <fieldname> or a number");
				if(sl->n==4 && sd->field && !sd->size)
					progerr("Third parameter of StoreDescription must be empty or a number");
				if(sw->storedescription)
					sd->next=sw->storedescription;
				else
					sd->next=NULL;
				sw->storedescription=sd;
			} else progerr("StoreDescription requires two or three values");
		}
		else if (strcasecmp(sl->word[0],"DefaultContents")==0) {
			if(sl->n>1) {
				if(strcasecmp(sl->word[1],"TXT")==0) {
					DocType=TXT;
				}
				else if(strcasecmp(sl->word[1],"HTML")==0) {
					DocType=HTML;
				}
				else if(strcasecmp(sl->word[1],"XML")==0) {
					DocType=XML;
				}
				else if(strcasecmp(sl->word[1],"MULTITXT")==0) {
					DocType=MULTITXT;
				}
				else if(strcasecmp(sl->word[1],"WML")==0) {
					DocType=WML;
				} else progerr("Unknown document type in DefaultContents");
				sw->DefaultDocType=DocType;
			} else progerr("IndexContents requires at least one value");
		}
		else if (strcasecmp(sl->word[0],"BumpPositionCounterCharacters")==0) {
			if(sl->n>1) {
				indexf->header.bumpposchars = SafeStrCopy(indexf->header.bumpposchars,sl->word[1],&indexf->header.lenbumpposchars);
				sortstring(indexf->header.bumpposchars);
				makelookuptable(indexf->header.bumpposchars,indexf->header.bumpposcharslookuptable);
			} else progerr("BumpPositionCounterCharacters requires at least one value");
		}
		else if (strcasecmp(sl->word[0], "tmpdir")==0) {
			if(sl->n==2) {
				sw->tmpdir = SafeStrCopy(sw->tmpdir,sl->word[1],&sw->lentmpdir);
				if(!isdirectory(sw->tmpdir)) {
					progerr("TempDir. %s is not a directory",sw->tmpdir);
				}
			} else progerr("TmpDir requires one value");
		}
/* #### Added UndefinedMetaTags as defined by Bill Moseley */
		else if (strcasecmp(sl->word[0], "UndefinedMetaTags")==0)	
		{
			if(sl->n==2) {
				if(strcasecmp(sl->word[1],"error")==0)
				{
					sw->OkNoMeta=0;  /* Error if meta name is found
							that's not listed in MetaNames*/
				} 
				else if(strcasecmp(sl->word[1],"ignore")==0)	
				{
					sw->OkNoMeta=1;  /* Do not error */
					sw->ReqMetaName=1;/* but do not index */
				} 
				else if(strcasecmp(sl->word[1],"index")==0)	
				{
					sw->OkNoMeta=1;  /* Do not error */
					sw->ReqMetaName=0;  /* place in main index, no meta name associated */ 
				}
				else if(strcasecmp(sl->word[1],"auto")==0)	
				{
					sw->OkNoMeta=1;  /* Do not error */
					sw->ReqMetaName=0;  /* do not ignore */
					sw->applyautomaticmetanames=1;  /* act as if all meta tags are listed in Metanames */	
				}
				else 
					progerr("Error: Values for UndefinedMetaTags are error, ignore, index or auto");
			} else progerr("UndefinedMetaTags requires one value");
		}
		else if (strcasecmp(sl->word[0], "FileInfoCompression")==0)	{
			if(sl->n==2) {
				indexf->header.applyFileInfoCompression = (lstrstr(sl->word[1], "yes")) ? 1 : 0;
			} else progerr("FileInfoCompression requires one value");
		}
		else if (strcasecmp(sl->word[0], "ConvertHTMLEntities")==0)	{
			if(sl->n==2) {
				sw->ConvertHTMLEntities = (lstrstr(sl->word[1], "yes")) ? 1 : 0;
			} else progerr("ConvertHTMLEntities requires one value");
		}
		else if (!parseconfline(sw,sl)) {
			printf("Bad directive on line #%d: %s", linenumber, line );
			baddirective = 1;
		}
		freeStringList(sl);
	}
	fclose(fp);
	
	if (baddirective)
		exit(1);
	if (gotdir && !(*hasdir))
		*hasdir = 1;
	if (gotindex && !(*hasindex))
		*hasindex = 1;
}

/* Checks that all the regex in the replace list are correct */
void checkReplaceList(SWISH *sw) 
{
struct swline *tmpReplace;
char *rule;
int lenpatt;
char *patt;
regex_t re;
int status;
	
	patt = (char *) emalloc((lenpatt=MAXSTRLEN) + 1);
	tmpReplace = sw->replacelist;
	while (tmpReplace) {
		rule = tmpReplace->line;
		
		/*  If it is not replace, just do nothing */
		if (lstrstr(rule,"append") || lstrstr(rule,"prepend") ) {
			if (tmpReplace->next){
				tmpReplace = tmpReplace->next;
			} else {
				efree(patt);
				return;
			}
		}
		if (lstrstr(rule,"replace")) {
			tmpReplace = tmpReplace->next;
			patt = SafeStrCopy(patt,tmpReplace->line,&lenpatt);
			status = regcomp(&re,patt, REG_EXTENDED);
			regfree(&re); /** Marc Perrin ## 18Jan99 **/
			if (status != 0) {
				printf ("Illegal regular expression %s\n",patt);
				efree(patt);
				exit(0);
			}
			
			if (tmpReplace->next) 
				tmpReplace = tmpReplace->next;
			else {
				efree(patt);
				return;
			}
		}
		tmpReplace = tmpReplace->next;
	}
	efree(patt);
}


/*
  read stop words from file
  lines beginning with # are comments
  2000-06-15 rasc

*/

void readstopwordsfile (SWISH *sw, IndexFILE *indexf, char *stopw_file) 
{
char line[MAXSTRLEN];
FILE *fp;
StringList *sl;
int   i;


  if ((fp=fopen(stopw_file, "r")) == NULL || !isfile(stopw_file) ) {
      progerr("Couldn't open the stopword file \"%s\".", stopw_file);
  }


  /* read all lines and store each word as stopword */

  while (fgets(line, MAXSTRLEN, fp) != NULL) {
      if (line[0] == '#' || line[0] == '\n') continue; 

      sl=parse_line(line);
      if(sl && sl->n) {
	for(i=0;i<sl->n;i++) {
	   addstophash(indexf,sl->word[i]);
	}
	freeStringList(sl);
      }
  }

  fclose (fp);
  return;
}

FILE* openIndexFILEForWrite(char *filename)
{
	return fopen(filename, FILEMODE_WRITE);
}

FILE* openIndexFILEForRead(char *filename)
{
	return fopen(filename, FILEMODE_READ);
}

FILE* openIndexFILEForReadAndWrite(char *filename)
{
	return fopen(filename, FILEMODE_READWRITE);
}

void CreateEmptyFile(SWISH *sw,char *filename)
{
FILE *fp;
	if(!(fp=openIndexFILEForWrite(filename))) {
		progerr("Couldn't write the file \"%s\".", filename);
	}
	fclose(fp);
}

/*
 * Invoke the methods of the current Indexing Data Source
 */
void indexpath(SWISH *sw,char *path)
{
	/* invoke routine to index a "path" */
	(*IndexingDataSource->indexpath_fn)(sw,path);
}

int parseconfline(SWISH *sw,StringList *sl)
{
	/* invoke routine to parse config file lines */
	return (*IndexingDataSource->parseconfline_fn)(sw,(void *)sl);
}


char *read_stream(FILE *fp,int filelen)
{
int c=0,offset,bufferlen=0;
unsigned char *buffer;
	if(filelen)
	{
		buffer=emalloc(filelen+1);
		fread(buffer,1,filelen,fp);
	} else {    /* if we are reading from a popen call, filelen is 0 */

		buffer=emalloc((bufferlen=MAXSTRLEN)+1);
		for(offset=0;(c=fread(buffer+offset,1,MAXSTRLEN,fp))==MAXSTRLEN;offset+=MAXSTRLEN)
		{
			bufferlen+=MAXSTRLEN;
			buffer=erealloc(buffer,bufferlen+1);
		}
		filelen=offset+c;
	}
	buffer[filelen]='\0';
	return (char *)buffer;
}


/*
  read "use" words from file
  lines beginning with # are comments
  2000-11 jmruiz
  
  Based on readstopwordsfile (from rasc) 

*/

void readusewordsfile (SWISH *sw,IndexFILE *indexf, char *usew_file) 
{
char line[MAXSTRLEN];
FILE *fp;
StringList *sl;
int   i;


  if ((fp=fopen(usew_file, "r")) == NULL || !isfile(usew_file) ) {
      progerr("Couldn't open the useword file \"%s\".", usew_file);
  }


  /* read all lines and store each word as useword */

  while (fgets(line, MAXSTRLEN, fp) != NULL) {
      if (line[0] == '#' || line[0] == '\n') continue; 

      sl=parse_line(line);
      if(sl && sl->n) {
	for(i=0;i<sl->n;i++) {
	   addusehash(indexf,sl->word[i]);
	}
	freeStringList(sl);
      }
  }

  fclose (fp);
  return;
}





/*
  -- file_properties
  -- Get/eval information about a file and return it.
  -- Some flags are calculated from swish configs for this "real_path"
  -- Structure has to be freed using free_file_properties
  -- 2000-11-15 rasc
  -- return: (FileProp *)
  -- A failed stat returns an empty (default) structure

  -- 2000-12
  -- Added StoreDescription
*/

FileProp *file_properties (char *real_path, char *work_file, SWISH *sw)
{
  FileProp     *fprop; 
  struct stat  stbuf;


  fprop = (FileProp *)emalloc(sizeof(FileProp));
  /* emalloc checks fail and aborts... */


  /* -- init */
  fprop->fp = (FILE *) NULL;
  fprop->real_path = fprop->work_path = (char *)NULL;
  fprop->fsize = 0;
  fprop->mtime = (time_t) 0;
  fprop->doctype = sw->DefaultDocType;  
  fprop->index_no_content = 0;	      /* former: was indextitleonly! */
  fprop->filterprog = NULL; 	      /* Default = No Filter */
  fprop->stordesc = NULL; 	      /* Default = No summary */

  fprop->real_path = real_path;
  fprop->work_path = (work_file) ? work_file : real_path;

  /* -- Get Properties of File
     --  return if error or file not exists
   */
  if ( stat(fprop->work_path, &stbuf) ) return fprop;
  fprop->fsize = (long) stbuf.st_size;
  fprop->mtime = stbuf.st_mtime;

  /* -- get Doc Type as is in IndexContents or Defaultcontents
     -- doctypes by jruiz
  */

  fprop->doctype = getdoctype(fprop->real_path,sw->indexcontents);
  if(fprop->doctype == NODOCTYPE && sw->DefaultDocType!=NODOCTYPE) {
     fprop->doctype = sw->DefaultDocType;  
  }


  /* -- index just the filename (or doc title tags)?
     -- this param was "wrongly" named indextitleonly */

  fprop->index_no_content =  (sw->nocontentslist != NULL) &&
                    isoksuffix(fprop->real_path, sw->nocontentslist);

  /* -- Any filter for this file type?
     -- NULL = No Filter, (char *) path to filter prog.
  */

  fprop->filterprog = hasfilter (fprop->real_path,sw->filterlist);

  fprop->stordesc = hasdescription (fprop->doctype,sw->storedescription);


#ifdef DEBUG
  fprintf (stderr,"file_properties: path=%s, (workpath=%s), fsize=%ld, last_mod=%ld Doctype: %d Filter: %p\n",
       fprop->real_path, fprop->work_path, (long)fprop->fsize, (long)fprop->mtime,fprop->doctype, fprop->filterprog);
#endif

  return fprop;
}


/* -- Free FileProp structure
   -- unless no alloc for strings simple free structure
*/

void free_file_properties (FileProp *fprop)
{
  efree (fprop);
}

