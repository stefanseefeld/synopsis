// $Id: Type.hh,v 1.1 2004/01/25 21:21:54 stefan Exp $
//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_AST_Type_hh
#define _Synopsis_AST_Type_hh

#include <Synopsis/Object.hh>
#include <Synopsis/TypedList.hh>

namespace Synopsis
{
namespace AST
{

typedef TypedList<std::string> ScopedName;
typedef TypedList<std::string> Modifiers;

class Type : public Object
{
public:
  Type() {}
  Type(const Object &o, bool check = true)
    : Object(o) { if (check) assert_type("Type");}

  std::string language() const { return narrow<std::string>(attr("language")());}

  void assert_type(const char *type) { Object::assert_type("Synopsis.Type", type);}
};

typedef TypedList<Type> TypeList;

class Named : public Type
{
public:
  Named() {}
  Named(const Object &o, bool check = true)
    : Type(o, false) { if (check) assert_type("Named");}

  ScopedName name() const { return attr("name")();}  
};

class Base : public Named
{
public:
  Base() {}
  Base(const Object &o, bool check = true)
    : Named(o, false) { if (check) assert_type("Base");}
};

class Dependent : public Named
{
public:
  Dependent() {}
  Dependent(const Object &o, bool check = true)
    : Named(o, false) { if (check) assert_type("Dependent");}
};

class Unknown : public Named
{
public:
  Unknown() {}
  Unknown(const Object &o, bool check = true)
    : Named(o, false) { if (check) assert_type("Unknown");}
};

class Modifier : public Type
{
public:
  Modifier() {}
  Modifier(const Object &o, bool check = true)
    : Type(o, false) { if (check) assert_type("Modifier");}

  Modifiers pre() const { return narrow<Modifiers>(attr("premod")());}  
  Modifiers post() const { return narrow<Modifiers>(attr("postmod")());}  
};

class Array : public Type
{
public:
  typedef TypedList<size_t> Sizes;

  Array() {}
  Array(const Object &o, bool check = true)
    : Type(o, false) { if (check) assert_type("Array");}

  Sizes sizes() const { return narrow<Sizes>(attr("sizes")());}  
};

class FunctionPtr : public Type
{
public:
  FunctionPtr() {}
  FunctionPtr(const Object &o, bool check = true)
    : Type(o, false) { if (check) assert_type("Function");}

  Type return_type() const { return narrow<Type>(attr("returnType")());}  
  Modifiers pre() const { return narrow<Modifiers>(attr("premod")());}  
  TypeList parameters() const { return narrow<TypeList>(attr("parameters")());}
};

}
}

#endif
