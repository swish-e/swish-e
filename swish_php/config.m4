dnl $Id$
dnl config.m4 for extension swishe

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

PHP_ARG_WITH(swishe, for swishe support,
[  --with-swishe=DIR             Include swishe support])

dnl Otherwise use enable:

if test "$PHP_SWISHE" != "no"; then
  SEARCH_FOR="libswish-e.a"  # you most likely want to change this
  if test -r $PHP_SWISHE/; then # path given as parameter
    SWISHE_DIR=$PHP_SWISHE
    AC_MSG_RESULT(found in $SWISHE_DIR)
  else # search default path list
    AC_MSG_ERROR([correct path to swish-e must be suplied in --with-swishe])
  fi
  dnl
  if test -z "$SWISHE_DIR"; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Please reinstall the swish-e distribution or fix path in --with-swishe])
  else
    if test -z "$SWISHE_DIR/include/swish.h"; then
      AC_MSG_RESULT([swish.h not found])
      AC_MSG_ERROR([Please install the swish-e distribution or give a proper path])
    else
      if test -z "$SWISHE_DIR/lib/libswish-e.a"; then
        AC_MSG_RESULT([libswish-e.a not found])
        AC_MSG_ERROR([Please build the swish-e distribution or give a proper path])
      fi
    fi
  fi

  dnl # --with-swishe -> add include path
  PHP_ADD_INCLUDE($SWISHE_DIR/include)

  dnl # --with-swishe -> chech for lib 

  PHP_SUBST(SWISHE_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH(swish-e, $SWISHE_DIR/lib, SWISHE_SHARED_LIBADD)
  AC_DEFINE(HAVE_SWISHELIB,1,[ ])
 

  PHP_EXTENSION(swishe, $ext_shared)
fi
