// $Id: AST.hh,v 1.1 2004/01/10 22:50:34 stefan Exp $
//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_AST_AST_hh
#define _Synopsis_AST_AST_hh

#include <Synopsis/Object.hh>
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

  Dict files() { return Dict(call("files"));}
  List declarations() { return List(call("declarations"));}
  void assert_type() throw(TypeError) { Object::assert_type("Synopsis.AST", "AST");}
};

}

#endif
