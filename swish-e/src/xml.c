#include "swish.h"
#include "xml.h"
#include "merge.h"
#include "mem.h"
#include "string.h"
#include "check.h"
#include "docprop.h"
#include "error.h"
#include "compress.h"
#include "index.h"
#include "file.h"
/* #### Added metanames.h */
#include "metanames.h"
/* #### */

struct metaEntry *getXMLField(indexf, tag, applyautomaticmetanames, verbose, OkNoMeta)
IndexFILE *indexf;
char* tag;
int *applyautomaticmetanames;
int verbose;
int OkNoMeta;
{
char* temp;
static int lenword=0;
static char *word=NULL;
int i;
struct metaEntry* e;

	if(!lenword) word =(char *)emalloc((lenword=MAXWORDLEN)+1);

	temp = tag;
	
	/* Get to the beginning of the word disreguarding blanks */
	while (temp && *temp) {
		if (*temp == ' ')
			temp++;
		else
			break;
	}

	/* Copy the word and convert to lowercase */
	for (i=0;temp && *temp && *temp != ' '; ) {
		if (i==lenword) {
			lenword *=2;
			word= (char *) erealloc(word,lenword+1);
		}
		word[i] = *temp++;
		word[i] = tolower(word[i]);
		i++;
	}
	if (i==lenword) {
		lenword *=2;
		word= (char *) erealloc(word,lenword+1);
	}
	word[i] = '\0';
	
	while(1) {
		if((e=getMetaNameData(indexf,word)))
		{
			if ((!is_meta_index(e)) && (*applyautomaticmetanames))
				e->metaType |=META_INDEX;
			return e;
		}
		/* 06/00 Jose Ruiz
		** If automatic MetaNames enabled add the MetaName
		** else break
		*/
		if(*applyautomaticmetanames) {
			if (verbose) 
				printf("\nAdding automatic MetaName %s\n",word);
			addMetaEntry(indexf,word,0,applyautomaticmetanames); 
		} else break;
	}
	/* If it is ok not to have the name listed, just index as no-name */
	if (OkNoMeta) {
		/*    printf ("\nwarning: metaName %s does not exiest in the user config file", word); */
		return NULL;
	}
	else {
		printf ("\nerr: INDEXING FAILURE\n");
		printf ("err: The metaName %s does not exist in the user config file\n", word);
		exit(0);
	}
	
}


/* Indexes all the words in a XML file and adds the appropriate information
** to the appropriate structures.
*/

int countwords_XML(SWISH *sw, FileProp *fprop, char *buffer)

{
int ftotalwords;
int *metaName;
int metaNamelen;
int *positionMeta;    /* Position of word in file */
int tmpposition=1;    /* Position of word in file */
int currentmetanames;
char *p, *tag, *endtag=NULL;
int structure;
struct file *thisFileEntry = NULL;
struct metaEntry *metaNameXML;
int i;
IndexFILE *indexf=sw->indexlist;
char *summary=NULL;
	
	sw->filenum++;

	if(fprop->stordesc)
		summary=parseXmlSummary(buffer,fprop->stordesc->field,fprop->stordesc->size);

	addtofilelist(sw,indexf, fprop->real_path, fprop->mtime, fprop->real_path, summary, 0, fprop->fsize, &thisFileEntry);
		/* Init meta info */
	metaName=(int *)emalloc((metaNamelen=1)*sizeof(int));
	positionMeta =(int *)emalloc(metaNamelen*sizeof(int));
	currentmetanames=ftotalwords=0;
	structure=IN_FILE;
	metaName[0]=1; positionMeta[0]=1;
	
	for(p=buffer;p && *p;) {
		if((tag=strchr(p,'<'))) {   /* Look for '<' */
				/* Index up to the tag */
			*tag++='\0';
			ftotalwords +=indexstring(sw, p, sw->filenum, structure, currentmetanames, metaName, positionMeta);
				/* Now let us look for '>' */
			if((endtag=strchr(tag,'>'))) {  
				*endtag++='\0';
				if ((tag[0]!='!') && (tag[0]!='/') && ((metaNameXML=getXMLField(indexf, tag,&sw->applyautomaticmetanames,sw->verbose,sw->OkNoMeta)))) 
				{
					/* If must be indexed add the metaName to the currentlist of metaNames */
					if(is_meta_index(metaNameXML))
					{
						/* realloc memory if needed */
						if(currentmetanames==metaNamelen) {metaName=(int *) erealloc(metaName,(metaNamelen *=2) *sizeof(int));positionMeta=(int *) erealloc(positionMeta,metaNamelen*sizeof(int));}
						/* add netaname to array of current metanames */
						metaName[currentmetanames]=metaNameXML->metaID;
						/* Preserve position counter */
						if(!currentmetanames) tmpposition=positionMeta[0];
						/* Init word counter for the metaname */
						positionMeta[currentmetanames++] = 1;
					}
					p=endtag;
					/* If it is also a property doc store it
					** Only store until a < is found */
					if(is_meta_property(metaNameXML)) {
					     if((endtag=strchr(p,'<'))) *endtag='\0';
					     addDocProperty(&thisFileEntry->docProperties,metaNameXML->metaID,p,strlen(p));
					     if(endtag) *endtag='<';
					} 
				}  /* Check for end of a XML field */
				else if((tag[0]!='!') && (tag[0]=='/') && ((metaNameXML=getXMLField(indexf, tag+1, &sw->applyautomaticmetanames,sw->verbose,sw->OkNoMeta)))) {
					/* search for the metaname in the
				        ** list of currentmetanames */
					if(currentmetanames) {
			        	   	for(i=currentmetanames-1;i>=0;i--) if(metaName[i]==metaNameXML->metaID) break;
						if(i>=0) currentmetanames=i;
						if(!currentmetanames) {
						    metaName[0] = 1;
							/* Restore position counter */
						    positionMeta[0] = tmpposition;
						}
					}	
					p=endtag;
				}  /*  Check for COMMENT */
				else if ((tag[0]=='!') && sw->indexComments) {
					ftotalwords +=parsecomment(sw,tag,sw->filenum,structure,1,positionMeta);
					p=endtag;
				}    /* Default: Continue */
				else {    
					p=endtag;
				}
			} else p=tag;    /* tag not closed: continue */
		} else {    /* No more '<' */
			ftotalwords +=indexstring(sw, p, sw->filenum, structure, currentmetanames, metaName, positionMeta);
			p=NULL;
		}
	}
	efree(metaName);
	efree(positionMeta);
	addtofwordtotals(indexf, sw->filenum, ftotalwords);
	if(sw->swap_flag)
		SwapFileData(sw, indexf->filearray[sw->filenum-1]);
	return ftotalwords;
}


char *parseXmlSummary(char *buffer,char *field,int size)
{
char *summary=NULL,*tmp=NULL;
int len;

       /* Get the summary if no metaname/field is given */
	if(!field && size)
	{
		tmp=estrdup(buffer);
		remove_tags(tmp);
	}
	else if(field)
	{
		if((tmp=parsetag(field, buffer, 0,CASE_SENSITIVE_ON)))
		{
			remove_tags(tmp);

		}
	}
	if(tmp)
	{
		remove_newlines(tmp);
		len=strlen(tmp);
		if(size && len>size) len=size;
		summary=emalloc(len+1);
		memcpy(summary,tmp,len);
		summary[len]='\0';
		efree(tmp);
	}
	return summary;
}
