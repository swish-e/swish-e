/* The prototypes */

struct metaEntry *getXMLField ( IndexFILE *, char *tag,int *, int, int); 
int countwords_XML (SWISH *sw, FileProp *fprop, char *buffer);
int _countwords_XML (SWISH *sw, FileProp *fprop, char *buffer, int start, int size);
char *parseXmlSummary (char *,char *,int);

