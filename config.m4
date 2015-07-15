dnl $Id$
dnl config.m4 for extension zyconf

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(zyconf, for zyconf support,
dnl Make sure that the comment is aligned:
dnl [  --with-zyconf             Include zyconf support])

dnl Otherwise use enable:

 PHP_ARG_ENABLE(zyconf, whether to enable zyconf support,
 Make sure that the comment is aligned:
 [  --enable-zyconf           Enable zyconf support])

if test "$PHP_ZYCONF" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-zyconf -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/zyconf.h"  # you most likely want to change this
  dnl if test -r $PHP_ZYCONF/$SEARCH_FOR; then # path given as parameter
  dnl   ZYCONF_DIR=$PHP_ZYCONF
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for zyconf files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       ZYCONF_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$ZYCONF_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the zyconf distribution])
  dnl fi

  dnl # --with-zyconf -> add include path
  dnl PHP_ADD_INCLUDE($ZYCONF_DIR/include)

  dnl # --with-zyconf -> check for lib and symbol presence
  dnl LIBNAME=zyconf # you may want to change this
  dnl LIBSYMBOL=zyconf # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $ZYCONF_DIR/$PHP_LIBDIR, ZYCONF_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_ZYCONFLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong zyconf lib version or lib not found])
  dnl ],[
  dnl   -L$ZYCONF_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(ZYCONF_SHARED_LIBADD)

  PHP_NEW_EXTENSION(zyconf, zyconf.c, $ext_shared)
fi

if test -z "$PHP_DEBUG" ; then
   AC_ARG_ENABLE(debug,
    [--enable-debug compile with debugging system],
    [PHP_DEBUG=$enableval],[PHP_DEBUG=no]
    )
fi

