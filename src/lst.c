/* 2001/03 jmruiz
** This module is intended for multidocument files.
** It is based on a xml document.
** Basically, one file can contain several documents in the following way:
**
**  <tag1>         <== First document
**   bla, bla
**  </tag1>
**  <tag2>
**   bla, bla
**  </tag2>
**  <tag1>         <== Second document
**  ...
** 
**  The first found tag (tag1 in the example is used as the break point
**  for a new document
**
**  This module is useful for indexing table database (eg: Oracle. MySQL, etc)
**  in combination with Rainer's filter option.
**
*/
#include "swish.h"
#include "xml.h"
#include "lst.h"
#include "mem.h"
#include "string.h"


int countwords_LST (SWISH *sw, FileProp *fprop, char *buffer)
{
char *begintag=NULL,*endtag=NULL,*p,*q,*tag;
IndexFILE *indexf=sw->indexlist;
int numwords=0,lentag=0;
		/* First time - Look for the first valid tag */
	for(p=buffer,tag=NULL;p;)
	{
		if((begintag=strchr(p,'<'))) {   /* Look for '<' */
			begintag++;
			if((endtag=strchr(begintag,'>'))) {   /* Found */
				*endtag='\0';
				if ((begintag[0]!='!') && (begintag[0]!='/') && ((getXMLField(indexf, begintag,&sw->applyautomaticmetanames,sw->verbose,sw->OkNoMeta)))) 
				{
						/* Build "<tag>" */
					lentag=strlen(begintag)+2;
					tag=(char *)emalloc(lentag+1);
					sprintf(tag,"<%s>",begintag);
					*endtag++='>';   /* restore val */
					break;
				} else {
					*endtag++='>';   /* restore val */
					p=endtag;
				}
			} else return 0;
		} else return 0;
	}
	for(q=begintag-1,p=endtag;p;)
	{
		/* Search for the end of the document inside the file (search for a new <tag>) */
		if((p=lstrstr(p,tag)))
		{
			*p='\0';    /* Delimite the document */
		}
			/* Index it using xml module */
		numwords += _countwords_XML(sw,fprop,q,q-buffer,strlen(q));
		if(p)
		{
			*p='<';
			q=p;
			p+=lentag;

		}
	}
	efree(tag);
	return numwords;	
}



