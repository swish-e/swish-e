/*
** soundex.h
*/

/*
** use _AP() for easier cross-compiler (non-ANSI) porting 
** <return value> <functionname> _AP( (<arg prototypes>) );
*/

int soundex _AP ((char *word));

int soundXit _AP ((void));
