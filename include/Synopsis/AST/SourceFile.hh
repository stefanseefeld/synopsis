// $Id: SourceFile.hh,v 1.3 2004/01/13 07:42:09 stefan Exp $
//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_AST_SourceFile_hh
#define _Synopsis_AST_SourceFile_hh

#include <Synopsis/Object.hh>
#include <Synopsis/Tuple.hh>
#include <Synopsis/Callable.hh>

namespace Synopsis
{

class SourceFile : public Object
{
public:
  SourceFile() {}
  SourceFile(const Object &o) : Object(o) {}
  std::string name() { return narrow<std::string>(Callable(attr("filename")).call());}
  std::string long_name() { return narrow<std::string>(Callable(attr("full_filename")).call());}
  bool is_main() { return narrow<bool>(Callable(attr("is_main")).call());}
  void is_main(bool flag) { Callable c(attr("set_is_main")); c.call(Tuple(flag));}
  List includes() { return List(Callable(attr("includes")).call());}
  Dict macro_calls() { return Dict(Callable(attr("macro_calls")).call());}
};

class Include : public Object
{
public:
  Include(const Object &o) throw(TypeError) : Object(o) { assert_type();}
  SourceFile target() const { return narrow<SourceFile>(Callable(attr("target")).call());}
  bool is_macro() const { return narrow<bool>(Callable(attr("is_macro")).call());}
  bool is_next() const { return narrow<bool>(Callable(attr("is_next")).call());}
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

#endif
