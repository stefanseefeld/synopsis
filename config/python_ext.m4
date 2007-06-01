#
# Copyright (C) 2007 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

AC_DEFUN([AC_PYTHON_EXT],
[AC_MSG_CHECKING(for python extension module build information)
AC_MSG_RESULT([])

AC_ARG_WITH(python, 
  [  --with-python=PATH      specify the Python interpreter],
  [PYTHON="$with_python"],
  [PYTHON="python"]
)

if test -n "$PYTHON" -a "$PYTHON" != yes; then
  AC_CHECK_PROG(PYTHON, $PYTHON, AC_MSG_ERROR([Cannot find Python interpreter]))
else
  AC_PATH_PROG(PYTHON, python2 python, python)
fi
PYTHON_INCLUDE=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_python_inc()" | tr -d "\r"`

AC_SUBST(PYTHON)
AC_SUBST(PYTHON_INCLUDE)

LIBEXT=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_config_var('SO')" | tr -d "\r"`
case `uname -s` in
CYGWIN*)
    PYTHON_PREFIX=`$PYTHON -c "import sys; print sys.prefix" | tr -d "\r"`

    if test `$PYTHON -c "import os; print os.name" | tr -d "\r"` = posix; then
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
        PYTHON_VERSION=`$PYTHON -c "import sys; print '%d%d'%(sys.version_info[[0]],sys.version_info[[1]])" | tr -d "\r"`
        PYTHON_LIBS="-L `cygpath -a $PYTHON_PREFIX | tr -d \"\r\"`/Libs -lpython$PYTHON_VERSION"
      fi
      PYTHON_INCLUDE=`cygpath -a $PYTHON_INCLUDE | tr -d "\r"`
      PYTHON_DEP_LIBS=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_config_var('LIBS') or ''" | tr -d "\r"`
    fi
    LDSHARED="$CXX -shared"
    CXXFLAGS="-D_REENTRANT $CXXFLAGS"
    LIBS="$LIBS $PYTHON_LIBS $PYTHON_DEP_LIBS"
    ;;
Darwin)
    LDSHARED="$CXX -dynamiclib"
    CXXFLAGS="$CXXFLAGS -fPIC"
    ;;
*)
    LDSHARED="$CXX -shared"
    CXXFLAGS="$CXXFLAGS -fPIC"
    ;;
esac

AC_SUBST(LIBEXT)
AC_SUBST(LDSHARED)

])dnl
