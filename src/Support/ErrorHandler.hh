//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef Synopsis_ErrorHandler_hh_
#define Synopsis_ErrorHandler_hh_

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
