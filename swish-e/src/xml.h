/*
$Id$ 
**
**
** The prototypes
*/

//struct metaEntry *getXMLField(IndexFILE *indexf, char* tag, int applyautomaticmetanames, int verbose, int OkNoMeta, char **parsed_tag, char *filename );
int countwords_XML (SWISH *sw, FileProp *fprop, char *buffer);
int _countwords_XML (SWISH *sw, FileProp *fprop, char *buffer, int start, int size);
//char *parseXmlSummary (char *,char *,int);
//int isDontBumpMetaName(SWISH *sw,char *);
//int isJunkMetaName(SWISH *,char *);
