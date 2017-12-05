dnl $Id$
dnl config.m4 for extension nplc

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(nplc, for nplc support,
dnl Make sure that the comment is aligned:
dnl [  --with-nplc             Include nplc support])

dnl Otherwise use enable:

 PHP_ARG_ENABLE(nplc, whether to enable nplc support,
 Make sure that the comment is aligned:
 [  --enable-nplc           Enable nplc support])

NPLC_FILES="nplc.c storage/npl_cache.c storage/npl_storage.c serializer/nplc_serializer.c log/log.c"
NPLC_FILES="${NPLC_FILES} storage/fastlz.c"

if test "$PHP_NPLC" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-nplc -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/nplc.h"  # you most likely want to change this
  dnl if test -r $PHP_NPLC/$SEARCH_FOR; then # path given as parameter
  dnl   NPLC_DIR=$PHP_NPLC
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for nplc files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       NPLC_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$NPLC_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the nplc distribution])
  dnl fi

  dnl # --with-nplc -> add include path
  dnl PHP_ADD_INCLUDE($NPLC_DIR/include)

  dnl # --with-nplc -> check for lib and symbol presence
  dnl LIBNAME=nplc # you may want to change this
  dnl LIBSYMBOL=nplc # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $NPLC_DIR/$PHP_LIBDIR, NPLC_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_NPLCLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong nplc lib version or lib not found])
  dnl ],[
  dnl   -L$NPLC_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(NPLC_SHARED_LIBADD)

  PHP_NEW_EXTENSION(nplc, ${NPLC_FILES}, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
  PHP_ADD_BUILD_DIR([$ext_builddir/storage])
  PHP_ADD_BUILD_DIR([$ext_builddir/serializer])
  PHP_ADD_BUILD_DIR([$ext_builddir/log])
fi
