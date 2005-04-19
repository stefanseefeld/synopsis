//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "Path.hh"

#ifdef __WIN32__
# include "Path-win32.cc"
#else
# include "Path-posix.cc"
#endif

using namespace Synopsis;

std::string Path::basename() const
{
  if (my_impl.empty()) return "";
  std::string::size_type i = my_impl.rfind(Path::SEPARATOR);
  return i == std::string::npos ? my_impl : my_impl.substr(i + 1);
}

Path Path::dirname() const
{
  if (my_impl.empty()) return Path("");
  std::string::size_type i = my_impl.rfind(Path::SEPARATOR);
  return i == std::string::npos ? Path("") : Path(my_impl.substr(0, i));
}

void Path::strip(const std::string &prefix)
{
  if (prefix.empty()) return;
  if (prefix == my_impl.substr(0, prefix.size()))
    my_impl = my_impl.substr(prefix.size());
}

