/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000, 2001 The PHP Group             |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Jose Ruiz <jmruiz@boe.es>                                   |
   +----------------------------------------------------------------------+
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_swishe.h"

typedef struct {
    int id;
    SW_HANDLE swishe_handle;
} php_swishe;

typedef struct {
    int id;
    void *swishe_search_handle;
    SW_HANDLE swishe_handle;
    php_swishe *swishe;
} php_swishe_search;

typedef struct {
    int id;
    SW_RESULTS swishe_results_handle;
    SW_HANDLE swishe_handle;
    php_swishe_search *search;
} php_swishe_results;

typedef struct {
    int id;
    void *swishe_result_handle;
    SW_RESULTS swishe_results_handle;
    SW_HANDLE swishe_handle;
    php_swishe_results *results;
} php_swishe_result;

/* True global resources - no need for thread safety here */
static int le_swishe;
static int le_swishe_search;
static int le_swishe_results;
static int le_swishe_result;

/* An OO interface as well */
static zend_class_entry *swishe_class_entry_ptr;
static zend_class_entry *swishe_search_class_entry_ptr;
static zend_class_entry *swishe_results_class_entry_ptr;
static zend_class_entry *swishe_result_class_entry_ptr;

#define FETCH_SWISHE() \
    if (this_ptr) { \
        if (zend_hash_find(HASH_OF(this_ptr), "swishe_handle", sizeof("swishe_handle"), (void **)&tmp) == FAILURE) { \
            php_error(E_WARNING, "unable to find my handle property"); \
            RETURN_FALSE; \
        } \
        ZEND_FETCH_RESOURCE(swishe, php_swishe *, tmp, -1, "swishe_handle", le_swishe); \
    }

#define FETCH_SEARCH() \
    if (this_ptr) { \
        if (zend_hash_find(HASH_OF(this_ptr), "swishe_search_handle", sizeof("swishe_search_handle"), (void **)&tmp) == FAILURE) { \
            php_error(E_WARNING, "unable to find my search_handle property"); \
            RETURN_FALSE; \
        } \
        ZEND_FETCH_RESOURCE(search, php_swishe_search *, tmp, -1, "swishe_search_handle", le_swishe_search); \
    }

#define FETCH_RESULTS() \
    if (this_ptr) { \
        if (zend_hash_find(HASH_OF(this_ptr), "swishe_results_handle", sizeof("swishe_results_handle"), (void **)&tmp) == FAILURE) { \
            php_error(E_WARNING, "unable to find my results_handle property"); \
            RETURN_FALSE; \
        } \
        ZEND_FETCH_RESOURCE(results, php_swishe_results *, tmp, -1, "swishe_results_handle", le_swishe_results); \
    }

#define FETCH_RESULT() \
    if (this_ptr) { \
        if (zend_hash_find(HASH_OF(this_ptr), "swishe_result_handle", sizeof("swishe_result_handle"), (void **)&tmp) == FAILURE) { \
            php_error(E_WARNING, "unable to find my result_handle property"); \
            RETURN_FALSE; \
        } \
        ZEND_FETCH_RESOURCE(result, php_swishe_result *, tmp, -1, "swishe_result_handle", le_swishe_result); \
    }
/* If you declare any globals in php_swishe.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(swishe)
*/


/* {{{ swishe_functions[]
 *
 * Every user visible function must have an entry in swishe_functions[].
 */
function_entry swishe_functions[] = {
    PHP_FE(confirm_swishe_compiled, NULL)       /* For testing, remove later. */
    PHP_FE(swishe_open,  NULL)
    PHP_FE(swishe_close,  NULL)
    PHP_FE(swishe_index_names,  NULL)
    PHP_FE(swishe_header_names, NULL)
    PHP_FE(swishe_header_value, NULL)
    PHP_FE(swishe_stem, NULL)
    PHP_FE(swishe_swish_words_by_letter, NULL)
    PHP_FE(swishe_error,    NULL)
    PHP_FE(swishe_critical_error,   NULL)
    PHP_FE(swishe_abort_last_error, NULL)
    PHP_FE(swishe_error_string, NULL)
    PHP_FE(swishe_last_error_msg,   NULL)
    PHP_FE(swishe_query,    NULL)
    PHP_FE(swishe_set_query,    NULL)
    PHP_FE(swishe_set_structure,    NULL)
    PHP_FE(swishe_set_phrase_delimiter, NULL)
    PHP_FE(swishe_set_search_limit, NULL)
    PHP_FE(swishe_reset_search_limit,   NULL)
    PHP_FE(swishe_set_sort, NULL)
    PHP_FE(swishe_execute,  NULL)
    PHP_FE(swishe_hits, NULL)
    PHP_FE(swishe_parsed_words, NULL)
    PHP_FE(swishe_removed_stopwords,    NULL)
    PHP_FE(swishe_seek_result,  NULL)
    PHP_FE(swishe_next_result,  NULL)
    PHP_FE(swishe_property, NULL)
    PHP_FE(swishe_result_index_value,   NULL)
    PHP_FE(swishe,  NULL)
    {NULL, NULL, NULL}
};
/* }}} */

