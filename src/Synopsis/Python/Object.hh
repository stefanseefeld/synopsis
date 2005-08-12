//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_Python_Object_hh
#define _Synopsis_Python_Object_hh

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
namespace Python
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

  struct KeyError : std::invalid_argument
  {
    KeyError(const std::string &msg = "") : std::invalid_argument(msg) {}
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
  operator bool () const 
  {
    int retn = PyObject_IsTrue(my_impl);
    if (retn == -1) check_exception();
    return retn == 1;
  }
  Object type() const { return PyObject_Type(my_impl);}
  Object repr() const { return PyObject_Repr(my_impl);}
  Object str() const { return PyObject_Str(my_impl);}
  int cmp(Object o) const;
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
  static Object import(const std::string &name);

  Object attr(const std::string &) const;
  bool set_attr(const std::string &, Object);
  PyObject *ref() { Py_INCREF(my_impl); return my_impl;}
  int ref_count() const { return my_impl->ob_refcnt;}
private:
  //. check for an exception and if set translate into
  //. a C++ exception that is thrown
  void check_exception() const;
  
  PyObject *my_impl;
};

inline bool operator == (Object o1, Object o2) { return !o1.cmp(o2);}
inline bool operator < (Object o1, Object o2) { return o1.cmp(o2) < 0;}
inline bool operator > (Object o1, Object o2) { return o1.cmp(o2)> 0;}

class Tuple : public Object
{
public:
  Tuple() : Object(PyTuple_New(0)) {}
  explicit Tuple(PyObject *);
  size_t size() const { return PyTuple_GET_SIZE(my_impl);}
  bool empty() const { return !size();}
  Tuple(Object);
  Tuple(Object, Object);
  Tuple(Object, Object, Object);
  Tuple(Object, Object, Object, Object);
  Tuple(Object, Object, Object, Object, Object);
  Tuple(Object, Object, Object, Object, Object, Object);
  Tuple(Object, Object, Object, Object, Object, Object, Object);
  Tuple(Object, Object, Object, Object, Object, Object, Object, Object);
  Object get(size_t i) const;
};

class List : public Object
{
public:
  class iterator;
  class reverse_iterator;

  List(size_t i = 0) : Object(PyList_New(i)) {}
  List(Object) throw(TypeError);
  template <typename I>
  List(I begin, I end);
  Tuple tuple() const { return Tuple(PyList_AsTuple(my_impl));}
  size_t size() const { return PyList_GET_SIZE(my_impl);}
  bool empty() const { return !size();}
  void set(int i, Object o);
  Object get(int i) const;
  void append(Object o) { PyList_Append(my_impl, o.my_impl);}
  void insert(int i, Object o) { PyList_Insert(my_impl, i, o.my_impl);}
  void extend(List l);
  void del(int i) { PySequence_DelItem(my_impl, i);}

  iterator begin() const;
  iterator end() const;
  iterator erase(iterator);

  reverse_iterator rbegin() const;
  reverse_iterator rend() const;

  List get_slice(int low, int high) const;
  List copy() const { return get_slice(0, -1);}
  bool sort() { return PyList_Sort(my_impl) == 0;}
  bool reverse() { return PyList_Reverse(my_impl) == 0;}
private:
  PyObject *impl() { return my_impl;} // extend friendship
};

class List::iterator
{
  friend class List;
public:
  iterator(const iterator &i)
    : my_list(i.my_list), my_pos(i.my_pos), my_current(i.my_current) {}
  iterator &operator = (const iterator &);

  bool operator == (iterator i);
  bool operator != (iterator i) { return !operator==(i);}

  const Object &operator *() { return my_current;}
  const Object *operator ->() { return &(operator *());}
  iterator operator ++(int) { incr(); return *this;}
  iterator operator ++() { iterator tmp = *this; incr(); return tmp;}
  iterator operator +(int i);
  iterator operator --(int) { decr(); return *this;}
  iterator operator --() { iterator tmp = *this; decr(); return tmp;}
  iterator operator -(int i);

private:
  iterator(List l, int i) : my_list(l), my_pos(i) 
  { if (my_pos >= 0) my_current = my_list.get(my_pos);}
  void incr();
  void decr();

  List my_list;
  int my_pos;

  Object my_current;
};

class List::reverse_iterator
{
  friend class List;
public:
  reverse_iterator(const reverse_iterator &i)
    : my_list(i.my_list), my_pos(i.my_pos), my_current(i.my_current) {}
  reverse_iterator &operator = (const reverse_iterator &);

  bool operator == (reverse_iterator i);
  bool operator != (reverse_iterator i) { return !operator==(i);}

  const Object &operator *() { return my_current;}
  const Object *operator ->() { return &(operator *());}
  reverse_iterator operator ++(int) { incr(); return *this;}
  reverse_iterator operator ++() { reverse_iterator tmp = *this; incr(); return tmp;}
  reverse_iterator operator +(int i);
  reverse_iterator operator --(int) { decr(); return *this;}
  reverse_iterator operator --() { reverse_iterator tmp = *this; decr(); return tmp;}
  reverse_iterator operator -(int i);

private:
  reverse_iterator(List l, int i) : my_list(l), my_pos(i) 
  { if (my_pos >= 0) my_current = my_list.get(my_pos);}
  void incr();
  void decr();

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

  iterator begin() const;
  iterator end() const;
   
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
    check_exception();
    my_impl = Py_None;
    Py_INCREF(Py_None);
  }
}

inline Object &Object::operator = (const Object &o)
{
  if (my_impl != o.my_impl)
  {
    Py_DECREF(my_impl);
    my_impl = o.my_impl;
    Py_INCREF(my_impl);
  }
  return *this;
}

