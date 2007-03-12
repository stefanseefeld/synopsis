# The following is an adaptation of AC_SEARCH_LIBS to
# check for flags required to link with pthread
# SYN_SEARCH_PTHREAD
# --------------------------------------------------------
# Search for a library defining FUNC, if it's not already available.
AC_DEFUN([SYN_SEARCH_PTHREAD],
[AC_CACHE_CHECK([for flags needed for threading], [ac_cv_search_pthread_create],
[ac_func_search_save_LIBS=$LIBS
ac_cv_search_pthread_create=no
AC_LINK_IFELSE([AC_LANG_CALL([], [pthread_create])],
	       [ac_cv_search_pthread_create="none required"])
if test "$ac_cv_search_pthread_create" = no; then
  for ac_lib in -lpthread -Kpthread -Kthread -pthread; do
    LIBS="$ac_lib $5 $ac_func_search_save_LIBS"
    AC_LINK_IFELSE([AC_LANG_CALL([], [pthread_create])],
		   [ac_cv_search_pthread_create="$ac_lib"
break])
  done
fi
LIBS=$ac_func_search_save_LIBS])
AS_IF([test "$ac_cv_search_pthread_create" != no],
  [test "$ac_cv_search_pthread_create" = "none required" || LIBS="$ac_cv_search_pthread_create $LIBS"],
  [])dnl
])
