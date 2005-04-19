//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_ErrorHandler_hh
#define _Synopsis_ErrorHandler_hh

#include <string>

namespace Synopsis
{

struct ErrorHandler
{
  typedef void (*Callback)();
  ErrorHandler(Callback = 0);
  ~ErrorHandler();
};

}

#endif
