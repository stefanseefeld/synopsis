/*
 * Fake GC
 */

#ifndef SYNOPSIS_FAKEGC_H
#define SYNOPSIS_FAKEGC_H

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

    inline cleanup::cleanup()
    {
	cleanup_next = FakeGC::head;
	FakeGC::head = this;
    }

}

//. Bring cleanup into global NS
using FakeGC::cleanup;

#endif
