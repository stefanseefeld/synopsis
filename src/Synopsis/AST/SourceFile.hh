//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_AST_SourceFile_hh
#define _Synopsis_AST_SourceFile_hh

#include <Synopsis/Python/Object.hh>
#include <Synopsis/AST/Declaration.hh>

namespace Synopsis
{
namespace AST
{

class Include : public Python::Object
{
public:
  Include(const Python::Object &o) throw(TypeError) : Python::Object(o) { assert_type();}
  SourceFile target() const { return narrow<SourceFile>(attr("target")());}
  std::string name() const { return narrow<std::string>(attr("name")());}
  bool is_macro() const { return narrow<bool>(attr("is_macro")());}
  bool is_next() const { return narrow<bool>(attr("is_next")());}
  void assert_type() throw(TypeError) { Python::Object::assert_type("Synopsis.AST", "Include");}
};

class MacroCall : public Python::Object
{
public:
  MacroCall(const Python::Object &o) throw(TypeError) : Python::Object(o) { assert_type();}
  void assert_type() throw(TypeError) { Python::Object::assert_type("Synopsis.AST", "MacroCall");}
  std::string name() { return narrow<std::string>(attr("name"));}
  long start() { return narrow<long>(attr("start"));}
  long end() { return narrow<long>(attr("end"));}
  long diff() { return narrow<long>(attr("diff"));}
};

}
}

#endif
