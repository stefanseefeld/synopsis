//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_Python_Kit_hh
#define _Synopsis_Python_Kit_hh

#include <Synopsis/Python/Module.hh>

namespace Synopsis
{
namespace Python
{

class Kit : public Module
{
public:
  Kit(const std::string &name) : Module(Module::import(name)) {}

  template <typename T>
  T create(const char *name, const Tuple &t = Tuple(), const Dict &d = Dict())
  { return dict().get(name)(t, d);}
};

}
}

#endif
