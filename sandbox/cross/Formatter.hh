//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Formatter_hh_
#define Formatter_hh_

#include <Synopsis/Buffer.hh>
#include <ostream>

//. Encodes an incoming stream as html.
class Formatter
{
public:
  Formatter(std::ostream &os);
  void format(Synopsis::Buffer::iterator begin, Synopsis::Buffer::iterator end);

private:
  std::ostream &my_os;
  unsigned long my_lineno;
  bool          my_comment;
  bool          my_ccomment;
};

#endif
