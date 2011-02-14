//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef Support_Timer_hh_
#define Support_Timer_hh_

#include <ctime>

namespace Synopsis
{

class Timer
{
public:
  Timer() : start_(std::clock()) {}
  void reset() { start_ = std::clock();}
  double elapsed() const { return  double(std::clock() - start_) / CLOCKS_PER_SEC;}
private:
  std::clock_t start_;
};

}

#endif
