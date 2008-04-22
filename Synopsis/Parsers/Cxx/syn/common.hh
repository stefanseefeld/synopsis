//
// Copyright (C) 2002 Stephen Davies
// Copyright (C) 2002 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef common_hh_
#define common_hh_

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
