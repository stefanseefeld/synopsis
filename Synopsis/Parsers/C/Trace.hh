//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Trace_hh
#define _Trace_hh

#include <iostream>
#include <string>

#if 1
class Trace
{
public:
  Trace(const std::string &s) : scope(s)
  {
    if (!debug) return;
    std::cout << indent() << "entering " << scope << std::endl;
    ++level;
  }
  ~Trace()
  {
    if (!debug) return;
    --level;
    std::cout << indent() << "leaving " << scope << std::endl;
  }

  static void enable_debug() { debug = true;}

private:
  std::string indent() { return std::string(level, ' ');}
  static bool debug;
  static int level;
  std::string scope;
};

#else

class Trace
{
public:
  Trace(const std::string &) {}
  ~Trace() {}

  static void enable_debug() { debug = 1;}
private:
  static bool debug;
  static int level;
};

#endif

#endif
