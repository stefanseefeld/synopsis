//
// Copyright (C) 2008 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef exception_hh_
#define exception_hh_

#include <Python.h>
#include <exception>

struct py_error_already_set : std::exception
{
  py_error_already_set(bool fetch = true)
    : type(0), value(0), stack(0)
  {
    if (fetch) PyErr_Fetch(&type, &value, &stack);
  }
  void restore() const
  {
    if (type) PyErr_Restore(type, value, stack);
  }
  PyObject *type;
  PyObject *value;
  PyObject *stack;
};

#endif