static zend_function_entry php_swishe_class_functions[] = {
    PHP_FE(swishe,    NULL)
    PHP_FALIAS(index_names,  swishe_index_names, NULL)
    PHP_FALIAS(header_names, swishe_header_names, NULL)
    PHP_FALIAS(header_value, swishe_header_value, NULL)
    PHP_FALIAS(error, swishe_error, NULL)
    PHP_FALIAS(critical_error, swishe_critical_error, NULL)
    PHP_FALIAS(abort_last_error, swishe_abort_last_error, NULL)
    PHP_FALIAS(error_string, swishe_error_string, NULL)
    PHP_FALIAS(stem, swishe_stem, NULL)
    PHP_FALIAS(swish_words_by_letter, swishe_swish_words_by_letter, NULL)
    PHP_FALIAS(new_search_object, swishe_search, NULL)
    PHP_FALIAS(query, swishe_query, NULL)
    {NULL, NULL, NULL}
};

static zend_function_entry php_swishe_search_class_functions[] = {
    PHP_FALIAS(set_query, swishe_set_query, NULL)
    PHP_FALIAS(set_structure, swishe_set_structure, NULL)
    PHP_FALIAS(set_phrase_delimiter, swishe_set_phrase_delimiter, NULL)
    PHP_FALIAS(set_search_limit, swishe_set_search_limit, NULL)
    PHP_FALIAS(reset_search_limit, swishe_reset_search_limit, NULL)
    PHP_FALIAS(set_sort, swishe_set_sort, NULL)
    PHP_FALIAS(execute, swishe_execute, NULL)
    {NULL, NULL, NULL}
};

static zend_function_entry php_swishe_results_class_functions[] = {
    PHP_FALIAS(hits, swishe_hits, NULL)
    PHP_FALIAS(parsed_words, swishe_parsed_words, NULL)
    PHP_FALIAS(removed_stopwords, swishe_removed_stopwords, NULL)
    PHP_FALIAS(seek_result, swishe_seek_result, NULL)
    PHP_FALIAS(next_result, swishe_next_result, NULL)
    {NULL, NULL, NULL}
};

static zend_function_entry php_swishe_result_class_functions[] = {
    PHP_FALIAS(property, swishe_property, NULL)
    PHP_FALIAS(result_index_value, swishe_result_index_value, NULL)
    {NULL, NULL, NULL}
};

/* {{{ swishe_module_entry
 */
zend_module_entry swishe_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    "swishe",
    swishe_functions,
    PHP_MINIT(swishe),
    PHP_MSHUTDOWN(swishe),
    NULL,
    NULL,
    PHP_MINFO(swishe),
#if ZEND_MODULE_API_NO >= 20010901
    PHP_SWISHE_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_SWISHE
ZEND_GET_MODULE(swishe)
#endif

/* {{{ php_swishe_init_globals 
static void php_swishe_init_globals(zend_swishe_globals *swishe_globals)
{
}
*/
/* }}} */

PHP_INI_BEGIN()
PHP_INI_END()


/* destroy a resource of type "swishe" */
static void _php_swishe_close(zend_rsrc_list_entry *rsrc)
{
php_swishe *swishe = (php_swishe *)rsrc->ptr;

    SwishClose(swishe->swishe_handle);
    efree(swishe);
}

/* destroy a resource of type "swishe_search" */
static void _php_swishe_search_close(zend_rsrc_list_entry *rsrc)
{
php_swishe_search *search = (php_swishe_search *)rsrc->ptr;

    efree(search);
}

