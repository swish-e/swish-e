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
**
*/

#include "swish.h"
#include "file.h"
#include "mem.h"
#include "string.h"
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

int isdirectory(path)
char *path;
{
	struct stat stbuf;
	
	if (stat(path, &stbuf))
		return 0;
	return ((stbuf.st_mode & S_IFMT) == S_IFDIR) ? 1 : 0;
}

/* Is a file a regular file?
*/

int isfile(path)
char *path;
{
	struct stat stbuf;
	
	if (stat(path, &stbuf))
		return 0;
	return ((stbuf.st_mode & S_IFMT) == S_IFREG) ? 1 : 0;
}

/* Is a file a link?
*/

int islink(path)
char *path;
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

int getsize(path)
char *path;
{
	struct stat stbuf;
	
	if (stat(path, &stbuf))
		return -1;
	return stbuf.st_size;
}

/* Add an entry to the metaEntryList with the given value and the
** appropriate index
*/
/* #### Changed the name isDocProp by metaType */
void addMetaEntry(IndexFILE *indexf, char *metaWord, int metaType, int *applyautomaticmetanames)
/* #### */
{
int i;
struct metaEntry* tmpEntry;
struct metaEntry* newEntry;
	
	if(metaWord == NULL || metaWord[0]=='\0') return;
	for( i=0; metaWord[i]; i++)
		metaWord[i] =  tolower(metaWord[i]);
	
	/* 06/00 Jose Ruiz - Check for automatic metanames
	*/
	if(((int)strlen(metaWord)==9) && memcmp(metaWord,"automatic",9)==0) {
		*applyautomaticmetanames =1;
		return;
	}
	if (indexf->Metacounter <2)
		indexf->Metacounter = 2;

/* #### Jose Ruiz - New Stuff. Use metaType */
	/* See if there is a previous metaname */
	for (tmpEntry=indexf->metaEntryList;tmpEntry;tmpEntry=tmpEntry->next)   
		if (strcmp(tmpEntry->metaName,metaWord)==0) break; /* found */

	if(!tmpEntry)      /* metaName not found - Create a new one */
	{
		newEntry=(struct metaEntry*) emalloc(sizeof(struct metaEntry));
		newEntry->metaType = 0;
		newEntry->metaName = (char*)estrdup(metaWord);
		newEntry->index = indexf->Metacounter++;
		newEntry->next = NULL;
			/* Add at the end of the list of metanames */
		if (indexf->metaEntryList)
		{
			for(tmpEntry=indexf->metaEntryList;tmpEntry->next!=NULL;tmpEntry=tmpEntry->next)
			;
			tmpEntry->next = newEntry;
		}
		else
			indexf->metaEntryList = newEntry;
		tmpEntry = newEntry;
	}
	/* Add metaType info */
	tmpEntry->metaType |= metaType;

	/* #### End of changes */
	return;
}

/*
 * Some handy routines for parsing the Configuration File
 */

int grabYesNoField(line, commandTag, yesNoValue)
char* line;
char* commandTag;
int* yesNoValue;
{
	char *value;
	if ((value = getconfvalue(line, commandTag)))
	{
		*yesNoValue = (lstrstr(value, "yes")) ? 1 : 0;
		efree(value);
		return 1;	/* matched commandTag */
	}
	return 0;
}

/* 05/00 Jose Ruiz
** Function rewritten
*/
char *grabStringValueField(line, commandTag)
char* line;        /* line of input to be parsed */
char* commandTag;  /* constant string to look for */
{
	return(getconfvalue(line, commandTag));
}

int grabIntValueField(line, commandTag, singleValue, dontToIt)
char* line;
char* commandTag;
int* singleValue;
int dontToIt;
{
	char *value;
	if (!(value = grabStringValueField(line, commandTag)))
		return 0;
	
	if ((value[0]) && (value[0] != '\n') && !dontToIt)
	{
		*singleValue = atoi(value);
	}
	if(value[0])efree(value);
	return 1;	/* matched commandTag */
}