inline int Object::cmp(Object o) const 
{
  int result = PyObject_Compare(my_impl, o.my_impl);
  check_exception();
  return result;
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
    msg += " (was ";
    Object type = attr("__class__").repr();
    msg += PyString_AS_STRING(type.my_impl);
    msg += ")";
    throw TypeError(msg);
  }
}

inline Object Object::import(const std::string &name)
{
  PyObject *retn = PyImport_ImportModule(const_cast<char *>(name.c_str()));
  if (!retn) throw ImportError(name);
  else return Object(retn);
}

inline Object Object::attr(const std::string &name) const
{
  PyObject *retn = PyObject_GetAttrString(my_impl, const_cast<char *>(name.c_str()));
  if (!retn) throw AttributeError(name.c_str());
  else return Object(retn);
}

inline bool Object::set_attr(const std::string &name, Object value)
{
  int retn = PyObject_SetAttrString(my_impl,
				    const_cast<char *>(name.c_str()),
				    value.ref());
  return retn != -1;
}

template <typename T>
inline T Object::narrow(Object o) throw(Object::TypeError)
{
  T retn(o.my_impl);
  Py_INCREF(o.my_impl);
  return retn;
}

template <typename T>
inline T Object::try_narrow(Object o)
{
  try 
  {
    T retn(o.my_impl);
    Py_INCREF(o.my_impl);
    return retn;
  }
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

inline void Object::check_exception() const
{
  PyObject *exc = PyErr_Occurred();
  if (!exc) return;
  PyObject *type, *value, *trace;
  PyErr_Fetch(&type, &value, &trace);
  Object t(type), v(value), tr(trace); // to release the reference at end of scope
  if (exc == PyExc_KeyError)
    throw KeyError(Object::narrow<std::string>(v.str()));
  else if (exc == PyExc_TypeError)
    throw TypeError(Object::narrow<std::string>(v.str()));
  else if (exc == PyExc_AttributeError)
    throw AttributeError();
  throw std::runtime_error(PyString_AsString(value));
}

inline std::ostream &operator << (std::ostream &os, const Object &o)
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

inline Tuple::Tuple(Object o1, Object o2, Object o3,
		    Object o4, Object o5, Object o6,
		    Object o7, Object o8)
  : Object(PyTuple_New(8))
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
   PyTuple_SET_ITEM(my_impl, 7, o8.my_impl);
   Py_INCREF(o8.my_impl);
}

inline Object Tuple::get(size_t i) const 
{
  PyObject *retn = PyTuple_GetItem(my_impl, i);
  if (!retn) check_exception();
  Py_INCREF(retn);
  return Object(retn);
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

template <typename I>
List::List(I begin, I end)
  : Object(PyList_New(0))
{
  for (I i = begin; i != end; ++i) append(*i);
}

inline void List::set(int i, Object o)
{
  Py_INCREF(o.my_impl);
  PyList_SetItem(my_impl, i, o.my_impl);
}

inline Object List::get(int i) const
{
  PyObject *retn = PyList_GetItem(my_impl, i);
  if (!retn) check_exception();
  Py_INCREF(retn);
  return Object(retn);
}

inline void List::extend(List l) 
{
  for (List::iterator i = l.begin(); i != l.end(); ++i)
    append(*i);
}

inline List::iterator List::begin() const
{
  return iterator(*this, size() ? 0 : -1);
}

inline List::iterator List::end() const
{
  return iterator(*this, -1);
}

inline List::iterator List::erase(List::iterator i)
{
  if (i.my_pos >= 0) del(i.my_pos);
  if (i.my_pos >= size()) // if the erased element was the last...
    --i.my_pos;           // ... decrement the iterator by one
  return i;
}

inline List::reverse_iterator List::rbegin() const
{
  return reverse_iterator(*this, size() - 1);
}

inline List::reverse_iterator List::rend() const
{
  return reverse_iterator(*this, -1);
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
  return *this;
}

inline bool List::iterator::operator == (List::iterator i)
{
  return i.my_list.impl() == my_list.impl() && i.my_pos == my_pos;
}

inline List::iterator List::iterator::operator + (int i)
{
  if (my_pos != -1) my_pos += i;

  if (my_pos < my_list.size())
    my_current = my_list.get(my_pos);
  else
    my_pos = -1;
  return *this;
}

inline List::iterator List::iterator::operator - (int i)
{
  my_pos = my_pos == -1 ? my_list.size() - i : my_pos - i;

  if (my_pos > -1)
    my_current = my_list.get(my_pos);
  else
    my_pos = -1;
  return *this;
}

inline void List::iterator::incr() 
{
  operator + (1);
}

inline void List::iterator::decr() 
{
  operator - (1);
}

inline bool List::reverse_iterator::operator == (List::reverse_iterator i)
{
  return i.my_list.impl() == my_list.impl() && i.my_pos == my_pos;
}

inline List::reverse_iterator List::reverse_iterator::operator + (int i)
{
  my_pos = my_pos == -1 ? my_list.size() - i : my_pos - i;

  if (my_pos > -1)
    my_current = my_list.get(my_pos);
  else
    my_pos = -1;
  return *this;
}

inline List::reverse_iterator List::reverse_iterator::operator - (int i)
{
  my_pos += i;

  if (my_pos < my_list.size())
    my_current = my_list.get(my_pos);
  else
    my_pos = -1;
  return *this;
}

inline void List::reverse_iterator::incr() 
{
  operator + (1);
}

inline void List::reverse_iterator::decr() 
{
  operator - (1);
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

inline Dict::iterator Dict::begin() const
{
  return iterator(*this, 0);
}

inline Dict::iterator Dict::end() const
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
  return *this;
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
}

#endif
