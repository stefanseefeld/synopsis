// $Id: Object.hh,v 1.5 2004/01/24 04:44:07 stefan Exp $
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
#include <iostream>

#if PY_VERSION_HEX >= 0x02030000
# define PYTHON_HAS_BOOL 1
#else
# define PYTHON_HAS_BOOL 0
#endif

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

  //. callable interface
  Object operator () ();
  Object operator () (Tuple);
  Object operator () (Tuple, Dict);

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

class Tuple : public Object
{
public:
  Tuple() : Object(PyTuple_New(0)) {}
  explicit Tuple(PyObject *);
  Tuple(Object);
  Tuple(Object, Object);
  Tuple(Object, Object, Object);
  Tuple(Object, Object, Object, Object);
  Tuple(Object, Object, Object, Object, Object);
  Tuple(Object, Object, Object, Object, Object, Object);
  Tuple(Object, Object, Object, Object, Object, Object, Object);
  Object get(size_t i) const { return PyTuple_GetItem(my_impl, i); }
};

class List : public Object
{
public:
  class iterator;

  List(size_t i = 0) : Object(PyList_New(i)) {}
  List(Object) throw(TypeError);
  Tuple tuple() const { return Tuple(PyList_AsTuple(my_impl));}
  size_t size() const { return PyList_GET_SIZE(my_impl);}
  void set(int i, Object o);
  Object get(int i) const;
  void append(Object o) { PyList_Append(my_impl, o.my_impl);}
  void insert(int i, Object o) { PyList_Insert(my_impl, i, o.my_impl);}

  iterator begin();
  iterator end();

  List get_slice(int low, int high) const;
  bool sort() { return PyList_Sort(my_impl) == 0;}
  bool reverse() { return PyList_Reverse(my_impl) == 0;}
private:
  PyObject *impl() { return my_impl;} // extend friendship
};

class List::iterator
{
  friend class List;
public:
  iterator(const iterator &i) : my_list(i.my_list), my_pos(i.my_pos) {}
  iterator &operator = (const iterator &);

  bool operator == (iterator i);
  bool operator != (iterator i) { return !operator==(i);}

  const Object &operator *() { return my_current;}
  const Object *operator ->() { return &(operator *());}
  iterator operator ++(int) { incr(); return *this;}
  iterator operator ++() { iterator tmp = *this; incr(); return tmp;}

private:
  iterator(List l, int i) : my_list(l), my_pos(i) 
  { if (my_pos >= 0) my_current = my_list.get(my_pos);}
  void incr();

  List my_list;
  int my_pos;

  Object my_current;
};

class Dict : public Object
{
public:
  class iterator;
  friend class iterator;
   
  Dict() : Object(PyDict_New()) {}
  Dict(Object o) throw(TypeError) : Object(o) 
  { if (!PyDict_Check(o.my_impl)) throw TypeError("object not a dict");}
  void set(Object k, Object v);
  Object get(Object k, Object d = Object()) const;
  bool has_key(Object k) const;
  bool del(Object k);

  iterator begin();
  iterator end();
   
  void clear() { PyDict_Clear(my_impl);}
  Dict copy() const { return Object(PyDict_Copy(my_impl));}
  bool update(Dict d) { return PyDict_Update(my_impl, d.my_impl) == 0;}
   
  List keys() const { return List(Object(PyDict_Keys(my_impl)));}
  List values() const { return List(Object(PyDict_Values(my_impl)));}
  List items() const { return List(Object(PyDict_Items(my_impl)));}

private:
  PyObject *impl() { return my_impl;} // extend friendship
};

class Dict::iterator
{
  friend class Dict;
public:
  iterator(const iterator &i) : my_dict(i.my_dict), my_pos(i.my_pos) {}
  iterator &operator = (const iterator &);

  bool operator == (iterator i);
  bool operator != (iterator i) { return !operator==(i);}

  const Tuple &operator *() { return my_current;}
  const Tuple *operator ->() { return &(operator *());}
  iterator operator ++(int) { incr(); return *this;}
  iterator operator ++() { iterator tmp = *this; incr(); return tmp;}

private:
  iterator(Dict, int);
  void incr();

  Dict my_dict;
  int my_pos;

