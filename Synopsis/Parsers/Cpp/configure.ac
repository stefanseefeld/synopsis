dnl $Id: configure.ac,v 1.3 2003/09/22 15:47:56 stefan Exp $
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
AC_REVISION($Revision: 1.3 $)
AC_INIT(occ, 1.0, synopsis-devel@fresco.org)

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

AC_ARG_ENABLE([gc], 
    [  --disable-gc            don't use garbage collector in C++ parser (default is enabled)],
    [USE_GC=no],[USE_GC=yes])

AC_SUBST(USE_GC)

dnl check for libdl
AC_CHECK_LIB(dl, dlopen, [LIBS="-ldl $LIBS"])

dnl Check host for things like -fPIC and library extension
AC_MSG_CHECKING([for library file extension])
cpp_profile=""
case `uname -s` in
CYGWIN*)
    LIBEXT=dll
    PYTHON_VERSION=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_config_var('VERSION')"`
    PYTHON_PREFIX=`$PYTHON -c "import sys; print sys.prefix"`
dnl The following lines assume we are using python from cygwin, not win32.
dnl Make that more flexible...
    PYTHON_LIBS="-L $PYTHON_PREFIX/lib/python$PYTHON_VERSION/config -lpython$PYTHON_VERSION"
    PYTHON_DEP_LIBS=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_config_var('LIBS') or ''"`
    LIBS="$LIBS $PYTHON_LIBS $PYTHON_DEP_LIBS"
    ;;
*)
    CXXFLAGS="$CXXFLAGS -fPIC"
    LIBEXT=so
    ;;
esac
AC_MSG_RESULT([$LIBEXT])

AC_SUBST(LIBEXT)

AC_CONFIG_FILES([Makefile])

#temporary hack until the file layout can be reworked a bit...
mkdir -p ucpp
mkdir -p occ
mkdir -p syn

AC_OUTPUT
