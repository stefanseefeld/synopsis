//
// Copyright (C) 2007 Bernhard Fischer
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
//

#include <boost/filesystem/operations.hpp>

namespace Synopsis
{

//. Get normalized absolute path of a filename.
inline boost::filesystem::path get_path(std::string const &filename)
{
  using namespace boost::filesystem;
  return system_complete(path(filename, native)).normalize();
}

//. Return true if base_path is set and path p does not contain
//. base_path.
inline bool matches_path(std::string const &p, std::string const &base_path)
{
  return !base_path.empty() && p.substr(0,base_path.size()) != base_path;
}
  
//. Return the string representation of file in native path
//. nomenclature.
inline std::string make_full_path(std::string const &p)
{
  return get_path(p).native_file_string();
}
  
//. Given a path, strip the base_path.
inline std::string make_short_path(std::string const &p, std::string const &base_path)
{
  std::string short_fname = make_full_path(p);
  if (matches_path(short_fname, base_path))
    short_fname.erase(0, base_path.size());
  return short_fname;
}

}
