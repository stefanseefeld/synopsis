//
// Copyright (C) 2006 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef config_h_
#define config_h_

#include <acconfig.h>

#define HAS_Cplusplus_Bool

#if defined(SIZEOF_LONG) && (SIZEOF_LONG == 8)
#  define HAS_LongLong
#  define _CORBA_LONGLONG_DECL     long
#  define _CORBA_ULONGLONG_DECL    unsigned long
#  define _CORBA_LONGLONG_CONST(x) (x)

#elif defined(SIZEOF_LONG_LONG) && (SIZEOF_LONG_LONG == 8)
#  define HAS_LongLong
#  define _CORBA_LONGLONG_DECL     long long
#  define _CORBA_ULONGLONG_DECL    unsigned long long
#  define _CORBA_LONGLONG_CONST(x) (x##LL)
#endif


#if defined(SIZEOF_LONG_DOUBLE) && (SIZEOF_LONG_DOUBLE == 16)
#  define HAS_LongDouble
#  define _CORBA_LONGDOUBLE_DECL long double
#endif

#if defined(SIZEOF_LONG_DOUBLE) && (SIZEOF_LONG_DOUBLE == 12) && defined(__i386__)
#  define HAS_LongDouble
#  define _CORBA_LONGDOUBLE_DECL long double
#endif

#endif
