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
**-----------------------------------------------------------------
** Fixed the merge option -M
** G. Hill 3/7/97
**
** Changed readindexline, mergeindexentries, printindexentry and
** added marknumMerge, addtoresultlistMerge, markentrylistMerge,
** ismarkedMerge to add support for METADATA
** G. Hill 3/26/97 ghill@library.berkeley.edu
**
** change sprintf to snprintf to avoid corruption
** added safestrcpy() macro to avoid corruption from strcpy overflow
** SRE 11/17/99
**
*/

#include <assert.h> /* for bug hunting */
#include "swish.h"
#include "merge.h"
#include "error.h"
#include "search.h"
#include "index.h"
#include "string.h"	
#include "hash.h"
#include "mem.h"
#include "file.h"
#include "docprop.h"
#include "list.h"
#include "compress.h"
/* #### Added metanames.h */
#include "metanames.h"
/* #### */

/* The main merge functions - it accepts three file names.
** This is a bit hairy. It basically acts as a zipper,
** zipping up both index files into one.
*/

void readmerge(file1, file2, outfile, verbose)
char *file1;
char *file2;
char *outfile;
int verbose;
{
int i, indexfilenum1, indexfilenum2, wordsfilenum1, wordsfilenum2, result, totalfiles, totalwords, skipwords, skipfiles;
long fileinfo1, fileinfo2, offsetstart, hashstart;
ENTRY *ip1, *ip2, *ip3;
ENTRY *buffer1, *buffer2;
struct metaMergeEntry *metaFile1, *metaFile2;
int firstTime = 1;
SWISH *sw1, *sw2, *sw;
IndexFILE *indexf1, *indexf2, *indexf;
struct file *fi;

	if (verbose) printf("Opening and reading file 1 header...\n");

	if(!(sw1 = SwishOpen(file1)))
	{
		sw1->errorstr=BuildErrorString(sw1->errorstr, &sw1->lenerrorstr, "Couldn't read the index file \"%s\".\n.\n", file1);
		progerr(sw1->errorstr);
	}

	if (verbose) printf("Opening and reading file 2 header...\n");

	if(!(sw2 = SwishOpen(file2)))
	{
		sw2->errorstr=BuildErrorString(sw2->errorstr, &sw2->lenerrorstr, "Couldn't read the index file \"%s\".\n.\n", file2);
		progerr(sw2->errorstr);
	}
	
	indexf1=sw1->indexlist;
	indexf2=sw2->indexlist;
		/* Output data */
	sw = SwishNew();
	indexf = sw->indexlist= addindexfile(sw->indexlist, outfile);

		/* Create and empty outfile */
	CreateEmptyFile(sw,outfile);
		/* and open it for read/write */
	if ((indexf->fp = openIndexFILEForReadAndWrite(outfile)) == NULL) {
		sw->errorstr=BuildErrorString(sw->errorstr, &sw->lenerrorstr, "Couldn't write the merged index file \"%s\".", outfile);
		progerr(sw->errorstr);
	}
		/* Force the economic mode to save memory */
	sw->swap_flag=1;

	initindexfilehashlist();
	
	metaFile1 = metaFile2 = NULL;
	
	initmapentrylist();

	/* Merge values of both files */
	if(strcmp(indexf1->header.wordchars,indexf2->header.wordchars)) {
		printf("Warning: WordCharacters do not match. Merging them\n");
		indexf->header.wordchars=mergestrings(indexf1->header.wordchars,indexf2->header.wordchars);
		indexf->header.lenwordchars=strlen(indexf->header.wordchars);
	} else {
		indexf->header.wordchars=SafeStrCopy(indexf->header.wordchars,indexf1->header.wordchars,&indexf->header.lenwordchars);
	}
	makelookuptable(indexf->header.wordchars,indexf->header.wordcharslookuptable);

	if(strcmp(indexf1->header.beginchars,indexf2->header.beginchars)) {
		printf("Warning: BeginCharacters do not match. Merging them\n");
		indexf->header.beginchars=mergestrings(indexf1->header.beginchars,indexf2->header.beginchars);
		indexf->header.lenbeginchars=strlen(indexf->header.beginchars);
	} else {
		indexf->header.beginchars=SafeStrCopy(indexf->header.beginchars,indexf1->header.beginchars,&indexf->header.lenbeginchars);
	}
	makelookuptable(indexf->header.beginchars,indexf->header.begincharslookuptable);

	if(strcmp(indexf1->header.endchars,indexf2->header.endchars)) {
		printf("Warning: EndCharacters do not match. Merging them\n");
		indexf->header.endchars=mergestrings(indexf1->header.endchars,indexf2->header.endchars);
		indexf->header.lenendchars=strlen(indexf->header.endchars);
	} else {
		indexf->header.endchars=SafeStrCopy(indexf->header.endchars,indexf1->header.endchars,&indexf->header.lenendchars);
	}
	makelookuptable(indexf->header.endchars,indexf->header.endcharslookuptable);

	if(strcmp(indexf1->header.ignorefirstchar,indexf2->header.ignorefirstchar)) {
		printf("Warning: IgnoreFirstChar do not match. Merging them\n");
		indexf->header.ignorefirstchar=mergestrings(indexf1->header.ignorefirstchar,indexf2->header.ignorefirstchar);
		indexf->header.lenignorefirstchar=strlen(indexf->header.ignorefirstchar);
	} else {
		indexf->header.ignorefirstchar=SafeStrCopy(indexf->header.ignorefirstchar,indexf1->header.ignorefirstchar,&indexf->header.lenignorefirstchar);
	}
	makelookuptable(indexf->header.ignorefirstchar,indexf->header.ignorefirstcharlookuptable);

	if(strcmp(indexf1->header.ignorelastchar,indexf2->header.ignorelastchar)) {
		printf("Warning: IgnoreLastChar do not match. Merging them\n");
		indexf->header.ignorelastchar=mergestrings(indexf1->header.ignorelastchar,indexf2->header.ignorelastchar);
		indexf->header.lenignorelastchar=strlen(indexf->header.ignorelastchar);
	} else {
		indexf->header.ignorelastchar=SafeStrCopy(indexf->header.ignorelastchar,indexf1->header.ignorelastchar,&indexf->header.lenignorelastchar);
	}
	makelookuptable(indexf->header.ignorelastchar,indexf->header.ignorelastcharlookuptable);

	indexf->header.applyStemmingRules=indexf1->header.applyStemmingRules && indexf2->header.applyStemmingRules;
	indexf->header.applySoundexRules=indexf1->header.applySoundexRules && indexf2->header.applySoundexRules;
	indexf->header.ignoreTotalWordCountWhenRanking=indexf1->header.ignoreTotalWordCountWhenRanking && indexf2->header.ignoreTotalWordCountWhenRanking;
	if(indexf1->header.minwordlimit<indexf2->header.minwordlimit) indexf->header.minwordlimit=indexf1->header.minwordlimit;
	else indexf->header.minwordlimit=indexf2->header.minwordlimit;
	if(indexf1->header.maxwordlimit<indexf2->header.maxwordlimit) indexf->header.maxwordlimit=indexf1->header.maxwordlimit;
	else indexf->header.maxwordlimit=indexf2->header.maxwordlimit;

	indexf->header.applyFileInfoCompression=indexf1->header.applyFileInfoCompression && indexf2->header.applyFileInfoCompression;

	ip1 = ip2 = ip3 = NULL;
	buffer1 = buffer2 = NULL;

	if (verbose) printf("Counting files... ");

	indexfilenum1 = indexf1->header.totalfiles;
	indexfilenum2 = indexf2->header.totalfiles;
	wordsfilenum1 = indexf1->header.totalwords;
	wordsfilenum2 = indexf2->header.totalwords;
	totalfiles = indexfilenum1 + indexfilenum2;
	totalwords = wordsfilenum1 + wordsfilenum2;

	if (verbose) printf("%d files.\n", indexfilenum1 + indexfilenum2);

	fileinfo1 = indexf1->offsets[FILELISTPOS];
	metaFile1 = readMergeMeta(sw1,indexf1->metaEntryList);
	
	fileinfo2 = indexf2->offsets[FILELISTPOS];
	metaFile2 = readMergeMeta(sw2,indexf2->metaEntryList);
	
	/* Create the merged list and modify the
	   individual ones with the new meta index
	*/
	indexf->metaEntryList = createMetaMerge(metaFile1, metaFile2);
	
	if (verbose) printf("\nReading file 1 info ...");
	fflush(stdout);

	fseek(indexf1->fp, fileinfo1, 0);
	for (i = 1; i <= indexfilenum1; i++) {
		fi = readFileEntry(indexf1,i, 1);
		addindexfilelist(sw, i, fi->fi.filename, fi->fi.mtime, fi->fi.title, fi->fi.summary, fi->fi.start, fi->fi.size, fi->docProperties, &totalfiles,indexf1->fileoffsetarray[i-1]->ftotalwords, metaFile1);
	}
	if (verbose) printf("\nReading file 2 info ...");

	fseek(indexf2->fp, fileinfo2, 0);
	for (i = 1; i <= indexfilenum2; i++) {
		fi = readFileEntry(indexf2,i, 1);
		addindexfilelist(sw, i + indexfilenum1, fi->fi.filename, fi->fi.mtime, fi->fi.title, fi->fi.summary, fi->fi.start, fi->fi.size, fi->docProperties, &totalfiles,indexf2->fileoffsetarray[i-1]->ftotalwords, metaFile2);
	}
	
	if (verbose) printf("\nCreating output file ... ");
	
	if (verbose) printf("\nMerging words... "); 
	
		/* Adjust file pointer to start of word info */
	fseek(indexf1->fp,indexf1->wordpos,0);
	fseek(indexf2->fp,indexf2->wordpos,0);
	
	for (i = 0; i < MAXCHARS; i++)
		indexf->offsets[i] = 0;
	for (i = 0; i < SEARCHHASHSIZE; i++)
		indexf->hashoffsets[i] = 0;
	skipwords = 0;
	while (1) {
		if (buffer1 == NULL) {
			ip1 = (ENTRY *) 
				readindexline(indexf1, metaFile1);
			if (ip1 == NULL) {
				if (ip2 == NULL && !firstTime) {
					break;
				}
			}
			buffer1 = ip1;
		}
		firstTime =0;
		if (buffer2 == NULL) {
			ip2 = (ENTRY *) 
				readindexline(indexf2, metaFile2);
			if (ip2 == NULL){
				if (ip1 == NULL) {
					break;
				}
			}
			else 
				addfilenums(ip2, indexfilenum1);
			buffer2 = ip2;
		}
		if (ip1 == NULL)
			result = 1;
		else if (ip2 == NULL)
			result = -1;
		else 
			result = strcmp(ip1->word, ip2->word);
		if (!result) {
			ip3 = (ENTRY *) mergeindexentries(ip1, ip2, indexfilenum1);
			/* ip1 and ip2 are freeded in mergeindexentries */
			buffer1 = buffer2 = NULL;
			skipwords++;
		}
		else if (result < 0) {
			ip3 = ip1;
			buffer1 = NULL;
		}
		else {
			ip3 = ip2;
			buffer2 = NULL;
		}
		sw->entrylist = (ENTRYARRAY *)addentryMerge(sw,ip3);
	}

	if (verbose) {
		if (skipwords)
			printf("%d redundant word%s.\n", skipwords,
			(skipwords == 1) ? "" : "s");
		else
			printf("no redundant words.\n");
	}

	if (verbose) printf("\nPrinting header... ");

	printheader(&indexf->header,indexf->fp, outfile, (totalwords-skipwords), totalfiles,1);

	offsetstart = ftell(indexf->fp);
	for (i = 0; i < MAXCHARS; i++)
		printlong(indexf->fp,(long) 0);
	
	hashstart = ftell(indexf->fp);
	for (i = 0; i < SEARCHHASHSIZE; i++)
		printlong(indexf->fp,(long) 0);
	
	if(verbose) printf("\nPrinting words... \n");

	if(verbose) printf("Writing index entries ...\n");

	printindex(sw,indexf);

	if(verbose) printf("Writing stopwords ...\n");

	printstopwords(indexf);
	
	if (verbose) printf("\nMerging file info... ");
	
	indexf->offsets[FILELISTPOS] = ftell(indexf->fp);
	printfilelist(sw,indexf);
	
	skipfiles = (indexfilenum1 + indexfilenum2) - totalfiles;

	if (verbose) {
		if (skipfiles)
			printf("%d redundant file%s.\n", skipfiles,
			(skipfiles == 1) ? "" : "s");
		else
			printf("no redundant files.\n");
	}

	if(verbose) printf("Writing file offsets ...\n");

	printfileoffsets(indexf);

	if(verbose) printf("Writing MetaNames ...\n");
	printMetaNames(indexf);

	if(verbose) printf("Writing Location lookup tables ...\n");
	printlocationlookuptables(indexf);
	printpathlookuptable(indexf);

	fclose(indexf->fp);

	indexf->fp = openIndexFILEForReadAndWrite(outfile);
	fseek(indexf->fp, offsetstart, 0);
	for (i = 0; i < MAXCHARS; i++)
		printlong(indexf->fp,indexf->offsets[i]);

	fseek(indexf->fp, hashstart, 0);
	for (i = 0; i < SEARCHHASHSIZE; i++)
		printlong(indexf->fp,indexf->hashoffsets[i]);
	fclose(indexf->fp);indexf->fp=NULL;
	
	SwishClose(sw1);
	SwishClose(sw2);

		/* Remove swap files */
	if(sw->swap_flag)
	{
		if (isfile(sw->swap_file_name))remove(sw->swap_file_name);
		if (isfile(sw->swap_location_name))remove(sw->swap_location_name);
	}
	SwishClose(sw);
	
	if (verbose) printf("\nDone.\n");
}


