
#ifndef header_slot_v0
#define header_slot_v0

#include "slot_generic.h"

/////////////////////////////////////////////////////////////////////////////
// Generic Slot

class CL_Slot_v0 : public CL_Slot_Generic
{
public:
	virtual void call()=0;
};

/////////////////////////////////////////////////////////////////////////////
// Function Slot

class CL_FunctionSlot_v0 : public CL_Slot_v0
{
public:
	typedef void (*Callback)();

	CL_FunctionSlot_v0(Callback callback) : callback(callback) { return; }

	virtual void call() { callback(); }

private:
	Callback callback;
};

inline static CL_Slot_v0 *CL_CreateSlot(void (*callback)())
{
	return new CL_FunctionSlot_v0(callback);
}

/////////////////////////////////////////////////////////////////////////////
// Method / Member function Slot

template <class CallbackClass>
class CL_MethodSlot_v0 : public CL_Slot_v0
{
public:
	typedef void (CallbackClass::*Callback)();

	CL_MethodSlot_v0(CallbackClass *cb_class, Callback callback)
	: cb_class(cb_class), callback(callback) { return; }

	virtual void call() { (cb_class->*callback)(); }

private:
	CallbackClass *cb_class;
	Callback callback;
};

template <class CallbackClass>
CL_Slot_v0 *CL_CreateSlot(CallbackClass *cb_class, void (CallbackClass::*callback)())
{
	return new CL_MethodSlot_v0<CallbackClass>(cb_class, callback);
}

/////////////////////////////////////////////////////////////////////////////
// Method / Member function Slot, with user data

template <class CallbackClass, class UserData>
class CL_UserDataMethodSlot_v0 : public CL_Slot_v0
{
public:
	typedef void (CallbackClass::*Callback)(UserData);

	CL_UserDataMethodSlot_v0(CallbackClass *cb_class, Callback callback, UserData user_data)
	: cb_class(cb_class), callback(callback), user_data(user_data) { return; }

	virtual void call() { (cb_class->*callback)(user_data); }

private:
	CallbackClass *cb_class;
	Callback callback;
	UserData user_data;
};

template <class CallbackClass, class UserData>
CL_Slot_v0 *CL_CreateSlot(
	CallbackClass *cb_class,
	void (CallbackClass::*callback)(UserData),
	UserData user_data)
{
	return new CL_UserDataMethodSlot_v0<CallbackClass, UserData>(cb_class, callback, user_data);
}

#endif
