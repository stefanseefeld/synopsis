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

class py_error_already_set : std::exception {};

#endif
