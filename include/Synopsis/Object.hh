// $Id: Object.hh,v 1.2 2004/01/10 22:50:34 stefan Exp $
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
  Object &operator = (Object o);
   
  int hash() const { return PyObject_Hash(my_impl);}
  operator bool () const { return PyObject_IsTrue(my_impl);}
  Object repr() const { return PyObject_Repr(my_impl);}
  Object str() const { return PyObject_Str(my_impl);}
  bool is_instance(Object) const;
  void assert_type(const char *module,
		   const char *type) const
    throw(TypeError);

  template <typename T>
  static T narrow(Object) throw(TypeError);
  static Object import(const char *name);

  Object attr(const char *name) const;

  Object call(const char *name);
  Object call(const char *name,
              Object);
  Object call(const char *name,
              Object,
              Object);
  Object call(const char *name,
              Object,
              Object,
              Object);
  Object call(const char *name,
              Object,
              Object,
              Object,
              Object);
  Object call(const char *name,
              Object,
              Object,
              Object,
              Object,
              Object);
  Object call(const char *name,
              Object,
              Object,
              Object,
              Object,
              Object,
              Object);
  Object call(const char *name,
              Object,
              Object,
              Object,
              Object,
              Object,
              Object,
              Object);
  Object call(const char *name,
              Object,
              Object,
              Object,
              Object,
              Object,
              Object,
              Object,
              Object);
  Object call(const char *name,
              Object,
              Object,
              Object,
              Object,
              Object,
              Object,
              Object,
              Object,
              Object);

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

inline Object &Object::operator = (Object o)
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

inline Object Object::call(const char *name)
{
  return PyObject_CallMethod(my_impl, const_cast<char *>(name), 0);
}

inline Object Object::call(const char *name,
                           Object o1)
{
  return PyObject_CallMethod(my_impl, const_cast<char *>(name),
                             "O",
                             o1.my_impl);
}

inline Object Object::call(const char *name,
                           Object o1,
                           Object o2)
{
  return PyObject_CallMethod(my_impl, const_cast<char *>(name),
                             "OO",
                             o1.my_impl,
                             o2.my_impl);
}

inline Object Object::call(const char *name,
                           Object o1,
                           Object o2,
                           Object o3)
{
  return PyObject_CallMethod(my_impl, const_cast<char *>(name),
                             "OOO",
                             o1.my_impl,
                             o2.my_impl,
                             o3.my_impl);
}

inline Object Object::call(const char *name,
                           Object o1,
                           Object o2,
                           Object o3,
                           Object o4)
{
  return PyObject_CallMethod(my_impl, const_cast<char *>(name),
                             "OOOO",
                             o1.my_impl,
                             o2.my_impl,
                             o3.my_impl,
                             o4.my_impl);
}

inline Object Object::call(const char *name,
                           Object o1,
                           Object o2,
                           Object o3,
                           Object o4,
                           Object o5)
{
  return PyObject_CallMethod(my_impl, const_cast<char *>(name),
                             "OOOOO",
                             o1.my_impl,
                             o2.my_impl,
                             o3.my_impl,
                             o4.my_impl,
                             o5.my_impl);
}

inline Object Object::call(const char *name,
                           Object o1,
                           Object o2,
                           Object o3,
                           Object o4,
                           Object o5,
                           Object o6)
{
  return PyObject_CallMethod(my_impl, const_cast<char *>(name),
                             "OOOOOO",
                             o1.my_impl,
                             o2.my_impl,
                             o3.my_impl,
                             o4.my_impl,
                             o5.my_impl,
                             o6.my_impl);
}

inline Object Object::call(const char *name,
                           Object o1,
                           Object o2,
                           Object o3,
                           Object o4,
                           Object o5,
                           Object o6,
                           Object o7)
{
  return PyObject_CallMethod(my_impl, const_cast<char *>(name),
                             "OOOOOOO",
                             o1.my_impl,
                             o2.my_impl,
                             o3.my_impl,
                             o4.my_impl,
                             o5.my_impl,
                             o6.my_impl,
                             o7.my_impl);
}

inline Object Object::call(const char *name,
                           Object o1,
                           Object o2,
                           Object o3,
                           Object o4,
                           Object o5,
                           Object o6,
                           Object o7,
                           Object o8)
{
  return PyObject_CallMethod(my_impl, const_cast<char *>(name),
                             "OOOOOOOO",
                             o1.my_impl,
                             o2.my_impl,
                             o3.my_impl,
                             o4.my_impl,
                             o5.my_impl,
                             o6.my_impl,
                             o7.my_impl,
                             o8.my_impl);
}

inline Object Object::call(const char *name,
                           Object o1,
                           Object o2,
                           Object o3,
                           Object o4,
                           Object o5,
                           Object o6,
                           Object o7,
                           Object o8,
                           Object o9)
{
  return PyObject_CallMethod(my_impl, const_cast<char *>(name),
                             "OOOOOOOOO",
                             o1.my_impl,
                             o2.my_impl,
                             o3.my_impl,
                             o4.my_impl,
                             o5.my_impl,
                             o6.my_impl,
                             o7.my_impl,
                             o8.my_impl,
                             o9.my_impl);
}

template <typename T>
inline T Object::narrow(Object o) throw(Object::TypeError)
{
  return T(o.my_impl);
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