/* destroy a resource of type "swishe_results" */
static void _php_swishe_results_close(zend_rsrc_list_entry *rsrc)
{
php_swishe_results *results = (php_swishe_results *)rsrc->ptr;

    efree(results);
}


/* destroy a resource of type "swishe_result" */
static void _php_swishe_result_close(zend_rsrc_list_entry *rsrc)
{
php_swishe_result *result = (php_swishe_result *)rsrc->ptr;

    efree(result);
}

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(swishe)
{
    zend_class_entry swishe_class_entry;
    zend_class_entry swishe_search_class_entry;
    zend_class_entry swishe_results_class_entry;
    zend_class_entry swishe_result_class_entry;

    le_swishe = zend_register_list_destructors_ex(_php_swishe_close, NULL, "swishe_handle", module_number);
    le_swishe_search = zend_register_list_destructors_ex(_php_swishe_search_close, NULL, "swishe_search_handle", module_number);
    le_swishe_results = zend_register_list_destructors_ex(_php_swishe_results_close, NULL, "swishe_results_handle", module_number);
    le_swishe_result = zend_register_list_destructors_ex(_php_swishe_result_close, NULL, "swishe_result_handle", module_number);

    /*
    ZEND_INIT_MODULE_GLOBALS(swishe, php_swishe_init_globals, NULL);
    */
    INIT_CLASS_ENTRY(swishe_class_entry, "swishe", php_swishe_class_functions);
    swishe_class_entry_ptr = zend_register_internal_class(&swishe_class_entry TSRMLS_CC);

    INIT_CLASS_ENTRY(swishe_search_class_entry, "swishe_search", php_swishe_search_class_functions);
    swishe_search_class_entry_ptr = zend_register_internal_class(&swishe_search_class_entry TSRMLS_CC);

    INIT_CLASS_ENTRY(swishe_results_class_entry, "swishe_results", php_swishe_results_class_functions);
    swishe_results_class_entry_ptr = zend_register_internal_class(&swishe_results_class_entry TSRMLS_CC);

    INIT_CLASS_ENTRY(swishe_result_class_entry, "swishe_result", php_swishe_result_class_functions);
    swishe_result_class_entry_ptr = zend_register_internal_class(&swishe_result_class_entry TSRMLS_CC);
    REGISTER_INI_ENTRIES();

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(swishe)
{
    UNREGISTER_INI_ENTRIES();
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(swishe)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "swishe support", "enabled");
    php_info_print_table_end();

    DISPLAY_INI_ENTRIES();
}
/* }}} */


/* Internal functions */

/* Opens a swish-e handle */
static void _php_swishe_open(INTERNAL_FUNCTION_PARAMETERS, int createobject)
{
    int argc = ZEND_NUM_ARGS();
    char *index_file_list = NULL;
    int index_file_list_len = 0;
    php_swishe *swishe;

    if (argc != 1) WRONG_PARAM_COUNT;
    if (zend_parse_parameters( argc TSRMLS_CC, "s", &index_file_list, &index_file_list_len) == FAILURE)
        return;


    swishe = (php_swishe *) emalloc(sizeof(php_swishe));
    swishe->swishe_handle = SwishInit( index_file_list );
    swishe->id = zend_list_insert(swishe,le_swishe);

    if(createobject)
    {
        object_init_ex(return_value, swishe_class_entry_ptr);
        add_property_resource(return_value,"swishe_handle",swishe->id);
    }
    else
    {
        RETURN_RESOURCE(swishe->id);
    }
}

/* {{{ proto string confirm_swishe_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_swishe_compiled)
{
    char *arg = NULL;
    int arg_len, len;
    char string[256];

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) ==
 FAILURE) {
        return;
    }

    len = sprintf(string, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "swishe", arg);
    RETURN_STRINGL(string, len, 1);
}
/* }}} */

/* {{{ proto resource swishe_open()
   SWISHE open */
