
#ifndef header_slotbuffer_v1
#define header_slotbuffer_v1

#include <vector>

template <class PARAM1>
class CL_SlotBuffer_v1
{
public:
	struct Params
	{
		PARAM1 p1;
	};

// Construction:
public:
	CL_SlotBuffer_v1()
	{
	}

	CL_SlotBuffer_v1(CL_Signal_v1<PARAM1> &signal)
	{
		connect(signal);
	}

// Attributes:
public:
	operator bool()
	{
		return !params.empty();
	}

	int size()
	{
		return params.size();
	}

	Params &operator[](int index)
	{
		return params[index];
	}

// Operations:
public:
	void connect(CL_Signal_v1<PARAM1> &signal)
	{
		slot = signal.connect(CL_CreateSlot(this, &CL_SlotBuffer_v1::callback));
	}

	void disconnect(CL_Signal_v1<PARAM1> &signal)
	{
		signal.disconnect(slot);
	}

// Implementation:
private:
	void callback(PARAM1 p1)
	{
		Params p;
		p.p1 = p1;

		params.push_back(p);
	}

	std::vector<Params> params;
	CL_Slot slot;
};

#endif
