#ifndef _Guard_hh
#define _Guard_hh

#include <Python.h>

struct Guard
{
  Guard() { Py_Initialize();}
  ~Guard() { Py_Finalize();}
};

Guard guard;

#endif