PHP_FUNCTION(swishe_open)
{
    _php_swishe_open(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}

/* {{{ proto void swishe_close()
   SWISHE close */
PHP_FUNCTION(swishe_close)
{
    if (ZEND_NUM_ARGS() != 1) {
        WRONG_PARAM_COUNT;
    }
    php_error(E_WARNING, "swishe_close: not yet implemented");
}

/* }}} */


/* {{{ proto array swishe_index_names()
   SWISHE index names */
PHP_FUNCTION(swishe_index_names)
{
zval **id, **tmp;
php_swishe *swishe;

const char **index_name;

    if (ZEND_NUM_ARGS() != 0) {
        WRONG_PARAM_COUNT;
    }

    FETCH_SWISHE();
    if(array_init(return_value) != SUCCESS)
    {
        /* do error handling here */
        RETURN_NULL();
    }
    index_name = SwishIndexNames( swishe->swishe_handle );
    while ( *index_name )
    {
       add_next_index_string(return_value,(char *)*index_name,1);
       *index_name++;
    }
}
/* }}} */

/* {{{ proto array swishe_header_names()
   SWISHE index header names */
PHP_FUNCTION(swishe_header_names)
{
zval **id, **tmp;
php_swishe *swishe;

const char **header_name;


    if (ZEND_NUM_ARGS() != 0) {
        WRONG_PARAM_COUNT;
    }

    FETCH_SWISHE();
    if(array_init(return_value) != SUCCESS)
    {
        /* do error handling here */
        RETURN_NULL();
    }

    header_name = SwishHeaderNames( swishe->swishe_handle );
    while ( *header_name )
    {
       add_next_index_string(return_value,(char *)*header_name,1);
       *header_name++;
    }
}
/* }}} */

/* {{{ proto array swishe_header_value(string index_file, string header_name)
   SWISHE index header value */
PHP_FUNCTION(swishe_header_value)
{
zval **id, **tmp;
php_swishe *swishe;

char *index_file = NULL;
char *header_name = NULL;
int index_file_len;
int header_name_len;
int i;
SWISH_HEADER_TYPE  header_type;
SWISH_HEADER_VALUE head_value;
char **string_list;


    if (ZEND_NUM_ARGS() != 2) {
        WRONG_PARAM_COUNT;
    }

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &index_file, &index_file_len, &header_name, &header_name_len) == FAILURE)
        return;

    FETCH_SWISHE();
    head_value = SwishHeaderValue( swishe->swishe_handle, index_file, header_name, &header_type );

    /* Convert to data types and return */
    switch(header_type)
    {
        case SWISH_NUMBER:
            RETURN_LONG(head_value.number);
            break;
        case SWISH_STRING:
            RETURN_STRING((char *)head_value.string,1);
            break;
        case SWISH_LIST:
            if(array_init(return_value) != SUCCESS)
            {
                RETURN_NULL();
            }
            string_list = (char **)head_value.string_list;
            while ( *string_list )
            {
                add_next_index_string(return_value,(char *)*string_list,1);
                string_list++;
            }
            return;
            break;
        case SWISH_BOOL:
            RETURN_BOOL(head_value.boolean?1:0);
            break;
        case SWISH_HEADER_ERROR: /* must check error in this case */
        default:
            RETURN_NULL();
            break;
    }
}
/* }}} */

/* {{{ proto array swishe_stem(string word, string lang)
   SWISHE stem a word */
PHP_FUNCTION(swishe_stem)
{
int argc = ZEND_NUM_ARGS();
zval **id, **tmp;
php_swishe *swishe;

char *word = NULL;
char *lang = NULL;
int word_len;
int lang_len;
char *stem;


    if (argc <1 || argc > 2) {
        WRONG_PARAM_COUNT;
    }

    if (zend_parse_parameters(argc TSRMLS_CC, "s|s", &word, &word_len, &lang, &lang_len) == FAILURE)
        return;

    FETCH_SWISHE();
    stem = SwishStemWord( swishe->swishe_handle, word);

    RETURN_STRING((char *)stem,1);
}
/* }}} */


/* {{{ proto array swishe_swish_words(string word, string lang)
   SWISHE Get swish words */
