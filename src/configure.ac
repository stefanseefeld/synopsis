dnl
dnl Copyright (C) 2003 Stefan Seefeld
dnl All rights reserved.
dnl Licensed to the public under the terms of the GNU LGPL (>= 2),
dnl see the file COPYING for details.
dnl

dnl ------------------------------------------------------------------
dnl Autoconf initialization
dnl ------------------------------------------------------------------
AC_PREREQ(2.56)
AC_REVISION($Revision: 1.9 $)
AC_INIT(libSynopsis, 0.8, synopsis-devel@fresco.org)

AC_ARG_ENABLE([gc], 
  [  --disable-gc            don't use garbage collector (default is enabled)],
  [ac_cv_enable_gc=no],
  [ac_cv_enable_gc=yes])

AC_ARG_WITH([gc_prefix],
  [  --with-gc-prefix=PATH   specify the prefix to the garbage collector],
  [ac_cv_with_gc_prefix="$with_gc_prefix"],
  [ac_cv_with_gc_prefix=""])


AC_PROG_CPP
AC_PROG_CC
AC_PROG_CXX

if test "$ac_cv_enable_gc" == "yes"; then
  GC_BRIDGE=$srcdir/Synopsis/PTree/GC-enabled.tmpl
  if test -n "$ac_cv_with_gc_prefix"; then
    CPPFLAGS="$CPPFLAGS -I$ac_cv_with_gc_prefix/include/gc"
    LIBS="-L$ac_cv_with_gc_prefix/lib -lgc $LIBS"
  else
    CPPFLAGS="$CPPFLAGS -I$srcdir/Synopsis/gc/include"
    GC_LIB="Synopsis/gc/.libs/libgc.a"
    AC_SUBST(GC_LIB)
  fi
else
  GC_BRIDGE=$srcdir/Synopsis/PTree/GC-disabled.tmpl
fi

AC_SUBST_FILE(GC_BRIDGE)

LIBEXT=".so"
case `uname -s` in
CYGWIN*)
    CXXFLAGS="-D_REENTRANT $CXXFLAGS"
    LIBS="$LIBS -lpthread"
    ;;
*)
    CXXFLAGS="-pthread $CXXFLAGS -fPIC"
    LDFLAGS="-pthread $LDFLAGS"
    ;;
esac

AC_SUBST(LIBEXT)

AC_CONFIG_FILES([Synopsis/PTree/GC.hh
                 Makefile 
                 synopsis.py 
                 Synopsis.pc])

mkdir -p lib
mkdir -p bin
mkdir -p Synopsis/gc
mkdir -p Synopsis/PTree
mkdir -p Synopsis/SymbolLookup
mkdir -p tools

AC_OUTPUT
