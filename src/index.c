/*
** Copyright (C) 1995, 1996, 1997, 1998 Hewlett-Packard Company
** Originally by Kevin Hughes, kev@kevcom.com, 3/11/94
**
** This program and library is free software; you can redistribute it and/or
** as published by the Free Software Foundation; either version 2
** of the License, or any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU (Library) General Public License for more details.
**
** You should have received a copy of the GNU (Library) General Public License
**  long with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
**--------------------------------------------------------------------
** ** ** PATCHED 5/13/96, CJC
**
** Added code to countwords and countwordstr to disreguard the last char
** if requiered by the config.h
** G. Hill  3/12/97  ghill@library.berkeley.edu
**
** Changed addentry, countwords, countwordstr, parsecomment, rintindex
** added createMetaEntryList, getMeta, parseMetaData
** to support METADATA
** G. Hill 3/18/97 ghill@library.berkeley.edu
**
** Changed removestops to support printing of stop words
** G. Hill 4/7/97
**
** Changed countwords, countwrdstr, and parseMetaData to disreguard the
** first char if required by the config.h
** G.Hill 10/16/97  ghill@library.berkeley.edu
**
** Added stripIgnoreLastChars and isIgnoreLastChar routines which iteratively
** remove all ignore characters from the end of each word.
** P. Bergner  10/5/97  bergner@lcse.umn.edu
**
** Added stripIgnoreFirstChars and isIgnoreFirstChar to make stripping of
** the ignore first chars iterative.
** G. Hill 11/19/97 ghill@library.berkeley.edu
**
** Added possibility of use of quotes and brackets in meta CONTENT countwords, parsemetadata
** G. Hill 1/14/98
**
** Added regex for replace rule G.Hill 1/98
**
** REQMETANAME - don't index meta tags not specified in MetaNames
** 10/11/99 - Bill Moseley
**
** change sprintf to snprintf to avoid corruption, use MAXPROPLEN instead of literal "20",
** added include of merge.h - missing declaration caused compile error in prototypes,
** added word length arg to Stem() call for strcat overflow checking in stemmer.c
** added safestrcpy() macro to avoid corruption from strcpy overflow
** SRE 11/17/99
**
** fixed misc problems pointed out by "gcc -Wall"
** SRE 2/22/00
**
** Added code for storing word positions in index file 
** Jose Ruiz 3/00 jmruiz@boe.es
**
** 04/00 - Jose Ruiz
** Added code for a hash table in index file for searching words
** via getfileinfo in search.c (Lots of addons). Better perfomance
** with big databases and or heavy searchs (a* or b* or c*)
**
** 04/00 - Jose Ruiz
** Improved number compression function (compress)
** New number decompress function
** Both converted into macros for better performance
**
** 07/00 and 08/00 - Jose Ruiz
** Many modifications to make some functions thread safe
**
** 08/00 - Jose Ruiz
** New function indexstring. Up to know there were 4 functions doing almost
** the same thing: countwords, countwordstr, parseMetaData and parsecomment
** From now on, these 4 functions calls indexstring wich is the common part
** to all of them. In fact, countwordstr, parseMetaData and parsecomment
** are now simple frontends to indexstring
**
** 2000-11 - rasc
** some redesgin, place common index code into a common routine
** FileProp structures, routines
**
** --
** TODO
** $$ there has still to be some resesign to be done.
** $$ swish-e was originally designed to index html only. So the routines
** $$ are for historically reasons scattered
** $$ (e.g. isoktitle (), is ishtml() etc.)
**
** 2000-12 Jose Ruiz
** obsolete routine ishtml removed
** isoktitle moved to html.c
**
*/


#include "swish.h"
#include "index.h"
#include "hash.h"
#include "mem.h"
#include "string.h"
#include "check.h"
#include "search.h"
#include "merge.h"
#include "docprop.h"
#include "stemmer.h"
#include "soundex.h"
#include "error.h"
#include "file.h"
#include "compress.h"
#include "deflate.h"
#include "xml.h"
#include "txt.h"
#include "metanames.h"


/*
   -- Start the real indexing process for a file.
   -- This routine will be called by the different indexing methods
   -- (httpd, filesystem, etc.)
   -- The indexed file may be the
   --   - real file on filesystem
   --   - tmpfile or work file (shadow of the real file)
   -- Checks if file has to be send thru filter (file stream)
   -- 2000-11-19 rasc
*/

void do_index_file (SWISH *sw, FileProp *fprop)
{
int      wordcount;
char     *filtercmd;
char     *rd_buffer=NULL;	/* complete file read into buffer */

	wordcount = -1;
	filtercmd = NULL;

	if (fprop->work_path) {

                /*
                  -- jeek! simple filter hack!
                  -- simple filter call "filter 'work_file' 'real_path_url'"
		  --    > (decoded output =stdout)
                  -- if no filter defined, call file-open
                */

                if (fprop->filterprog) {
                    filtercmd=emalloc(strlen(fprop->filterprog)
                                        +strlen(fprop->work_path)
                                        +strlen(fprop->real_path)+6+1);
                    sprintf(filtercmd, "%s \'%s\' \'%s\'", fprop->filterprog,
					fprop->work_path,fprop->real_path);
#ifdef DEBUG
                    fprintf(stderr, "DBG: FilterOpen: %s ::%p:\n",filtercmd,fprop->fp);
#endif
                    fprop->fp = popen (filtercmd,"r");  /* Open stream */

                } else {

                    fprop->fp = fopen(fprop->work_path, "r" );

                }
		

                if (fprop->fp) {

		    /* -- Read  all data  (len = 0 if filtered...) */
		    rd_buffer = read_stream(fprop->fp,(
					fprop->filterprog) ?0 :fprop->fsize); 

		    switch(fprop->doctype) {

		      case TXT:
			if(sw->verbose == 3) printf(" - Using TXT filter - ");
			wordcount = countwords_TXT(sw, fprop, rd_buffer); 
			break;

		      case HTML:
			if(sw->verbose == 3) printf(" - Using HTML filter - ");
			wordcount = countwords_HTML(sw, fprop, rd_buffer); 
			break;

		      case XML:
			if(sw->verbose == 3) printf(" - Using XML filter - ");
			wordcount = countwords_XML(sw, fprop, rd_buffer); 
			break;

		      case MULTITXT:
			if(sw->verbose == 3) printf(" - Using MULTITXT filter - ");
			wordcount = countwords_HTML(sw, fprop, rd_buffer); 
			break;

		      case WML:
			if(sw->verbose == 3) printf(" - Using WML filter - ");
			wordcount = countwords_HTML(sw, fprop, rd_buffer); 
			break;

		      default:
			if(sw->verbose == 3) printf(" - Using DEFAULT filter - ");
			wordcount = countwords_HTML(sw, fprop, rd_buffer); 
			break;
		    }

                    if (fprop->filterprog) {
			pclose(fprop->fp); /* close filter pipe */
			efree(filtercmd);
		    } else {
			fclose (fprop->fp);  /* close file */
		    }

		}
		efree(rd_buffer);

		if (sw->verbose == 3) {
			if (wordcount > 0)
				printf(" (%d words)\n", wordcount);
			else if (wordcount == 0) 
				printf(" (no words)\n");
			else if (wordcount == -1)
				printf(" (not opened)\n");
			else if (wordcount == -2)
				printf(" (title is not ok)\n");
			fflush(stdout);
		}
	}

	return;
}





/* Stores file names in alphabetical order so they can be
** indexed alphabetically. No big whoop.
*/

DOCENTRYARRAY *addsortentry(e, filename)
DOCENTRYARRAY *e;
char *filename;
{
int i,j,k,isbigger;
	
	isbigger=0;
	if (e == NULL) {
		e = (DOCENTRYARRAY *) emalloc(sizeof(DOCENTRYARRAY));
		e->maxsize = SEARCHHASHSIZE;   /* Put what you like */
		e->filenames = (char **) emalloc(e->maxsize*sizeof(char *));
		e->currentsize = 1;
		e->filenames[0] = (char *) estrdup(filename);
	}
	else {
		/* Look for the position to insert using a binary search */
		i=e->currentsize-1;
		j=k=0;
		while(i>=j) {
			k=j+(i-j)/2;
			isbigger = strcmp(filename,e->filenames[k]);
			if(!isbigger) 
				progerr("SWISHE: Congratulations. You have found a bug!! Contact the author");
			else if(isbigger > 0) j=k+1;
			else i=k-1;
		}

		if (isbigger > 0) k++;
		e->currentsize++;
		if(e->currentsize==e->maxsize) {
			e->maxsize +=1000;
			e->filenames=(char **) erealloc(e->filenames,e->maxsize*sizeof(char *)); 
		}
		/* for(i=e->currentsize;i>k;i--) e->filenames[i]=e->filenames[i-1]; */
		/* faster!! */
		memmove(e->filenames+k+1,e->filenames+k,(e->currentsize-1-k)*sizeof(char *));
		e->filenames[k] = (char *)estrdup(filename);
	}
	return e;
}

