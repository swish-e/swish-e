#ifndef DOUBLE_METAPHONE__H
#define DOUBLE_METAPHONE__H

BEGIN_C_DECLS

typedef struct
{
    char *str;
    int length;
    int bufsize;
    int free_string_on_destroy;
}
metastring;      


void
DoubleMetaphone(char *str,
                char **codes);

END_C_DECLS


#endif /* DOUBLE_METAPHONE__H */
