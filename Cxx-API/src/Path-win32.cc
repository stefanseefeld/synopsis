//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/Path.hh>
#include <vector>
#include <stdexcept>
#include <cerrno>
#include <windows.h>

using namespace Synopsis;

const char Path::SEPARATOR = '\\';

std::string Path::cwd()
{
  static std::string path;
  if (path.empty())
  {
    DWORD size;
    if ((size = ::GetCurrentDirectoryA(0, 0)) == 0)
    {
      throw std::runtime_error("error accessing current working directory");
    }
    char *buf = new char[size];
    if (::GetCurrentDirectoryA(size, buf) == 0)
    {
      delete [] buf;
      throw std::runtime_error("error accessing current working directory");
    }
    path = buf;
    delete [] buf;
  }
  return path;
}

std::string Path::normalize(const std::string &filename)
{
  std::string value = filename;
  char separator = '\\';
  const char *pat1 = "\\.\\";
  const char *pat2 = "\\..\\";
  if (value[0] != separator &&
      value.size() > 2 && value[1] != ':' && value[2] != '\\')
    value.insert(0, cwd() + separator);
  // nothing to do...
  if (value.find(pat1) == std::string::npos &&
      value.find(pat2) == std::string::npos) return value;
  
  // for the rest we'll operate on a decomposition of the filename...
  typedef std::vector<std::string> Components;
  Components components;

  std::string::size_type b = 0;
  while (b < value.size())
  {
    std::string::size_type e = value.find(separator, b);
    components.push_back(value.substr(b, e-b));
    b = e == std::string::npos ? std::string::npos : e + 1;
  }

  // remove all '.' and '' components
  components.erase(std::remove(components.begin(), components.end(), "."), components.end());
  components.erase(std::remove(components.begin(), components.end(), ""), components.end());
  // now collapse '..' components with the preceding one
  while (true)
  {
    Components::iterator i = std::find(components.begin(), components.end(), "..");
    if (i == components.end()) break;
    if (i == components.begin()) throw std::invalid_argument("invalid path");
    components.erase(i - 1, i + 1); // remove two components
  }

  // now rebuild the path as a string
  std::string retn = '/' + components.front();
  for (Components::iterator i = components.begin() + 1; i != components.end(); ++i)
    retn += '/' + *i;
  return retn;
}