/* Adds a word to the master index tree.
*/

void addentry(sw, word, filenum, structure, metaID, position)
SWISH *sw;
char *word;
int filenum;
int structure;
int metaID;
int position;
{
int l;
ENTRY *en,*efound;
LOCATION *tp;
int hashval;
IndexFILE *indexf=sw->indexlist;
	if(!sw->entryArray)
	{
		sw->entryArray=(ENTRYARRAY *)emalloc(sizeof(ENTRYARRAY));
		sw->entryArray->numWords=0;
		sw->entryArray->elist=NULL;
	}
		/* Compute hash value of word */	
	hashval=searchhash(word);
		/* Look for the word in the hash array */
	for(efound=sw->hashentries[hashval];efound;efound=efound->nexthash)
		if(strcmp(efound->word,word)==0) break;


	if (!efound) {
		en = (ENTRY *) emalloc(sizeof(ENTRY));
		en->word = (char *) estrdup(word);
		en->tfrequency = 1;
		en->locationarray = (LOCATION **) emalloc(sizeof(LOCATION *));
		tp = (LOCATION *) emalloc(sizeof(LOCATION));
		en->currentlocation = 0;
		en->u1.max_locations=1;
		en->nexthash=sw->hashentries[hashval];
		sw->hashentries[hashval]=en;
		tp->filenum = filenum;
		tp->frequency = 1;
		tp->structure = structure;
		tp->metaID = metaID;
		tp->position[0]=position;

		en->locationarray[0] = tp;

		sw->entryArray->numWords++;
		indexf->header.totalwords++;
	}
	else {
		for(l=efound->currentlocation; l<efound->u1.max_locations; l++) 
		{
			tp = efound->locationarray[l];
			if (tp->filenum == filenum && tp->metaID == metaID) 
				break;
		}
		if (l == efound->u1.max_locations) {  /* filenum and metaname not found */
			efound->locationarray=(LOCATION **) erealloc(efound->locationarray,(++efound->u1.max_locations)*sizeof(LOCATION *));
			tp = (LOCATION *) emalloc(sizeof(LOCATION));
			tp->filenum = filenum;
			tp->frequency = 1;
			tp->structure = structure;
			tp->metaID = metaID;
			tp->position[0]=position;
			efound->locationarray[l]=tp;
			if (efound->locationarray[efound->currentlocation]->filenum != filenum) {
				/* Compress previous location data */
				CompressPrevLocEntry(sw,indexf,efound);
				efound->tfrequency++;
				efound->currentlocation = l;
			}
		}
		else {    /* Found filenum and metaname */
			tp=efound->locationarray[l];
			tp=erealloc(tp,sizeof(LOCATION)+tp->frequency*sizeof(int));
			tp->position[tp->frequency++]=position;
			tp->structure |= structure;
			efound->locationarray[l]=tp;
		}
	}
}

/* Adds a file to the master list of files and file numbers.
*/

void addtofilelist(sw,indexf,filename, mtime, title, summary, start, size,  newFileEntry)
SWISH *sw;
IndexFILE *indexf;
char *filename;
time_t mtime;
char *title;
char *summary;
int start;
int size;
struct file ** newFileEntry;
{
struct file *newnode;
FILEOFFSET *newnode2;
unsigned char *ruleparsedfilename,*p1,*p2,*p3;
unsigned char c;
struct metaEntry *q;
unsigned long int tmp;
	
	if(!indexf->filearray || !indexf->filearray_maxsize) 
	{
		indexf->filearray=(struct file **) emalloc((indexf->filearray_maxsize=BIGHASHSIZE)*sizeof(struct file *));
		indexf->filearray_cursize=0;
	}
	if(indexf->filearray_maxsize==indexf->filearray_cursize)
		indexf->filearray=(struct file **) erealloc(indexf->filearray,(indexf->filearray_maxsize+=1000)*sizeof(struct file *));

	if(!indexf->fileoffsetarray || !indexf->fileoffsetarray_maxsize) 
	{
		indexf->fileoffsetarray=(FILEOFFSET **) emalloc((indexf->fileoffsetarray_maxsize=BIGHASHSIZE)*sizeof(FILEOFFSET *));
		indexf->fileoffsetarray_cursize=0;
	}
	if(indexf->fileoffsetarray_maxsize==indexf->fileoffsetarray_cursize)
		indexf->fileoffsetarray=(FILEOFFSET **) erealloc(indexf->fileoffsetarray,(indexf->fileoffsetarray_maxsize+=1000)*sizeof(FILEOFFSET *));

	newnode = (struct file *) emalloc(sizeof(struct file));
	if (newFileEntry != NULL)
	{
		*newFileEntry = newnode;	/* pass object pointer up to caller */
	}
	ruleparsedfilename = ruleparse(sw, filename);
	/* look for last DIRDELIMITER (FS) and last / (HTTP) */
	p1=strrchr(ruleparsedfilename,DIRDELIMITER);
	p2=strrchr(ruleparsedfilename,'/');
	if(p1 && p2)
	{
		if(p1>=p2) p3=p1;
		else p3=p2;
	} else if(p1 && !p2) p3=p1;
	else if(!p1 && p2) p3=p2;
	else p3=NULL;
	if(!p3) 
	{
		newnode->fi.lookup_path=get_lookup_path(&indexf->pathlookup,"");
	} else {
		c=*++p3;
		*p3='\0';
		newnode->fi.lookup_path=get_lookup_path(&indexf->pathlookup,ruleparsedfilename);
		*p3=c;
		ruleparsedfilename=p3;
	}
	newnode->fi.filename = (char *) estrdup(ruleparsedfilename);
	/* Not used in indexing mode - They are in properties */
	/* NULL must be set to not get a segfault in freefileinfo */
	newnode->fi.title=newnode->fi.summary=NULL;
	/* Init docproperties */
	newnode->docProperties = NULL;

/* #### Added summary,filename, title and mtime as properties if specified */

	if(filename)
	{
                        /* Check if filename is internal swish metadata */
		if((q=getMetaNameData(indexf,AUTOPROPERTY_DOCPATH)))
		{
                        /* Check if it is also a property (META_PROP flag) */
			if(is_meta_property(q))
			{
				addDocProperty(&newnode->docProperties,q->metaID,filename,strlen(filename));
			}
			/* Perhaps we want it to be indexed ... */
			if(is_meta_index(q))
			{
				int metaID[1],positionMeta[1];
				metaID[0]=q->metaID; positionMeta[0]=1;
				indexstring(sw, filename, sw->filenum, IN_FILE, 1, metaID, positionMeta);
			}
		}
	}
	if(title)
	{
                        /* Check if title is internal swish metadata */
		if((q=indexf->titleProp))
		{
                        /* Check if it is also a property (META_PROP flag) */
			if(is_meta_property(q))
			{
				addDocProperty(&newnode->docProperties,q->metaID,title,strlen(title));
			}
			/* Perhaps we want it to be indexed ... */
			if(is_meta_index(q))
			{
				int metaID[1],positionMeta[1];
				metaID[0]=q->metaID; positionMeta[0]=1;
				indexstring(sw, title, sw->filenum, IN_FILE, 1, metaID, positionMeta);
			}
		}
	}
	if(summary)
	{
                        /* Check if summary is internal swish metadata */
		if((q=indexf->summaryProp))
		{
                        /* Check if it is also a property (META_PROP flag) */
			if(is_meta_property(q))
			{
				addDocProperty(&newnode->docProperties,q->metaID,summary,strlen(summary));
			}
			/* Perhaps we want it to be indexed ... */
			if(is_meta_index(q))
			{
				int metaID[1],positionMeta[1];
				metaID[0]=q->metaID; positionMeta[0]=1;
				indexstring(sw, summary, sw->filenum, IN_FILE, 1, metaID, positionMeta);
			}
		}
	}
                        /* Check if filedate is an internal swish metadata */
	if((q=indexf->filedateProp))
	{
                        /* Check if it is also a property (META_PROP flag) */
		if(is_meta_property(q))
		{
			tmp=mtime;
			PACKLONG(tmp);    /* make it portable */
			addDocProperty(&newnode->docProperties,q->metaID,(unsigned char *)&tmp,sizeof(tmp));
		}
	}
                        /* Check if size is internal swish metadata */
	if((q=indexf->sizeProp))
	{
                        /* Check if it is also a property (META_PROP flag) */
		if(is_meta_property(q))
		{
			tmp=size;
			PACKLONG(tmp);    /* make it portable */
			addDocProperty(&newnode->docProperties,q->metaID,(unsigned char *)&tmp,sizeof(tmp));
		}
	}
/* #### */
	indexf->filearray[indexf->filearray_cursize++]=newnode;