int grabCmdOptionsMega(line, commandTag, listOfWords, gotAny, dontToIt)
char* line;
char* commandTag;
struct swline **listOfWords;
int* gotAny;
int dontToIt;
{
char *value=NULL;
int skiplen;

	/*
	 * parse the line if it contains commandTag 
	 * (commandTag is not required to be the first token in the line)
	 * Grab all of the words after commandTag and place them in the listOfWords.
	 * If "gotAny" is not NULL then set it to 1 if we grabbed any words.
	 * If dontDoIt is "1" then do not grab the words.
	 * Line may be "<commandTag> <stringValue> .." but it could also
	 * be "<other commands> <commandTag> <stringValue> .."
	 */
	line = lstrstr(line, commandTag);	/* includes main command tag? */
	if (line == NULL)
		return 0;
	line += strlen(commandTag);
	
	/* grab all words after the command tag */
	if (!dontToIt)
	{
		while (1) 
		{
			value=getword(line, &skiplen);
			/* BUG 2/22/00 - SRE - next line had one | and one || */
			if (!skiplen || value[0] == '\0' || value[0] == '\n')
			{
				break;
			}
			else 
			{
				line += skiplen;
				*listOfWords = (struct swline *) addswline(*listOfWords, value);
				efree(value);
				if (gotAny) *gotAny = 1;
			}
		}
	}
	return 1;
}

int grabCmdOptions(line, commandTag, listOfWords)
char* line;
char* commandTag;
struct swline **listOfWords;
{
	return grabCmdOptionsMega(line, commandTag, listOfWords, NULL, 0);
}

/* Reads the configuration file and puts all the right options
** in the right variables and structures.
*/