PHP_FUNCTION(swishe_swish_words_by_letter)
{
int argc = ZEND_NUM_ARGS();
zval **id, **tmp;
php_swishe *swishe;

char *index_file = NULL;
char *letter = NULL;
int index_file_len;
int letter_len;
unsigned char c;
char *Words,*wtmp;
int c2;

    if (argc <1 || argc > 2) {
        WRONG_PARAM_COUNT;
    }

    if (zend_parse_parameters(argc TSRMLS_CC, "s|s", &index_file, &index_file_len, &letter, &letter_len) == FAILURE)
        return;

    if(letter && letter[0])
    {
        c = letter[0];
    }
    else
    {
        c = '*';
    }
    
    FETCH_SWISHE();
    if(array_init(return_value) != SUCCESS)
    {
        RETURN_NULL();
    }
    if(c=='*')
    {
        for(c2=1;c2<256;c2++)
        {
            Words=(char *)SwishWordsByLetter(swishe->swishe_handle,index_file,(unsigned char)c2);
            for(wtmp=Words;wtmp && wtmp[0];wtmp+=strlen(wtmp)+1)
            {
                add_next_index_string(return_value,(char *)wtmp,1);
            }
        }
    } else {
        Words=(char *)SwishWordsByLetter(swishe->swishe_handle,index_file,c);
        for(wtmp=Words;wtmp && wtmp[0];wtmp+=strlen(wtmp)+1)
        {
            add_next_index_string(return_value,(char *)wtmp,1);
        }
    }
}
/* }}} */

/* {{{ proto long swishe_error()
   SWISHE error */
PHP_FUNCTION(swishe_error)
{
zval **id, **tmp;
php_swishe *swishe;
long err;

    if (ZEND_NUM_ARGS() != 0) {
        WRONG_PARAM_COUNT;
    }
    FETCH_SWISHE();
    err = SwishError(swishe->swishe_handle);
    RETURN_LONG(err);
}
/* }}} */

/* {{{ proto bool swishe_critical_error()
   SWISHE critical error */
PHP_FUNCTION(swishe_critical_error)
{
zval **id, **tmp;
php_swishe *swishe;
long err;

    if (ZEND_NUM_ARGS() != 0) {
        WRONG_PARAM_COUNT;
    }
    FETCH_SWISHE();
    err = SwishCriticalError(swishe->swishe_handle);
    RETURN_BOOL(err?1:0);
}
/* }}} */

/* {{{ proto bool swishe_abort_last_error()
   SWISHE aborts and reports error to STDERR */
PHP_FUNCTION(swishe_abort_last_error)
{
    if (ZEND_NUM_ARGS() != 0) {
        WRONG_PARAM_COUNT;
    }
    php_error(E_WARNING, "swishe_abort_last_error: not yet implemented");
}
/* }}} */

/* {{{ proto string swishe_error_string()
   SWISHE error string based on swishe_error  */
PHP_FUNCTION(swishe_error_string)
{
zval **id, **tmp;
php_swishe *swishe;
char *str;

    if (ZEND_NUM_ARGS() != 0) {
        WRONG_PARAM_COUNT;
    }
    FETCH_SWISHE();
    str = SwishErrorString(swishe->swishe_handle);
    RETURN_STRING(str,1);
}
/* }}} */

/* {{{ proto string swishe_last_error_msg()
   SWISHE last error message */
PHP_FUNCTION(swishe_last_error_msg)
{
    if (ZEND_NUM_ARGS() != 0) {
        WRONG_PARAM_COUNT;
    }
    php_error(E_WARNING, "swishe_last_error_msg: not yet implemented");
}
/* }}} */


/* {{{ proto long swishe_query(string query)
   SWISHE short-cut to avoid creation a search object */
PHP_FUNCTION(swishe_query)
{
int argc = ZEND_NUM_ARGS();
zval **id, **tmp;
php_swishe *swishe;
php_swishe_results *results;
char *query = NULL;
int query_len = 0;
void *swishe_search_handle = NULL;

    if (argc > 1) {
        WRONG_PARAM_COUNT;
    }

    if (zend_parse_parameters(argc TSRMLS_CC, "|s", &query, &query_len) == FAILURE)
        return;

    FETCH_SWISHE();
    results = (php_swishe_results *) emalloc(sizeof(php_swishe_results));
    results->search = NULL;
    results->swishe_handle = swishe->swishe_handle;
    swishe_search_handle = New_Search_Object( swishe->swishe_handle, query );
    results->swishe_results_handle = SwishExecute( swishe_search_handle, query );
    results->id = zend_list_insert(results,le_swishe_results);

    object_init_ex(return_value, swishe_results_class_entry_ptr);
    add_property_resource(return_value,"swishe_results_handle",results->id);
}
/* }}} */

/* {{{ proto void swishe_set_query(string query)
   SWISHE set query search */
