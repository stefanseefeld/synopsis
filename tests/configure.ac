dnl
dnl Copyright (C) 2003 Stefan Seefeld
dnl All rights reserved.
dnl Licensed to the public under the terms of the GNU LGPL (>= 2),
dnl see the file COPYING for details.
dnl

AC_PREREQ(2.56)
AC_REVISION($Revision: 1.1 $)
AC_INIT(tests, 1.0, synopsis-devel@fresco.org)

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

AC_PYTHON_EXT

dnl find all template files and strip off the $srcdir and .in suffix
templates=`find $srcdir -name '*.in' -print | sed -e "s,^$srcdir/,,g" -e "s,\.in$,,g"`

AC_CONFIG_FILES([$templates])

mkdir -p Cxx/src
mkdir -p Cxx/bin

AC_OUTPUT

