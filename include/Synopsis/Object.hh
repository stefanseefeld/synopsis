// $Id: Object.hh,v 1.4 2004/01/14 04:03:48 stefan Exp $
//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_Object_hh
#define _Synopsis_Object_hh

#include <Python.h>
#include <string>
#include <stdexcept>

namespace Synopsis
{

//. Object provides basic
//. functionality to access the underlaying
//. python object, to be used in subclasses and
//. generic accessors such as (python) lists and tuples
class Object
{
  friend class List;
  friend class Tuple;
  friend class Dict;
  friend class Callable;
  friend class Module;
  friend class Interpreter;
public:
  struct TypeError : std::invalid_argument
  {
    TypeError(const std::string &msg = "") : std::invalid_argument(msg) {}
  };

  struct AttributeError : std::invalid_argument
  {
    AttributeError(const std::string &msg = "") : std::invalid_argument(msg) {}
  };

  struct ImportError : std::invalid_argument
  {
    ImportError(const std::string &msg = "") : std::invalid_argument(msg) {}
  };

  Object() : my_impl(Py_None) { Py_INCREF(Py_None);}
  Object(PyObject *);
  Object(const Object &o) : my_impl(o.my_impl) { Py_INCREF(my_impl);}
  Object(const std::string &value) : my_impl(PyString_FromString(value.c_str())) {}
  Object(const char *value) : my_impl(PyString_FromString(value)) {}
  Object(char value) : my_impl(PyString_FromStringAndSize(&value, 1)) {}
  Object(double value) : my_impl(PyFloat_FromDouble(value)) {}
  Object(int value) : my_impl(PyInt_FromLong(value)) {}
  Object(long value) : my_impl(PyInt_FromLong(value)) {}
  Object(bool value) : my_impl(PyInt_FromLong(value)) {}
  virtual ~Object() { Py_DECREF(my_impl);}
  Object &operator = (const Object &o);
   
  int hash() const { return PyObject_Hash(my_impl);}
  operator bool () const { return my_impl != Py_None;}
  Object repr() const { return PyObject_Repr(my_impl);}
  Object str() const { return PyObject_Str(my_impl);}
  Object boolean() const { return PyObject_IsTrue(my_impl);}
  bool is_instance(Object) const;
  void assert_type(const char *module,
		   const char *type) const
    throw(TypeError);

  //. try to downcast to T, throw on failure
  template <typename T>
  static T narrow(Object) throw(TypeError);
  //. more relaxed form of downcast, return None on failure
  template <typename T>
  static T try_narrow(Object);
  static Object import(const char *name);

  Object attr(const char *name) const;

  PyObject *ref() { Py_INCREF(my_impl); return my_impl;}
private:
   PyObject *my_impl;
};

inline Object::Object(PyObject *o)
  : my_impl(o)
{
  if (!my_impl)
  {
    my_impl = Py_None;
    Py_INCREF(Py_None);
  }
}

inline Object &Object::operator = (const Object &o)
{
   Py_DECREF(my_impl);
   my_impl = o.my_impl;
   Py_INCREF(my_impl);
}

inline bool Object::is_instance(Object o) const
{
  return PyObject_IsInstance(my_impl, o.my_impl) == 1;
}

inline void Object::assert_type(const char *module_name,
				const char *type_name) const
  throw(TypeError)
{
  Object module = Object::import(module_name);
  if (!this->is_instance(module.attr(type_name)))
  {
    std::string msg = "object not a ";
    msg += module_name;
    msg += ".";
    msg += type_name;
    throw TypeError(msg);
  }
}

inline Object Object::import(const char *name)
{
  PyObject *retn = PyImport_ImportModule(const_cast<char *>(name));
  if (!retn) throw ImportError(name);
  else return Object(retn);
}

inline Object Object::attr(const char *name) const
{
  PyObject *retn = PyObject_GetAttrString(my_impl, const_cast<char *>(name));
  if (!retn) throw AttributeError(name);
  else return Object(retn);
}

template <typename T>
inline T Object::narrow(Object o) throw(Object::TypeError)
{
  return T(o.my_impl);
}

template <typename T>
inline T Object::try_narrow(Object o)
{
  try { return T(o.my_impl);}
  catch (const TypeError &) { return T();}
}

template <>
inline char Object::narrow(Object o) throw(Object::TypeError)
{
  if (!PyString_Check(o.my_impl)
      || PyString_GET_SIZE(o.my_impl) != 1) throw TypeError("object not a character");
  char *value;
  int length;
  PyString_AsStringAndSize(o.my_impl, &value, &length);
  return value[0];
}

template <>
inline const char *Object::narrow(Object o) throw(Object::TypeError)
{
  if (!PyString_Check(o.my_impl)) throw TypeError("object not a string");
  return PyString_AS_STRING(o.my_impl);
}

template <>
inline std::string Object::narrow(Object o) throw(Object::TypeError)
{
  if (!PyString_Check(o.my_impl)) throw TypeError("object not a string");
  return PyString_AS_STRING(o.my_impl);
}

template <>
inline double Object::narrow(Object o) throw(Object::TypeError)
{
  if (!PyFloat_Check(o.my_impl)) throw TypeError("object not a float");
  return PyFloat_AsDouble(o.my_impl);
}

template <>
inline long Object::narrow(Object o) throw(Object::TypeError)
{
  if (!PyInt_Check(o.my_impl)) throw TypeError("object not an integer");
  return PyInt_AsLong(o.my_impl);
}

template <>
inline bool Object::narrow(Object o) throw(Object::TypeError)
{
  if (!PyInt_Check(o.my_impl)) throw TypeError("object not an integer");
  return PyInt_AsLong(o.my_impl);
}

}

#endif
