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
AC_REVISION($Revision: 1.10 $)
AC_INIT(Cpp, 1.0, synopsis-devel@fresco.org)

AC_PROG_CPP
AC_PROG_CC
AC_PROG_CXX

AC_PYTHON_EXT
CPPFLAGS="$CPPFLAGS -I$PYTHON_INCLUDE"

AC_LANG(C++)
AC_BOOST([1.44])
SYN_BOOST_LIB_FILESYSTEM
SYN_BOOST_LIB_WAVE
SYN_BOOST_LIB_PYTHON

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
