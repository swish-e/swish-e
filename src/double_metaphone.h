#ifndef DOUBLE_METAPHONE__H
#define DOUBLE_METAPHONE__H

#ifdef __cplusplus
extern "C" {
#endif



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

#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* DOUBLE_METAPHONE__H */
