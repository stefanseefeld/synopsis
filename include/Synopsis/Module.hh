// $Id: Module.hh,v 1.3 2004/01/24 04:44:07 stefan Exp $
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
  Module(const std::string &);
  std::string name() const { return PyModule_GetName(my_impl);}
  std::string filename() const { return PyModule_GetFilename(my_impl);}
  Dict dict() const;
};

inline Module::Module(const std::string &name)
  : Object(PyImport_ImportModule(const_cast<char *>(name.c_str())))
{
  Py_INCREF(my_impl);
}

inline Dict Module::dict() const
{
  PyObject *d = PyModule_GetDict(my_impl);
  Py_INCREF(d);
  return Object(d);
}

}

#endif