PHP_FUNCTION(swishe_set_query)
{
int argc = ZEND_NUM_ARGS();
zval **id, **tmp;
php_swishe_search *search;
char *query = NULL;
int query_len = 0;
int rc;

    if (argc != 1) {
        WRONG_PARAM_COUNT;
    }

    if (zend_parse_parameters(argc TSRMLS_CC, "s", &query, &query_len) == FAILURE)
        return;

    FETCH_SEARCH();
    SwishSetQuery(search->swishe_search_handle, query);
}
/* }}} */

/* {{{ proto void swishe_set_structure(long structure)
   SWISHE set structure bits */
PHP_FUNCTION(swishe_set_structure)
{
int argc = ZEND_NUM_ARGS();
zval **id, **tmp;
php_swishe_search *search;
int structure = 0;
int rc;

    if (argc != 1) {
        WRONG_PARAM_COUNT;
    }

    if (zend_parse_parameters(argc TSRMLS_CC, "l", &structure) == FAILURE)
        return;

    FETCH_SEARCH();
    SwishSetStructure(search->swishe_search_handle, structure);
}
/* }}} */

/* {{{ proto void swishe_set_phrase_delimiter()
   char char) SWISHE set phrase delimiter char */
PHP_FUNCTION(swishe_set_phrase_delimiter)
{
int argc = ZEND_NUM_ARGS();
zval **id, **tmp;
php_swishe_search *search;
char *phrase = NULL;
int phrase_len = 0;
int rc;

    if (argc != 1) {
        WRONG_PARAM_COUNT;
    }

    if (zend_parse_parameters(argc TSRMLS_CC, "s", &phrase, &phrase_len) == FAILURE)
        return;

    FETCH_SEARCH();
    if(phrase_len)
    {
        SwishPhraseDelimiter(search->swishe_search_handle, phrase[0]);
    }
}
/* }}} */

/* {{{ proto void swishe_set_search_limit(string property, string low, string hi
gh)
   SWISHE set search limits */
PHP_FUNCTION(swishe_set_search_limit)
{
int argc = ZEND_NUM_ARGS();
zval **id, **tmp;
php_swishe_search *search;
char *property = NULL;
char *low = NULL;
char *high = NULL;
int property_len=0;
int low_len=0;
int high_len=0;
int rc;

    if (argc != 3) {
        WRONG_PARAM_COUNT;
    }

    if (zend_parse_parameters(argc TSRMLS_CC, "sss", &property, &property_len, &low, &low_len, &high, &high_len) == FAILURE)
        return;

    FETCH_SEARCH();
    rc = SwishSetSearchLimit(search->swishe_search_handle, property, low, high);

    RETURN_LONG((long)rc);
}
/* }}} */

/* {{{ proto void swishe_reset_search_limit()
   SWISHE clears search limits */
PHP_FUNCTION(swishe_reset_search_limit)
{
int argc = ZEND_NUM_ARGS();
zval **id, **tmp;
php_swishe_search *search;

    if (argc != 0) {
        WRONG_PARAM_COUNT;
    }

    FETCH_SEARCH();
    SwishResetSearchLimit(search->swishe_search_handle);
}
/* }}} */

/* {{{ proto void swishe_set_sort(string sort_string)
   SWISHE clears search limits */
PHP_FUNCTION(swishe_set_sort)
{
int argc = ZEND_NUM_ARGS();
zval **id, **tmp;
php_swishe_search *search;
char *sort_string = NULL;
int sort_string_len = 0;
int rc;

    if (argc != 1) {
        WRONG_PARAM_COUNT;
    }

    if (zend_parse_parameters(argc TSRMLS_CC, "s", &sort_string, &sort_string_len) == FAILURE)
        return;

    FETCH_SEARCH();
    SwishSetSort(search->swishe_search_handle, sort_string);
}
/* }}} */

/* {{{ proto object swishe_execute([string query])
   SWISHE executes query */