	newnode2 = (FILEOFFSET *) emalloc(sizeof(FILEOFFSET));
	indexf->fileoffsetarray[indexf->fileoffsetarray_cursize++]=newnode2;
}

/* Just goes through the master list of files and
** counts 'em.
*/

int getfilecount(indexf)
IndexFILE *indexf;
{
	return indexf->fileoffsetarray_cursize;
}

/* Returns the nicely formatted date.
*/

char *getthedate()
{
char *date;
time_t time;

	date=emalloc(MAXSTRLEN);
	
	time = (time_t) getthetime();
/*	strftime(date, MAXSTRLEN, "%x %X", (struct tm *) localtime(&time));*/
	/* 2/22/00 - switched to 4-digit year (%Y vs. %y) */
	strftime(date, MAXSTRLEN, "%d/%m/%Y %H:%M:%S %Z", (struct tm *) localtime(&time)); 
	
	return date;
}


/* Indexes the words in a string, such as a file name or an
** HTML title.
*/

int countwordstr(sw, s, filenum)
SWISH *sw;
char *s;
int filenum;
{
int position=1;    /* Position of word */
int metaID=1;
int structure=IN_FILE;
	return indexstring(sw, s, filenum, structure, 1, &metaID, &position);
}

/* Parses the words in a comment.
*/

int parsecomment(sw, tag, filenum, structure, metaID, position)
SWISH *sw;
char *tag;
int filenum;
int structure;
int metaID;
int *position;
{
	structure |= IN_COMMENTS;
	return indexstring(sw, tag+1, filenum, structure, 1, &metaID, position);
}

/* Removes words that occur in over _plimit_ percent of the files and
** that occur in over _flimit_ files (marks them as stopwords, that is).
*/
/* 05/00 Jose Ruiz
** Recompute positions when a stopword is removed from lists
** This piece of code is terrorific because the first goal
** was getting the best possible performace. So, the code is not
** very clear.
** The main problem is to recalculate word positions for all
** the words after removing the automatic stop words. This means
** looking at all word's positions for each automatic stop word
** and decrement its position
*/
int removestops(sw, totalfiles)
SWISH *sw;
int totalfiles;
{
int i, j, k, l, m, n, percent, stopwords, stoppos, res;
LOCATION *lp, *lpstop;
ENTRY *e;
ENTRY *ep,*ep2;
ENTRY **estop=NULL;
int estopsz=0, estopmsz=0;
int hashval;
struct swline *sp;
unsigned char *p;
int modified,lpstop_metaID,lpstop_filenum,lpstop_index,totalwords;
IndexFILE *indexf=sw->indexlist;

        /* Now let's count the number of stopwords!!
        */

        for (stopwords=0,hashval = 0; hashval < HASHSIZE; hashval++) {
                sp = indexf->hashstoplist[hashval];
                while (sp != NULL) {
                        stopwords++;
                        sp = sp->next;
                }
        }

	totalwords=indexf->header.totalwords;

	if(!totalwords || sw->plimit>=NO_PLIMIT) return stopwords;

	if(sw->verbose)
		printf("Warning: This proccess can take some time. For a faster one, use IgnoreWords instead of IgnoreLimit\n");

	if(!estopmsz) {
		estopmsz=1;
		estop=(ENTRY **)emalloc(estopmsz*sizeof(ENTRY *));
	}
		/* this is the easy part: Remove the automatic stopwords from
		** the hash array */
        for(i=0;i<SEARCHHASHSIZE;i++)
	{
		for(ep2=NULL,ep=sw->hashentries[i];ep;ep=ep->nexthash)
		{
			percent = (ep->tfrequency * 100 )/ totalfiles;
			if (percent >= sw->plimit && ep->tfrequency >= sw->flimit) {
				addStopList(indexf,ep->word);
					addstophash(indexf,ep->word);
				stopwords++;
					/* Remove entry from  the hash array */
				if(ep2) ep2->nexthash=ep->nexthash;
				else sw->hashentries[i]=ep->nexthash;
				totalwords--;
				sw->entryArray->numWords--;
				indexf->header.totalwords--;
				if(estopsz==estopmsz) {  /* More memory? */
					estopmsz*=2;
					estop=(ENTRY **)erealloc(estop,estopmsz*sizeof(ENTRY *));
				}
					/* estop is an array for storing the
					** automatic stopwords */
				estop[estopsz++]=ep;
			} else ep2=ep;
		}
	}
		/* If we have automatic stopwords we have to recalculate
		** word positions */
	if(estopsz)
	{
				/* Now we need to recalculate all positions
				** of words because we have removed the
				** word in the index array */
				/* Sorry for the code but it is the fastest
				** I could achieve!! */
		for(i=0;i<estopsz;i++) 
		{
			e=estop[i];
			if(sw->verbose) printf("\nRemoving word #%d '%s' (%d occureneces)\n",i,e->word,e->u1.max_locations);
       		 	for(j=0;j<SEARCHHASHSIZE;j++)
			{
				for(ep=sw->hashentries[j];ep;ep=ep->nexthash)
				{
					if(sw->verbose) printf("Computing new positions for %s (%d occurences)                        \r",ep->word,ep->u1.max_locations);
					fflush(stdout);
					for(m=0,modified=0;m<ep->u1.max_locations;m++)
					{
						if(m<ep->currentlocation)
							lp=uncompress_location(sw,indexf,(unsigned char *)ep->locationarray[m]);
						else 
							lp=ep->locationarray[m];
						for(n=0;n<e->u1.max_locations;n++)
						{
							if(n<e->currentlocation)
							{
								p=(unsigned char *)e->locationarray[n];
								uncompress2(lpstop_index,p);
								lpstop_metaID=indexf->locationlookup->all_entries[lpstop_index-1]->val[0];
								uncompress2(lpstop_filenum,p);
							} else {
								lpstop_filenum=e->locationarray[n]->filenum;
								lpstop_metaID=e->locationarray[n]->metaID;
							}
							res=lp->filenum-lpstop_filenum;
							if(res<0) break;
							if(res==0) {
								res=lp->metaID-lpstop_metaID;
								if(res<0) break; 
								if(res==0)
						   		{ 
						      			if(n<e->currentlocation)
										lpstop=uncompress_location(sw,indexf,(unsigned char *)e->locationarray[n]);
						      			else
										lpstop=e->locationarray[n];
						      			for(k=lpstop->frequency;k;) 
						         		for(stoppos=lpstop->position[--k],l=lp->frequency;l;) {if(lp->position[--l]>stoppos) lp->position[l]--; else break;}
									modified=1;
									if(n<e->currentlocation) efree(lpstop);
						    		}
							}
						}
						if(m<ep->currentlocation) 
						{
					   		if(modified)  
					   		{
								efree(ep->locationarray[m]);
								ep->locationarray[m]=(LOCATION *)compress_location(sw,indexf,lp);
							} else efree(lp);
						} 
					}
				}
			}

			if(sw->verbose) printf("\n");
				/* Free Memory used by stopword */
			efree(e->word);
			for (m=0;m<e->u1.max_locations;m++)
			{
				efree(e->locationarray[m]);
			}
		}
	}
	efree(estop);
	return stopwords;
}

