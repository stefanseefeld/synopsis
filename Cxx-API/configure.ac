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
AC_INIT(Cxx-API, 1.0, synopsis-devel@fresco.org)

AC_ARG_WITH(python, 
  [  --with-python=PATH      specify the Python interpreter],
  PYTHON="$with_python",
  PYTHON=""
)

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

case `uname -s` in
CYGWIN*)
    PYTHON_PREFIX=`$PYTHON -c "import sys; print sys.prefix"`

    if test `$PYTHON -c "import os; print os.name"` = nt; then
      if test "$CXX" = "g++"; then
        CFLAGS="-mno-cygwin $CFLAGS"
        CXXFLAGS="-mno-cygwin $CXXFLAGS"
        LDFLAGS="-mno-cygwin $LDFLAGS"
      fi
      PYTHON_INCLUDE=`cygpath -a $PYTHON_INCLUDE`
    fi
    CXXFLAGS="-D_REENTRANT $CXXFLAGS"
    LDFLAGS="$LDFLAGS"
    LIBS="$LIBS -lpthread"
    ;;
*)
    CXXFLAGS="-pthread $CXXFLAGS -fPIC"
    LDFLAGS="-pthread $LDFLAGS"
    ;;
esac

topdir=`$PYTHON -c "from os.path import *; print normpath('$srcdir/..')"`
AC_SUBST(topdir)

AC_CONFIG_FILES([Makefile synopsis.py])

mkdir -p src
mkdir -p lib

AC_OUTPUT
