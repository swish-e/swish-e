#include <windows.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#define libexecdir "/usr/lib/swish-e"
/* return 0 for success, 1 for failure */
/* Flip any backslashes to forward slashes, and remove trailing slash */
char *
get_prefix(void){
	char *fn;
	#ifdef _WIN32
	char *tr;
	int pos;
    fn = malloc(MAX_PATH+1);
	/* get the full name of the executable */
	if(!GetModuleFileNameA(NULL,fn,MAX_PATH))
		return(libexecdir);
	
	/* get the base directory */
	tr = strrchr(fn, '\\');
	pos = tr - fn;
	fn[pos]='\0';
	
	/* get the prefix directory */
	tr = strrchr(fn, '\\');
	pos = tr - fn;
	/* if we're in bin we'll assume prefix is up one level */
	if(!strncasecmp(&fn[pos+1], "bin\0", 4))
		fn[pos]='\0';
	#else
	fn = malloc(strlen(libexecdir)+1);
	strcpy(fn,libexecdir);
	#endif
	
	return(fn);
}


int
main(int argc, char *argv[])
{
	printf("Path: %s\n", get_prefix() );
	return 0;
}