/* This adds an offset to the file numbers in a particular
** result list. For instance, file 1 has file numbers going from
** 1 to 10, but so does file 2, so I have to add 10 to all the
** file numbers in file 2 before merging.
*/

void addfilenums(ip, num)
ENTRY *ip;
int num;
{
int i;
	for(i=0;i<ip->u1.max_locations;i++)
		ip->locationarray[i]->filenum = getmap(ip->locationarray[i]->filenum + num);
}


/* This reads the next line in the index file and puts the results
** in a result structure.
*/

ENTRY *readindexline(indexf, metaFile)
IndexFILE *indexf;
struct metaMergeEntry *metaFile;
{
int i, j, x, tfrequency, filenum, structure,metaName, metaName2, frequency, *position, index_structure,index_structfreq;
static int filewordlen=0;
static char *fileword=NULL;
LOCATION *loc;
ENTRY *ip;
struct metaMergeEntry* tmp=NULL;
long nextposmetaname;
long nextword,worddata;
FILE *fp=indexf->fp;
	
	j=tfrequency=filenum=structure=metaName=frequency=0;
	position=NULL;
	nextposmetaname=0L;
	if(!filewordlen) fileword = (char *) emalloc((filewordlen=MAXWORDLEN) + 1);

	loc = NULL;
	
