// $Id: SourceFile.hh,v 1.1 2004/01/10 22:50:34 stefan Exp $
//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_AST_SourceFile_hh
#define _Synopsis_AST_SourceFile_hh

#include <Synopsis/Object.hh>

namespace Synopsis
{

class SourceFile : public Object
{
public:
  SourceFile() {}
  SourceFile(const Object &o) : Object(o) {}
  std::string name() { return narrow<std::string>(call("filename"));}
  std::string long_name() { return narrow<std::string>(call("full_filename"));}
  bool is_main() { return narrow<bool>(call("is_main"));}
  void is_main(bool flag) { call("set_is_main", flag);}
  List includes() { return List(call("includes"));}
};

class Include : public Object
{
public:
  Include(const Object &o) throw(TypeError) : Object(o) { assert_type();}
  SourceFile target() const { return narrow<SourceFile>(attr("target"));}
  bool is_macro() const { return narrow<bool>(attr("is_macro"));}
  bool is_next() const { return narrow<bool>(attr("is_next"));}
  void assert_type() throw(TypeError) { Object::assert_type("Synopsis.AST", "Include");}
};

}

#endif
