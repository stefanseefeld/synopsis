//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_ASG_Type_hh
#define _Synopsis_ASG_Type_hh

#include <Synopsis/Python/Object.hh>
#include <Synopsis/Python/TypedList.hh>
#include <Synopsis/ASG/Visitor.hh>

namespace Synopsis
{
typedef Python::TypedList<std::string> ScopedName;
namespace ASG
{

typedef Python::TypedList<std::string> Modifiers;

class Type : public Python::Object
{
public:
  Type() {}
  Type(const Python::Object &o, bool check = true)
    : Python::Object(o) { if (check && o) assert_type("Type");}

  std::string language() const { return narrow<std::string>(attr("language")());}

  virtual void accept(TypeVisitor *v) { v->visit_type(this);}

  void assert_type(const char *type) { Python::Object::assert_type("Synopsis.ASG", type);}
};

typedef Python::TypedList<Type> TypeList;

class Named : public Type
{
public:
  Named() {}
  Named(const Python::Object &o, bool check = true)
    : Type(o, false) { if (check && o) assert_type("NamedType");}

  ScopedName name() const { return attr("name")();}  

  virtual void accept(TypeVisitor *v) { v->visit_named(this);}
};

class Base : public Named
{
public:
  Base() {}
  Base(const Python::Object &o, bool check = true)
    : Named(o, false) { if (check && o) assert_type("BaseType");}

  virtual void accept(TypeVisitor *v) { v->visit_base(this);}
};

class Dependent : public Named
{
public:
  Dependent() {}
  Dependent(const Python::Object &o, bool check = true)
    : Named(o, false) { if (check && o) assert_type("Dependent");}

  virtual void accept(TypeVisitor *v) { v->visit_dependent(this);}
};

class Unknown : public Named
{
public:
  Unknown() {}
  Unknown(const Python::Object &o, bool check = true)
    : Named(o, false) { if (check && o) assert_type("UnknownType");}

  virtual void accept(TypeVisitor *v) { v->visit_unknown(this);}
};

class Modifier : public Type
{
public:
  Modifier() {}
  Modifier(const Python::Object &o, bool check = true)
    : Type(o, false) { if (check && o) assert_type("ModifierType");}

  Type alias() const { return narrow<Type>(attr("alias")());}
  Modifiers pre() const { return narrow<Modifiers>(attr("premod")());}  
  Modifiers post() const { return narrow<Modifiers>(attr("postmod")());}  

  virtual void accept(TypeVisitor *v) { v->visit_modifier(this);}
};

class Array : public Type
{
public:
  typedef Python::TypedList<size_t> Sizes;

  Array() {}
  Array(const Python::Object &o, bool check = true)
    : Type(o, false) { if (check && o) assert_type("Array");}

  Sizes sizes() const { return narrow<Sizes>(attr("sizes")());}  

  virtual void accept(TypeVisitor *v) { v->visit_array(this);}
};

class FunctionPtr : public Type
{
public:
  FunctionPtr() {}
  FunctionPtr(const Python::Object &o, bool check = true)
    : Type(o, false) { if (check && o) assert_type("FunctionType");}

  Type return_type() const { return narrow<Type>(attr("return_type")());}  
  Modifiers pre() const { return narrow<Modifiers>(attr("premod")());}  
  TypeList parameters() const { return narrow<TypeList>(attr("parameters")());}

  virtual void accept(TypeVisitor *v) { v->visit_function_ptr(this);}
};

}
}

#endif
