/*
$Id$
**
**
** 2001-03-17  rasc  save real_filename as title (instead full real_path)
**                   was: compatibility issue to v 1.x.x
** 2001-05-09  rasc  entities changed (new module)
*/

#include "swish.h"
#include "xml.h"
#include "html.h"
#include "merge.h"
#include "mem.h"
#include "string.h"
#include "check.h"
#include "docprop.h"
#include "error.h"
#include "compress.h"
#include "index.h"
#include "file.h"
#include "metanames.h"
#include "entities.h"

struct metaEntry *getXMLField(indexf, tag, applyautomaticmetanames, verbose, OkNoMeta)
IndexFILE *indexf;
char* tag;
int *applyautomaticmetanames;
int verbose;
int OkNoMeta;
{
unsigned char *temp,*temp2,c;
int isendtag;
struct metaEntry* e;

	temp = (unsigned char *)tag;

	if(!temp) return NULL;
	
	/* Get to the beginning of the word disreguarding blanks */
	while (*temp) {
		if(isspace((int)(*(unsigned char *)temp))) 
			temp++;
		else
			break;
	}

	/* Are we at the start or end tag ? */
	if(*temp=='/') 
	{
		temp++;
		isendtag=1;
	} else
		isendtag=0;

	/* XML is case sensitive - Do not convert to lowercase !!!*/
	/* Jump spaces */
	for (temp2=temp;*temp2 && !isspace((int)(*(unsigned char *)temp2));temp2++);
	
	if(temp==temp2) return NULL;

	/* Check for empty xml tag . Eg:  <mytag/> */
	if(!isendtag && (*(temp2-1))=='/') return NULL;

	c=*temp2;
	*temp2='\0';

		/* Go lowercase as discussed even if we are in xml */
                /* Use Rainer's routine */
	strtolower(temp);

	while(1) {
		if((e=getMetaNameData(&indexf->header,temp)))
		{
			if ((!is_meta_index(e)) && (*applyautomaticmetanames))
				e->metaType |=META_INDEX;
			*temp2=c;
			return e;
		}
		/* 06/00 Jose Ruiz
		** If automatic MetaNames enabled add the MetaName
		** else break
		*/
		if(*applyautomaticmetanames) {
			if (verbose) 
				printf("\nAdding automatic MetaName %s\n",temp);
			addMetaEntry(&indexf->header,temp,0,0,0, applyautomaticmetanames); 
		} else break;
	}
	/* If it is ok not to have the name listed, just index as no-name */
	if (OkNoMeta) {
		/*    printf ("\nwarning: metaName %s does not exiest in the user config file", temp); */
		*temp2=c;
		return NULL;
	}
	else {
		printf ("err: INDEXING FAILURE: The metaName %s does not exist in the user config file\n", temp);
		exit(-1);
	}
	
}


/* Indexes all the words in a XML file and adds the appropriate information
** to the appropriate structures.
*/

int countwords_XML(SWISH *sw, FileProp *fprop, char *buffer)
{
	return _countwords_XML(sw,fprop,buffer,0,fprop->fsize);
}

