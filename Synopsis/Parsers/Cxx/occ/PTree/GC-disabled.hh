//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _PTree_GC_hh
#define _PTree_GC_hh

// define dummy replacements for GC allocator operators

namespace PTree
{

enum GCPlacement {GC, NoGC};
class LightObject {};
class Object {};
inline void cleanup_gc() {}

}

inline void *operator new(size_t size, PTree::GCPlacement)
{
  return ::operator new(size);
}

inline void *operator new [](size_t size, PTree::GCPlacement)
{
  return ::operator new [](size);
}

#endif
