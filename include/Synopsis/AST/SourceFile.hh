// $Id: SourceFile.hh,v 1.2 2004/01/11 19:46:29 stefan Exp $
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

}

#endif
