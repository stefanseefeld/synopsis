
#ifndef header_slotbuffer_v0
#define header_slotbuffer_v0

class CL_SlotBuffer_v0
{
// Construction:
public:
	CL_SlotBuffer_v0() : count(0)
	{
	}

	CL_SlotBuffer_v0(CL_Signal_v0 &signal) : count(0)
	{
		connect(signal);
	}

// Attributes:
public:
	operator bool()
	{
		return count > 0;
	}

	int size()
	{
		return count;
	}

// Operations:
public:
	void connect(CL_Signal_v0 &signal)
	{
		slot = signal.connect(CL_CreateSlot(this, &CL_SlotBuffer_v0::callback));
	}

	void disconnect(CL_Signal_v0 &signal)
	{
		signal.disconnect(slot);
	}

// Implementation:
private:
	void callback()
	{
		count++;
	}

	CL_Slot slot;
	int count;
};

#endif
