
#ifndef header_signal_v2
#define header_signal_v2

#ifdef WIN32
#pragma warning ( disable : 4786 )
#endif

#include "slot_v2.h"
#include <list>

template <class PARAM1, class PARAM2>
class CL_Signal_v2
{
public:
	typedef CL_Slot_v2<PARAM1, PARAM2> *Slot;
	typedef std::list<Slot>::iterator SlotIterator;

// Construction:
public:
	CL_Signal_v2()
	{
	}

	~CL_Signal_v2()
	{
		for (SlotIterator slot_it = slots.begin(); slot_it != slots.end(); slot_it++)
		{
			Slot slot = *slot_it;
			slot->release_signal_ref();
		}
	}

// Operations:
public:
	void operator() (PARAM1 param1, PARAM2 param2)
	{
		call(param1, param2);
	}

	void call(PARAM1 param1, PARAM2 param2)
	{
		std::list<SlotIterator> remove_slots;

		// call slots connected to signal:
		for (SlotIterator slot_it = slots.begin(); slot_it != slots.end(); slot_it++)
		{
			Slot slot = *slot_it;

			// skip slot if it has no more references left in application.
			// (make it pending for removal)
			if (slot->get_slot_ref() == 0)
			{
				remove_slots.push_back(slot_it);
				continue;
			}
			
			slot->call(param1, param2);
		}

		// remove all slots no longer connected to any CL_Slot.
		std::list<SlotIterator>::iterator remove_it;
		for (remove_it = remove_slots.begin(); remove_it != remove_slots.end(); remove_it++)
		{
			Slot slot = **remove_it;
			slot->release_signal_ref();
			slots.erase(*remove_it);
		}
	}

	CL_Slot connect(Slot slot)
	{
		slot->add_signal_ref();
		slots.push_back(slot);
		return CL_Slot(slot);
	}

	void disconnect(CL_Slot &disconnect_slot)
	{
		for (SlotIterator slot_it = slots.begin(); slot_it != slots.end();)
		{
			Slot slot = *slot_it;
			if (disconnect_slot.impl == slot)
			{
				slot->release_signal_ref();
				slot_it = slots.erase(slot_it);
			}
			else
				slot_it++;
		}
	}

// Implementation:
private:
	std::list<Slot> slots;
};

#endif