	   /* Read Word len */
	uncompress1(i,fp);
	if (!i)   /* No more words */
		return NULL;
	if(i > filewordlen) {
		filewordlen = i + 100;
		fileword = (char *) erealloc(fileword,filewordlen + 1);
	}
	fread(fileword,1,i,fp);
	fileword[i]='\0';

	ip = (ENTRY *) emalloc(sizeof(ENTRY));
	ip->word = (char *) estrdup(fileword);
        ip->locationarray = (LOCATION **) emalloc(sizeof(LOCATION *));
	ip->u1.max_locations=0;
	ip->u2.currentlocation=0;
	/* Jump hash offset */
	readlong(fp);
	worddata=readlong(fp);
	nextword=ftell(fp);

	fseek(fp,worddata,SEEK_SET);
	
	uncompress1(tfrequency,fp);
	ip->tfrequency = tfrequency;

	uncompress1(metaName,fp);
	while(metaName) {
		nextposmetaname=readlong(fp);
		do {
			uncompress1(filenum,fp);
			uncompress1(index_structfreq,fp);
			frequency=indexf->structfreqlookup->all_entries[index_structfreq-1]->val[0];
			index_structure=indexf->structfreqlookup->all_entries[index_structfreq-1]->val[1];
			structure=indexf->structurelookup->all_entries[index_structure-1]->val[0];

			loc=(LOCATION *)emalloc(sizeof(LOCATION)+(frequency-1)*sizeof(int));
			loc->filenum=filenum;
			loc->structure=structure;
			loc->frequency=frequency;
			for(j=0;j<frequency;j++){
				uncompress1(x,fp);
				loc->position[j] = x;
			}
			/*Need to modify metaName with new list*/
			metaName2=1;
			if(metaName!=1)
			{
				for(tmp=metaFile;tmp;tmp=tmp->next) {
					if (tmp->oldIndex == metaName) {
						metaName2 = tmp->newIndex;
						break;
					}
				}
			}
				/* Severe bug if metaName not found */
			if(!tmp && metaName!=1)
				progerr("Merge internal error: Could not translate metaname\n.\n");
			loc->metaName=metaName2;

			if(!ip->u1.max_locations) 
				ip->locationarray=(LOCATION **) emalloc((++ip->u1.max_locations)*sizeof(LOCATION *)); 
			else
				ip->locationarray=(LOCATION **) erealloc(ip->locationarray,(++ip->u1.max_locations)*sizeof(LOCATION *)); 
			ip->locationarray[ip->u1.max_locations-1]=loc;
		} while (ftell(fp)!=nextposmetaname);
		uncompress1(metaName,fp);
	}
		/* restore offset to next word */
	fseek(fp,nextword,SEEK_SET);
	return ip;
}