/* This is somewhat similar to the rank calculation algorithm
** from WAIS (I think). Any suggestions for improvements?
** Note that ranks can't be smaller than 1, emphasized words
** (words in titles, headers) have ranks multiplied by at least 5
** (just a guess), and ranks divisible by 128 are bumped up by one
** (to make the compression scheme with '\0' as a line delimiter
** work). Fudging with the ranks doesn't seem to make much difference.
*/

int getrank(sw, freq, tfreq, words, structure)
SWISH *sw;
int freq;
int tfreq;
int words;
int structure;
{
double d, e, f;
int tmprank;
int emphasized;
	if ((EMPHASIZECOMMENTS && (structure & IN_COMMENTS)) || (structure & IN_HEADER) || (structure & IN_TITLE)) emphasized=5;
	else emphasized=0;

	if (freq < 5)
		freq = 5;
	d = 1.0 / (double) tfreq;
	e = (log((double) freq) + 10.0) * d;
	if (!sw->mergedheader.ignoreTotalWordCountWhenRanking)
	{
		e /= words;
	}
	else
	{
		/* scale the rank down a bit. a larger has the effect of
		   making small differences in work frequency wash out */ 
		e /= 100;
	}
	f = e * 10000.0;
	
	tmprank = (int) f;
	if (tmprank <= 0)
		tmprank = 1;
	if (emphasized)
		tmprank *= emphasized;
	
	return tmprank;
}

/* Prints the index information at the head of index files.
** 06/00 - If fp==stdout ptints the header to screen
*/


#define PrintHeaderStr(ID,TXT,fp) id=ID;compress1(id,fp);len=strlen(TXT)+1; compress1(len,fp);fwrite(TXT,len,1,fp);
#define PrintHeaderInt(ID,INT,fp) id=ID;compress1(id,fp);itmp=INT+1; compress1(itmp,fp);

void printheader(header, fp, filename, totalwords, totalfiles, merged)
INDEXDATAHEADER *header;
FILE *fp;
char *filename;
int totalwords;
int totalfiles;
int merged;
{
char *c,*tmp;
int id,len;
long itmp;
	
	c = (char *) strrchr(filename, '/');
	if (c == NULL && c + 1 != '\0') 
		c = filename;
	else
		c+=1;
	if (fp!=stdout) {
		itmp=SWISH_MAGIC; printlong(fp,itmp);
		PrintHeaderStr(INDEXHEADER_ID,INDEXHEADER,fp);
		PrintHeaderStr(INDEXVERSION_ID,INDEXVERSION,fp);
		PrintHeaderInt(MERGED_ID,merged,fp);
		PrintHeaderStr(NAMEHEADER_ID,header->indexn,fp);
		PrintHeaderStr(SAVEDASHEADER_ID,c,fp);
		PrintHeaderInt(COUNTSHEADER_ID,totalwords,fp);
		itmp=totalfiles+1; compress1(itmp,fp);
		tmp=getthedate();PrintHeaderStr(INDEXEDONHEADER_ID,tmp,fp); efree(tmp);
		PrintHeaderStr(DESCRIPTIONHEADER_ID,header->indexd,fp);
		PrintHeaderStr(POINTERHEADER_ID,header->indexp,fp);
		PrintHeaderStr(MAINTAINEDBYHEADER_ID,header->indexa,fp);
		PrintHeaderInt(DOCPROPENHEADER_ID,1,fp);
		PrintHeaderInt(STEMMINGHEADER_ID,header->applyStemmingRules,fp);
		PrintHeaderInt(SOUNDEXHEADER_ID,header->applySoundexRules,fp);
		PrintHeaderInt(IGNORETOTALWORDCOUNTWHENRANKING_ID,header->ignoreTotalWordCountWhenRanking,fp);
		PrintHeaderStr(WORDCHARSHEADER_ID,header->wordchars,fp);
		PrintHeaderInt(MINWORDLIMHEADER_ID,header->minwordlimit,fp);
		PrintHeaderInt(MAXWORDLIMHEADER_ID,header->maxwordlimit,fp);
		PrintHeaderStr(BEGINCHARSHEADER_ID,header->beginchars,fp);
		PrintHeaderStr(ENDCHARSHEADER_ID,header->endchars,fp);
		PrintHeaderStr(IGNOREFIRSTCHARHEADER_ID,header->ignorefirstchar,fp);
		PrintHeaderStr(IGNORELASTCHARHEADER_ID,header->ignorelastchar,fp);
		PrintHeaderStr(IGNORELASTCHARHEADER_ID,header->ignorelastchar,fp);
		PrintHeaderInt(FILEINFOCOMPRESSION_ID,header->applyFileInfoCompression,fp);
		/* Jose Ruiz 06/00 Added this line to delimite the header */
		fputc(0,fp);    
	} else {
		fprintf(fp, "%s\n", INDEXVERSION);
		fprintf(fp, "# %s\n", (merged) ? "MERGED INDEX" : "");
		fprintf(fp, "%s %s\n", NAMEHEADER, (header->indexn[0] == '\0') ? "(no name)" : header->indexn);
		fprintf(fp, "%s %s\n", SAVEDASHEADER,c);
		fprintf(fp, "%s %d words, %d files\n", COUNTSHEADER,totalwords,totalfiles);
		fprintf(fp, "%s %s\n", INDEXEDONHEADER,header->indexedon);
		fprintf(fp, "%s %s\n", DESCRIPTIONHEADER,(header->indexd[0] == '\0') ? "(no description)" : header->indexd);
		fprintf(fp, "%s %s\n", POINTERHEADER,(header->indexp[0] == '\0') ? "(no pointer)" : header->indexp);
		fprintf(fp, "%s %s\n",MAINTAINEDBYHEADER, (header->indexa[0] == '\0') ? "(no maintainer)" : header->indexa);
		fprintf(fp, "%s %s\n", DOCPROPENHEADER, "Enabled");
		fprintf(fp, "%s %d\n", STEMMINGHEADER, header->applyStemmingRules);
		fprintf(fp, "%s %d\n", SOUNDEXHEADER, header->applySoundexRules);
		fprintf(fp, "%s %d\n", IGNORETOTALWORDCOUNTWHENRANKING, header->ignoreTotalWordCountWhenRanking);
		fprintf(fp, "%s %s\n", WORDCHARSHEADER, header->wordchars);
		fprintf(fp, "%s %d\n", MINWORDLIMHEADER, header->minwordlimit);
		fprintf(fp, "%s %d\n", MAXWORDLIMHEADER, header->maxwordlimit);
		fprintf(fp, "%s %s\n", BEGINCHARSHEADER, header->beginchars);
		fprintf(fp, "%s %s\n", ENDCHARSHEADER, header->endchars);
		fprintf(fp, "%s %s\n", IGNOREFIRSTCHARHEADER, header->ignorefirstchar);
		fprintf(fp, "%s %s\n", IGNORELASTCHARHEADER, header->ignorelastchar);
		fprintf(fp, "%s %d\n", FILEINFOCOMPRESSION, header->applyFileInfoCompression);

	}
}


/* Sort entry by MetaName, FileNum */
void sortentry(sw,indexf,e)
SWISH *sw;
IndexFILE *indexf;
ENTRY *e;
{
int i, j, k, num;
unsigned char *ptmp,*ptmp2,*compressed_data;
int *pi=NULL;
                      /* Very trivial case */
                if(!e) return;
	
                if (!(i=e->u1.max_locations)) {
			return;
		}
                        /* Compute array wide */
                j=2*sizeof(int)+sizeof(void *);
                        /* Compute array size */
                ptmp=(void *)emalloc(j*i);
                        /* Build an array with the elements to compare
                                 and pointers to data */
                for(k=0,ptmp2=ptmp;k<i;k++) {
                        pi=(int *)ptmp2;
			if (sw->swap_flag)
				e->locationarray[k]=(LOCATION *)unSwapLocData(sw,(long)e->locationarray[k]);
			compressed_data=(char *)e->locationarray[k];
			uncompress2(num,compressed_data); /* index to lookuptable */
                        pi[0] = indexf->locationlookup->all_entries[num-1]->val[0];
			uncompress2(num,compressed_data); /* filenum */
                        pi[1] = num;
                        ptmp2+=2*sizeof(int);
			memcpy((char *)ptmp2,(char *)&e->locationarray[k],sizeof(LOCATION *));
                        ptmp2+=sizeof(void *);
		}
                        /* Sort them */
                qsort(ptmp,i,j,&icomp2);
			/* Store results */
                for(k=0,ptmp2=ptmp;k<i;k++){
                        ptmp2+=2*sizeof(int);
                        memcpy((char *)&e->locationarray[k],(char*)ptmp2,sizeof(LOCATION *));
                        ptmp2+=sizeof(void *);
                }
                        /* Free the memory of the array */
                efree(ptmp);
}


