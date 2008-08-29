# Copyright (C) 2007 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.

#
# The code in this file got heavily inspired from monotone's m4 code.
#



#   AC_BOOST([MINIMUM-VERSION])
#
#   Test for the Boost C++ libraries of a particular version (or newer)
#
#   If no path to the installed boost library is given the macro
#   searchs under /usr, /usr/local, and /opt, and evaluates the
#   $BOOST_ROOT environment variable.
#
#   This macro substitutes:
#
#     BOOST_VERSION, BOOST_CPPFLAGS, and BOOST_LDFLAGS
#
#   and sets:
#
#     HAVE_BOOST
#
AC_DEFUN([AC_BOOST],
[
AC_ARG_WITH([boost_prefix],
	    AS_HELP_STRING([--with-boost-prefix=PREFIX],
                           [specify boost installation prefix]))
AC_ARG_WITH([boost_version],
	    AS_HELP_STRING([--with-boost-version=VERSION],
                           [specify boost version]),
  [boost_version="$withval"],[boost_version=$1])

boost_version_req=ifelse([$boost_version], ,1.35.0,$boost_version)
boost_version_req_shorten=`expr $boost_version_req : '\([[0-9]]*\.[[0-9]]*\)'`
boost_version_req_major=`expr $boost_version_req : '\([[0-9]]*\)'`
boost_version_req_minor=`expr $boost_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
boost_version_req_sub_minor=`expr $boost_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
if test "x$boost_version_req_sub_minor" = "x" ; then
  boost_version_req_sub_minor="0"
fi
REQ_BOOST_VERSION=`expr $boost_version_req_major \* 100000 \+  $boost_version_req_minor \* 100 \+ $boost_version_req_sub_minor`
AC_MSG_CHECKING(for boost >= $boost_version_req)
succeeded=no

dnl first we check the system location for boost libraries
dnl this location ist chosen if boost libraries are installed 
dnl with the --layout=system option.
if test "$with_boost_prefix" != ""; then
  BOOST_LDFLAGS="-L$with_boost_prefix/lib"
  BOOST_CPPFLAGS="-I$with_boost_prefix/include"
else
  for prefix in /usr /usr/local /opt ; do
    if test -d "$prefix/include/boost" && test -r "$prefix/include/boost"; then
      BOOST_LDFLAGS="-L$prefix/lib"
      BOOST_CPPFLAGS="-I$prefix/include"
      break;
    fi
  done
fi

CPPFLAGS_SAVED="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"

LDFLAGS_SAVED="$LDFLAGS"
LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"

AC_LANG_PUSH(C++)
AC_RUN_IFELSE([
  AC_LANG_PROGRAM(
  [
   #include <boost/version.hpp>
   #include <fstream>
  ],
  [dnl
#if BOOST_VERSION >= $REQ_BOOST_VERSION
  std::ofstream ofs("conftest.out");
  ofs << BOOST_VERSION / 100000 << '.' << BOOST_VERSION / 100 % 1000;
#else
#  error Boost version is too old
#endif
  ])],
  [
    BOOST_VERSION=`cat conftest.out`
    AC_MSG_RESULT(yes)
    succeeded=yes
    found_system=yes
  ],
  [])

AC_LANG_POP([C++])

dnl if we found no boost with system layout we search for boost libraries
dnl built and installed without the --layout=system option 
dnl or for a staged(not installed) version
if test "x$succeeded" != "xyes"; then
  _version=0
  if test "$with_boost_prefix" != ""; then
    BOOST_LDFLAGS="-L$with_boost_prefix/lib"
    if test -d "$with_boost_prefix" && test -r "$with_boost_prefix"; then
      for i in `ls -d $with_boost_prefix/include/boost-* 2>/dev/null`; do
        _version_tmp=`echo $i | sed "s#$with_boost_prefix##" | sed 's/\/include\/boost-//' | sed 's/_/./'`
        V_CHECK=`expr $_version_tmp \> $_version`
        if test "$V_CHECK" = "1" ; then
          _version=$_version_tmp
        fi
        VERSION_UNDERSCORE=`echo $_version | sed 's/\./_/'`
        BOOST_CPPFLAGS="-I$with_boost_prefix/include/boost-$VERSION_UNDERSCORE"
      done
    fi
  else
    for prefix in /usr /usr/local /opt ; do
      if test -d "$prefix" && test -r "$prefix"; then
        for i in `ls -d $prefix/include/boost-* 2>/dev/null`; do
          _version_tmp=`echo $i | sed "s#$prefix##" | sed 's/\/include\/boost-//' | sed 's/_/./'`
          V_CHECK=`expr $_version_tmp \> $_version`
          if test "$V_CHECK" = "1" ; then
            _version=$_version_tmp
            best_path=$prefix
          fi
        done
      fi
    done

    VERSION_UNDERSCORE=`echo $_version | sed 's/\./_/'`
    BOOST_CPPFLAGS="-I$best_path/include/boost-$VERSION_UNDERSCORE"
    BOOST_LDFLAGS="-L$best_path/lib"

    if test "x$BOOST_ROOT" != "x"; then
      if test -d "$BOOST_ROOT" && test -r "$BOOST_ROOT" && test -d "$BOOST_ROOT/stage/lib" && test -r "$BOOST_ROOT/stage/lib"; then
        version_dir=`expr //$BOOST_ROOT : '.*/\(.*\)'`
        stage_version=`echo $version_dir | sed 's/boost_//' | sed 's/_/./g'`
        stage_version_shorten=`expr $stage_version : '\([[0-9]]*\.[[0-9]]*\)'`
        V_CHECK=`expr $stage_version_shorten \>\= $_version`
        if test "$V_CHECK" = "1" ; then
          AC_MSG_NOTICE(We will use a staged boost library from $BOOST_ROOT)
          BOOST_CPPFLAGS="-I$BOOST_ROOT"
          BOOST_LDFLAGS="-L$BOOST_ROOT/stage/lib"
        fi
      fi
    fi
  fi
  CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
  LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"

  AC_LANG_PUSH(C++)
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
    [[#include <boost/version.hpp>]],
    [[#if BOOST_VERSION >= $REQ_BOOST_VERSION
      // Everything is okay
      #else
      # error Boost version is too old
      #endif
    ]])],
    [
      AC_MSG_RESULT(yes)
      succeeded=yes
      found_system=yes
    ],[])
  AC_LANG_POP([C++])
  BOOST_VERSION="$_version"
fi

if test "$succeeded" != "yes" ; then
  if test "$_version" = "0" ; then
    AC_MSG_ERROR([We could not detect the boost libraries (version $boost_version_req_shorten or higher). If you have a staged boost library (still not installed) please specify \$BOOST_ROOT in your environment and do not give a PATH to --with-boost option.  If you are sure you have boost installed, then check your version number looking in <boost/version.hpp>. See http://randspringer.de/boost for more documentation.])
  else
    AC_MSG_ERROR([boost $_version too old; version $boost_version_req or newer required.])
  fi
else
  AC_SUBST(BOOST_VERSION)
  AC_SUBST(BOOST_CPPFLAGS)
  AC_SUBST(BOOST_LDFLAGS)
  AC_DEFINE(HAVE_BOOST,,[define if the Boost library is available])
fi

CPPFLAGS="$CPPFLAGS_SAVED"
LDFLAGS="$LDFLAGS_SAVED"

])


# Boost libraries have a string suffix that identifies the compiler
# they were built with, among other details.  For example, it can be
# '-gcc', '-gcc-mt', '-gcc-mt-1_31', and many other combinations
# depending on the build system.  Some systems provide symlinks that
# hide these suffixes, avoiding this mess.  However, other systems
# don't; we have to provide a way to let the user manually specify a
# suffix.  Guessing can be very difficult, given the variety of
# possibilities.  Note that you cannot expand a variable inside the
# second argument to AC_ARG_VAR, so we're stuck listing it twice.
AC_DEFUN([BOOST_LIB_SUFFIX_ARG],
[AC_ARG_VAR([BOOST_LIB_SUFFIX],
            [Space-separated list of suffixes to try appending to the names
	     of Boost libraries.  "none" means no suffix. The default is:
	     "none -gcc -mipspro -mt -sunpro -sw -mgw -gcc-mt -gcc-mt-s"])
if test x"$BOOST_LIB_SUFFIX" = x; then
  BOOST_LIB_SUFFIX="none -gcc -mipspro -mt -sunpro -sw -mgw -gcc-mt -gcc-mt-s"
fi
])

# Alteratively, allow users to set the suffix (list) via --with-boost-suffix
AC_DEFUN([WITH_BOOST_LIB_SUFFIX],
[AC_ARG_WITH([boost-lib-suffix],
   AS_HELP_STRING([--with-boost-lib-suffix=list],
                  [use the given list of suffixes when searching for boost libraries.]),
[ BOOST_LIB_SUFFIX="$withval"],
[ BOOST_LIB_SUFFIX="none -gcc -mipspro -mt -sunpro -sw -mgw -gcc-mt -gcc-mt-s"])]
)


# BOOST_LIB_IFELSE(library, testprog, if_found, if_not_found)
# This is tricksome, as we only want to process a list of suffixes
# until we've selected one; once we've done that, it must be used for
# all libraries.  (But we need the shell loop in all uses, as previous
# scans might be unsuccessful.)
AC_DEFUN([BOOST_LIB_IFELSE],
[AC_LANG_ASSERT(C++)
 AC_REQUIRE([WITH_BOOST_LIB_SUFFIX])
dnl AC_REQUIRE([BOOST_STATIC_LINK_OPTION])
 found=no
 LIBS_SAVED="$LIBS"
 CPPFLAGS_SAVED="$CPPFLAGS"
 CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"

 LDFLAGS_SAVED="$LDFLAGS"
 LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"

 for s in $BOOST_LIB_SUFFIX
   do
     if test "x$s" = xnone; then
       s=''
     fi
     if test "x${BOOST_LIBDIR}" != x; then
       lib="${BOOST_LIBDIR}/libboost_$1${s}.a"
     else
       lib="-lboost_$1$s"
     fi

     LIBS="$lib $BOOST_LIBS $LIBS_SAVED"
     cv=AS_TR_SH([ac_cv_boost_lib_$1${s}_${BOOST_LIBDIR}])
     AC_CACHE_CHECK([for the boost_$1$s library],
     		    $cv,
       [AC_LINK_IFELSE([$2],
	 	       [eval $cv=yes],
		       [eval $cv=no])])
     if eval "test \"\${$cv}\" = yes"; then
        found=yes
        break
     fi
   done
 CPPFLAGS="$CPPFLAGS_SAVED"
 LDFLAGS="$LDFLAGS_SAVED"
 LIBS="$LIBS_SAVED"
 AS_IF([test $found = yes],
        [BOOST_LIB_SUFFIX=${s:-none}
	 $3],
        [$4])
])

# Checks for specific boost libraries.
# These are named SYN_BOOST_* because their actions are synopsis-specific.

AC_DEFUN([SYN_NEED_BOOST_LIB],
[BOOST_LIB_IFELSE([$1], [$2],
    [BOOST_LIBS="$lib $BOOST_LIBS"],
    [AC_MSG_FAILURE([the boost_$1 library is required])])
 AC_SUBST(BOOST_LIBS)
])

AC_DEFUN([SYN_MAY_NEED_BOOST_LIB],
[BOOST_LIB_IFELSE([$1], [$2],
    [BOOST_LIBS="$lib $BOOST_LIBS"])
 AC_SUBST(BOOST_LIBS)
])

AC_DEFUN([SYN_BOOST_LIB_FILESYSTEM],
[SYN_MAY_NEED_BOOST_LIB([system],
  [AC_LANG_PROGRAM([[
      #include <boost/system/error_code.hpp>
      using namespace boost::system;
    ]],[[
      error_code c;
    ]])])
 SYN_NEED_BOOST_LIB([filesystem],
  [AC_LANG_PROGRAM([[
      #include <boost/filesystem/path.hpp>
      #include <boost/filesystem/operations.hpp>
      using namespace boost::filesystem;
    ]],[[
      exists(path("/boot"));
    ]])])])

AC_DEFUN([SYN_BOOST_LIB_REGEX],
[SYN_NEED_BOOST_LIB([regex],
  [AC_LANG_PROGRAM([[
      #include <boost/regex.hpp>
      using namespace boost;
    ]],[[
      regex expr("foo");
    ]])])])

AC_DEFUN([SYN_BOOST_LIB_WAVE],
[SYN_MAY_NEED_BOOST_LIB([thread],
  [AC_LANG_PROGRAM([[
      #include <boost/thread.hpp>
      using namespace boost;
    ]],[[
      thread t;
    ]])])
 SYN_NEED_BOOST_LIB([wave],
  [AC_LANG_PROGRAM([[
     #include <boost/wave.hpp>
     #include <boost/wave/cpplexer/cpp_lex_token.hpp>
     #include <boost/wave/cpplexer/cpp_lex_iterator.hpp>

     using namespace boost::wave;

     typedef cpplexer::lex_token<> Token;
     typedef cpplexer::lex_iterator<Token> lex_iterator_type;
    ]],[[
      return 0;
    ]])])])

AC_DEFUN([SYN_BOOST_LIB_PYTHON],
[
save_LIBS=$LIBS
LIBS="$LIBS $PYTHON_LIBS"
SYN_NEED_BOOST_LIB([python],
  [AC_LANG_PROGRAM([[
      #include <boost/python.hpp>
      using namespace boost::python;
    ]],[[
      object("dummy");
    ]])])
LIBS=$save_LIBS
])