/* This puts all the file info into a hash table so that it can
** be looked up by its pathname and filenumber. This is how
** we find redundant file information.
*/

void addindexfilelist(sw, num, filename, mtime, title, summary, start, size, docProperties, totalfiles, ftotalwords, metaFile)
SWISH *sw;
int num;
char *filename;
time_t mtime;
char *title;
char *summary;
int start;
int size;
struct docPropertyEntry *docProperties;
int *totalfiles;
int ftotalwords;
struct metaMergeEntry *metaFile;
{
int i,hashval;
struct file *thisFileEntry = NULL;
struct mergeindexfileinfo *ip;
	
	i = lookupindexfilepath(filename,start,size);
	if (i != -1) {
		*totalfiles = *totalfiles - 1;
		remap(num, i);
		return;
	}
	
	sw->filenum++;
	remap(num, sw->filenum);

	ip=(struct mergeindexfileinfo *)emalloc(sizeof(struct mergeindexfileinfo));
	ip->filenum=num;
	ip->path=(char *)estrdup(filename);
	ip->start=start;
	ip->size=size;

	hashval = bighash(ip->path);
	ip->next = indexfilehashlist[hashval];
	indexfilehashlist[hashval] = ip;

	addtofilelist(sw,sw->indexlist, filename, mtime, title, summary, start, size, &thisFileEntry);
	addtofwordtotals(sw->indexlist, sw->filenum, ftotalwords);
	thisFileEntry->docProperties = DupProps(docProperties);