void getdefaults(sw, conffile, hasdir, hasindex, hasverbose)
SWISH *sw;
char *conffile;
int *hasdir;
int *hasindex;
int hasverbose;
{
int i, gotdir, gotindex;
char *c, line[MAXSTRLEN], *StringValue;
FILE *fp;
int linenumber = 0;
int baddirective = 0;
StringList *sl;
int DocType=0;
struct IndexContents *ic;
struct StoreDescription *sd;
IndexFILE *indexf=NULL;
	
	gotdir = gotindex = 0;
	
	if ((fp = fopen(conffile, "r")) == NULL  ||
		!isfile(conffile) ) 
	{
		sw->errorstr=BuildErrorString(sw->errorstr, &sw->lenerrorstr, "Couldn't open the configuration file \"%s\".", conffile);
		progerr(sw->errorstr);
	}

	/* Init default index file */
	indexf=sw->indexlist=(IndexFILE *) addindexfile(sw->indexlist, INDEXFILE);
	while (fgets(line, MAXSTRLEN, fp) != NULL) 
	{
		linenumber++;
		if (line[0] == '#' || line[0] == '\n')
			continue;
		if (grabCmdOptionsMega(line, "IndexDir", &sw->dirlist, &gotdir, *hasdir)) {}
		else if (grabCmdOptions(line, "NoContents", &sw->nocontentslist)) {}
		else if ((StringValue=grabStringValueField(line, "IndexFile"))) {
			efree(indexf->line);
			indexf->line=StringValue;
		}
		else if (grabIntValueField(line, "IndexReport", &sw->verbose, hasverbose))	{}
		else if (grabIntValueField(line, "MinWordLimit", &indexf->header.minwordlimit, 0))	{}
		else if (grabIntValueField(line, "IndexComments", &sw->indexComments, 0))	{}
		else if (grabIntValueField(line, "MaxWordLimit", &indexf->header.maxwordlimit, 0))	{}
		else if ((StringValue=grabStringValueField(line, "WordCharacters")))	{
			indexf->header.wordchars = SafeStrCopy(indexf->header.wordchars,StringValue,&indexf->header.lenwordchars);
			efree(StringValue);
			sortstring(indexf->header.wordchars);
			makelookuptable(indexf->header.wordchars,indexf->header.wordcharslookuptable);
		}
		else if ((StringValue=grabStringValueField(line, "BeginCharacters")))	{
			indexf->header.beginchars = SafeStrCopy(indexf->header.beginchars,StringValue,&indexf->header.lenbeginchars);
			efree(StringValue);
			sortstring(indexf->header.beginchars);
			makelookuptable(indexf->header.beginchars,indexf->header.begincharslookuptable);
		}
		else if ((StringValue=grabStringValueField(line, "EndCharacters")))	{
			indexf->header.endchars = SafeStrCopy(indexf->header.endchars,StringValue,&indexf->header.lenendchars);
			efree(StringValue);
			sortstring(indexf->header.endchars);
			makelookuptable(indexf->header.endchars,indexf->header.endcharslookuptable);
		}
		else if ((StringValue=grabStringValueField(line, "IgnoreLastChar")))	{
			indexf->header.ignorelastchar = SafeStrCopy(indexf->header.ignorelastchar,StringValue,&indexf->header.lenignorelastchar);
			efree(StringValue);
			sortstring(indexf->header.ignorelastchar);
			makelookuptable(indexf->header.ignorelastchar,indexf->header.ignorelastcharlookuptable);
		}
		else if ((StringValue=grabStringValueField(line, "IgnoreFirstChar")))	{
			indexf->header.ignorefirstchar = SafeStrCopy(indexf->header.ignorefirstchar,StringValue,&indexf->header.lenignorefirstchar);
			efree(StringValue);
			sortstring(indexf->header.ignorefirstchar);
			makelookuptable(indexf->header.ignorefirstchar,indexf->header.ignorefirstcharlookuptable);
		}
		else if (grabCmdOptions(line, "ReplaceRules", &sw->replacelist)) { checkReplaceList(sw); }
		else if (grabYesNoField(line, "FollowSymLinks", &sw->followsymlinks))	{}
		else if ((StringValue=grabStringValueField(line, "IndexName")))	{
			indexf->header.indexn = SafeStrCopy(indexf->header.indexn,StringValue,&indexf->header.lenindexn);
			efree(StringValue);
		}
		else if ((StringValue=grabStringValueField(line, "IndexDescription")))	{
			indexf->header.indexd = SafeStrCopy(indexf->header.indexd,StringValue,&indexf->header.lenindexd);
			efree(StringValue);
		}
		else if ((StringValue=grabStringValueField(line, "IndexPointer")))	{
			indexf->header.indexp = SafeStrCopy(indexf->header.indexp,StringValue,&indexf->header.lenindexp);
			efree(StringValue);
		}
		else if ((StringValue=grabStringValueField(line, "IndexAdmin")))	{
			indexf->header.indexa = SafeStrCopy(indexf->header.indexa,StringValue,&indexf->header.lenindexa);
			efree(StringValue);
		}
		else if (grabYesNoField(line, "UseStemming", &indexf->header.applyStemmingRules))	{}	/* 11/24/98 MG */
		else if (grabYesNoField(line, "IgnoreTotalWordCountWhenRanking", &indexf->header.ignoreTotalWordCountWhenRanking))	{}	/* 11/24/98 MG */
                else if (grabYesNoField(line, "UseSoundex", &indexf->header.applySoundexRules))        {}      /* 09/01/99 DN */
                else if ((StringValue=grabStringValueField(line, "FilterDir")))    {      /* 1999-05-05 rasc */
			sw->filterdir = SafeStrCopy(sw->filterdir,StringValue,&sw->lenfilterdir);
			if(!isdirectory(sw->filterdir)) 
			{
				sw->errorstr=BuildErrorString(sw->errorstr,&sw->lenerrorstr,"Error in FilterDir. %s is not a directory\n.\n",sw->filterdir);
				progerr(sw->errorstr);
			}
			efree(StringValue);
		}
                else if ((c = (char *) lstrstr(line, "FileFilter"))) {
        /* 1999-05-05 rasc */
                                     /* FileFilter fileextension  filerprog */
                        c += strlen("FileFilter");
			sl=parse_line(c);
			if(sl && sl->n==2) {
				sw->filterlist = (struct filter *) addfilter(sw->filterlist,sl->word[0],sl->word[1],sw->filterdir);
				freeStringList(sl);
			} else progerr("FileFilter requires two values");
                }
		else if ((c = (char *) lstrstr(line, "MetaNames")) != 0)  /* gcc -Wall, 2/22/00 */
		{
			c += strlen("MetaNames");
			sl=parse_line(c);
			if(sl && sl->n) {
				for(i=0;i<sl->n;i++)
/* #### changed 0 by META */
					addMetaEntry(indexf,sl->word[i], META_INDEX, &sw->applyautomaticmetanames);
/* #### */
				freeStringList(sl);
			} else progerr("MetaNames requires at least one value");
		}
		else if ((c = (char *) lstrstr(line, "TranslateCharacters")) != 0)  
		{
			c += strlen("TranslateCharacters");
			sl=parse_line(c);
			if(sl && sl->n==2) {
				indexf->header.translatechars1=SafeStrCopy(indexf->header.translatechars1,sl->word[0],&indexf->header.lentranslatechars1);
				indexf->header.translatechars2=SafeStrCopy(indexf->header.translatechars2,sl->word[1],&indexf->header.lentranslatechars2);
				freeStringList(sl);
				if(strlen(indexf->header.translatechars1)!=strlen(indexf->header.translatechars2)) progerr("TranslateCharacters option requires two values of the same length");
				
			} else progerr("TranslateCharacters requires two values");
		}
		else if ((c = (char *) lstrstr(line, "PropertyNames")) != 0)	/* 11/24/98 MG */ /* gcc -Wall, 2/22/00 */
		{
			c += strlen("PropertyNames");
			sl=parse_line(c);
			if(sl && sl->n) {
				for(i=0;i<sl->n;i++)
/* #### changed 1 by META_PROP */
					addMetaEntry(indexf,sl->word[i], META_PROP, &sw->applyautomaticmetanames);
/* #### */
				freeStringList(sl);
			} else progerr("PropertyNames requires at least one value");
		}
		else if ((c = (char *) lstrstr(line, "IgnoreWords")) != 0) {  /* gcc -Wall, 2/22/00 */
			c += strlen("IgnoreWords");
			sl=parse_line(c);
			if(sl && sl->n) {
				if (lstrstr(sl->word[0], "SwishDefault")) {
						readdefaultstopwords(indexf);
				} else if (lstrstr(sl->word[0], "File:")) {  /* 2000-06-15 rasc */
				        if (sl->n == 2) 
					    readstopwordsfile(sw,indexf,sl->word[1]);	
					else progerr ("IgnoreWords File: requires path");
				} else for(i=0;i<sl->n;i++) {
					addstophash(indexf,sl->word[i]);
				}
				freeStringList(sl);
			} else progerr("IgnoreWords requires at least one value");
		}
		else if ((c = (char *) lstrstr(line, "UseWords")) != 0) {  /* 11/00 Jmruiz */
			indexf->is_use_words_flag=1;
			c += strlen("UseWords");
			sl=parse_line(c);
			if(sl && sl->n) {
				if (lstrstr(sl->word[0], "File:")) {  /* 2000-06-15 rasc */
				        if (sl->n == 2) 
					    readusewordsfile(sw,indexf,sl->word[1]);	
					else progerr ("UseWords File: requires path");
				} else for(i=0;i<sl->n;i++) {
					addusehash(indexf,sl->word[i]);
				}
				freeStringList(sl);
			} else progerr("UseWords requires at least one value");
		}
		else if ((c = (char *) lstrstr(line, "IgnoreLimit"))) {
			c += strlen("IgnoreLimit");
			sl=parse_line(c);
			if(sl && sl->n==2) {
				sw->plimit = atol(sl->word[0]);
				sw->flimit = atol(sl->word[1]);
				freeStringList(sl);
			} else progerr("IgnoreLimit requires two values");
		}
		/* IndexVerbose is supported for backwards compatibility */
		else if ((c = (char *) lstrstr(line, "IndexVerbose")) != 0) {  /* gcc -Wall, 2/22/00 */
			c += strlen("IndexVerbose");
			sl=parse_line(c);
			if(sl && sl->n==1) {
				sw->verbose = (lstrstr(sl->word[0], "yes")) ? 3 : 0;
				freeStringList(sl);
			} else progerr("IndexVerbose requires one value");
		}
		else if (grabCmdOptions(line, "IndexOnly", &sw->suffixlist)) {}
		else if ((c = (char *)lstrstr(line,"IndexContents")) !=0) {
			c += strlen("IndexContents");
			sl=parse_line(c);
			if(sl && sl->n>1) {
				if(strcasecmp(sl->word[0],"TXT")==0) {
					DocType=TXT;
				}
				else if(strcasecmp(sl->word[0],"HTML")==0) {
					DocType=HTML;
				}
				else if(strcasecmp(sl->word[0],"XML")==0) {
					DocType=XML;
				}
				else if(strcasecmp(sl->word[0],"MULTITXT")==0) {
					DocType=MULTITXT;
				}
				else if(strcasecmp(sl->word[0],"WML")==0) {
					DocType=WML;
				} else progerr("Unknown document type in IndexContents");
				ic=(struct IndexContents *)emalloc(sizeof(struct IndexContents));
				ic->DocType=DocType;
				ic->patt=NULL;
				for(i=1;i<sl->n;i++)
					ic->patt=addswline(ic->patt,sl->word[i]);
				if(sw->indexcontents)
					ic->next=sw->indexcontents;
				else
					ic->next=NULL;
				sw->indexcontents=ic;
			} else progerr("IndexContents requires at least two values");
		}
		else if ((c = (char *)lstrstr(line,"StoreDescription")) !=0) {
			c += strlen("StoreDescription");
			sl=parse_line(c);
			if(sl && (sl->n==2 || sl->n==3)) {
				if(strcasecmp(sl->word[0],"TXT")==0) {
					DocType=TXT;
				}
				else if(strcasecmp(sl->word[0],"HTML")==0) {
					DocType=HTML;
				}
				else if(strcasecmp(sl->word[0],"XML")==0) {
					DocType=XML;
				}
				else if(strcasecmp(sl->word[0],"MULTITXT")==0) {
					DocType=MULTITXT;
				}
				else if(strcasecmp(sl->word[0],"WML")==0) {
					DocType=WML;
				} else progerr("Unknown document type in StoreDescription");
				sd=(struct StoreDescription *)emalloc(sizeof(struct StoreDescription));
				sd->DocType=DocType;
				sd->size=0;
				sd->field=NULL;
				i=1;

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
				if(sl->n==2 && !sd->field && !sd->size)
					progerr("Second parameter of StoreDescription must be <fieldname> or a number");
				if(sl->n==3 && sd->field && !sd->size)
					progerr("Third parameter of StoreDescription must be empty or a number");
				if(sw->storedescription)
					sd->next=sw->storedescription;
				else
					sd->next=NULL;
				sw->storedescription=sd;
			} else progerr("StoreDescription requires two or three values");
		}
		else if ((c = (char *)lstrstr(line,"DefaultContents")) !=0) {
			c += strlen("DefaultContents");
			sl=parse_line(c);
			if(sl && sl->n) {
				if(strcasecmp(sl->word[0],"TXT")==0) {
					DocType=TXT;
				}
				else if(strcasecmp(sl->word[0],"HTML")==0) {
					DocType=HTML;
				}
				else if(strcasecmp(sl->word[0],"XML")==0) {
					DocType=XML;
				}
				else if(strcasecmp(sl->word[0],"MULTITXT")==0) {
					DocType=MULTITXT;
				}
				else if(strcasecmp(sl->word[0],"WML")==0) {
					DocType=WML;
				} else progerr("Unknown document type in DefaultContents");
				sw->DefaultDocType=DocType;
			} else progerr("IndexContents requires at least one value");
		}
		else if ((c = (char *)lstrstr(line,"BumpPositionCounterCharacters")) !=0) {
			c += strlen("BumpPositionCounterCharacters");
			sl=parse_line(c);
			if(sl && sl->n) {
				indexf->header.bumpposchars = SafeStrCopy(indexf->header.bumpposchars,sl->word[0],&indexf->header.lenbumpposchars);
				sortstring(indexf->header.bumpposchars);
				makelookuptable(indexf->header.bumpposchars,indexf->header.bumpposcharslookuptable);
			} else progerr("BumpPositionCounterCharacters requires at least one value");
		}
		else if ((StringValue=grabStringValueField(line, "tmpdir")))    		{
			sw->tmpdir = SafeStrCopy(sw->tmpdir,StringValue,&sw->lentmpdir);
			if(!isdirectory(sw->tmpdir)) 
			{
				sw->errorstr=BuildErrorString(sw->errorstr,&sw->lenerrorstr,"Error in TempDir. %s is not a directory\n.\n",sw->tmpdir);
				progerr(sw->errorstr);
			}
		}
		else if (grabYesNoField(line, "FileInfoCompression", &indexf->header.applyFileInfoCompression))	{}
		else if (grabYesNoField(line, "OkNoMeta", &sw->OkNoMeta))	{}
		else if (grabYesNoField(line, "ReqMetaName", &sw->ReqMetaName))	{}
		else if (grabYesNoField(line, "AsciiEntities", &sw->AsciiEntities))	{}
		else if (!parseconfline(sw,line)) {
			printf("Bad directive on line #%d: %s", linenumber, line );
			baddirective = 1;
		}
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


#ifdef DEBUG
	   printf ("Open StopWordfile:  %s\n",stopw_file);
#endif

  if ((fp=fopen(stopw_file, "r")) == NULL || !isfile(stopw_file) ) {
      sw->errorstr=BuildErrorString(sw->errorstr, &sw->lenerrorstr, "Couldn't open the stopword file \"%s\".", stopw_file);
      progerr(sw->errorstr);
  }


  /* read all lines and store each word as stopword */

  while (fgets(line, MAXSTRLEN, fp) != NULL) {
      if (line[0] == '#' || line[0] == '\n') continue; 

      sl=parse_line(line);
      if(sl && sl->n) {
	for(i=0;i<sl->n;i++) {
	   addstophash(indexf,sl->word[i]);
#ifdef DEBUG
	   printf ("  %s\n",sl->word[i]);
#endif
        }
	freeStringList(sl);
      }
  }

  fclose (fp);
  return;
}

FILE* openIndexFILEForWrite(filename)
char* filename;
{
	return fopen(filename, FILEMODE_WRITE);
}

FILE* openIndexFILEForRead(filename)
char* filename;
{
	return fopen(filename, FILEMODE_READ);
}

FILE* openIndexFILEForReadAndWrite(filename)
char* filename;
{
	return fopen(filename, FILEMODE_READWRITE);
}

void CreateEmptyFile(SWISH *sw,char *filename)
{
FILE *fp;
	if(!(fp=openIndexFILEForWrite(filename)))
	{
		sw->errorstr=BuildErrorString(sw->errorstr, &sw->lenerrorstr, "Couldn't write the file \"%s\".", filename);
		progerr(sw->errorstr);
	}
	fclose(fp);
}

/*
 * Invoke the methods of the current Indexing Data Source
 */
void indexpath(sw,path)
SWISH *sw;
char *path;
{
	/* invoke routine to index a "path" */
	(*IndexingDataSource->indexpath_fn)(sw,path);
}

int parseconfline(sw,line)
SWISH *sw;
char *line;
{
	/* invoke routine to parse config file lines */
	return (*IndexingDataSource->parseconfline_fn)(sw,line);
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


#ifdef DEBUG
	   printf ("Open StopWordfile:  %s\n",usew_file);
#endif

  if ((fp=fopen(usew_file, "r")) == NULL || !isfile(usew_file) ) {
      sw->errorstr=BuildErrorString(sw->errorstr, &sw->lenerrorstr, "Couldn't open the useword file \"%s\".", usew_file);
      progerr(sw->errorstr);
  }


  /* read all lines and store each word as useword */

  while (fgets(line, MAXSTRLEN, fp) != NULL) {
      if (line[0] == '#' || line[0] == '\n') continue; 

      sl=parse_line(line);
      if(sl && sl->n) {
	for(i=0;i<sl->n;i++) {
	   addusehash(indexf,sl->word[i]);
#ifdef DEBUG
	   printf ("  %s\n",sl->word[i]);
#endif
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

