/*
$Id$ 
**
**
** The prototypes
*/

int parse_HTML (SWISH *sw, FileProp *fprop, char *buffer);
int parse_XML (SWISH *sw, FileProp *fprop, char *buffer);
int parse_HTML_push(SWISH * sw, FileProp * fprop, char *buffer);
int parse_XML_push(SWISH * sw, FileProp * fprop, char *buffer);
char *parse_HTML_title(SWISH * sw, FileProp * fprop, char *buffer);

