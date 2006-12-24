dnl
dnl Copyright (C) 2003 Stefan Seefeld
dnl All rights reserved.
dnl Licensed to the public under the terms of the GNU LGPL (>= 2),
dnl see the file COPYING for details.
dnl

AC_PREREQ(2.56)
AC_REVISION($Revision: 1.1 $)
AC_INIT(tests, 1.0, synopsis-devel@fresco.org)

AC_ARG_WITH(python, 
  [  --with-python=PATH      specify the Python interpreter],
  PYTHON="$with_python",
  PYTHON="python"
)

AC_ARG_WITH(syn-cxx,
  [  --with-syn-cxx=PATH     specify the Synopsis C++ library prefix],
  SYN_CXX="$with_syn_cxx",
  SYN_CXX="/usr/local"
)

AC_PROG_CXX

SYN_CPP="`PKG_CONFIG_PATH=$SYN_CXX pkg-config --cflags Synopsis-uninstalled`"
SYN_LIBS="`PKG_CONFIG_PATH=$SYN_CXX pkg-config --libs Synopsis-uninstalled`"
CPPFLAGS="$CPPFLAGS $SYN_CPP"
LIBS="$LIBS $SYN_LIBS"

if test -n "$PYTHON" -a "$PYTHON" != yes; then
  AC_CHECK_PROG(PYTHON, $PYTHON, AC_MSG_ERROR([Cannot find Python interpreter]))
else
  AC_PATH_PROG(PYTHON, python2 python, python)
fi
PYTHON_PREFIX=`$PYTHON -c "import sys; print sys.prefix"`
PYTHON_INCLUDE=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_python_inc()"`
PYTHON_VERSION=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_config_var('VERSION')"`
PYTHON_LIBS="-L $PYTHON_PREFIX/lib/python$PYTHON_VERSION/config -lpython$PYTHON_VERSION"
PYTHON_DEP_LIBS=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_config_var('LIBS') or ''"`

case `uname -s` in
CYGWIN*)
    if test `$PYTHON -c "import os; print os.name"` = posix; then
dnl Cygwin doesn't have an -lutil, but some versions of distutils tell us to use it anyway.
dnl It would be better to check for each library it tells us to use with AC_CHECK_LIB, but
dnl to do that, we need the name of a function in each one, so we'll just hack -lutil out 
dnl of the list.
      PYTHON_DEP_LIBS=`$PYTHON -c "from distutils import sysconfig; import re; print re.sub(r'\\s*-lutil', '', sysconfig.get_config_var('LIBS') or '')"`
    else dnl this is 'nt'
      if test "$CXX" = "g++"; then
        CXXFLAGS="-mno-cygwin $CXXFLAGS"
        LDFLAGS="-mno-cygwin $LDFLAGS"
      fi
      PYTHON_INCLUDE=`cygpath -a $PYTHON_INCLUDE`
      PYTHON_DEP_LIBS=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_config_var('LIBS') or ''"`
    fi
    CXXFLAGS="-D_REENTRANT $CXXFLAGS"
    LDFLAGS="-rdynamic $LDFLAGS"
    ;;
*)
    LDFLAGS="-rdynamic $LDFLAGS"
    ;;
esac

CPPFLAGS="-I $PYTHON_INCLUDE $CPPFLAGS"
LIBS="$LIBS $PYTHON_LIBS $PYTHON_DEP_LIBS"

dnl find all template files and strip off the $srcdir and .in suffix
templates=`find $srcdir -name '*.in' -print | sed -e "s,^$srcdir/,,g" -e "s,\.in$,,g"`

AC_CONFIG_FILES([$templates])

mkdir -p Cxx/src
mkdir -p Cxx/bin

AC_OUTPUT

