#ifndef __HasSeenModule_regex
#define __HasSeenModule_regex       1

void add_regex_patterns( char *name, regex_list **reg_list, char **params, int regex_pattern );
void  add_replace_expression( char *name, regex_list **reg_list, char *expression );

int match_regex_list( char *str, regex_list *regex, char *comment );
char *process_regex_list( char *str, regex_list *regex, int *matched );

void free_regex_list( regex_list **reg_list );
void add_regular_expression( regex_list **reg_list, char *pattern, char *replace, int cflags, int global, int negate );

#endif /* __HasSeenModule_regex */
