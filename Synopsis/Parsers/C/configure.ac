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

AC_ARG_WITH(syn-cxx,
  [  --with-syn-cxx=PATH     specify the Synopsis C++ library prefix],
  SYN_CXX="$with_syn_cxx",
  SYN_CXX="/usr/local"
)

AC_PROG_CPP
AC_PROG_CC
AC_PROG_CXX

SYN_CPP="`PKG_CONFIG_PATH=$SYN_CXX pkg-config --cflags synopsis-uninstalled`"
SYN_LIBS="`PKG_CONFIG_PATH=$SYN_CXX pkg-config --libs synopsis-uninstalled`"
AC_SUBST(SYN_CPP)
AC_SUBST(SYN_LIBS)

AC_PYTHON_EXT

AC_CONFIG_FILES([Makefile synopsis.py])

AC_OUTPUT
