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
		for(i=e->currentsize;i>k;i--) e->filenames[i]=e->filenames[i-1];
		e->filenames[k] = (char *)estrdup(filename);
	}
	return e;
}

/* Adds a word to the master index tree.
*/

ENTRYARRAY *addentry(sw, e, word, filenum, structure, metaName, position)
SWISH *sw;
ENTRYARRAY *e;
char *word;
int filenum;
int structure;
int metaName;
int position;
{
int i,j,k,l,isbigger;
ENTRY *en;
LOCATION *tp, *oldtp;
IndexFILE *indexf=sw->indexlist;
	
	isbigger=0;
	oldtp=NULL;
	if (e == NULL) {
		e = (ENTRYARRAY *) emalloc(sizeof(ENTRYARRAY));
		e->maxsize = SEARCHHASHSIZE;   /* Put what you like */
		e->elist = (ENTRY **) emalloc(e->maxsize*sizeof(ENTRY *));
		e->currentsize = 1;
		en = (ENTRY *) emalloc(sizeof(ENTRY));
		en->word = (char *) estrdup(word);
		en->tfrequency = 1;
		en->locationarray = (LOCATION **) emalloc(sizeof(LOCATION *));
		tp = (LOCATION *) emalloc(sizeof(LOCATION));
		en->u2.currentlocation = 0;
		en->u1.max_locations=1;
		tp->filenum = filenum;
		tp->frequency = 1;
		tp->structure = structure;
		tp->metaName = metaName;
		tp->position[0]=position;

		en->locationarray[0] = tp;

		e->elist[0]=en;
		indexf->header.totalwords++;
	}
	else {
		/* Look for the position to insert using a binary search */
		i=e->currentsize-1;
		j=k=0;
		while(i>=j) {
			k=j+(i-j)/2;
			isbigger = strcmp(word,e->elist[k]->word);
			if(!isbigger) break;
			else if(isbigger > 0) j=k+1;
			else i=k-1;
		}
		if (isbigger == 0) {
			for(l=e->elist[k]->u2.currentlocation; l<e->elist[k]->u1.max_locations; l++) {
				tp = e->elist[k]->locationarray[l];
				if (tp->filenum == filenum && tp->metaName == metaName) 
					break;
			}
			if (l == e->elist[k]->u1.max_locations) {  /* filenum and metaname not found */
				e->elist[k]->locationarray=(LOCATION **) erealloc(e->elist[k]->locationarray,(++e->elist[k]->u1.max_locations)*sizeof(LOCATION *));
				tp = (LOCATION *) emalloc(sizeof(LOCATION));
				tp->filenum = filenum;
				tp->frequency = 1;
				tp->structure = structure;
				tp->metaName = metaName;
				tp->position[0]=position;
				e->elist[k]->locationarray[l]=tp;
				if (e->elist[k]->locationarray[e->elist[k]->u2.currentlocation]->filenum != filenum) {
					/* Compress previous location data */
					CompressPrevLocEntry(sw,indexf,e->elist[k]);
					e->elist[k]->tfrequency++;
					e->elist[k]->u2.currentlocation = l;
				}
			}
			else {    /* Found filenum and metaname */
				tp=e->elist[k]->locationarray[l];
				tp=erealloc(tp,sizeof(LOCATION)+tp->frequency*sizeof(int));
				tp->position[tp->frequency++]=position;
				tp->structure |= structure;
				e->elist[k]->locationarray[l]=tp;
			}
		}
		else 
		{
			en = (ENTRY *) emalloc(sizeof(ENTRY));
			en->word = (char *) estrdup(word);
			en->tfrequency = 1;
			en->u1.max_locations=1;
			en->locationarray = (LOCATION **) emalloc(sizeof(LOCATION *));
			tp = (LOCATION *) emalloc(sizeof(LOCATION));
			en->u2.currentlocation = 0;
			tp->filenum = filenum;
			tp->frequency = 1;
			tp->structure = structure;
			tp->metaName = metaName;
			tp->position[0]=position;
			en->locationarray[0]=tp;

			indexf->header.totalwords++;

			if (isbigger > 0) k++;
			e->currentsize++;
			if(e->currentsize==e->maxsize) {
				e->maxsize += 1000;
				e->elist=(ENTRY **) erealloc(e->elist,e->maxsize*sizeof(ENTRY *)); 
			}
			for(i=e->currentsize;i>k;i--) e->elist[i]=e->elist[i-1];
			e->elist[k] = en;
		}
	}
	return e;
}