/* Print the index entries that hold the word, rank, and other information.
*/

void printindex(sw,indexf)
SWISH *sw;
IndexFILE *indexf;
{
int i;
ENTRYARRAY *ep;
ENTRY *epi;
int totalwords;
	BuildSortedArrayOfWords(sw,indexf);
	ep=sw->entryArray;
	if(ep)
	{
		totalwords=ep->numWords;
		for(i=0; i<totalwords; i++) 
		{
			epi=ep->elist[i];
			if (!isstopword(indexf,epi->word)) 
			{
		                /* Compress remaining data */
        			CompressCurrentLocEntry(sw,indexf,epi);
			}
		}
		for(i=0; i<totalwords; i++) 
		{
			epi=ep->elist[i];
			if (!isstopword(indexf,epi->word)) 
			{
				/* Sort locationlist by MetaName, Filenum
				** for faster search */
				sortentry(sw,indexf,epi);
				/* Write entry to file */
				printword(sw,epi,indexf);
			} else epi->u1.fileoffset=-1L;
		}
		fputc(0,indexf->fp);   /* End of words mark */
		printhash(sw->hashentries, indexf);
		for(i=0; i<totalwords; i++) 
		{
			epi=ep->elist[i];
			if (epi->u1.fileoffset>=0L) 
			{
				printworddata(sw,epi,indexf);
			}
			efree(epi->word);
			efree(epi);   
		}
	}
}

/* Jose Ruiz 11/00
** Function to write a word to the index file
*/
void printword(sw, ep,indexf)
SWISH *sw;
ENTRY *ep;
IndexFILE *indexf;
{
int i,wordlen,hashval;
long f_offset;
FILE *fp=indexf->fp;

	f_offset = ftell(fp);
	i=(int)((unsigned char)ep->word[0]);
	/* if(isindexchar(indexf->header,i) && !indexf->offsets[i]) indexchars stuff removed */
	if(!indexf->offsets[i])
		indexf->offsets[i] = f_offset;
		
		/* Get HashOffset for direct access */
	hashval=searchhash(ep->word);
	if (sw->hashentries[hashval] == ep) 
		indexf->hashoffsets[hashval] = f_offset;
	
		/* Write word length, word and a NULL offset */
	wordlen=strlen(ep->word);
	compress1(wordlen,fp);
	fwrite(ep->word, wordlen, 1, fp);

	printlong(fp,(long)0);     /* hash offset */
	printlong(fp,(long)0);     /* word's data pointer */
		/* Store word offset for futher hash computing */
	ep->u1.fileoffset = f_offset;
}

/* Jose Ruiz 11/00
** Function to write all word's data to the index file
*/
void printworddata(sw, ep,indexf)
SWISH *sw;
ENTRY *ep;
IndexFILE *indexf;
{
int i,index,wordlen,curmetaID;
long tmp,curmetanamepos,f_offset;
int metaID,filenum,frequency,position;
int index_structfreq;
unsigned char *compressed_data,*p;
FILE *fp=indexf->fp;

	curmetaID=0;
	curmetanamepos=0L;
	fseek(fp,0,SEEK_END);
	f_offset = ftell(fp);

        fseek(fp,ep->u1.fileoffset,SEEK_SET);
        uncompress1(wordlen,fp);   /* Get Word length */
        fseek(fp,(long)wordlen,SEEK_CUR);  /* Jump Word */
        fseek(fp,(long)sizeof(long),SEEK_CUR);  /* Jump Hash pointer */
	printlong(fp,f_offset);
	fseek(fp,0,SEEK_END);   /* Come back to the end */

		/* Write tfrequency */
	compress1(ep->tfrequency,fp);
		/* Write location list */
	for(i=0;i<ep->u1.max_locations;i++) 
	{
		p = compressed_data = (unsigned char *)ep->locationarray[i];
		uncompress2(index,p);
		metaID=indexf->locationlookup->all_entries[index-1]->val[0];
		if(curmetaID!=metaID) {
			if(curmetaID) {
				/* Write in previous meta (curmetaID)
				** file offset to next meta */
				tmp=ftell(fp);
				fseek(fp,curmetanamepos,0);
				printlong(fp,tmp);
				fseek(fp,tmp,0);
			}
			curmetaID=metaID;
			compress1(curmetaID,fp);
			curmetanamepos=ftell(fp);
			printlong(fp,(long)0);
		}
		/* Write filenum, structure and position information to index file */
		uncompress2(filenum,p);
		index_structfreq=indexf->locationlookup->all_entries[index-1]->val[1];
		frequency=indexf->structfreqlookup->all_entries[index_structfreq-1]->val[0];
		compress1(filenum,fp);
		compress1(index_structfreq,fp);
		for(;frequency;frequency--)
		{
			uncompress2(position,p);
			compress1(position,fp);
		}
		efree(compressed_data);
	}
	/* Write in previous meta (curmetaID)
	** file offset to end of metas */
	tmp=ftell(fp);
	fseek(fp,curmetanamepos,0);
	printlong(fp,tmp);
	fseek(fp,tmp,0);
		/* A NULL byte to indicate end of word data */
	fputc(0,fp);
}

/* Prints the list of stopwords into the index file.
*/

void printstopwords(indexf)
IndexFILE *indexf;
{
int hashval,len;
struct swline *sp=NULL;
FILE *fp=indexf->fp;
	
	indexf->offsets[STOPWORDPOS] = ftell(fp);
	for (hashval = 0; hashval < HASHSIZE; hashval++) {
		sp = indexf->hashstoplist[hashval];
		while (sp != NULL) {
			len=strlen(sp->line);
			compress1(len,fp);
			fwrite(sp->line, len, 1, fp);
			sp = sp->next;
		}
	}
	fputc(0,fp);
}

unsigned char *buildFileEntry(filename, fp, docProperties,lookup_path,sz_buffer)
char *filename;
FILE *fp;
struct docPropertyEntry **docProperties;
int *sz_buffer;
int lookup_path;
{
int len_filename;
unsigned char *buffer1,*buffer2,*buffer3,*p;
int lenbuffer1;
int datalen1, datalen2,datalen3;
	len_filename = strlen(filename)+1;
	lenbuffer1=len_filename+2*6;
	buffer1=emalloc(lenbuffer1);
	p=buffer1;
	lookup_path++;   /* To avoid the 0 problem in compress increase 1 */
	compress3(lookup_path,p);
		/* We store length +1 to avoid problems with 0 length - So 
		it also writes the null terminator */
	compress3(len_filename,p);
	memcpy(p,filename,len_filename);p+=len_filename;
	datalen1=p-buffer1;
	buffer2=storeDocProperties(*docProperties, &datalen2);
	buffer3=emalloc((datalen3=datalen1+datalen2+1));
	memcpy(buffer3,buffer1,datalen1);
	if(datalen2)memcpy(buffer3+datalen1,buffer2,datalen2);
	buffer3[datalen1+datalen2]='\0';
	efree(buffer1);
	efree(buffer2);
	*sz_buffer=datalen3;
	return(buffer3);
}

struct file *readFileEntry(indexf, filenum)
IndexFILE *indexf;
int filenum;
{
long pos;
int total_len,len1,len4,lookup_path;
char *buffer,*p;
char *buf1;
struct file *fi;
FILEOFFSET *fo;
FILE *fp=indexf->fp;
	fi=indexf->filearray[filenum-1];
	fo=indexf->fileoffsetarray[filenum-1];
	if(fi->read) return fi;   /* Read it previously */
	pos=fo->filelong;

	fseek(fp, pos, 0);

	if(indexf->header.applyFileInfoCompression)
	{
		buffer=p=zfread(indexf->dict,&total_len,fp);
	}
	else
	{
		uncompress1(total_len,fp);
		buffer=p=emalloc(total_len);
		fread(buffer,total_len,1,fp);
	}

	uncompress2(lookup_path,p);  /* Index to lookup table of paths */
	lookup_path--;
	uncompress2(len1,p);   /* Read length of filename */
	buf1 = emalloc(len1);  /* Includes NULL terminator */
	memcpy(buf1,p,len1);   /* Read filename */
	p+=len1;

	fi->fi.lookup_path = lookup_path;
		/* Add the path to filename */
	len4=strlen(indexf->pathlookup->all_entries[lookup_path]->val);
	len1=strlen(buf1);
	fi->fi.filename = emalloc(len4+len1+1);
	memcpy(fi->fi.filename,indexf->pathlookup->all_entries[lookup_path]->val,len4);
	memcpy(fi->fi.filename+len4,buf1,len1);
	fi->fi.filename[len1+len4]='\0';
	efree(buf1);

	/* read the document properties section  */
	fi->docProperties = fetchDocProperties( p);

	/* Read internal swish properties */
	/* first init them */
	fi->fi.mtime= (unsigned long)0L;
	fi->fi.title = NULL;
	fi->fi.summary = NULL;
	fi->fi.start = 0;
	fi->fi.size = 0;

	/* Read values */
	getSwishInternalProperties(fi, indexf);

	/* Add empty strings if NULL */
	if(!fi->fi.title) fi->fi.title=estrdup("");
	if(!fi->fi.summary) fi->fi.summary=estrdup("");
	fi->read=1;


	efree(buffer);
	return fi;
}

/* Prints the list of files, titles, and sizes into the index file.
*/

void printfilelist(sw, indexf)
SWISH *sw;
IndexFILE *indexf;
{
int i;
struct file *filep;
FILEOFFSET *fileo;
FILE *fp=indexf->fp;
unsigned char *buffer;
int sz_buffer;
struct buffer_pool *bp=NULL;
	indexf->offsets[FILELISTPOS] = ftell(fp);
	for(i=0;i<indexf->filearray_cursize;i++)
	{
		if(sw->swap_flag)
			filep=unSwapFileData(sw);
		else
			filep=indexf->filearray[i];
		fileo=indexf->fileoffsetarray[i];
		buffer=buildFileEntry(filep->fi.filename, fp, &filep->docProperties,filep->fi.lookup_path,&sz_buffer);
		if(indexf->header.applyFileInfoCompression)
		{
			bp=zfwrite(bp,buffer,sz_buffer,&fileo->filelong,fp);
		}
		else
		{
			fileo->filelong=ftell(fp);
			compress1(sz_buffer,fp);   /* Write length */
			fwrite(buffer,sz_buffer,1,fp);  /* Write data */
		}

		efree(buffer);
		freefileinfo(filep);
		indexf->filearray[i]=NULL;
	}
	if(indexf->header.applyFileInfoCompression)
	{
		zfflush(bp,fp);
		printdeflatedictionary(bp,indexf);
	}
}

/* Prints the list of metaNames into the file index
*/

void printMetaNames(indexf)
IndexFILE *indexf;
{
struct metaEntry* entry=NULL;
int i,len;
FILE *fp=indexf->fp;
	
/* #### Use new metaType schema - see metanames.h */
	/* Format of metaname is
        <len><metaName><metaType>
	len and metaType are compressed numbers
	metaName is the ascii name of the metaname
	
	The list of metanames is delimited by a 0
	*/
	indexf->offsets[METANAMEPOS] = ftell(fp);
	for (i=0;i<indexf->metaCounter;i++)
	{
		entry = indexf->metaEntryArray[i];
		len = strlen(entry->metaName);
		compress1(len,fp);
		fwrite(entry->metaName,len,1,fp);
		compress1(entry->metaType,fp);
	}
	/* End of metanames */
	fputc(0,fp);   /* write 0 delimiter */
/* #### */
}


/* Prints the list of file offsets into the index file.
 */

void printfileoffsets(indexf)
IndexFILE *indexf;
{
int i;
long offset, totwords;
FILE *fp=indexf->fp;
	
	indexf->offsets[FILEOFFSETPOS] = ftell(fp);
	for (i = 0; i<indexf->filearray_cursize;i++ ){
		offset=(long)indexf->fileoffsetarray[i]->filelong;
		totwords=(long)indexf->fileoffsetarray[i]->ftotalwords;
		printlong(fp, offset);
		printlong(fp, totwords);
	}
	printlong(fp, (long)0);
}

/* Print the info lookuptable of structures and frequency */
/* These lookuptables make the file index small and decreases I/O op */
void printlocationlookuptables(IndexFILE *indexf)
{
int i,n,tmp;
FILE *fp=indexf->fp;
	
	indexf->offsets[LOCATIONLOOKUPTABLEPOS] = ftell(fp);
	
	/* Let us begin with structure lookuptable */
	if(!indexf->structurelookup)
	{
		fputc(0,fp);
		fputc(0,fp);
		return;
	}
	n=indexf->structurelookup->n_entries;
	compress1(n,fp);
	for(i=0;i<n;i++)
	{
		tmp=indexf->structurelookup->all_entries[i]->val[0]+1;
		compress1(tmp,fp);
	}
	/* Let us continue with structure_lookup,frequency lookuptable */
	n=indexf->structfreqlookup->n_entries;
	compress1(n,fp);
	for(i=0;i<n;i++)
	{
		/* frequency */
		tmp=indexf->structfreqlookup->all_entries[i]->val[0]+1;
		compress1(tmp,fp);
		/* structure lookup value */
		tmp=indexf->structfreqlookup->all_entries[i]->val[1]+1;
		compress1(tmp,fp);
	}
	fputc(0,fp);
}

/* Print the info lookuptable of paths/urls */
/* This lookuptable make the file index small and decreases I/O op */
void printpathlookuptable(IndexFILE *indexf)
{
int n,i,len;
char *tmp;
FILE *fp=indexf->fp;
	
	indexf->offsets[PATHLOOKUPTABLEPOS] = ftell(fp);
	if(!indexf->pathlookup)
	{
		fclose(fp);  /* Close file */
		remove(indexf->line); /* Remove file: It is useless */
		progerr("No valid documents have been found. Check your files, directories and/or urls. Index file removed");
	}
	n=indexf->pathlookup->n_entries;
	compress1(n,fp);
	for(i=0;i<n;i++)
	{
		tmp=indexf->pathlookup->all_entries[i]->val;
		len=strlen(tmp)+1;
		compress1(len,fp);
		fwrite(tmp,len,1,fp);
	}
	fputc(0,fp);
}


/* Prints out the decompressed values in an index file.*/

