dnl $Id$
dnl config.m4 for extension ptask

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(ptask, for ptask support,
dnl Make sure that the comment is aligned:
dnl [  --with-ptask             Include ptask support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(ptask, whether to enable ptask support,
dnl Make sure that the comment is aligned:
[  --enable-ptask           Enable ptask support])

if test "$PHP_PTASK" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-ptask -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/ptask.h"  # you most likely want to change this
  dnl if test -r $PHP_PTASK/$SEARCH_FOR; then # path given as parameter
  dnl   PTASK_DIR=$PHP_PTASK
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for ptask files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       PTASK_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$PTASK_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the ptask distribution])
  dnl fi

  dnl # --with-ptask -> add include path
  PHP_ADD_INCLUDE(/usr/local/include)

  dnl # --with-ptask -> check for lib and symbol presence
  dnl LIBNAME=ptask # you may want to change this
  dnl LIBSYMBOL=ptask # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $PTASK_DIR/lib, PTASK_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH(task, /usr/local/lib, PTASK_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_PTASKLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong ptask lib version or lib not found])
  dnl ],[
  dnl   -L$PTASK_DIR/lib -lm
  dnl ])
  dnl
  PHP_SUBST(PTASK_SHARED_LIBADD)

  PHP_NEW_EXTENSION(ptask, ptask.c, $ext_shared)
fi
