//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_Module_hh
#define _Synopsis_Module_hh

#include <Synopsis/Object.hh>

namespace Synopsis
{

class Module : public Object
{
public:
  Module(const Object &o) : Object(o) {}
  std::string name() const { return PyModule_GetName(my_impl);}
  std::string filename() const { return PyModule_GetFilename(my_impl);}
  Dict dict() const;

  static Module import(const std::string &name) { return Object::import(name);}
  static Module define(const std::string &name, PyMethodDef *methods);
private:
  Module(PyObject *m) : Object(m) {}
};

inline Module Module::define(const std::string &name, PyMethodDef *methods)
{
  PyObject *m = Py_InitModule(const_cast<char *>(name.c_str()), methods);
  Py_INCREF(m);
  return Module(m);
}

inline Dict Module::dict() const
{
  PyObject *d = PyModule_GetDict(my_impl);
  Py_INCREF(d);
  return Object(d);
}

}

#endif
