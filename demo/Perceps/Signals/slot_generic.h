
#ifndef header_slot_generic
#define header_slot_generic

class CL_Slot_Generic
{
// Construction:
public:
	CL_Slot_Generic() : slot_ref_count(0), signal_ref_count(0) { return; }

	virtual ~CL_Slot_Generic() { return; }

// Attributes:
public:
	int get_slot_ref() const { return slot_ref_count; }

	int get_signal_ref() const { return signal_ref_count; }

// Operations:
public:
	void add_slot_ref() { slot_ref_count++; }

	void release_slot_ref() { slot_ref_count--; check_delete(); }

	void add_signal_ref() { signal_ref_count++; }

	void release_signal_ref() { signal_ref_count--; check_delete(); }

// Implementation:
private:
	void check_delete()
	{
		if (slot_ref_count == 0 && signal_ref_count == 0) delete this;
	}

	int slot_ref_count;

	int signal_ref_count;
};

#endif
