
#ifndef header_slot_v3
#define header_slot_v3

#include "slot_generic.h"

/////////////////////////////////////////////////////////////////////////////
// Generic Slot

template <class PARAM1, class PARAM2, class PARAM3>
class CL_Slot_v3 : public CL_Slot_Generic
{
public:
	virtual void call(PARAM1 param1, PARAM2 param2, PARAM3 param3)=0;
};

/////////////////////////////////////////////////////////////////////////////
// Function Slot

template <class PARAM1, class PARAM2, class PARAM3>
class CL_FunctionSlot_v3 : public CL_Slot_v3<PARAM1, PARAM2, PARAM3>
{
public:
	typedef void (*Callback)(PARAM1, PARAM2, PARAM3);

	CL_FunctionSlot_v3(Callback callback) : callback(callback) { return; }

	virtual void call(PARAM1 param1, PARAM2 param2, PARAM3 param3) { callback(param1, param2, param3); }

private:
	Callback callback;
};

template <class PARAM1, class PARAM2, class PARAM3>
CL_Slot_v3<PARAM1, PARAM2, PARAM3> *CL_CreateSlot(void (*callback)(PARAM1, PARAM2, PARAM3))
{
	return new CL_FunctionSlot_v3<PARAM1,PARAM2,PARAM3>(callback);
}

/////////////////////////////////////////////////////////////////////////////
// Method / Member function Slot

template <class CallbackClass, class PARAM1, class PARAM2, class PARAM3>
class CL_MethodSlot_v3 : public CL_Slot_v3<PARAM1, PARAM2, PARAM3>
{
public:
	typedef void (CallbackClass::*Callback)(PARAM1, PARAM2, PARAM3);

	CL_MethodSlot_v3(CallbackClass *cb_class, Callback callback)
	: cb_class(cb_class), callback(callback) { return; }

	virtual void call(PARAM1 param1, PARAM2 param2, PARAM3 param3) { (cb_class->*callback)(param1, param2, param3); }

private:
	CallbackClass *cb_class;
	Callback callback;
};

template <class CallbackClass, class PARAM1, class PARAM2, class PARAM3>
CL_Slot_v3<PARAM1, PARAM2, PARAM3> *CL_CreateSlot(
	CallbackClass *cb_class,
	void (CallbackClass::*callback)(PARAM1, PARAM2, PARAM3))
{
	return new CL_MethodSlot_v3<CallbackClass, PARAM1, PARAM2, PARAM3>(cb_class, callback);
}

/////////////////////////////////////////////////////////////////////////////
// Method / Member function Slot, with user data

template <class CallbackClass, class UserData, class PARAM1, class PARAM2, class PARAM3>
class CL_UserDataMethodSlot_v3 : public CL_Slot_v3<PARAM1, PARAM2, PARAM3>
{
public:
	typedef void (CallbackClass::*Callback)(UserData, PARAM1, PARAM2, PARAM3);

	CL_UserDataMethodSlot_v3(CallbackClass *cb_class, Callback callback, UserData user_data)
	: cb_class(cb_class), callback(callback), user_data(user_data) { return; }

	virtual void call(PARAM1 param1, PARAM2 param2, PARAM3 param3) { (cb_class->*callback)(user_data, param1, param2, param3); }

private:
	CallbackClass *cb_class;
	Callback callback;
	UserData user_data;
};

template <class CallbackClass, class UserData, class PARAM1, class PARAM2, class PARAM3>
CL_Slot_v3<PARAM1, PARAM2, PARAM3> *CL_CreateSlot(
	CallbackClass *cb_class,
	void (CallbackClass::*callback)(UserData, PARAM1, PARAM2, PARAM3),
	UserData user_data)
{
	return new CL_UserDataMethodSlot_v3<CallbackClass, UserData, PARAM1, PARAM2, PARAM3>(cb_class, callback, user_data);
}

#endif
