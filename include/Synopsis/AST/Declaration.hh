// $Id: Declaration.hh,v 1.2 2004/01/11 19:46:29 stefan Exp $
//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_AST_Declaration_hh
#define _Synopsis_AST_Declaration_hh

#include <Synopsis/Object.hh>
#include <Synopsis/AST/SourceFile.hh>

namespace Synopsis
{

class Declaration : public Object
{
public:
  Declaration(const Object &o) : Object(o) { assert_type();}
  SourceFile file() { return narrow<SourceFile>(Callable(attr("file")).call());}
  int line() { return narrow<int>(Callable(attr("line")).call());}
  bool language() { return narrow<const char *>(Callable(attr("language")).call());}
  const char *type() { return narrow<const char *>(Callable(attr("type")).call());}
  const char *name() { return narrow<const char *>(Callable(attr("name")).call());}
  List comments() { return List(Callable(attr("comments")).call());}
  void assert_type() throw(TypeError) 
  { Object::assert_type("Synopsis.AST", "Declaration");}
};

class Macro : public Declaration
{
public:
  Macro(const Object &o) : Declaration(o) {}
  List parameters() { return List(Callable(attr("parameters")).call());}
  const char *text() { return narrow<const char *>(Callable(attr("text")).call());}
};

}

#endif
