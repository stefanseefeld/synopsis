dnl $Id: configure.ac,v 1.10 2003/12/30 19:32:21 stefan Exp $
dnl
dnl This source file is a part of the Synopsis Project.
dnl Copyright (C) 2003 Stefan Seefeld <stefan@fresco.org> 
dnl http://synopsis.fresco.org
dnl
dnl This library is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU Library General Public
dnl License as published by the Free Software Foundation; either
dnl version 2 of the License, or (at your option) any later version.
dnl
dnl This library is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl Library General Public License for more details.
dnl
dnl You should have received a copy of the GNU Library General Public
dnl License along with this library; if not, write to the
dnl Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
dnl MA 02139, USA.
dnl
dnl Process this file with autoconf to produce a configure script.

dnl ------------------------------------------------------------------
dnl Autoconf initialization
dnl ------------------------------------------------------------------
AC_PREREQ(2.56)
AC_REVISION($Revision: 1.10 $)
AC_INIT(ucpp, 1.0, synopsis-devel@fresco.org)

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

AC_CONFIG_FILES([Makefile])

mkdir -p ucpp

AC_OUTPUT
