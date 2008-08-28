//
// Copyright (C) 2002 Stephen Davies
// Copyright (C) 2002 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef QName_hh_
#define QName_hh_

#include <string>
#include <vector>

//. A qualified name, containing zero or more elements. This typedef makes it
//. easier to use qualified name types, and also makes it clearer than using the
//. raw vector in your code.
typedef std::vector<std::string> QName;

//. Joins the elements of the qualified name using the separator string
inline std::string join(QName const & strs, std::string const & sep = " ")
{
  QName::const_iterator i = strs.begin();
  if (i == strs.end())
    return "";
  std::string str = *i++;
  while (i != strs.end())
    str += sep + *i++;
  return str;
}

//. Formats a QName to the output, joining the strings with ::'s.
inline std::ostream& operator <<(std::ostream& out, QName const &name)
{
  return out << join(name, "::");
}

#endif
