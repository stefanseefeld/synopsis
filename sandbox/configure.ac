dnl
dnl Copyright (C) 2005 Stefan Seefeld
dnl All rights reserved.
dnl Licensed to the public under the terms of the GNU LGPL (>= 2),
dnl see the file COPYING for details.
dnl

dnl ------------------------------------------------------------------
dnl Autoconf initialization
dnl ------------------------------------------------------------------
AC_PREREQ(2.56)
AC_REVISION($Revision: 1.9 $)
AC_INIT(sandbox, 1.0, synopsis-devel@fresco.org)

AC_ARG_WITH(boost-prefix,
  [  --with-boost-prefix=PATH      specify the boost installation prefix],
  BOOST_PREFIX="$with_boost_prefix",
  BOOST_PREFIX="/usr"
)

AC_ARG_WITH(boost-version,
  [  --with-boost-version=VERSION      specify the boost version],
  BOOST_VERSION="$with_boost_version",
  BOOST_VERSION="1.33"
)

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

AC_LANG(C++)
if test -n "$with_boost_prefix"; then
  BOOST_CPPFLAGS="-I$with_boost_prefix/include"
  BOOST_LDFLAGS="-L$with_boost_prefix/lib"
fi
save_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
AC_CHECK_HEADER([boost/wave.hpp], [], [AC_MSG_ERROR([boost.wave could not be found])])
CPPFLAGS="$save_CPPFLAGS"
dnl save_LIBS=$LIBS
dnl LIBS="$LIBS $BOOST_LDFLAGS -lboost_wave"
dnl AC_CHECK_LIB(boost_wave, boost::wave::wave_init)
AC_SUBST(BOOST_CPPFLAGS)
AC_SUBST(BOOST_LDFLAGS)

dnl find all template files and strip off the $srcdir and .in suffix
templates=`find $srcdir -name '*.in' -print | sed -e "s,^$srcdir/,,g" -e "s,\.in$,,g"`

AC_CONFIG_FILES([$templates])

AC_OUTPUT
