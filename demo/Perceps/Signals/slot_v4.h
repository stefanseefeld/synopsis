
#ifndef header_slot_v4
#define header_slot_v4

#include "slot_generic.h"

/////////////////////////////////////////////////////////////////////////////
// Generic Slot

template <class PARAM1, class PARAM2, class PARAM3, class PARAM4>
class CL_Slot_v4 : public CL_Slot_Generic
{
public:
	virtual void call(PARAM1 param1, PARAM2 param2, PARAM3 param3, PARAM4 param4)=0;
};

/////////////////////////////////////////////////////////////////////////////
// Function Slot

template <class PARAM1, class PARAM2, class PARAM3, class PARAM4>
class CL_FunctionSlot_v4 : public CL_Slot_v4<PARAM1, PARAM2, PARAM3, PARAM4>
{
public:
	typedef void (*Callback)(PARAM1, PARAM2, PARAM3, PARAM4);

	CL_FunctionSlot_v4(Callback callback) : callback(callback) { return; }

	virtual void call(PARAM1 param1, PARAM2 param2, PARAM3 param3, PARAM4 param4) { callback(param1, param2, param3, param4); }

private:
	Callback callback;
};

template <class PARAM1, class PARAM2, class PARAM3, class PARAM4>
CL_Slot_v4<PARAM1, PARAM2, PARAM3, PARAM4> *CL_CreateSlot(void (*callback)(PARAM1, PARAM2, PARAM3, PARAM4))
{
	return new CL_FunctionSlot_v4<PARAM1,PARAM2,PARAM3,PARAM4>(callback);
}

/////////////////////////////////////////////////////////////////////////////
// Method / Member function Slot

template <class CallbackClass, class PARAM1, class PARAM2, class PARAM3, class PARAM4>
class CL_MethodSlot_v4 : public CL_Slot_v4<PARAM1, PARAM2, PARAM3, PARAM4>
{
public:
	typedef void (CallbackClass::*Callback)(PARAM1, PARAM2, PARAM3, PARAM4);

	CL_MethodSlot_v4(CallbackClass *cb_class, Callback callback)
	: cb_class(cb_class), callback(callback) { return; }

	virtual void call(PARAM1 param1, PARAM2 param2, PARAM3 param3, PARAM4 param4) { (cb_class->*callback)(param1, param2, param3, param4); }

private:
	CallbackClass *cb_class;
	Callback callback;
};

template <class CallbackClass, class PARAM1, class PARAM2, class PARAM3, class PARAM4>
CL_Slot_v4<PARAM1, PARAM2, PARAM3, PARAM4> *CL_CreateSlot(
	CallbackClass *cb_class,
	void (CallbackClass::*callback)(PARAM1, PARAM2, PARAM3, PARAM4))
{
	return new CL_MethodSlot_v4<CallbackClass, PARAM1, PARAM2, PARAM3, PARAM4>(cb_class, callback);
}

/////////////////////////////////////////////////////////////////////////////
// Method / Member function Slot, with user data

template <class CallbackClass, class UserData, class PARAM1, class PARAM2, class PARAM3, class PARAM4>
class CL_UserDataMethodSlot_v4 : public CL_Slot_v4<PARAM1, PARAM2, PARAM3, PARAM4>
{
public:
	typedef void (CallbackClass::*Callback)(UserData, PARAM1, PARAM2, PARAM3, PARAM4);

	CL_UserDataMethodSlot_v4(CallbackClass *cb_class, Callback callback, UserData user_data)
	: cb_class(cb_class), callback(callback), user_data(user_data) { return; }

	virtual void call(PARAM1 param1, PARAM2 param2, PARAM3 param3, PARAM4 param4) { (cb_class->*callback)(user_data, param1, param2, param3, param4); }

private:
	CallbackClass *cb_class;
	Callback callback;
	UserData user_data;
};

template <class CallbackClass, class UserData, class PARAM1, class PARAM2, class PARAM3, class PARAM4>
CL_Slot_v4<PARAM1, PARAM2, PARAM3, PARAM4> *CL_CreateSlot(
	CallbackClass *cb_class,
	void (CallbackClass::*callback)(UserData, PARAM1, PARAM2, PARAM3, PARAM4),
	UserData user_data)
{
	return new CL_UserDataMethodSlot_v4<CallbackClass, UserData, PARAM1, PARAM2, PARAM3, PARAM4>(cb_class, callback, user_data);
}

#endif
