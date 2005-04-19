//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_AST_AST_hh
#define _Synopsis_AST_AST_hh

#include <Synopsis/Python/Object.hh>
#include <Synopsis/AST/Declaration.hh>

namespace Synopsis
{
namespace AST
{

class AST : public Python::Object
{
public:
  AST() {}
  AST(const Python::Object &o) throw(TypeError) : Python::Object(o) { assert_type();}

  Python::Dict files() { return Python::Dict(attr("files")());}
  Python::Object types() { return attr("types")();}
  Python::List declarations() { return Python::List(attr("declarations")());}
  void assert_type() throw(TypeError) { Python::Object::assert_type("Synopsis.AST", "AST");}
};

}
}

#endif