PHP_FUNCTION(swishe_execute)
{
int argc = ZEND_NUM_ARGS();
zval **id, **tmp;
php_swishe_search *search;
php_swishe_results *results;
char *query = NULL;
int query_len = 0;

    if (argc > 1) {
        WRONG_PARAM_COUNT;
    }

    if (zend_parse_parameters(argc TSRMLS_CC, "|s", &query, &query_len) == FAILURE)
        return;

    FETCH_SEARCH();
    results = (php_swishe_results *) emalloc(sizeof(php_swishe_results));
    results->search = search;
    results->swishe_handle = search->swishe_handle;
    results->swishe_results_handle = SwishExecute( search->swishe_search_handle, query );
    results->id = zend_list_insert(results,le_swishe_results);

    object_init_ex(return_value, swishe_results_class_entry_ptr);
    add_property_resource(return_value,"swishe_results_handle",results->id);
}
/* }}} */

/* {{{ proto long swishe_hits()
   returns number of results */
PHP_FUNCTION(swishe_hits)
{
int argc = ZEND_NUM_ARGS();
zval **id, **tmp;
php_swishe_results *results;
long hits;


    if (argc != 0) {
        WRONG_PARAM_COUNT;
    }

    FETCH_RESULTS();
    hits = SwishHits(results->swishe_results_handle);
    RETURN_LONG(hits);
}
/* }}} */

/* {{{ proto array swishe_parsed_words(string index_name)
   returns array of tokenized words and operators  */
PHP_FUNCTION(swishe_parsed_words)
{
int argc = ZEND_NUM_ARGS();
zval **id, **tmp;
php_swishe_results *results;
char *index_name = NULL;
int index_name_len = 0;
SWISH_HEADER_VALUE value;
char **parsed_words;

    if (argc != 1) {
        WRONG_PARAM_COUNT;
    }
    if (zend_parse_parameters(argc TSRMLS_CC, "s", &index_name, &index_name_len) == FAILURE)
        return;

    if(index_name_len)
    {
       FETCH_RESULTS();
       value = SwishParsedWords(results->swishe_results_handle,index_name);
       parsed_words = (char **)value.string_list;
       if(array_init(return_value) != SUCCESS)
       {
           RETURN_NULL();
       }
       while ( *parsed_words )
       {
           add_next_index_string(return_value,(char *)*parsed_words,1);
           parsed_words++;
       }
    }
}
/* }}} */

/* {{{ proto array swishe_removed_stopwords(string index_name)
   returns array of removed stopwords */
PHP_FUNCTION(swishe_removed_stopwords)
{
int argc = ZEND_NUM_ARGS();
zval **id, **tmp;
php_swishe_results *results;
char *index_name = NULL;
int index_name_len = 0;
SWISH_HEADER_VALUE value;
char **removed_stopwords;

    if (argc != 1) {
        WRONG_PARAM_COUNT;
    }
    if (zend_parse_parameters(argc TSRMLS_CC, "s", &index_name, &index_name_len) == FAILURE)
        return;

    if(index_name_len)
    {
       FETCH_RESULTS();
       value = SwishRemovedStopwords(results->swishe_results_handle,index_name);
       removed_stopwords = (char **)value.string_list;
       if(array_init(return_value) != SUCCESS)
       {
           RETURN_NULL();
       }
       while ( *removed_stopwords )
       {
           add_next_index_string(return_value,(char *)*removed_stopwords,1);
           removed_stopwords++;
       }
    }
}
/* }}} */

/* {{{ proto bool swishe_seek_result(long position)
   seeks to a result */
PHP_FUNCTION(swishe_seek_result)
{
int argc = ZEND_NUM_ARGS();
zval **id, **tmp;
php_swishe_results *results;
long position;
int rc;

    if (argc != 1) {
        WRONG_PARAM_COUNT;
    }
    if (zend_parse_parameters(argc TSRMLS_CC, "l", &position) == FAILURE)
        return;

    FETCH_RESULTS();
    rc = SwishSeekResult(results->swishe_results_handle,position);
    RETURN_LONG(rc);
}
/* }}} */

/* {{{ proto object swishe_next_result(void)
   fetches a result and advances pointer */
PHP_FUNCTION(swishe_next_result)
{
int argc = ZEND_NUM_ARGS();
zval **id, **tmp;
php_swishe_results *results;
php_swishe_result *result;
void *r;

    if (argc != 0) {
        WRONG_PARAM_COUNT;
    }

    FETCH_RESULTS();
    if(!(r = SwishNextResult(results->swishe_results_handle)))
    {
        RETURN_NULL();
    }
    result = (php_swishe_result *) emalloc(sizeof(php_swishe_result));
    result->results = results;
    result->swishe_handle = results->swishe_handle;
    result->swishe_result_handle = r;
    result->id = zend_list_insert(result,le_swishe_result);

    object_init_ex(return_value, swishe_result_class_entry_ptr);
    add_property_resource(return_value,"swishe_result_handle",result->id);
}
/* }}} */