		/* swap metaName values for properties */
	swapDocPropertyMetaNames(docProperties, metaFile);

	if(sw->swap_flag)
		SwapFileData(sw, sw->indexlist->filearray[sw->filenum-1]);

}


/* This returns the file number corresponding to a pathname.
*/

int lookupindexfilepath(path,start,size)
char *path;
int start;
int size;
{
unsigned hashval;
struct mergeindexfileinfo *ip;
	
	hashval = bighash(path);
	ip = indexfilehashlist[hashval];
	
	while (ip != NULL) {
		if ((strcmp(ip->path, path)==0) && ip->start==start && ip->size==size)
			return ip->filenum;
		ip = ip->next;
	}
	return -1;
}

/* This simply concatenates two information lists that correspond
** to a word found in both index files.
*/

ENTRY *mergeindexentries(ip1, ip2, num)
ENTRY *ip1;
ENTRY *ip2;
int num;
{
ENTRY *ep;
int i,j=0;
	ep = (ENTRY *) emalloc(sizeof(ENTRY));
	ep->locationarray=(LOCATION **)emalloc((ip1->u1.max_locations+ip2->u1.max_locations)*sizeof(LOCATION *));
	for(j=0;j<ip1->u1.max_locations;j++)
		ep->locationarray[j]=ip1->locationarray[j];
	for(i=0;i<ip2->u1.max_locations;i++)
	{
		if(ip2->locationarray[i]->filenum > num) 
		{
			ep->locationarray[j++]=ip2->locationarray[i];
		}
	}
	ep->u1.max_locations=j;
	ep->u2.currentlocation=0;
	ep->tfrequency=ip1->tfrequency+ip2->tfrequency;
	ep->word = ip1->word;
	efree(ip1->locationarray);
	efree(ip1);
	efree(ip2->word);
	efree(ip2->locationarray);
	efree(ip2);
	return ep;
}

