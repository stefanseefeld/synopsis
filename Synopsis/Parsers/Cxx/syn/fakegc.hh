// Synopsis C++ Parser: fakegc.hh header file
// A fake garbage collector which just remembers all objects and deletes them
// at the end.

// $Id: fakegc.hh,v 1.1 2002/11/17 12:11:43 chalky Exp $
//
// This file is a part of Synopsis.
// Copyright (C) 2002 Stephen Davies
//
// Synopsis is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.

#ifndef H_SYNOPSIS_CPP_FAKEGC
#define H_SYNOPSIS_CPP_FAKEGC

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