void decompress(sw,indexf)
SWISH *sw;
IndexFILE *indexf;
{
int i, c, x, wordlen, fieldnum, frequency, metaname, index_structure, structure,  index_structfreq, filenum;
long pos;
long num;
long worddata,nextword;
long nextposmetaname;
FILE *fp=indexf->fp;
struct file *fi=NULL;
struct docPropertyEntry *docProperties=NULL;
char ISOTime[20];

	metaname=0;
	nextposmetaname=0L;
	
	frequency=0;


	fseek(fp, 0, 0);
	readheader(indexf);
	readoffsets(indexf);
	readMetaNames(indexf);

	readlocationlookuptables(indexf);

	if(indexf->header.applyFileInfoCompression)
		readdeflatepatterns(indexf);

	readpathlookuptable(indexf);
	readfileoffsets(indexf);

	fseek(fp, 0, 0);

	readheader(indexf);
	printheader(&indexf->header, stdout,indexf->line , indexf->header.totalwords , indexf->header.totalfiles, 0);

	fieldnum = 0;
	
	printf("\n----> OFFSETS INFO. Hexadecimal Numbers <----\n");
	for(i=0;i<MAXCHARS;i++) {
		num=readlong(fp);
		printf("%04lx ", num);
	}

	printf("\n----> HASH OFFSETS INFO. Hexadecimal Numbers <----\n");
	for(i=0;i<SEARCHHASHSIZE;i++) {
		num=readlong(fp);
		printf("%04lx ", num);
	}

	printf("\n-----> WORD INFO <-----\n");
			/* Decode word Info */	
	uncompress1(wordlen,fp);
	if(!wordlen)
		printf("WARNING!! NO unique index words in index file!!\n");
	while (wordlen) 
	{
		for(i=0; i<wordlen; i++) putchar(fgetc(fp));
		putchar((int)':');
			/* Jump offset hash link */
		readlong(fp);
			/* Read offset to word data */
		worddata=readlong(fp);
			/* Get offset to next word */
		nextword=ftell(fp);  
			/* Read data from word */
		fseek(fp,worddata,SEEK_SET);
		uncompress1(x,fp);  /* tfrequency */
		uncompress1(x,fp);  /* metaname */
		if((metaname=x)) {
			nextposmetaname=readlong(fp);
			uncompress1(x,fp);  /* First file */
		}
		while(x)
		{
			filenum=x;
                        uncompress1(index_structfreq,fp);
                        frequency=indexf->structfreqlookup->all_entries[index_structfreq-1]->val[0];
                        index_structure=indexf->structfreqlookup->all_entries[index_structfreq-1]->val[1];
                        structure=indexf->structurelookup->all_entries[index_structure-1]->val[0];

			if (sw->verbose == 4)
			{
				struct file *fileInfo;
				printf(" Meta:%d", metaname);
				pos = ftell(fp);
				fileInfo = readFileEntry(indexf, filenum);
				printf(" %s", fileInfo->fi.filename);
				fseek(fp, pos, 0);
				printf(" Strct:%d", structure);
				printf(" Freq:%d", frequency);
				printf(" Pos:");
			}
			else {
				printf(" %d",metaname);
				printf(" %d",filenum);
				printf(" %d",structure);
				printf(" %d",frequency);
			}
			for(i=0;i<frequency;i++)
			{
				uncompress1(x,fp);	
				if (sw->verbose == 4)
				{
					if(i) printf(",%d", x);
					else printf("%d",x);
				} else
					printf(" %d",x);
			}
			if(ftell(fp) == nextposmetaname) {
				uncompress1(x,fp);
				if((metaname=x)) {
					nextposmetaname=readlong(fp);
					uncompress1(x,fp);
				} else nextposmetaname=0L;
			} else uncompress1(x,fp);
		}
		putchar((int)'\n');
			/* Point to next word */
		fseek(fp,nextword,SEEK_SET);
		uncompress1(wordlen,fp);
	}

		/* Decode Stop Words: All them are in just one line */
	printf("\n\n-----> STOP WORDS <-----\n");
	fseek(fp,indexf->offsets[STOPWORDPOS],SEEK_SET);
	uncompress1(wordlen,fp);
	while(wordlen) {
		for(i=0;i<wordlen;i++)putchar(fgetc(fp));
		putchar((int)' ');
		uncompress1(wordlen,fp);
	}
	putchar((int)'\n');

		/* Decode File Info */
	printf("\n\n-----> FILES <-----\n");
	fflush(stdout);
	for (i=0; i<indexf->filearray_cursize; i++)
	{
		fi=readFileEntry(indexf,i+1);
		
  		strftime(ISOTime,sizeof(ISOTime),"%Y/%m/%d %H:%M:%S",(struct tm *)localtime((time_t *)&fi->fi.mtime));

		fflush(stdout);
		if(fi->fi.summary)
		{
			printf("%s \"%s\" \"%s\" \"%s\" %d %d",fi->fi.filename,ISOTime, fi->fi.title,fi->fi.summary,fi->fi.start,fi->fi.size);fflush(stdout);  /* filename */
		} else {
			printf("%s \"%s\" \"%s\" \"\" %d %d",fi->fi.filename,ISOTime, fi->fi.title,fi->fi.start,fi->fi.size);fflush(stdout);  /* filename */
		}
		for(docProperties=fi->docProperties;docProperties;docProperties=docProperties->next) {
			printf(" PROP_%d: \"%s\"",docProperties->metaID, getPropAsString(indexf,docProperties)); 
		}
		putchar((int)'\n');fflush(stdout);
		freefileinfo(fi);
	}
	printf("\nNumber of File Entries: %d\n",indexf->filearray_cursize);
	fflush(stdout);
		/* Jump File Offsets */
	for(c = fgetc(fp); c!=EOF && ftell(fp)!=indexf->offsets[METANAMEPOS];c = fgetc(fp));
	if(c!=EOF) {
		/* Meta Names */
		printf("\n\n-----> METANAMES <-----\n");
		uncompress1(wordlen,fp);
		while(wordlen) {
			for(i=0;i<wordlen;i++)putchar(fgetc(fp));
			putchar((int)'\"');
			uncompress1(i,fp);
/* #### Line modified */
			printf("%d",i);
/* #### */
			putchar((int)' ');
			uncompress1(wordlen,fp);
		}
		putchar((int)'\n');
	}

	if (sw->verbose != 4)
	printf("\nUse -v 4 for a more complete info\n");
}

/* Parses lines according to the ReplaceRules directives.
*/

char *ruleparse(sw, line)
SWISH *sw;
char *line;
{
static int lenrule=0;
static char *rule=NULL;
static int lentmpline=0;
static char *tmpline=NULL;
static int lennewtmpline=0;
static char *newtmpline=NULL;
static int lenline1=0;
static char *line1=NULL;
static int lenline2=0;
static char *line2=NULL;
struct swline *tmplist=NULL;
int ilen1,ilen2;
	
	if(!lenrule) rule=(char *)emalloc((lenrule=MAXSTRLEN)+1);
	if(!lentmpline) tmpline=(char *)emalloc((lentmpline=MAXSTRLEN)+1);
	if(!lennewtmpline)newtmpline=(char *)emalloc((lennewtmpline=MAXSTRLEN)+1);
	if(!lenline1) line1=(char *)emalloc((lenline1=MAXSTRLEN)+1);
	if(!lenline2) line2=(char *)emalloc((lenline2=MAXSTRLEN)+1);

	if (sw->replacelist == NULL)
		return line;
	
	tmplist = sw->replacelist;
	tmpline = SafeStrCopy(tmpline, line,&lentmpline);
	while (1) 
	{
		if (tmplist == NULL)
			return tmpline;
		rule =SafeStrCopy(rule, tmplist->line,&lenrule);
		tmplist = tmplist->next;
		if (tmplist == NULL)
			return tmpline;
		if (rule == NULL) {
			sw->replacelist = tmplist;
			return tmpline;
		}
		else {
			if (lstrstr(rule, "replace")) {
				line1 = SafeStrCopy(line1, tmplist->line,&lenline1);
				tmplist = tmplist->next;
				if (tmplist)
				{
					line2 = SafeStrCopy(line2, tmplist->line,&lenline2);
					tmplist = tmplist->next;
				}
				else
				{
					/* Handle case where 2nd part of replace rule
					** is an empty string. Config-file parsing
					** idiosyncrasies cause a replace of "x" to ""
					** to incompletely represent the rule.
					*/
					line2[0] = '\0';
				}
				newtmpline=SafeStrCopy(newtmpline, (char *) matchAndChange(tmpline, line1, line2),&lennewtmpline);
			}
			else if (lstrstr(rule, "append")) {
				ilen1=strlen(tmpline);
				ilen2=strlen(tmplist->line);
				if((ilen1+ilen2)>=lennewtmpline) {
					lennewtmpline=ilen1+ilen2+200;
					newtmpline=erealloc(newtmpline,lennewtmpline+1);
				}
				memcpy(newtmpline,tmpline,ilen1);
				memcpy(newtmpline+ilen1,tmplist->line,ilen2);
				newtmpline[ilen1+ilen2]='\0';
				tmplist = tmplist->next;
			}
			else if (lstrstr(rule, "prepend")) {
				ilen1=strlen(tmpline);
				ilen2=strlen(tmplist->line);
				if((ilen1+ilen2)>=lennewtmpline) {
					lennewtmpline=ilen1+ilen2+200;
					newtmpline=erealloc(newtmpline,lennewtmpline+1);
				}
				memcpy(newtmpline,tmplist->line,ilen2);
				memcpy(newtmpline+ilen2,tmpline,ilen1);
				newtmpline[ilen1+ilen2]='\0';
				tmplist = tmplist->next;
			}
			else if (lstrstr(rule,"remove")) {
				newtmpline = SafeStrCopy(newtmpline, (char *)matchAndChange(tmpline,tmplist->line,""),&lennewtmpline);
			}
			tmpline=SafeStrCopy(tmpline, newtmpline,&lentmpline);
		}
	}
}


/*  These 2 routines fix the problem when a word ends with mutiple
**  IGNORELASTCHAR's (eg, qwerty'. ).  The old code correctly deleted
**  the ".", but didn't check if the new last character ("'") is also
**  an ignore character.
*/

