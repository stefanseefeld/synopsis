// $Id: AST.hh,v 1.2 2004/01/11 19:46:29 stefan Exp $
//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_AST_AST_hh
#define _Synopsis_AST_AST_hh

#include <Synopsis/Object.hh>
#include <Synopsis/Callable.hh>
#include <Synopsis/Dict.hh>
#include <Synopsis/Tuple.hh>
#include <Synopsis/AST/Declaration.hh>

namespace Synopsis
{

class AST : public Object
{
public:
  AST() {}
  AST(const Object &o) throw(TypeError) : Object(o) { assert_type();}

  Dict files() { return Dict(Callable(attr("files")).call());}
  List declarations() { return List(Callable(attr("declarations")).call());}
  void assert_type() throw(TypeError) { Object::assert_type("Synopsis.AST", "AST");}
};

}

#endif
