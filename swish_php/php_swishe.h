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
   | Authors: Jose Ruiz (jmruiz@boe.es)                                   |
   +----------------------------------------------------------------------+
 */

#ifndef PHP_SWISHE_H
#define PHP_SWISHE_H

extern zend_module_entry swishe_module_entry;
#define phpext_swishe_ptr &swishe_module_entry

#ifdef PHP_WIN32
#define PHP_SWISHE_API __declspec(dllexport)
#else
#define PHP_SWISHE_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#include <swish-e.h>

#define PHP_SWISHE_VERSION "0.1"

PHP_MINIT_FUNCTION(swishe);
PHP_MSHUTDOWN_FUNCTION(swishe);
PHP_MINFO_FUNCTION(swishe);

PHP_FUNCTION(confirm_swishe_compiled);      /* For testing, remove later. */
PHP_FUNCTION(swishe_open);
PHP_FUNCTION(swishe_close);
PHP_FUNCTION(swishe_index_names);
PHP_FUNCTION(swishe_header_names);
PHP_FUNCTION(swishe_header_value);
PHP_FUNCTION(swishe_stem);
PHP_FUNCTION(swishe_swish_words_by_letter);
PHP_FUNCTION(swishe_error);
PHP_FUNCTION(swishe_critical_error);
PHP_FUNCTION(swishe_abort_last_error);
PHP_FUNCTION(swishe_error_string);
PHP_FUNCTION(swishe_last_error_msg);
PHP_FUNCTION(swishe_query);
PHP_FUNCTION(swishe_set_query);
PHP_FUNCTION(swishe_set_structure);
PHP_FUNCTION(swishe_set_phrase_delimiter);
PHP_FUNCTION(swishe_set_search_limit);
PHP_FUNCTION(swishe_reset_search_limit);
PHP_FUNCTION(swishe_set_sort);
PHP_FUNCTION(swishe_execute);
PHP_FUNCTION(swishe_hits);
PHP_FUNCTION(swishe_parsed_words);
PHP_FUNCTION(swishe_removed_stopwords);
PHP_FUNCTION(swishe_seek_result);
PHP_FUNCTION(swishe_next_result);
PHP_FUNCTION(swishe_property);
PHP_FUNCTION(swishe_result_index_value);
PHP_FUNCTION(swishe);
PHP_FUNCTION(swishe_search);
PHP_FUNCTION(swishe_results);

/*
ZEND_BEGIN_MODULE_GLOBALS(swishe)
ZEND_END_MODULE_GLOBALS(swishe)
*/

/* In every utility function you add that needs to use variables
   in php_swishe_globals, call TSRM_FETCH(); after declaring other
   variables used by that function, or better yet, pass in TSRMG_CC
   after the last function argument and declare your utility function
   with TSRMG_DC after the last declared argument.  Always refer to
   the globals in your function as SWISHE_G(variable).  You are
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define SWISHE_G(v) TSRMG(swishe_globals_id, zend_swishe_globals *, v)
#else
#define SWISHE_G(v) (swishe_globals.v)
#endif

#endif  /* PHP_SWISHE_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */

