#include <stdio.h>
#include <stdlib.h>

#include "swish.h"

int main()
{
SWISH *sw;
int structure=1;
int i,j,num_results,numPropertiesToDisplay;
RESULT *r;

if(!(sw=SwishOpen("../tests/test.index ../tests/test.index")))
{
	printf("Could not open index files\n");
	exit(0);
}


for(i=0;i<2;i++) {

printf("---->Begin iteration %d\n",i);

num_results=SwishSearch(sw,"meta1=metatest1",structure,"meta1","meta2 asc");

if (num_results<0) 
{
	printf("Search error: %d\n",num_results);
	exit(0);
} else printf("Search results: %d\n",num_results);

numPropertiesToDisplay=getnumPropertiesToDisplay(sw);
while((r=SwishNext(sw)))
{
   printf("%d %s \"%s\" %d %d",r->rank,r->filename,r->title,r->start,r->size);
   for(j=0;j<numPropertiesToDisplay;j++) printf(" \"%s\"",r->Prop[j]);
   printf("\n");
}

printf("---->End iteration %d\n",i);

}  /* for */

SwishClose(sw);

#ifdef DEBUGMEMORY
checkmem();
#endif

return 0;
}
