// $Id: Declaration.hh,v 1.1 2004/01/10 22:50:34 stefan Exp $
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
  SourceFile file() { return narrow<SourceFile>(call("file"));}
  int line() { return narrow<int>(call("line"));}
  bool language() { return narrow<const char *>(call("language"));}
  const char *type() { return narrow<const char *>(call("type"));}
  const char *name() { return narrow<const char *>(call("name"));}
  List comments() { return List(call("comments"));}
  void assert_type() throw(TypeError) 
  { Object::assert_type("Synopsis.AST", "Declaration");}
};

class Macro : public Declaration
{
public:
  Macro(const Object &o) : Declaration(o) {}
  List parameters() { return List(call("parameters"));}
  const char *text() { return narrow<const char *>(call("text"));}
};

}

#endif
