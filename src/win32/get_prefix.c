#include <windows.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>

/* return 0 for success, 1 for failure */
int
get_prefix(char *fn){
	char *tr;
	int pos;

	/* get the full name of the executable */
	if(!GetModuleFileNameA(NULL,fn,MAX_PATH))
		return 1;
	
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
	
	return 0;
}

/*
int
main(int argc, char *argv[])
{
	char *filename;
	if(get_prefix(filename)) return 1;
	printf("Path: %s\n", filename );
	return 0;
}
*/

