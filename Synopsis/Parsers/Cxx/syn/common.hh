// Synopsis C++ Parser: common.hh header file
// Common types, utility functions and classes

// $Id: common.hh,v 1.3 2002/11/17 12:11:43 chalky Exp $
//
// This file is a part of Synopsis.
// Copyright (C) 2002 Stephen Davies
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

#ifndef H_SYNOPSIS_CPP_COMMON
#define H_SYNOPSIS_CPP_COMMON

#include <string>
#include <vector>

#include "fakegc.hh"


//. A scoped name, containing zero or more elements. This typedef makes it
//. easier to use scoped name types, and also makes it clearer than using the
//. raw vector in your code.
typedef std::vector<std::string> ScopedName;

//. Prototype for scoped name output operator (defined in builder.cc)
std::ostream& operator <<(std::ostream& out, const ScopedName& name);

//. Joins the elements of the scoped name using the separator string
std::string join(const ScopedName& strs, const std::string& sep = " ");

#endif
// vim: set ts=8 sts=4 sw=4 et:
