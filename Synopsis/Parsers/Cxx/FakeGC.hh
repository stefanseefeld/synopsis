//
// Copyright (C) 2001 Stephen Davies
// Copyright (C) 2001 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef FakeGC_hh_
#define FakeGC_hh_

//. The fake Garbage Collector namespace
namespace FakeGC
{

// Provide a very simple garbage collection mechanism:
// All objects of this type are assumed to be heap-allocated.
// During construction they build up a single-linked list, and once
// the program is finished, the whole list is taken down by deleting
// the first node.
struct LightObject
{
  LightObject() : next(head) { LightObject::head = this;}
  virtual ~LightObject() {}

  LightObject *next;
  static LightObject *head;
};

} // namespace FakeGC

#endif
