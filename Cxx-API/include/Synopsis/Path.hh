//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_Path_hh
#define _Synopsis_Path_hh

#include <string>

namespace Synopsis
{

class Path
{
public:
  //. take a possibly relative path
  //. and turn it into an absolute one.
  Path(const std::string &from) : my_impl(from) {}
  //. strip prefix
  void strip(const std::string &prefix);
  //. return the path as a string
  std::string str() const { return my_impl;}
private:
  //. return the current working directory
  static std::string cwd();
  //. return the normalized and absolutized path
  static std::string normalize(const std::string &);
  std::string my_impl;
};

}

#endif