  Tuple my_current;
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
  if (!is_instance(module.attr(type_name)))
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
  if (!PyString_Check(o.my_impl) || PyString_GET_SIZE(o.my_impl) != 1)
    throw TypeError("object not a character");
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
#if 0 //PYTHON_HAS_BOOL
  if (!PyBool_Check(o.my_impl)) throw TypeError("object not a boolean");
#else
  if (!PyInt_Check(o.my_impl)) throw TypeError("object not an integer");
#endif
  return PyInt_AsLong(o.my_impl);
}

std::ostream &operator << (std::ostream &os, const Object &o)
{
  return os << Object::narrow<std::string>(o.str());
}

inline Object Object::operator () ()
{
  return PyObject_CallObject(my_impl, 0);
}

inline Object Object::operator () (Tuple args)
{
  return PyObject_Call(my_impl, args.my_impl, 0);
}

inline Object Object::operator () (Tuple args, Dict kwds)
{
  return PyObject_Call(my_impl, args.my_impl, kwds.my_impl);
}

inline Tuple::Tuple(PyObject *o)
  : Object(o)
{
  if (!PyTuple_Check(o)) throw TypeError("object not a tuple");
}

inline Tuple::Tuple(Object o)
  : Object(PyTuple_New(1))
{
   PyTuple_SET_ITEM(my_impl, 0, o.my_impl);
   Py_INCREF(o.my_impl);
}

inline Tuple::Tuple(Object o1, Object o2)
  : Object(PyTuple_New(2))
{
   PyTuple_SET_ITEM(my_impl, 0, o1.my_impl);
   Py_INCREF(o1.my_impl);
   PyTuple_SET_ITEM(my_impl, 1, o2.my_impl);
   Py_INCREF(o2.my_impl);
}

inline Tuple::Tuple(Object o1, Object o2, Object o3)
  : Object(PyTuple_New(3))
{
   PyTuple_SET_ITEM(my_impl, 0, o1.my_impl);
   Py_INCREF(o1.my_impl);
   PyTuple_SET_ITEM(my_impl, 1, o2.my_impl);
   Py_INCREF(o2.my_impl);
   PyTuple_SET_ITEM(my_impl, 2, o3.my_impl);
   Py_INCREF(o3.my_impl);
}

inline Tuple::Tuple(Object o1, Object o2, Object o3,
		    Object o4)
  : Object(PyTuple_New(4))
{
   PyTuple_SET_ITEM(my_impl, 0, o1.my_impl);
   Py_INCREF(o1.my_impl);
   PyTuple_SET_ITEM(my_impl, 1, o2.my_impl);
   Py_INCREF(o2.my_impl);
   PyTuple_SET_ITEM(my_impl, 2, o3.my_impl);
   Py_INCREF(o3.my_impl);
   PyTuple_SET_ITEM(my_impl, 3, o4.my_impl);
   Py_INCREF(o4.my_impl);
}

inline Tuple::Tuple(Object o1, Object o2, Object o3,
		    Object o4, Object o5)
  : Object(PyTuple_New(5))
{
   PyTuple_SET_ITEM(my_impl, 0, o1.my_impl);
   Py_INCREF(o1.my_impl);
   PyTuple_SET_ITEM(my_impl, 1, o2.my_impl);
   Py_INCREF(o2.my_impl);
   PyTuple_SET_ITEM(my_impl, 2, o3.my_impl);
   Py_INCREF(o3.my_impl);
   PyTuple_SET_ITEM(my_impl, 3, o4.my_impl);
   Py_INCREF(o4.my_impl);
   PyTuple_SET_ITEM(my_impl, 4, o5.my_impl);
   Py_INCREF(o5.my_impl);
}

inline Tuple::Tuple(Object o1, Object o2, Object o3,
		    Object o4, Object o5, Object o6)
  : Object(PyTuple_New(6))
{
   PyTuple_SET_ITEM(my_impl, 0, o1.my_impl);
   Py_INCREF(o1.my_impl);
   PyTuple_SET_ITEM(my_impl, 1, o2.my_impl);
   Py_INCREF(o2.my_impl);
   PyTuple_SET_ITEM(my_impl, 2, o3.my_impl);
   Py_INCREF(o3.my_impl);
   PyTuple_SET_ITEM(my_impl, 3, o4.my_impl);
   Py_INCREF(o4.my_impl);
   PyTuple_SET_ITEM(my_impl, 4, o5.my_impl);
   Py_INCREF(o5.my_impl);
   PyTuple_SET_ITEM(my_impl, 5, o6.my_impl);
   Py_INCREF(o6.my_impl);
}

