//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _PTree_GC_hh
#define _PTree_GC_hh

#include <gc_cpp.h>

namespace PTree
{

using ::GC;
using ::NoGC;

typedef gc LightObject;
typedef gc_cleanup Object;
inline void cleanup_gc() { GC_gcollect();}

}

#endif
