//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_ASG_IR_hh
#define _Synopsis_ASG_IR_hh

#include <Synopsis/Python/Object.hh>
#include <Synopsis/ASG/Declaration.hh>

namespace Synopsis
{
class IR : public Python::Object
{
public:
  IR() {}
  IR(const Python::Object &o) throw(TypeError) : Python::Object(o) { assert_type();}

  Python::Dict files() { return Python::Dict(attr("files"));}
  Python::Object types() { return attr("asg").attr("types");}
  Python::List declarations() { return Python::List(attr("asg").attr("declarations"));}
  void assert_type() throw(TypeError) { Python::Object::assert_type("Synopsis.IR", "IR");}
};

}

#endif
