
#ifndef header_slot_v1
#define header_slot_v1

#include "slot_generic.h"

/////////////////////////////////////////////////////////////////////////////
// Generic Slot

template <class PARAM1>
class CL_Slot_v1 : public CL_Slot_Generic
{
public:
	virtual void call(PARAM1 param1)=0;
};

/////////////////////////////////////////////////////////////////////////////
// Function Slot

template <class PARAM1>
class CL_FunctionSlot_v1 : public CL_Slot_v1<PARAM1>
{
public:
	typedef void (*Callback)(PARAM1);

	CL_FunctionSlot_v1(Callback callback) : callback(callback) { return; }

	virtual void call(PARAM1 param1) { callback(param1); }

private:
	Callback callback;
};

template <class PARAM1>
CL_Slot_v1<PARAM1> *CL_CreateSlot(void (*callback)(PARAM1))
{
	return new CL_FunctionSlot_v1<PARAM1>(callback);
}

/////////////////////////////////////////////////////////////////////////////
// Method / Member function Slot

template <class CallbackClass, class PARAM1>
class CL_MethodSlot_v1 : public CL_Slot_v1<PARAM1>
{
public:
	typedef void (CallbackClass::*Callback)(PARAM1);

	CL_MethodSlot_v1(CallbackClass *cb_class, Callback callback)
	: cb_class(cb_class), callback(callback) { return; }

	virtual void call(PARAM1 param1) { (cb_class->*callback)(param1); }

private:
	CallbackClass *cb_class;
	Callback callback;
};

template <class CallbackClass, class PARAM1>
CL_Slot_v1<PARAM1> *CL_CreateSlot(
	CallbackClass *cb_class,
	void (CallbackClass::*callback)(PARAM1))
{
	return new CL_MethodSlot_v1<CallbackClass, PARAM1>(cb_class, callback);
}

/////////////////////////////////////////////////////////////////////////////
// Method / Member function Slot, with user data

template <class CallbackClass, class UserData, class PARAM1>
class CL_UserDataMethodSlot_v1 : public CL_Slot_v1<PARAM1>
{
public:
	typedef void (CallbackClass::*Callback)(UserData, PARAM1);

	CL_UserDataMethodSlot_v1(CallbackClass *cb_class, Callback callback, UserData user_data)
	: cb_class(cb_class), callback(callback), user_data(user_data) { return; }

	virtual void call(PARAM1 param1) { (cb_class->*callback)(user_data, param1); }

private:
	CallbackClass *cb_class;
	Callback callback;
	UserData user_data;
};

template <class CallbackClass, class UserData, class PARAM1>
CL_Slot_v1<PARAM1> *CL_CreateSlot(
	CallbackClass *cb_class,
	void (CallbackClass::*callback)(UserData, PARAM1),
	UserData user_data)
{
	return new CL_UserDataMethodSlot_v1<CallbackClass, UserData, PARAM1>(cb_class, callback, user_data);
}

#endif
