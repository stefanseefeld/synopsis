//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/Path.hh>

#ifdef __WIN32__
# include "Path-win32.cc"
#else
# include "Path-posix.cc"
#endif

using namespace Synopsis;

void Path::strip(const std::string &prefix)
{
  if (prefix.empty()) return;
  if (prefix == my_impl.substr(0, prefix.size()))
    my_impl = my_impl.substr(prefix.size());
}