/* This prints a new word entry into the merged index file,
** removing redundant file information as it goes along.
*/
ENTRYARRAY *addentryMerge(sw,ip)
SWISH *sw;
ENTRY *ip;
{
ENTRYARRAY *e=sw->entrylist;
IndexFILE *indexf=sw->indexlist;
	if (e == NULL) {
		e = (ENTRYARRAY *) emalloc(sizeof(ENTRYARRAY));
		e->maxsize = SEARCHHASHSIZE;   
		e->elist = (ENTRY **) emalloc(e->maxsize*sizeof(ENTRY *));
		e->currentsize = 1;
		e->elist[0]=ip;
	}
	else {
		/* Note that the entries are sorted in the original files */
		/* So there is no need to sort here */
		e->currentsize++;
		if(e->currentsize==e->maxsize) {
			e->maxsize +=1000;
			e->elist=(ENTRY **) erealloc(e->elist,e->maxsize*sizeof(ENTRY *)); 
		}
		e->elist[e->currentsize-1]=ip;
	}
	CompressCurrentLocEntry(sw,indexf,ip);
	ip->u2.currentlocation=ip->u1.max_locations; /* Avoid compress again in printindex */
	return e;
}

/* This associates a number with a new number.
** This function is used to remap file numbers from index
** files to a new merged index file.
*/

void remap(oldnum, newnum)
int oldnum;
int newnum;
{
	unsigned hashval;
	struct mapentry *mp;
	
	mp = (struct mapentry *) emalloc(sizeof(struct mapentry));
	mp->oldnum = oldnum;
	mp->newnum = newnum;
	
	hashval = bignumhash(oldnum);
	mp->next = mapentrylist[hashval];
	mapentrylist[hashval] = mp;
}

/* This retrieves the number associated with another.
*/

int getmap(num)
int num;
{
	unsigned hashval;
	struct mapentry *mp;
	
	hashval = bignumhash(num);
	mp = mapentrylist[hashval];
	
	while (mp != NULL) {
		if (mp->oldnum == num)
			return mp->newnum;
		mp = mp->next;
	}
	return num;
}

