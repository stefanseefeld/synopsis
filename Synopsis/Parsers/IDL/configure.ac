dnl
dnl Copyright (C) 2006 Stefan Seefeld
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
AC_CONFIG_HEADERS(acconfig.h)

AC_ARG_WITH(syn-cxx,
  [  --with-syn-cxx=PATH     specify the Synopsis C++ library prefix],
  SYN_CXX="$with_syn_cxx",
  SYN_CXX="/usr/local"
)

AC_PROG_CPP
AC_PROG_CC
AC_PROG_CXX

dnl ** Integer representation

AC_C_BIGENDIAN
AC_CHECK_SIZEOF(char, 1)
AC_CHECK_SIZEOF(unsigned char, 1)
AC_CHECK_SIZEOF(bool, 1)
AC_CHECK_SIZEOF(short, 2)
AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(long, 4)
AC_CHECK_SIZEOF(long long, 8)
AC_CHECK_SIZEOF(float, 4)
AC_CHECK_SIZEOF(double, 8)
AC_CHECK_SIZEOF(long double, 16)
AC_CHECK_SIZEOF(wchar_t, 4)
AC_CHECK_SIZEOF(void*, 4)
AC_CHECK_FUNCS(strcasecmp)

AC_PYTHON_EXT

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