inline Tuple::Tuple(Object o1, Object o2, Object o3,
		    Object o4, Object o5, Object o6,
		    Object o7)
  : Object(PyTuple_New(7))
{
   PyTuple_SET_ITEM(my_impl, 0, o1.my_impl);
   Py_INCREF(o1.my_impl);
   PyTuple_SET_ITEM(my_impl, 1, o2.my_impl);
   Py_INCREF(o2.my_impl);
   PyTuple_SET_ITEM(my_impl, 2, o3.my_impl);
   Py_INCREF(o3.my_impl);
   PyTuple_SET_ITEM(my_impl, 3, o4.my_impl);
   Py_INCREF(o4.my_impl);
   PyTuple_SET_ITEM(my_impl, 4, o5.my_impl);
   Py_INCREF(o5.my_impl);
   PyTuple_SET_ITEM(my_impl, 5, o6.my_impl);
   Py_INCREF(o6.my_impl);
   PyTuple_SET_ITEM(my_impl, 6, o7.my_impl);
   Py_INCREF(o7.my_impl);
}

inline List::List(Object o) throw(TypeError)
  : Object(o)
{
  if (PyTuple_Check(o.my_impl)) // copy elements into new list
  {
    Py_DECREF(my_impl);
    my_impl = PyList_New(PyTuple_Size(o.my_impl));
    for (int i = 0; i != PyList_Size(my_impl); ++i)
    {
      PyObject *item = PyTuple_GetItem(o.my_impl, i);
      Py_INCREF(item);
      PyList_SetItem(my_impl, i, item);
    }
  }
  else if (!PyList_Check(o.my_impl)) throw TypeError("object not a list");
}

inline void List::set(int i, Object o)
{
  Py_INCREF(o.my_impl);
  PyList_SetItem(my_impl, i, o.my_impl);
}

inline Object List::get(int i) const
{
  PyObject *retn = PyList_GetItem(my_impl, i);
  Py_INCREF(retn);
  return Object(retn);
}

inline List::iterator List::begin()
{
  return iterator(*this, 0);
}

inline List::iterator List::end()
{
  return iterator(*this, -1);
}

inline List List::get_slice(int low, int high) const 
{
  return List(PyList_GetSlice(my_impl, low, high));
}

inline List::iterator &List::iterator::operator = (const List::iterator &i)
{
  my_list = i.my_list;
  my_pos = i.my_pos;
  my_current = i.my_current;
}

inline bool List::iterator::operator == (List::iterator i)
{
  return i.my_list.impl() == my_list.impl() && i.my_pos == my_pos;
}

inline void List::iterator::incr() 
{
  if (++my_pos < my_list.size())
    my_current = my_list.get(my_pos);
  else 
    my_pos = -1;
}

inline void Dict::set(Object k, Object v)
{
  PyDict_SetItem(my_impl, k.my_impl, v.my_impl);
}

inline Object Dict::get(Object k, Object d) const
{
  PyObject *retn = PyDict_GetItem(my_impl, k.my_impl);
  if (retn) Py_INCREF(retn);
  return retn ? Object(retn) : d;
}

inline bool Dict::has_key(Object k) const 
{
  return PyDict_GetItem(my_impl, k.my_impl) != 0;
}

inline bool Dict::del(Object k)
{
  return PyDict_DelItem(my_impl, k.my_impl) == 0;
}

inline Dict::iterator Dict::begin()
{
  return iterator(*this, 0);
}

inline Dict::iterator Dict::end()
{
  return iterator(*this, -1);
}

inline Dict::iterator::iterator(Dict dict, int pos)
  : my_dict(dict), my_pos(pos)
{
  if (pos != -1) incr();
}

inline Dict::iterator &Dict::iterator::operator = (const Dict::iterator &i)
{
  my_dict = i.my_dict;
  my_pos = i.my_pos;
  my_current = i.my_current;
}

inline void Dict::iterator::incr()
{
  PyObject *key = 0, *value = 0;
  bool valid = PyDict_Next(my_dict.impl(), &my_pos, &key, &value);
  if (!valid) my_pos = -1;
  else
  {
     Py_INCREF(key);
     Py_INCREF(value);   
     my_current = Tuple(key, value);
  }
}

inline bool Dict::iterator::operator == (Dict::iterator i)
{
  return i.my_dict.impl() == my_dict.impl() && i.my_pos == my_pos;
}

}

#endif