/* This marks a number as having been printed.
*/

void marknum(num)
int num;
{
	unsigned hashval;
	struct markentry *mp;
	
	mp = (struct markentry *) emalloc(sizeof(struct markentry));
	mp->num = num;
	
	hashval = bignumhash(num);
	mp->next = markentrylist[hashval];
	markentrylist[hashval] = mp;
}


/* Has a number been printed?
*/

int ismarked(num)
int num;
{
	unsigned hashval;
	struct markentry *mp;
	
	hashval = bignumhash(num);
	mp = markentrylist[hashval];
	
	while (mp != NULL) {
		if (mp->num == num)
			return 1;
		mp = mp->next;
	}
	return 0;
}

/* Initialize the marking list.
*/

void initmarkentrylist()
{
	int i;
	struct markentry *mp;
	
	for (i = 0; i < BIGHASHSIZE; i++) {
		mp = markentrylist[i]; /* minor optimization */
		if (mp != NULL)
			efree(mp);
		markentrylist[i] = NULL;
	}
}

/* TAB */
/* gprof suggests that this is a major CPU eater  :-(, that's
   because it gets called a _lot_ rather than because it is inefficient...

   ... and it's probably an indicator that free() is a major CPU hog :-(

   you'd think that putting the NULL assignment into the if() condition
   would be fastest, but either gcc is really stupid, or really smart,
   because gprof showed that unconditionally setting it after reading it
   saved about 10% over setting it unconditionally at the end of the loop
   (that could be sampling error though), and setting it inside the loop
   definitely increased it by 15-20% ... go figure?   - TAB oct/99

   For reference:
   Reading specs from /usr/lib/gcc-lib/i386-redhat-linux/2.7.2.3/specs
   gcc version 2.7.2.3

   hhmm... I wonder if we should consider making it a macro? No, there are
   routines using 1/4 the CPU, getting called 20 times as often (compress)
   so obviously subrouting overhead isn't the issue...

*/

void initmarkentrylistMerge()
{
	int i;
	struct markentryMerge *mp;
	
	for (i = 0; i < BIGHASHSIZE; i++) {
		mp = markentrylistMerge[i];
		markentrylistMerge[i] = NULL; /* TAB */
		if (mp != NULL)
			efree(mp);
	}
}      


/* Initialize the main file list.
*/

void initindexfilehashlist()
{
	int i;
	struct mergeindexfileinfo *ip;
	
	for (i = 0; i < BIGHASHSIZE; i++) {
		ip = indexfilehashlist[i];
		if (ip != NULL)
			efree(ip);
		indexfilehashlist[i] = NULL;
	}
}

/* Initialize the mapentrylist 
*/

void initmapentrylist()
{
	int i;
	struct mapentry *ip;
	
	for (i = 0; i < BIGHASHSIZE; i++) {
		ip = mapentrylist[i];
		if (ip != NULL)
			efree(ip);
		mapentrylist[i] = NULL;
	}
}

/* Reads the meta names from the index. Needs to be different from
** readMetaNames because needs to zero out the counter.
*/
struct metaMergeEntry* readMergeMeta(sw,metaEntryList)
SWISH *sw;
struct metaEntry* metaEntryList;
{
struct metaMergeEntry *mme=NULL, *tmp=NULL, *tmp2=NULL;
	while(metaEntryList)
	{
		tmp2=tmp;
		tmp=(struct metaMergeEntry *)emalloc(sizeof(struct metaMergeEntry));
		tmp->metaName=(char *)estrdup(metaEntryList->metaName);
		tmp->oldIndex=metaEntryList->index;
/* #### Changed isDocPoperty and isOnlyDocProperty by metaType */
		tmp->metaType=metaEntryList->metaType;
/* #### */
		tmp->next=NULL;
		if(!mme)
			mme=tmp;
		else {
			tmp2->next=tmp;
			tmp2=tmp;
		}
		metaEntryList = metaEntryList->next;
	}
	return mme;
}