/* Adds a file to the master list of files and file numbers.
*/

void addtofilelist(sw,indexf,filename, title, summary, start, size,  newFileEntry)
SWISH *sw;
IndexFILE *indexf;
char *filename;
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
			/* Just to save a little memory */
	if(strcmp(title,newnode->fi.filename)==0)
		newnode->fi.title = newnode->fi.filename;
	else
		newnode->fi.title = (char *) estrdup(title);
	if(summary)
		newnode->fi.summary= (char *) estrdup(summary);
	else
		newnode->fi.summary=NULL;
	newnode->fi.start = start;
	newnode->fi.size = size;
	newnode->docProperties = NULL;
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
int metaName=1;
int structure=IN_FILE;
	return indexstring(sw, s, filenum, structure, 1, &metaName, &position);
}

/* Parses the words in a comment.
*/

int parsecomment(sw, tag, filenum, structure, metaName, position)
SWISH *sw;
char *tag;
int filenum;
int structure;
int metaName;
int *position;
{
	structure |= IN_COMMENTS;
	return indexstring(sw, tag+1, filenum, structure, 1, &metaName, position);
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
int removestops(sw, ep, totalfiles)
SWISH *sw;
ENTRYARRAY *ep;
int totalfiles;
{
int i, j, k, l, m, n, percent, stopwords, stoppos, res;
LOCATION *lp, *lpstop;
ENTRY *e;
ENTRY **estop=NULL;
int estopsz=0, estopmsz=0;
int hashval;
struct swline *sp;
unsigned char *p;
int modified,lpstop_metaName,lpstop_filenum,lpstop_index;
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

	if(!ep || !ep->currentsize || sw->plimit>=NO_PLIMIT) return stopwords;

	if(sw->verbose)
		printf("Warning: This proccess can take some time. For a faster one, use IgnoreWords instead of IgnoreLimit\n");

	if(!estopmsz) {
		estopmsz=1;
		estop=(ENTRY **)emalloc(estopmsz*sizeof(ENTRY *));
	}
		/* this is the easy part: Remove the automatic stopwords from
		** the array */
	for(i=0; i<ep->currentsize; ) 
	{
		percent = (ep->elist[i]->tfrequency * 100 )/ totalfiles;
		if (percent >= sw->plimit && ep->elist[i]->tfrequency >= sw->flimit) {
			addStopList(indexf,ep->elist[i]->word);
			addstophash(indexf,ep->elist[i]->word);
			stopwords++;
			e = ep->elist[i];
				/* Remove entry from array */
			for(j=i+1;j<ep->currentsize;j++) ep->elist[j-1]=ep->elist[j];
			ep->currentsize--;
			if(estopsz==estopmsz) {  /* More memory? */
				estopmsz*=2;
				estop=(ENTRY **)erealloc(estop,estopmsz*sizeof(ENTRY *));
			}
				/* estop is an array for storing the
				** automatic stopwords */
			estop[estopsz++]=e;
		}
		else i++;
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
		for(i=0;i<estopsz;i++) {
			e=estop[i];
			if(sw->verbose)
				printf("\nRemoving word %s (%d occureneces)\n",e->word,e->u1.max_locations);
			for(j=0;j<ep->currentsize;j++) {
				if(sw->verbose)
					printf("Computing new positions for %s (%d occurences)\r",ep->elist[j]->word,ep->elist[j]->u1.max_locations);
				fflush(stdout);
				for(m=0,modified=0;m<ep->elist[j]->u1.max_locations;m++)
				{
					if(m<ep->elist[j]->u2.currentlocation)
						lp=uncompress_location(sw,indexf,(unsigned char *)ep->elist[j]->locationarray[m]);
					else 
						lp=ep->elist[j]->locationarray[m];
					for(n=0;n<e->u1.max_locations;n++)
					{
						if(n<e->u2.currentlocation)
						{
							p=(unsigned char *)e->locationarray[n];
							uncompress2(lpstop_index,p);
							lpstop_metaName=indexf->locationlookup->all_entries[lpstop_index-1]->val[0];
							uncompress2(lpstop_filenum,p);
						} else {
							lpstop_filenum=e->locationarray[n]->filenum;
							lpstop_metaName=e->locationarray[n]->metaName;
						}
						res=lp->filenum-lpstop_filenum;
						if(res<0) 
							break;
						if(res==0) {
						   res=lp->metaName-lpstop_metaName;
						   if(res<0) 
							break;
						   if(res==0)
						   {
						      if(n<e->u2.currentlocation)
							lpstop=uncompress_location(sw,indexf,(unsigned char *)e->locationarray[n]);
						      else
							lpstop=e->locationarray[n];
						      for(k=lpstop->frequency;k;) 
						         for(stoppos=lpstop->position[--k],l=lp->frequency;l;) {if(lp->position[--l]>stoppos) lp->position[l]--; else break;}
						      modified=1;
						      if(n<e->u2.currentlocation)
							 efree(lpstop);
						    }
						}
					}
					if(m<ep->elist[j]->u2.currentlocation) 
					{
					   if(modified)  
					   {
						efree(ep->elist[j]->locationarray[m]);
						ep->elist[j]->locationarray[m]=(LOCATION *)compress_location(sw,indexf,lp);
					   } else efree(lp);
					} 
				}
			}
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
ENTRYARRAY *ep=sw->entrylist;
	if(ep)
	{
		for(i=0; i<ep->currentsize; i++) 
		{
			if (!isstopword(indexf,ep->elist[i]->word)) 
			{
		                /* Compress remaining data */
        			CompressCurrentLocEntry(sw,indexf,ep->elist[i]);
			}
		}
		for(i=0; i<ep->currentsize; i++) 
		{
			if (!isstopword(indexf,ep->elist[i]->word)) 
			{
				/* Sort locationlist by MetaName, Filenum
				** for faster search */
				sortentry(sw,indexf,ep->elist[i]);
				/* Compute hash table for direct search */
				computehashentry(sw->hashentries,ep->elist[i]);
				/* Write entry to file */
				printword(sw,ep->elist[i],indexf);
			}
			else
			{
				/* Word content is not longer needed */
				efree(ep->elist[i]->word);   
				efree(ep->elist[i]);   
				ep->elist[i]=NULL;
			}
		}
		fputc(0,indexf->fp);   /* End of words mark */
		printhash(sw->hashentries, indexf);
		for(i=0; i<ep->currentsize; i++) 
		{
			if (ep->elist[i])   /* Not a stopword */
			{
				printworddata(sw,ep->elist[i],indexf);
				efree(ep->elist[i]);   
			}
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
int i,wordlen;
long f_offset;
FILE *fp=indexf->fp;

	f_offset = ftell(fp);
	for (i = 0; indexchars[i] != '\0'; i++)
		if ((ep->word)[0] == indexchars[i] && !indexf->offsets[i])
			indexf->offsets[i] = f_offset;
		
		/* Get HashOffset for direct access */
	for (i = 0; i<SEARCHHASHSIZE; i++)
		if (sw->hashentries[i] == ep) {
			indexf->hashoffsets[i] = f_offset;
			break;
		}
	
		/* Write word length, word and a NULL offset */
	wordlen=strlen(ep->word);
	compress1(wordlen,fp);
	fwrite(ep->word, wordlen, 1, fp);

		/* Word content is not longer needed */
	efree(ep->word);   

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
int i,index,wordlen,curmetaname;
long tmp,curmetanamepos,f_offset;
int metaName,filenum,frequency,position;
int index_structfreq;
unsigned char *compressed_data,*p;
FILE *fp=indexf->fp;

	curmetaname=0;
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
		metaName=indexf->locationlookup->all_entries[index-1]->val[0];
		if(curmetaname!=metaName) {
			if(curmetaname) {
				/* Write in previous metaname (curmetaname)
				** file offset to next metaname */
				tmp=ftell(fp);
				fseek(fp,curmetanamepos,0);
				printlong(fp,tmp);
				fseek(fp,tmp,0);
			}
			curmetaname=metaName;
			compress1(curmetaname,fp);
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
	/* Write in previous metaname (curmetaname)
	** file offset to end of metanames */
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

unsigned char *buildFileEntry(filename, title, summary, start, size, fp, docProperties,lookup_path,sz_buffer)
char *filename;
char *title;
char *summary;
int start;     
int size;     
FILE *fp;
struct docPropertyEntry **docProperties;
int *sz_buffer;
int lookup_path;
{
int len,len_filename,lentitle,lensummary;
unsigned char *buffer1,*buffer2,*buffer3,*p;
int lenbuffer1;
int datalen1, datalen2,datalen3;
	len_filename = strlen(filename)+1;
	lentitle = strlen(title)+1;
	if(summary)
		lensummary=strlen(summary)+1;
	else
		lensummary=0;
	lenbuffer1=len_filename+lentitle+lensummary+6*6;
	buffer1=emalloc(lenbuffer1);
	p=buffer1;
	lookup_path++;   /* To avoid the 0 problem in compress increase 1 */
	compress3(lookup_path,p);
		/* We store length +1 to avoid problems with 0 length - So 
		it also writes the null terminator */
	compress3(len_filename,p);
	memcpy(p,filename,len_filename);p+=len_filename;
	if(lentitle==len_filename)
	{
		if(memcmp(filename,title,lentitle)==0)
		{
			lentitle=0;  /* Flag to indicate that filename
					** and title are identical */
		}
	}
	if(lentitle)
	{
		compress3(lentitle,p);
		memcpy(p,title,lentitle);p+=lentitle;
	} else *p++='\0';
	if(summary)
	{
		compress3(lensummary,p);
		memcpy(p,summary,lensummary);p+=lensummary;
	} else *p++='\0';
	len = start +1;    /* We store start + 1 to avoid problems with
			  * files with start 0 */
	compress3(len,p);
	len = size +1;    /* We store size + 1 to avoid problems with
			  * files with size 0 */
	compress3(len,p);
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

struct file *readFileEntry(indexf, filenum, readdocproperties)
IndexFILE *indexf;
int filenum;
int readdocproperties;
{
long pos;
int total_len,len1,len2,len3,bytes,begin,lookup_path;
char *buffer,*p;
char *buf1;
char *buf2;
char *buf3;
struct file *fi;
FILEOFFSET *fo;
FILE *fp=indexf->fp;
	fi=indexf->filearray[filenum-1];
	fo=indexf->fileoffsetarray[filenum-1];
	if(fi->read && (fi->docProperties || !readdocproperties)) 
		return fi;   /* Read it previously */
	pos=fo->filelong;

	fseek(fp, 0, 0);  /* Do I have a buggy gcc ? */
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

	uncompress3(lookup_path,p);  /* Index to lookup table of paths */
	lookup_path--;
	uncompress3(len1,p);   /* Read length of filename */
	buf1 = emalloc(len1);  /* Includes NULL terminator */
	memcpy(buf1,p,len1);   /* Read filename */
	p+=len1;
	uncompress3(len2,p);   /* Read length of title */
		/* If 0 then filename == title */
	if(!len2)
		buf2=buf1;
	else {
		buf2 = emalloc(len2);
		memcpy(buf2,p,len2);   /* Read title */
		p+=len2;
	}
	uncompress3(len3,p);   /* Read length of summary */
	if(!len3)
		buf3=NULL;
	else {
		buf3 = emalloc(len3);
		memcpy(buf3,p,len3);   /* Read summary */
		p+=len3;
	}
	uncompress3(begin,p);           /* Read start */
	begin--;
	uncompress3(bytes,p);           /* Read size */
	bytes--;

	fi->fi.lookup_path = lookup_path;
		/* Add the path to filename */
	len1=strlen(indexf->pathlookup->all_entries[lookup_path]->val);
	len2=strlen(buf2);
	fi->fi.filename = emalloc(len1+len2+1);
	memcpy(fi->fi.filename,indexf->pathlookup->all_entries[lookup_path]->val,len1);
	memcpy(fi->fi.filename+len1,buf1,len2);
	fi->fi.filename[len1+len2]='\0';
	efree(buf1);
	fi->fi.title = buf2;
	fi->fi.summary = buf3;
	fi->fi.start = begin;
	fi->fi.size = bytes;
	fi->read=1;

	/* read (or skip over) the document properties section  */
	fi->docProperties=NULL;
	if (readdocproperties) 
		fi->docProperties = fetchDocProperties( p);

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
		buffer=buildFileEntry(filep->fi.filename, filep->fi.title, filep->fi.summary, filep->fi.start, filep->fi.size, fp, &filep->docProperties,filep->fi.lookup_path,&sz_buffer);
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
int len, style;
FILE *fp=indexf->fp;
	
	indexf->offsets[METANAMEPOS] = ftell(fp);
	for (entry = indexf->metaEntryList; entry; entry = entry->next)
    {
		len = strlen(entry->metaName);
		compress1(len,fp);
		fwrite(entry->metaName,len,1,fp);
		if (entry->isDocProperty)
		{
			/* write the meta name style:
			 * <name>"0   -> normal meta name [default, so does not have to be written]
			 * <name>"1   -> doc property name
			 * <name>"2   -> both
			 */
			/* Add one to compress non 0 value */
			style = (entry->isOnlyDocProperty) ? 2 : 3;
		} else style=1;
		compress1(style,fp);
    }
	fputc(0,fp);
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

	metaname=0;
	nextposmetaname=0L;
	
	frequency=0;


	fseek(fp, 0, 0);
	readheader(indexf);
	readoffsets(indexf);
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
				fileInfo = readFileEntry(indexf, filenum,0);
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
		fi=readFileEntry(indexf,i+1,1);
		fflush(stdout);
		if(fi->fi.summary)
		{
			printf("%s \"%s\" \"%s\" %d %d",fi->fi.filename,fi->fi.title,fi->fi.summary,fi->fi.start,fi->fi.size);fflush(stdout);  /* filename */
		} else {
			printf("%s \"%s\" \"\" %d %d",fi->fi.filename,fi->fi.title,fi->fi.start,fi->fi.size);fflush(stdout);  /* filename */
		}
		for(docProperties=fi->docProperties;docProperties;docProperties=docProperties->next) {
			printf(" PROP_%d:\"%s\"",docProperties->metaName,docProperties->propValue); 
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
			putchar((int)(i-1+(int)'0'));
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
	while ( isIgnoreLastChar(header, word[--i]) )
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
	while ( isIgnoreFirstChar(header,word[i]) )
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


/* Jose Ruiz 04/00 */
/* Function to build a hash table with all the words for direct access */
void computehashentry(hashentries,e)
ENTRY **hashentries;
ENTRY *e;
{
unsigned hashval;
	hashval = searchhash(e->word);
	if(!hashentries[hashval]) 
	{
		hashentries[hashval] = e;
		e->u2.nexthash = NULL;
	} else {		
		e->u2.nexthash = hashentries[hashval];
		hashentries[hashval] = e;
	}
}

/* 
** Jose Ruiz 04/00
** Store a portable long with just four bytes
*/
void printlong(fp, num)
FILE *fp;
long num;
{
int i;
	if(!num) 
		for(i=0;i<MAXLONGLEN;i++) fputc(0,fp);
	else 
		for(i=0;i<MAXLONGLEN;i++,num/=256) 
			fputc((num % 256),fp);
}

/* 
** Jose Ruiz 04/00
** Read a portable long (just four bytes)
*/
long readlong(fp)
FILE *fp;
{
int i, val[MAXLONGLEN];
long num=0;
	for(i=0;i<MAXLONGLEN;i++) 
		val[i]=fgetc(fp);
	for(i=MAXLONGLEN;i>0;){
		num*=256;
		num+=val[--i];
	}
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
			while(ep) {
				fseek(fp,ep->u1.fileoffset,0);
				uncompress1(wordlen,fp);
				fseek(fp,(long)wordlen,SEEK_CUR);
				if((epn = ep->u2.nexthash)) {
					printlong(fp,epn->u1.fileoffset);
					ep = epn;
				} else {
					printlong(fp,(long)0);
					break;
				}
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



int indexstring(sw, s, filenum, structure, numMetaNames, metaName, position)
SWISH *sw;
char *s;
int filenum;
int structure;
int numMetaNames;
int *metaName;
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
				for (i = 0; word[i]; i++)
					word[i] = tolower(word[i]);
                                /* Get rid of the last char's */
                                bump_position_flag=stripIgnoreLastChars(indexf->header,word);

                                /* Get rid of the first char */
                                stripIgnoreFirstChars(indexf->header,word);

				/* Translate chars */
				TranslateChars(indexf->header,word);

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
								sw->entrylist = (ENTRYARRAY *) addentry(sw, sw->entrylist, word, filenum, structure, metaName[k], position[k]);
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