int stripIgnoreLastChars(INDEXDATAHEADER header, char *word)
{
int bump_pocistion_counter_flag=0;
int i=strlen(word);
	
	/* Get rid of specified last char's */
	/* for (i=0; word[i] != '\0'; i++); */
	/* Iteratively strip off the last character if it's an ignore character */
	while ( (i>0) && (isIgnoreLastChar(header, word[--i])) )
	{
		if (isBumpPositionCounterChar(header, word[i]))
			bump_pocistion_counter_flag++;
		word[i] = '\0';
	}
	return bump_pocistion_counter_flag;
}

void stripIgnoreFirstChars(INDEXDATAHEADER header, char *word)
{
	int j, k;
	int i = 0;
	
	/* Keep going until a char not to ignore is found */
	while ( word[i] && isIgnoreFirstChar(header,word[i]) )
		i++;
	
	/* If all the char's are valid, just return */
	if (0 == i)
		return;
	else
    {
		for ( k=i, j=0; word[k] != '\0'; j++,k++)
		{
			word[j] = word[k];
		}
		/* Add the NULL */
		word[j] = '\0';
    }
}



int entrystructcmp(const void *e1, const void *e2)
{
const ENTRY *ep1=*(ENTRY * const *)e1;
const ENTRY *ep2=*(ENTRY * const *)e2;
	return(strcmp(ep1->word,ep2->word));
}


/* Builds a sorted array with all of the words */
void BuildSortedArrayOfWords(SWISH *sw,IndexFILE *indexf)
{
int i,j;
ENTRY *e;
	if(sw->verbose) {
		printf("Sorting Words alphabetically\n");fflush(stdout);
	}
	if(!sw->entryArray || !sw->entryArray->numWords) return;

		/* Build the array with the pointers to the entries */
	sw->entryArray->elist=(ENTRY **)emalloc(sw->entryArray->numWords*sizeof(ENTRY *));
		/* Fill the array with all the entries */
	for(i=0,j=0;i<SEARCHHASHSIZE;i++)
		for(e=sw->hashentries[i];e;e=e->nexthash)
			sw->entryArray->elist[j++]=e;
		/* Sort them */
	qsort(sw->entryArray->elist,sw->entryArray->numWords,sizeof(ENTRY *),&entrystructcmp);
}


/* 
** Jose Ruiz 04/00
** Store a portable long with just four bytes
*/
void printlong(fp, num)
FILE *fp;
long num;
{
	PACKLONG(num);     /* Make the number portable */
	fwrite(&num,MAXLONGLEN,1,fp);
}

/* 
** Jose Ruiz 04/00
** Read a portable long (just four bytes)
*/
long readlong(fp)
FILE *fp;
{
long num;
	fread(&num,MAXLONGLEN,1,fp);
	UNPACKLONG(num);     /* Make the number readable */
	return(num);
}

/* Jose Ruiz 04/00 */
/* Function to print to the index file the hash table with all the words */
void printhash(hashentries, indexf)
ENTRY **hashentries;
IndexFILE *indexf;
{
int i, wordlen;
ENTRY *ep, *epn;
FILE *fp=indexf->fp;
	for(i=0; i<SEARCHHASHSIZE; i++) {
		if((ep = hashentries[i])) {
			while(ep) 
			{
				epn=ep->nexthash;
				if(ep->u1.fileoffset>=0L)
				{
					fseek(fp,ep->u1.fileoffset,SEEK_SET);
					uncompress1(wordlen,fp);
					fseek(fp,(long)wordlen,SEEK_CUR);
					/* Get next non stopword */
					while(epn && epn->u1.fileoffset<0L) epn=epn->nexthash;
					if(epn)
						printlong(fp,epn->u1.fileoffset);
					else 
						printlong(fp,(long)0);
				}
				ep = epn;
			}
		}
	}
}

void TranslateChars(INDEXDATAHEADER header, char *s)
{
char *p,*q;
	if(!header.translatechars1 || !header.translatechars1[0]) return;
        for(p=s;p;){
                p=strpbrk(p,header.translatechars1);
                if(p) {
                        q=strchr(header.translatechars1,p[0]);
                        *p++=header.translatechars2[q-header.translatechars1];
                }
        }
}



int indexstring(sw, s, filenum, structure, numMetaNames, metaID, position)
SWISH *sw;
char *s;
int filenum;
int structure;
int numMetaNames;
int *metaID;
int *position;
{
int i, j, k, inword, wordcount;
int c;
static int lenword=0;
static char *word=NULL;
int bump_position_flag=0;
IndexFILE *indexf=sw->indexlist;
	
	i=0;
	if(!lenword) word = (char *)emalloc((lenword=MAXWORDLEN) +1);
	
	for (j = 0, inword = wordcount = 0, c = 1; c ; j++) {
		c = (int) ((unsigned char) s[j]);
		if (!inword) {
			if (!c) break;
			if (iswordchar(indexf->header,c)) {
				i = 0;
				word[i++] = c;
				inword = 1;
			}
		}
		else {
			if (!c || !iswordchar(indexf->header,c)) {
				if (i == lenword) {	
					lenword *= 2;
					word =erealloc(word,lenword +1);
				}
				word[i] = '\0';

				/* Now go to lowercase */
				strtolower (word);
                                /* Get rid of the last char's */
                                bump_position_flag=stripIgnoreLastChars(indexf->header,word);

                                /* Get rid of the first char */
                                stripIgnoreFirstChars(indexf->header,word);

				/* Translate chars */
				TranslateChars(indexf->header,word);

				/* Call to ISO8 -> ISO7 (rasc) */
				if(sw->applyAscii7)
					word = str_ISO_normalize(word);
				

				if (indexf->header.applyStemmingRules)
				{
					/* apply stemming algorithm to the word to index */
					Stem(&word,&lenword);
				}
                                if (indexf->header.applySoundexRules)
                         	{
                                   /* apply soundex algorithm to the search term */
                                      	soundex(word);
                                }
				if (hasokchars(indexf,word))
				{
					if (isokword(sw,word,indexf))
					{
						if(!numMetaNames)numMetaNames=1;
						/* Add word if not UseWords is set or, if set, add it if it is in the hash list */
						if(!indexf->is_use_words_flag || isuseword(indexf,word))
						{
							for(k=0;k<numMetaNames;k++)
							{
								addentry(sw, word, filenum, structure, metaID[k], position[k]);
							}
						}
						for(k=0;k<numMetaNames;k++)
						{
							position[k]++;
                                			if(bump_position_flag)
								position[k]++;
						}
						wordcount++;
					}
					else
					{
						if ((int)strlen(word) <indexf->header.minwordlimit && !isstopword(indexf,word)) 
						{
							addStopList(indexf,word);
							addstophash(indexf,word);
						}
					}
				}
				/* Move word position */
				if (isBumpPositionCounterChar(indexf->header,(int)c)) 
				{
					for(k=0;k<numMetaNames;k++)
						position[k]++;
				}
				else if (c==' ')
				{
					/* Jump spaces */
					for(;s[j+1]==' ';j++);
					c = (int) ((unsigned char) s[j+1]);
					if(c) 
					{
						if (isBumpPositionCounterChar(indexf->header,(int)c)) 
						{
							for(k=0;k<numMetaNames;k++)
								position[k]++;
							if (!iswordchar(indexf->header,c)) 
								j++;
						}
					}
					c = (int) ((unsigned char) s[j]);
				}
				inword = 0;
			}
			else {
				if (i == lenword) {	
					lenword *= 2;
					word =erealloc(word,lenword +1);
				}
				word[i++] = c;
			}
		}
	}
	
	return wordcount;
}

void addtofwordtotals(indexf, filenum, ftotalwords)
IndexFILE *indexf;
int filenum;
int ftotalwords;
{
        if(filenum>indexf->fileoffsetarray_cursize)
                progerr("Internal error in addtofwordtotals");
        else
                indexf->fileoffsetarray[filenum-1]->ftotalwords=ftotalwords;
}


void addsummarytofile(indexf, filenum, summary)
IndexFILE *indexf;
int filenum;
char *summary;
{
        if(filenum>indexf->fileoffsetarray_cursize)
                progerr("Internal error in addsummarytofile");
        else
                indexf->filearray[filenum-1]->fi.summary=estrdup(summary);
}