/* Creates a list of all the meta names in the indexes
*/
struct metaEntry* createMetaMerge(metaFile1, metaFile2)
struct metaMergeEntry* metaFile1;
struct metaMergeEntry* metaFile2;
{
	struct metaMergeEntry* tmpEntry;
	int counter;
	
	struct metaEntry *metaEntryList = NULL;
	counter = 0;
	for (tmpEntry=metaFile1;tmpEntry;tmpEntry=tmpEntry->next)
		metaEntryList = addMetaMergeList(metaEntryList,tmpEntry,&counter);
	
	for (tmpEntry=metaFile2;tmpEntry;tmpEntry=tmpEntry->next)
		metaEntryList = addMetaMergeList(metaEntryList,tmpEntry,&counter);
	return metaEntryList;
}

/* Adds an entry to the merged meta names list and changes the
 ** new index in the idividual file entry
 */

struct metaEntry* addMetaMergeList(metaEntryList,metaFileEntry,count)
struct metaEntry* metaEntryList;
struct metaMergeEntry* metaFileEntry;
int* count;
{
int wordExists, newIndex;
struct metaEntry* newEntry;
struct metaEntry* tmpEntry;
struct metaEntry* last;
char *metaWord, *compWord;
/* #### Changed isDocPoperty and isOnlyDocProperty by metaType */
int metaType = 0;
/* #### */

	newIndex=0;
	last=NULL;	
	wordExists = 0;
	if ((*count) <2)
		*count = 2;

	metaWord = metaFileEntry->metaName;
/* #### Changed isDocPoperty and isOnlyDocProperty by metaType */
	metaType = metaFileEntry->metaType;
/* #### */
	
	if (metaEntryList)
	{
		for(tmpEntry=metaEntryList;tmpEntry;tmpEntry=tmpEntry->next)
		{
			if (tmpEntry->next == NULL)
				last = tmpEntry;
			compWord = tmpEntry->metaName;
			if (!strcmp(compWord,metaWord) ) 
			{
				wordExists = 1;
				newIndex = tmpEntry->index;
/* #### Changed isDocPoperty and isOnlyDocProperty by metaType */
				/*
				 * Keep the docProperties fields in synch.
				 * The semantics we want for the metaEntry are:
				 *	set META_PROP if either index is using as PropertyName
				 *	set META if either index is using as MetaName
				 */
				if (metaType & META_PROP)	/* new entry is docProp, so assert it */
					tmpEntry->metaType |= META_PROP;

				if (metaType & META_INDEX)	/* new entry is not *only* docProp, so unassert that */
					tmpEntry->metaType |= META_INDEX;
				/* Finally check that the rest of the */
				/* metaType does match */
				if(!match_meta_type(metaType,tmpEntry->metaType))
				{
					fprintf(stderr,"Couldn't merge: metaname \"%s\" :",metaWord);
					progerr("types do not match.");
				}
/* #### */
				break;
			}
		}
		if (wordExists)
		{
			metaFileEntry->newIndex = newIndex;
		}
		else 
		{
			newEntry = (struct metaEntry*) emalloc(sizeof(struct metaEntry));
			newEntry->metaName = (char*)estrdup(metaWord);
			newEntry->index = *count;
			newEntry->next = NULL;
/* #### Changed isDocPoperty and isOnlyDocProperty by metaType */
			newEntry->metaType = metaType;
/* #### */
			metaFileEntry->newIndex = (*count)++;
			last->next = newEntry;
		}
	} else {
		newEntry=(struct metaEntry*) emalloc(sizeof(struct metaEntry));
		newEntry->metaName = (char*)estrdup(metaWord);
		newEntry->index = *count;
		newEntry->next = NULL;
/* #### Changed isDocPoperty and isOnlyDocProperty by metaType */
			newEntry->metaType = metaType;
/* #### */
		metaFileEntry->newIndex = (*count)++;
		metaEntryList = newEntry;
	}
	return metaEntryList;
}



  