/* {{{ proto string swishe_property(string prop_name)
   reads a property from results */
PHP_FUNCTION(swishe_property)
{
int argc = ZEND_NUM_ARGS();
zval **id, **tmp;
php_swishe_result *result;
char *prop_name = NULL;
int prop_name_len;
PropValue *pv;

    if (argc != 1) {
        WRONG_PARAM_COUNT;
    }

    if (zend_parse_parameters(argc TSRMLS_CC, "s", &prop_name, &prop_name_len) == FAILURE)
        return;

    FETCH_RESULT();
    if(!(pv = getResultPropValue( result->swishe_result_handle, prop_name, 0 )))
    {
        RETURN_NULL();
    }
    switch(pv->datatype)
    {
        case PROP_INTEGER:
            RETURN_LONG((long)pv->value.v_int);
            break;
        case PROP_ULONG:
            RETURN_LONG((long)pv->value.v_ulong);
            break;
        case PROP_STRING:
            RETURN_STRING(pv->value.v_str,1);
            break;
        case PROP_DATE:
            RETURN_LONG((long)pv->value.v_date);
            break;
        default:
            RETURN_NULL();
            break;
    }
}
/* }}} */

/* {{{ proto string swishe_result_index_value(string prop_name)
   returns header value based on the result */
PHP_FUNCTION(swishe_result_index_value)
{
int argc = ZEND_NUM_ARGS();
zval **id, **tmp;
php_swishe_result *result;
char *header_name = NULL;
int header_name_len;
SWISH_HEADER_TYPE  header_type;
SWISH_HEADER_VALUE head_value;
char **string_list;

    if (argc != 1) {
        WRONG_PARAM_COUNT;
    }

    if (zend_parse_parameters(argc TSRMLS_CC, "s", &header_name, &header_name_len) == FAILURE)
        return;

    FETCH_RESULT();
    head_value = SwishResultIndexValue( result->swishe_result_handle, header_name, &header_type );
    /* Convert to data types and return */
    switch(header_type)
    {
        case SWISH_NUMBER:
            RETURN_LONG(head_value.number);
            break;
        case SWISH_STRING:
            RETURN_STRING((char *)head_value.string,1);
            break;
        case SWISH_LIST:
            if(array_init(return_value) != SUCCESS)
            {
                RETURN_NULL();
            }
            string_list = (char **)head_value.string_list;
            while ( *string_list )
            {
                add_next_index_string(return_value,(char *)*string_list,1);
                string_list++;
            }
            return;
            break;
        case SWISH_BOOL:
            RETURN_BOOL(head_value.boolean?1:0);
            break;
        case SWISH_HEADER_ERROR: /* must check error in this case */
        default:
            RETURN_NULL();
            break;
    }

}
/* }}} */

/* {{{ proto object swishe(string index_file_list)
   SWISHE class constructor */
PHP_FUNCTION(swishe)
{
    _php_swishe_open(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

/* {{{ proto object swishe_search(string query)
   SWISHE_SEARCH class constructor */
PHP_FUNCTION(swishe_search)
{
int argc = ZEND_NUM_ARGS();
zval **id, **tmp;
php_swishe *swishe;
php_swishe_search *search;
char *query;
int query_len;

    if (argc != 1) {
        WRONG_PARAM_COUNT;
    }

    if (argc != 1) WRONG_PARAM_COUNT;
    if (zend_parse_parameters( argc TSRMLS_CC, "s", &query, &query_len) == FAILURE)
        return;

    FETCH_SWISHE();
    search = (php_swishe_search *) emalloc(sizeof(php_swishe_search));
    search->swishe = swishe;
    search->swishe_handle = swishe->swishe_handle;
    search->swishe_search_handle = New_Search_Object( swishe->swishe_handle, query );
    search->id = zend_list_insert(search,le_swishe_search);

    object_init_ex(return_value, swishe_search_class_entry_ptr);
    add_property_resource(return_value,"swishe_search_handle",search->id);
}
/* }}} */

