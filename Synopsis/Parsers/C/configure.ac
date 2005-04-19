dnl
dnl Copyright (C) 2004 Stefan Seefeld
dnl All rights reserved.
dnl Licensed to the public under the terms of the GNU LGPL (>= 2),
dnl see the file COPYING for details.
dnl

dnl ------------------------------------------------------------------
dnl Autoconf initialization
dnl ------------------------------------------------------------------
AC_PREREQ(2.56)
AC_REVISION($Revision: 1.4 $)
AC_INIT(Synopsis, 1.0, synopsis-devel@fresco.org)

AC_ARG_WITH(python, 
  [  --with-python=PATH      specify the Python interpreter],
  [PYTHON="$with_python"],
  [PYTHON="python"]
)

AC_ARG_WITH(syn-cxx,
  [  --with-syn-cxx=PATH     specify the Synopsis C++ library prefix],
  SYN_CXX="$with_syn_cxx",
  SYN_CXX="/usr/local"
)

AC_PROG_CPP
AC_PROG_CC
AC_PROG_CXX

SYN_CPP="`PKG_CONFIG_PATH=$SYN_CXX pkg-config --cflags Synopsis-uninstalled`"
SYN_LIBS="`PKG_CONFIG_PATH=$SYN_CXX pkg-config --libs Synopsis-uninstalled`"
AC_SUBST(SYN_CPP)
AC_SUBST(SYN_LIBS)

if test -n "$PYTHON" -a "$PYTHON" != yes; then
  AC_CHECK_FILE($PYTHON,,AC_MSG_ERROR([Cannot find Python interpreter]))
else
  AC_PATH_PROG(PYTHON, python2 python, python)
fi
PYTHON_INCLUDE=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_python_inc()"`

AC_SUBST(PYTHON)
AC_SUBST(PYTHON_INCLUDE)

LIBEXT=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_config_var('SO')"`
case `uname -s` in
CYGWIN*)
    PYTHON_PREFIX=`$PYTHON -c "import sys; print sys.prefix"`

    if test `$PYTHON -c "import os; print os.name"` = posix; then
      PYTHON_VERSION=`$PYTHON -c "import sys; print '%d.%d'%(sys.version_info[[0]],sys.version_info[[1]])"`
      PYTHON_LIBS="-L $PYTHON_PREFIX/lib/python$PYTHON_VERSION/config -lpython$PYTHON_VERSION"
dnl Cygwin doesn't have an -lutil, but some versions of distutils tell us to use it anyway.
dnl It would be better to check for each library it tells us to use with AC_CHECK_LIB, but
dnl to do that, we need the name of a function in each one, so we'll just hack -lutil out 
dnl of the list.
      PYTHON_DEP_LIBS=`$PYTHON -c "from distutils import sysconfig; import re; print re.sub(r'\\s*-lutil', '', sysconfig.get_config_var('LIBS') or '')"`
    else dnl this is 'nt'
      if test "$CXX" = "g++"; then
        CPPFLAGS="$CPPFLAGS -D PARSE_MSVC"
        CFLAGS="-mno-cygwin $CFLAGS"
        CXXFLAGS="-mno-cygwin $CXXFLAGS"
        LDFLAGS="-mno-cygwin $LDFLAGS"
        PYTHON_VERSION=`$PYTHON -c "import sys; print '%d%d'%(sys.version_info[[0]],sys.version_info[[1]])"`
        PYTHON_LIBS="-L `cygpath -a $PYTHON_PREFIX`/Libs -lpython$PYTHON_VERSION"
      fi
      PYTHON_INCLUDE=`cygpath -a $PYTHON_INCLUDE`
      PYTHON_DEP_LIBS=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_config_var('LIBS') or ''"`
    fi
    CXXFLAGS="-D_REENTRANT $CXXFLAGS"
    LIBS="$LIBS $PYTHON_LIBS $PYTHON_DEP_LIBS -lpthread"
    ;;
*)
    CXXFLAGS="-pthread $CXXFLAGS -fPIC"
    LDFLAGS="-pthread $LDFLAGS"
    ;;
esac

AC_SUBST(LIBEXT)

AC_CONFIG_FILES([Makefile synopsis.py])

AC_OUTPUT
