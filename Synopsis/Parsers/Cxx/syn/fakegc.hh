//
// Copyright (C) 2001 Stephen Davies
// Copyright (C) 2001 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef fakegc_hh_
#define fakegc_hh_

//. The fake Gargage Collector namespace
namespace FakeGC
{

//. Base class of objects that will be cleaned up
struct cleanup
{
    //. A pointer to the next node in the list
    cleanup* cleanup_next;

    //. Constructor
    cleanup();

    //. Virtual Destructor
    virtual ~cleanup() {}
};

//. A pointer to the head of the linked list
extern cleanup* head;

//. Delete all memory blocks allocated with the GC.
//. *CAUTION* Make sure they are not still in use first!
void delete_all();

//. inline constructor for efficiency
inline cleanup::cleanup()
{
    cleanup_next = FakeGC::head;
    FakeGC::head = this;
}

} // namespace FakeGC

//. Bring cleanup into global NS
using FakeGC::cleanup;

#endif
// vim: set ts=8 sts=4 sw=4 et:
