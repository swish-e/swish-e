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
        if(*applyautomaticmetanames && temp && *temp ) {
            if (verbose) 
                printf("\nAdding automatic MetaName %s\n",temp);

            /* $$$ addMetaEntry needs to return something here */    
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
int *metaID;
int metaIDlen;
int positionMeta;    /* Position of word in file */
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

	addtofilelist(sw, indexf, fprop->real_path, &thisFileEntry );
    addCommonProperties( sw, indexf, fprop->mtime, "", summary, start, size );


        /* Init meta info */
    metaID=(int *)emalloc((metaIDlen=1)*sizeof(int));
    currentmetanames=ftotalwords=0;
    structure=IN_FILE;
    metaID[0]=1; 
    positionMeta=1; 
    
    for(p=buffer;p && *p;) {
        if((tag=strchr(p,'<'))) {   /* Look for '<' */
                /* Index up to the tag */
            *tag++='\0';
            if((currentmetanames || (!currentmetanames && !sw->ReqMetaName)) && !in_junk)
            {

                newp = sw_ConvHTMLEntities2ISO(sw, p);

                ftotalwords +=indexstring(sw, newp, idx->filenum, structure, currentmetanames, metaID, &positionMeta);
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
                            if(currentmetanames==metaIDlen) {
                                metaID=(int *) erealloc(metaID,(metaIDlen *=2) *sizeof(int));
                            }
                            /* add netaname to array of current metanames */
                            metaID[currentmetanames]=metaNameXML->metaID;
                            /* Preserve position counter */
                            if(!currentmetanames) 
                            {
                                position_no_meta=positionMeta;
                                positionMeta = position_meta;
                            }
                            
                            /* Bump position for all metanames unless metaname in dontbumppositionOnmetatags */
                            if(!isDontBumpMetaName(sw,tag))
                                positionMeta++;
        
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
                            remove_newlines(tempprop);  /** why isn't this just done for the entire doc? */
                            remove_tags(tempprop);
                            tempprop = sw_ConvHTMLEntities2ISO(sw, tempprop);
                            
                            if ( !addDocProperty(&thisFileEntry->docProperties,metaNameXML,tempprop,strlen(tempprop),0) )
                                progwarn("property '%s' not added for document '%s'\n", metaNameXML->metaName, fprop->real_path );
                                    

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
                            for(i=currentmetanames-1;i>=0;i--) if(metaID[i]==metaNameXML->metaID) break;
                            if(i>=0) currentmetanames=i;
                            if(!currentmetanames) {
                                metaID[0] = 1;
                                position_meta = positionMeta;
                                /* Restore position counter */
                                positionMeta = position_no_meta;
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
                    ftotalwords +=parsecomment(sw,tag,idx->filenum,structure,1,&positionMeta);
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

                ftotalwords +=indexstring(sw, newp, idx->filenum, structure, currentmetanames, metaID, &positionMeta);
                if(newp!=p) efree(newp);
            }
            p=NULL;
        }
    }
    efree(metaID);
    addtofwordtotals(indexf, idx->filenum, ftotalwords);

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

