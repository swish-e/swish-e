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
*/

/* New file created from file.c 02/2001 jmruiz */
/* Contains routines for parsing the configuration file */

/*
** 2001-02-15 rasc    ResultExtFormatName 
** 2001-03-03 rasc    EnableAltaVistaSyntax
**                    code optimize: getYesNoOrAbort 
**
*/


#include "swish.h"
#include "mem.h"
#include "list.h"
#include "string.h"
#include "file.h"
#include "metanames.h"
#include "hash.h"
#include "error.h"
#include "filter.h"
#include "result_output.h"
#include "parse_conffile.h"


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
char line[MAXSTRLEN];
FILE *fp;
int linenumber = 0;
int baddirective = 0;
StringList *sl;
int DocType=0;
struct IndexContents *ic;
struct StoreDescription *sd;
IndexFILE *indexf=NULL;
unsigned char *StringValue=NULL;
char *w0;
	
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
		w0 = sl->word[0];				/* Config Direct. = 1. word */
		
		if (w0[0] == '#') continue;		/* comment */

		if (strcasecmp(w0,"IndexDir")==0) {
			if(sl->n>1) {
				if(!*hasdir)
				{
					gotdir=1;
					grabCmdOptions(sl,1, &sw->dirlist);
				}
			} else progerr("IndexDir requires one value");
		}
		else if (strcasecmp(w0, "IncludeConfigFile")==0) {
			if(sl->n==2) {
				getdefaults(sw,sl->word[1],hasdir,hasindex,hasverbose);
			} else progerr("IncludeConfigfile requires one value");
		}
		else if (strcasecmp(w0,"NoContents")==0) {
			if(sl->n>1) {
				grabCmdOptions(sl,1,&sw->nocontentslist);
			} else progerr("NoContents requires one value");
		}
		else if (strcasecmp(w0, "IndexFile")==0) {
			if(sl->n==2) {
				if(indexf->line) efree(indexf->line);
				indexf->line=estrdup(sl->word[1]);
			} else progerr("Indexfile requires one value");
		}
		else if (strcasecmp(w0,"IndexReport")==0){
			if(sl->n==2) {
				if(!hasverbose)
				{
					sw->verbose=atoi(sl->word[1]);
				}
			} else progerr("IndexReport requires one value");
		}
		else if (strcasecmp(w0, "MinWordLimit")==0){
			if(sl->n==2) {
				indexf->header.minwordlimit=atoi(sl->word[1]);
			} else progerr("MinWordLimit requires one value");
		}
		else if (strcasecmp(w0, "MaxWordLimit")==0){
			if(sl->n==2) {
				indexf->header.maxwordlimit=atoi(sl->word[1]);
			} else progerr("MaxWordLimit requires one value");
		}
		else if (strcasecmp(w0, "IndexComments")==0){
			if(sl->n==2) {
				sw->indexComments=atoi(sl->word[1]);
			} else progerr("IndexComments requires one value");
		}
		else if (strcasecmp(w0, "WordCharacters")==0)	{
			if(sl->n==2) {
				indexf->header.wordchars = SafeStrCopy(indexf->header.wordchars,sl->word[1],&indexf->header.lenwordchars);
				sortstring(indexf->header.wordchars);
				makelookuptable(indexf->header.wordchars,indexf->header.wordcharslookuptable);
			} else progerr("WordCharacters requires one value");
		}
		else if (strcasecmp(w0, "BeginCharacters")==0)	{
			if(sl->n==2) {
				indexf->header.beginchars = SafeStrCopy(indexf->header.beginchars,sl->word[1],&indexf->header.lenbeginchars);
				sortstring(indexf->header.beginchars);
				makelookuptable(indexf->header.beginchars,indexf->header.begincharslookuptable);
			} else progerr("BeginCharacters requires one value");
		}
		else if (strcasecmp(w0, "EndCharacters")==0)	{
			if(sl->n==2) {
				indexf->header.endchars = SafeStrCopy(indexf->header.endchars,sl->word[1],&indexf->header.lenendchars);
				sortstring(indexf->header.endchars);
				makelookuptable(indexf->header.endchars,indexf->header.endcharslookuptable);
			} else progerr("EndCharacters requires one value");
		}
		else if (strcasecmp(w0, "IgnoreLastChar")==0)	{
			if(sl->n==2) {
				indexf->header.ignorelastchar = SafeStrCopy(indexf->header.ignorelastchar,sl->word[1],&indexf->header.lenignorelastchar);
				sortstring(indexf->header.ignorelastchar);
				makelookuptable(indexf->header.ignorelastchar,indexf->header.ignorelastcharlookuptable);
			} else progerr("IgnoreLastChar requires one value");
		}
		else if (strcasecmp(w0, "IgnoreFirstChar")==0)	{
			if(sl->n==2) {
				indexf->header.ignorefirstchar = SafeStrCopy(indexf->header.ignorefirstchar,sl->word[1],&indexf->header.lenignorefirstchar);
				sortstring(indexf->header.ignorefirstchar);
				makelookuptable(indexf->header.ignorefirstchar,indexf->header.ignorefirstcharlookuptable);
			} else progerr("IgnoreFirstChar requires one value");
		}
		else if (strcasecmp(w0, "ReplaceRules")==0)	{
			if(sl->n>1) {
				grabCmdOptions(sl,1, &sw->replacelist);
				checkReplaceList(sw);
			} else progerr("ReplaceRules requires at least one value");
		}
		else if (strcasecmp(w0, "FollowSymLinks")==0)	{
			sw->followsymlinks = getYesNoOrAbort (sl, 1, 1);
		}
		else if (strcasecmp(w0, "IndexName")==0)	{
			if(sl->n>1) {
				StringValue=StringListToString(sl,1);
				indexf->header.indexn = SafeStrCopy(indexf->header.indexn,StringValue,&indexf->header.lenindexn);
				efree(StringValue);
			} else progerr("IndexName requires a value");
		}
		else if (strcasecmp(w0, "IndexDescription")==0)	{
			if(sl->n>1) {
				StringValue=StringListToString(sl,1);
				indexf->header.indexd = SafeStrCopy(indexf->header.indexd,StringValue,&indexf->header.lenindexd);
				efree(StringValue);
			} else progerr("IndexDescription requires a value");
		}
		else if (strcasecmp(w0, "IndexPointer")==0)	{
			if(sl->n>1) {
				StringValue=StringListToString(sl,1);
				indexf->header.indexp = SafeStrCopy(indexf->header.indexp,StringValue,&indexf->header.lenindexp);
				efree(StringValue);
			} else progerr("IndexPointer requires a value");
		}
		else if (strcasecmp(w0, "IndexAdmin")==0)	{
			if(sl->n>1) {
				StringValue=StringListToString(sl,1);
				indexf->header.indexa = SafeStrCopy(indexf->header.indexa,StringValue,&indexf->header.lenindexa);
				efree(StringValue);
			} else progerr("IndexAdmin requires one value");
		}
		else if (strcasecmp(w0, "UseStemming")==0) {
			indexf->header.applyStemmingRules = getYesNoOrAbort (sl, 1,1);
		}
		else if (strcasecmp(w0, "IgnoreTotalWordCountWhenRanking")==0)	{
			indexf->header.ignoreTotalWordCountWhenRanking = getYesNoOrAbort (sl, 1,1);
		}
		else if (strcasecmp(w0, "UseSoundex")==0)	{
			indexf->header.applySoundexRules = getYesNoOrAbort (sl, 1,1);
		}
                else if (strcasecmp(w0, "FilterDir")==0)    {      /* 1999-05-05 rasc */
			if(sl->n==2) {
				sw->filterdir = estrredup(sw->filterdir,sl->word[1]);
				if(!isdirectory(sw->filterdir)) {
					progerr("FilterDir. %s is not a directory",sw->filterdir);
				}
			} else progerr("FilterDir requires one value");
		}
                else if (strcasecmp(w0, "FileFilter")==0) {  /* 1999-05-05 rasc */
                                     /* FileFilter fileextension  filerprog */
			if(sl->n==3) {
				sw->filterlist = (struct filter *) addfilter(sw->filterlist,sl->word[1],sl->word[2],sw->filterdir);
			} else progerr("FileFilter requires two values");
                }
                else if (strcasecmp(w0, "ResultExtFormatName")==0) {  /* 2001-02-15 rasc */
                                     /* ResultExt...   name  fmtstring */
                                     /* $$$ this will not work unless swish is reading the config file also for search ... */
			if(sl->n==3) {
			   sw->resultextfmtlist = (struct ResultExtFmtStrList *)
				 addResultExtFormatStr(sw->resultextfmtlist,sl->word[1],sl->word[2]);
			} else progerr("ResultExtFormatName requires \"name\" \"fmtstr\"");
                }
		else if (strcasecmp(w0, "MetaNames")== 0) 
		{
			if(sl->n>1) {
				for(i=1;i<sl->n;i++)
					addMetaEntry(indexf,sl->word[i], META_INDEX, 0, 0, &sw->applyautomaticmetanames);
			} else progerr("MetaNames requires at least one value");
		}
		else if (strcasecmp(w0, "TranslateCharacters")== 0) {
			if (sl->n >= 2) {
				if (! BuildTranslateChars(indexf->header.translatecharslookuptable,
					sl->word[1],sl->word[2]) ) {
				   progerr ("TranslateCharacters requires two values (same length) or one translation rule");
				}
			}
		}
		else if (strcasecmp(w0, "PropertyNames")== 0) {
			if(sl->n>1) {
				for(i=1;i<sl->n;i++)
					addMetaEntry(indexf,sl->word[i], META_PROP, 0, 0, &sw->applyautomaticmetanames);
			} else progerr("PropertyNames requires at least one value");
		}
		else if (strcasecmp(w0, "IgnoreWords")== 0) {
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
		else if (strcasecmp(w0, "UseWords")== 0) {  /* 11/00 Jmruiz */
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
		else if (strcasecmp(w0, "IgnoreLimit")==0) {
			if(sl->n==3) {
				sw->plimit = atol(sl->word[1]);
				sw->flimit = atol(sl->word[2]);
			} else progerr("IgnoreLimit requires two values");
		}
		/* IndexVerbose is supported for backwards compatibility */
		else if (strcasecmp(w0, "IndexVerbose")==0) {
			sw->verbose = getYesNoOrAbort (sl, 1,1);
			if (sw->verbose) sw->verbose = 3; 
		}
		else if (strcasecmp(w0, "IndexOnly")==0)	{
			if(sl->n>1) {
				grabCmdOptions(sl,1, &sw->suffixlist);
			} else progerr("IndexOnly requires at least one value");
		}
		else if (strcasecmp(w0,"IndexContents")==0) {
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
				else if(strcasecmp(sl->word[1],"LST")==0) {
					DocType=LST;
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
		else if (strcasecmp(w0,"StoreDescription")==0) {
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
				else if(strcasecmp(sl->word[1],"LST")==0) {
					DocType=LST;
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
		else if (strcasecmp(w0,"DefaultContents")==0) {
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
				else if(strcasecmp(sl->word[1],"LST")==0) {
					DocType=LST;
				}
				else if(strcasecmp(sl->word[1],"WML")==0) {
					DocType=WML;
				} else progerr("Unknown document type in DefaultContents");
				sw->DefaultDocType=DocType;
			} else progerr("IndexContents requires at least one value");
		}
		else if (strcasecmp(w0,"BumpPositionCounterCharacters")==0) {
			if(sl->n>1) {
				indexf->header.bumpposchars = SafeStrCopy(indexf->header.bumpposchars,sl->word[1],&indexf->header.lenbumpposchars);
				sortstring(indexf->header.bumpposchars);
				makelookuptable(indexf->header.bumpposchars,indexf->header.bumpposcharslookuptable);
			} else progerr("BumpPositionCounterCharacters requires at least one value");
		}
		else if (strcasecmp(w0, "tmpdir")==0) {
			if(sl->n==2) {
				sw->tmpdir = SafeStrCopy(sw->tmpdir,sl->word[1],&sw->lentmpdir);
				if(!isdirectory(sw->tmpdir)) {
					progerr("TempDir. %s is not a directory",sw->tmpdir);
				}
			} else progerr("TmpDir requires one value");
		}
/* #### Added UndefinedMetaTags as defined by Bill Moseley */
		else if (strcasecmp(w0, "UndefinedMetaTags")==0)	
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
		else if (strcasecmp(w0, "FileInfoCompression")==0)	{
			indexf->header.applyFileInfoCompression = getYesNoOrAbort (sl, 1,1);
		}
		else if (strcasecmp(w0, "ConvertHTMLEntities")==0)	{
			sw->ConvertHTMLEntities = getYesNoOrAbort (sl, 1,1);
		}
		else if (strcasecmp(w0, "EnableAltaVistaSyntax")==0)	{
			sw->enableAVSearchSyntax = getYesNoOrAbort (sl, 1,1);
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



/*
  -- some config helper routines
*/

/*
  --  check if word "n" in StringList  is yes/no
  --  "lastparam": 0/1 = is param last one for config directive?
  --  returns 1 (yes) or 0 (no)
  --  aborts if not "yes" or "no" (and prints first word of array)
  --  aborts if lastparam set and is not last param...
  --  2001-03-04 rasc
*/

int getYesNoOrAbort (StringList *sl, int n, int lastparam)
{
   if (lastparam && n < (sl->n-1)) {
     progerr ("%s has too many paramter",sl->word[0],n);
     return 0;
   }

   if (n < sl->n) {
      if (!strcasecmp (sl->word[n], "yes")) return 1;
      if (!strcasecmp (sl->word[n], "no"))  return 0;
   }
   progerr ("%s requires as %d. parameter \"Yes\" or \"No\" value",
        sl->word[0],n);
   return 0;
}


/* --------------------------------------------------------- */




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

int parseconfline(SWISH *sw,StringList *sl)
{
	/* invoke routine to parse config file lines */
	return (*IndexingDataSource->parseconfline_fn)(sw,(void *)sl);
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



