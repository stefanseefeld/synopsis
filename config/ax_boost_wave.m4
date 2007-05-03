#
# SYNOPSIS
#
#   AX_BOOST_WAVE
#
# DESCRIPTION
#
#   This macro checks to see if the Boost.Wave library is installed.
#   It also attempts to guess the currect library name using several
#   attempts. It tries to build the library name using a user supplied
#   name or suffix and then just the raw library.
#
#   If the library is found, HAVE_BOOST_WAVE is defined and
#   BOOST_WAVE_LIB is set to the name of the library.
#
# LAST MODIFICATION
#
#   2007-04-29
#
# COPYLEFT
#
#   Copyright (c) 2007 Stefan Seefeld <seefeld@sympatico.ca>
#
#   This program is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public License as
#   published by the Free Software Foundation; either version 2 of the
#   License, or (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#   General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
#   02111-1307, USA.
#
#   As a special exception, the respective Autoconf Macro's copyright
#   owner gives unlimited permission to copy, distribute and modify the
#   configure scripts that are the output of Autoconf when processing
#   the Macro. You need not follow the terms of the GNU General Public
#   License when using or distributing such scripts, even though
#   portions of the text of the Macro appear in them. The GNU General
#   Public License (GPL) does govern all other use of the material that
#   constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the
#   Autoconf Macro released by the Autoconf Macro Archive. When you
#   make and distribute a modified version of the Autoconf Macro, you
#   may extend this special exception to the GPL to apply to your
#   modified version as well.

AC_DEFUN([AX_BOOST_WAVE],
[
AC_CACHE_CHECK(whether the Boost::Wave library is available,
ac_cv_boost_wave,
[AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 CPPFLAGS_SAVE=$CPPFLAGS
 AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[
   #include <boost/wave.hpp>
   #include <boost/wave/cpplexer/cpp_lex_token.hpp>
   #include <boost/wave/cpplexer/cpp_lex_iterator.hpp>

   using namespace boost::wave;

   typedef cpplexer::lex_token<> Token;
   typedef cpplexer::lex_iterator<Token> lex_iterator_type;
 ]],
 [[return 0;]]),
 ac_cv_boost_wave=yes, ac_cv_boost_wave=no)
 AC_LANG_RESTORE
 CPPFLAGS=$CPPFLAGS_SAVE
])
if test "$ac_cv_boost_wave" = "yes"; then
  AC_DEFINE(HAVE_BOOST_WAVE,,[define if the Boost::Wave library is available])
  ax_wave_lib=boost_wave
  AC_ARG_WITH([boost-wave],AS_HELP_STRING([--with-boost-wave],[specify the boost wave library or suffix to use]),
  [if test "x$with_boost_wave" != "xno"; then
     ax_wave_lib=$with_boost_wave
     ax_boost_wave_lib=boost_wave-$with_boost_wave
   fi])
  for ax_lib in $ax_wave_lib $ax_boost_wave_lib boost_wave; do
    AC_CHECK_LIB($ax_lib, main, [BOOST_WAVE_LIB=-l$ax_lib break])
  done
  AC_SUBST(BOOST_WAVE_LIB)
fi
])dnl
