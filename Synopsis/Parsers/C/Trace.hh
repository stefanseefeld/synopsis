// $Id: Trace.hh,v 1.1 2003/08/20 02:16:37 stefan Exp $
//
// This file is a part of Synopsis.
// Copyright (C) 2003 Stefan Seefeld
//
// Synopsis is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
// $Log: Trace.hh,v $
// Revision 1.1  2003/08/20 02:16:37  stefan
// first steps towards a C parser backend (based on the ctool)
//
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
    std::cout << indent() << "entering " << scope << std::endl;
    ++level;
  }
  ~Trace()
  {
    --level;
    std::cout << indent() << "leaving " << scope << std::endl;
  }
private:
  std::string indent() { return std::string(level, ' ');}
  static int level;
  std::string scope;
};

#else

class Trace
{
public:
  Trace(const std::string &) {}
  ~Trace() {}
};

#endif

#endif
