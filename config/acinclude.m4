# ENABLE_DEFINE( variable, define variable, help message)
# Provides an --enable-foo option and a way to set a acconfig.h define switch
# e.g. ENABLE_DEFINE([memdebug], [MEM_DEBUG], [Memory Debugging])
#----------------------------------------------------------------

AC_DEFUN([ENABLE_DEFINE],
        [ AC_MSG_CHECKING([config option $1 for setting $2])
        AC_ARG_ENABLE([$1],
                AC_HELP_STRING([--enable-$1], [$3]),
                ,
                enableval=no)
        AC_MSG_RESULT($enableval)
        if test x"$enableval" != xno ; then
                AC_DEFINE([$2], 1, [$3])
        fi])


