
#ifndef header_slot
#define header_slot

#include "slot_generic.h"
#include <stdlib.h>

class CL_Slot
{
//!Construction:
public:
	CL_Slot()
	: impl(NULL)
	{
	}
	// Create an unconnected slot.

	CL_Slot(const CL_Slot &copy)
	: impl(copy.impl)
	{
		if (impl != NULL) impl->add_slot_ref();
	}
	// Copy a slot.

	~CL_Slot()
	{
		if (impl != NULL) impl->release_slot_ref();
	}

	void operator =(const CL_Slot &copy)
	{
		if (impl) impl->release_slot_ref();
		impl = copy.impl;
		if (impl) impl->add_slot_ref();
	}

//!Implementation:
public:
	CL_Slot(CL_Slot_Generic *impl)
	: impl(impl)
	{
		if (impl != NULL) impl->add_slot_ref();
	}

	CL_Slot_Generic *impl;
};

#endif
