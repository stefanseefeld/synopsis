//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_Exception_hh_
#define Synopsis_Exception_hh_

#include <string>

namespace Synopsis
{

class InternalError : public std::exception
{
public:
  InternalError(std::string const &what) : my_what(what) {}
  virtual ~InternalError() throw() {}
  virtual char const * what() const throw() { return my_what.c_str();}
private:
  std::string my_what;
};

}

#endif