int _countwords_XML(SWISH *sw, FileProp *fprop, char *buffer, int start, int size)
{
int ftotalwords;
int *metaName;
int metaNamelen;
int *positionMeta;    /* Position of word in file */
int position_no_meta=1;    /* Counter for words in file (excluding metanames) */
int position_meta=1;   /* Counter for words in metanames */
int currentmetanames;
unsigned char *newp,*p, *tag, *endtag=NULL,*endproptag=NULL,*tempprop;
int structure,dummy;
struct file *thisFileEntry = NULL;
struct metaEntry *metaNameXML,*metaNameXML2;
int i;
IndexFILE *indexf=sw->indexlist;
struct MOD_Index *idx = sw->Index;
char *summary=NULL;
int in_junk=0;
	dummy=0;
	idx->filenum++;

	if(fprop->stordesc)
		summary=parseXmlSummary(buffer,fprop->stordesc->field,fprop->stordesc->size);

	addtofilelist(sw,indexf, fprop->real_path, fprop->mtime, fprop->real_filename, summary, start, size, &thisFileEntry);
		/* Init meta info */
	metaName=(int *)emalloc((metaNamelen=1)*sizeof(int));
	positionMeta = (int *)emalloc(metaNamelen*sizeof(int));
	currentmetanames=ftotalwords=0;
	structure=IN_FILE;
	metaName[0]=1; positionMeta[0]=1; 
	
	for(p=buffer;p && *p;) {
		if((tag=strchr(p,'<'))) {   /* Look for '<' */
				/* Index up to the tag */
			*tag++='\0';
			if((currentmetanames || (!currentmetanames && !sw->ReqMetaName)) && !in_junk)
			{

				newp = sw_ConvHTMLEntities2ISO(sw, p);

				ftotalwords +=indexstring(sw, newp, idx->filenum, structure, currentmetanames, metaName, positionMeta);
				if(newp!=p) efree(newp);
			}
				/* Now let us look for '>' */
			if((endtag=strchr(tag,'>'))) {  
				*endtag++='\0';
				if ((tag[0]!='!') && (tag[0]!='/'))
				{
					if((metaNameXML=getXMLField(indexf, tag,&sw->applyautomaticmetanames,sw->verbose,sw->OkNoMeta)))
					{
					/* If the data must be indexed add the metaName to the currentlist of metaNames */
						if(is_meta_index(metaNameXML) && !in_junk)
						{
							/* realloc memory if needed */
							if(currentmetanames==metaNamelen) {
								metaName=(int *) erealloc(metaName,(metaNamelen *=2) *sizeof(int));
								positionMeta=(int *) erealloc(positionMeta,metaNamelen*sizeof(int));
							}
							/* add netaname to array of current metanames */
							metaName[currentmetanames]=metaNameXML->metaID;
							/* Preserve position counter */
							if(!currentmetanames) position_no_meta=positionMeta[0];
							
							/* Bump position for all metanames unless metaname in dontbumppositionOnmetatags */
							if(!isDontBumpMetaName(sw,tag))
								for(i=0;i<currentmetanames;i++)
									positionMeta[i]++;
		
							/* Init word counter for the metaname */
							if(currentmetanames)
								positionMeta[currentmetanames] = positionMeta[0];
							else
								positionMeta[currentmetanames] = position_meta;

							currentmetanames++;
							/* $$$$$ TODO Check for XML properties here. Eg: <mytag myprop="bla bla" myotherprop="bla bla"> */
						
						}
						p=endtag;
						/* If it is also a property doc store it
						** Only store until a < is found */
						if(is_meta_property(metaNameXML)) {
							/* Look for the end of the data */
							while((endtag=strstr(endtag,"</")))
							{
								if(!(endproptag=strchr(endtag+2,'>')))
								{
									endtag=NULL;
									break;
								} else *endproptag='\0';
								if((metaNameXML2=getXMLField(indexf, endtag+1,&dummy,sw->verbose,1))) 
						        {
									if(metaNameXML2->metaID==metaNameXML->metaID)
									{
										*endtag='\0';
										*endproptag='>';
										break;
									} 
								} 
								*endproptag='>';
								endtag=endproptag+1;
							}
					     /* Remove tags and convert entities */
							tempprop=estrdup(p);
							remove_newlines(tempprop);
							remove_tags(tempprop);
							addDocProperty(&thisFileEntry->docProperties,metaNameXML->metaID,tempprop,strlen(tempprop));
							efree(tempprop);
							if(endtag) *endtag='<';
						} 
					} else {
						/* Check for junk metaname */
						if(isJunkMetaName(sw,tag))
						{
							in_junk++;
						}
							/* continue */
						p=endtag;
					} 
				}  /* Check for end of a XML field */
				else if(tag[0]=='/') 
				{
					if((metaNameXML=getXMLField(indexf, tag, &sw->applyautomaticmetanames,sw->verbose,sw->OkNoMeta)))
					{
						/* search for the metaname in the
				        ** list of currentmetanames */
						if(currentmetanames) 
						{
							for(i=currentmetanames-1;i>=0;i--) if(metaName[i]==metaNameXML->metaID) break;
							if(i>=0) currentmetanames=i;
							if(!currentmetanames) {
								metaName[0] = 1;
								position_meta = positionMeta[0];
								/* Restore position counter */
								positionMeta[0] = position_no_meta;
							}
						}
					}
					else if (isJunkMetaName(sw,tag+1))
					{
						if(in_junk>0) in_junk--;
					}
					p=endtag;
				}  /*  Check for COMMENT */
				else if ((tag[0]=='!') && sw->indexComments) {
					ftotalwords +=parsecomment(sw,tag,idx->filenum,structure,1,positionMeta);
					p=endtag;
				}    /* Default: Continue */
				else {    
					p=endtag;
				}
			} else p=tag;    /* tag not closed: continue */
		} else {    /* No more '<' */
			if((currentmetanames || (!currentmetanames && !sw->ReqMetaName)) && !in_junk)
			{

				newp = sw_ConvHTMLEntities2ISO(sw, p);

				ftotalwords +=indexstring(sw, newp, idx->filenum, structure, currentmetanames, metaName, positionMeta);
				if(newp!=p) efree(newp);
			}
			p=NULL;
		}
	}
	efree(metaName);
	efree(positionMeta);
	addtofwordtotals(indexf, idx->filenum, ftotalwords);
	if(idx->economic_flag)
		SwapFileData(sw, indexf->filearray[idx->filenum-1]);
	if(summary)
		efree(summary);  
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


int isJunkMetaName(SWISH *sw,char *tag)
{
struct swline *tmplist=sw->ignoremetalist;
char *tmptag;
	if(!tmplist) return 0;
	tmptag=estrdup(tag);
	tmptag=strtolower(tmptag);
	while(tmplist)
	{
		if(strcmp(tmptag,tmplist->line)==0)
		{
			efree(tmptag);
			return 1;
		}
		tmplist=tmplist->next;
	}
	efree(tmptag);
	return 0;
}

