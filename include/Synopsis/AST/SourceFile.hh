// $Id: SourceFile.hh,v 1.4 2004/01/25 21:21:54 stefan Exp $
//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_AST_SourceFile_hh
#define _Synopsis_AST_SourceFile_hh

#include <Synopsis/Object.hh>
#include <Synopsis/AST/Declaration.hh>

namespace Synopsis
{
namespace AST
{

class Include : public Object
{
public:
  Include(const Object &o) throw(TypeError) : Object(o) { assert_type();}
  SourceFile target() const { return narrow<SourceFile>(attr("target")());}
  bool is_macro() const { return narrow<bool>(attr("is_macro")());}
  bool is_next() const { return narrow<bool>(attr("is_next")());}
  void assert_type() throw(TypeError) { Object::assert_type("Synopsis.AST", "Include");}
};

class MacroCall : public Object
{
public:
  MacroCall(const Object &o) throw(TypeError) : Object(o) { assert_type();}
  void assert_type() throw(TypeError) { Object::assert_type("Synopsis.AST", "MacroCall");}
  std::string name() { return narrow<std::string>(attr("name"));}
  int start() { return narrow<int>(attr("start"));}
  int end() { return narrow<int>(attr("end"));}
  int diff() { return narrow<int>(attr("diff"));}
};

}
}

#endif
