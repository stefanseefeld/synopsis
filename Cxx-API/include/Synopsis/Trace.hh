//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_Trace_hh
#define _Synopsis_Trace_hh

#include <iostream>
#include <string>

namespace Synopsis
{

class Trace
{
public:
  Trace(const std::string &s) : my_scope(s)
  {
    if (!my_debug) return;
    std::cout << indent() << "entering " << my_scope << std::endl;
    ++my_level;
  }
  ~Trace()
  {
    if (!my_debug) return;
    --my_level;
    std::cout << indent() << "leaving " << my_scope << std::endl;
  }

  static void enable_debug() { my_debug = true;}

private:
  std::string indent() { return std::string(my_level, ' ');}

  static bool   my_debug;
  static size_t my_level;
  std::string   my_scope;
};

}

#endif
