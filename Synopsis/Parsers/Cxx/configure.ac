dnl
dnl Copyright (C) 2003 Stefan Seefeld
dnl All rights reserved.
dnl Licensed to the public under the terms of the GNU LGPL (>= 2),
dnl see the file COPYING for details.
dnl

dnl ------------------------------------------------------------------
dnl Autoconf initialization
dnl ------------------------------------------------------------------
AC_PREREQ(2.56)
AC_REVISION($Revision: 1.9 $)
AC_INIT(occ, 1.0, synopsis-devel@fresco.org)

AC_ARG_WITH(python, 
  [  --with-python=PATH      specify the Python interpreter],
  PYTHON="$with_python",
  PYTHON=""
)

AC_ARG_WITH(gc_prefix, 
  [  --with-gc-prefix=PATH   specify the prefix to the garbage collector],
  GC_PREFIX="$with_gc_prefix",
  GC_PREFIX=""
)
AC_SUBST(GC_PREFIX)

AC_PROG_CPP
AC_PROG_CC
AC_PROG_CXX

if test -n "$PYTHON" -a "$PYTHON" != yes; then
  AC_CHECK_FILE($PYTHON,,AC_MSG_ERROR([Cannot find Python interpreter]))
else
  AC_PATH_PROG(PYTHON, python2 python, python)
fi
PYTHON_INCLUDE=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_python_inc()"`

AC_SUBST(PYTHON)
AC_SUBST(PYTHON_INCLUDE)

AC_ARG_ENABLE([gc], 
    [  --disable-gc            don't use garbage collector in C++ parser (default is enabled)],
    [USE_GC=no],[USE_GC=yes])

AC_SUBST(USE_GC)

LIBEXT=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_config_var('SO')"`
case `uname -s` in
CYGWIN*)
    PYTHON_PREFIX=`$PYTHON -c "import sys; print sys.prefix"`

    if test `$PYTHON -c "import os; print os.name"` = posix; then
      PYTHON_VERSION=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_config_var('VERSION')"`
      PYTHON_LIBS="-L $PYTHON_PREFIX/lib/python$PYTHON_VERSION/config -lpython$PYTHON_VERSION"
dnl Cygwin doesn't have an -lutil, but some versions of distutils tell us to use it anyway.
dnl It would be better to check for each library it tells us to use with AC_CHECK_LIB, but
dnl to do that, we need the name of a function in each one, so we'll just hack -lutil out 
dnl of the list.
      PYTHON_DEP_LIBS=`$PYTHON -c "from distutils import sysconfig; import re; print re.sub(r'\\s*-lutil', '', sysconfig.get_config_var('LIBS') or '')"`
    else dnl this is 'nt'
      if test "$CXX" = "g++"; then
        CFLAGS="-mno-cygwin $CFLAGS"
        CXXFLAGS="-mno-cygwin $CXXFLAGS"
        LDFLAGS="-mno-cygwin $LDFLAGS"
      fi
      PYTHON_INCLUDE=`cygpath -a $PYTHON_INCLUDE`
      PYTHON_DEP_LIBS=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_config_var('LIBS') or ''"`
    fi
    LIBS="$LIBS $PYTHON_LIBS $PYTHON_DEP_LIBS"
    ;;
*)
    CXXFLAGS="$CXXFLAGS -fPIC"
    ;;
esac

AC_SUBST(LIBEXT)

topdir=`$PYTHON -c "from os.path import *; print normpath('$srcdir/../../..')"`
AC_SUBST(topdir)

AC_CONFIG_FILES([Makefile synopsis.py])

#temporary hack until the file layout can be reworked a bit...
mkdir -p ucpp
mkdir -p occ
mkdir -p syn

AC_OUTPUT
