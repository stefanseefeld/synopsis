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
AC_REVISION(1)
AC_INIT(documentation, 1.0, synopsis-devel@fresco.org)

AC_ARG_WITH(python, 
  [  --with-python=PATH      specify the Python interpreter],
  PYTHON="$with_python",
  PYTHON="python"
)

if test -n "$PYTHON" -a "$PYTHON" != yes; then
  AC_CHECK_PROG(PYTHON, $PYTHON, AC_MSG_ERROR([Cannot find Python interpreter]))
else
  AC_PATH_PROG(PYTHON, python2 python, python)
fi

AC_SUBST(PYTHON)

topdir=`$PYTHON -c "from os.path import *; print normpath(join('$srcdir', '..'))"`
AC_SUBST(topdir)

AC_CONFIG_FILES([Tutorial/Makefile Manual/Makefile Manual/synopsis.py])

AC_OUTPUT
