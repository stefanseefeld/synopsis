//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_ASG_TypeId_hh
#define _Synopsis_ASG_TypeId_hh

#include <Synopsis/Python/Object.hh>
#include <Synopsis/Python/TypedList.hh>
#include <Synopsis/ASG/Visitor.hh>

namespace Synopsis
{
typedef Python::TypedList<std::string> ScopedName;
namespace ASG
{

typedef Python::TypedList<std::string> Modifiers;

class TypeId : public Python::Object
{
public:
  TypeId() {}
  TypeId(const Python::Object &o, bool check = true)
    : Python::Object(o) { if (check && o) assert_type("TypeId");}

  std::string language() const { return narrow<std::string>(attr("language")());}

  virtual void accept(TypeIdVisitor *v) { v->visit_type_id(this);}

  void assert_type(const char *type_id) { Python::Object::assert_type("Synopsis.ASG", type_id);}
};

typedef Python::TypedList<TypeId> TypeIdList;

class NamedTypeId : public TypeId
{
public:
  NamedTypeId() {}
  NamedTypeId(const Python::Object &o, bool check = true)
    : TypeId(o, false) { if (check && o) assert_type("NamedTypeId");}

  ScopedName name() const { return attr("name")();}  

  virtual void accept(TypeIdVisitor *v) { v->visit_named_type_id(this);}
};

class BuiltinTypeId : public NamedTypeId
{
public:
  BuiltinTypeId() {}
  BuiltinTypeId(const Python::Object &o, bool check = true)
    : NamedTypeId(o, false) { if (check && o) assert_type("BuiltinTypeId");}

  virtual void accept(TypeIdVisitor *v) { v->visit_builtin_type_id(this);}
};

class DependentTypeId : public NamedTypeId
{
public:
  DependentTypeId() {}
  DependentTypeId(const Python::Object &o, bool check = true)
    : NamedTypeId(o, false) { if (check && o) assert_type("DependentTypeId");}

  virtual void accept(TypeIdVisitor *v) { v->visit_dependent_type_id(this);}
};

class UnknownTypeId : public NamedTypeId
{
public:
  UnknownTypeId() {}
  UnknownTypeId(const Python::Object &o, bool check = true)
    : NamedTypeId(o, false) { if (check && o) assert_type("UnknownTypeId");}

  virtual void accept(TypeIdVisitor *v) { v->visit_unknown_type_id(this);}
};

class ModifierTypeId : public TypeId
{
public:
  ModifierTypeId() {}
  ModifierTypeId(const Python::Object &o, bool check = true)
    : TypeId(o, false) { if (check && o) assert_type("ModifierTypeId");}

  TypeId alias() const { return narrow<TypeId>(attr("alias")());}
  Modifiers pre() const { return narrow<Modifiers>(attr("premod")());}  
  Modifiers post() const { return narrow<Modifiers>(attr("postmod")());}  

  virtual void accept(TypeIdVisitor *v) { v->visit_modifier_type_id(this);}
};

class ArrayTypeId : public TypeId
{
public:
  typedef Python::TypedList<size_t> Sizes;

  ArrayTypeId() {}
  ArrayTypeId(const Python::Object &o, bool check = true)
    : TypeId(o, false) { if (check && o) assert_type("ArrayTypeId");}

  Sizes sizes() const { return narrow<Sizes>(attr("sizes")());}  

  virtual void accept(TypeIdVisitor *v) { v->visit_array_type_id(this);}
};

class FunctionTypeId : public TypeId
{
public:
  FunctionTypeId() {}
  FunctionTypeId(const Python::Object &o, bool check = true)
    : TypeId(o, false) { if (check && o) assert_type("FunctionTypeId");}

  TypeId return_type() const { return narrow<TypeId>(attr("return_type")());}  
  Modifiers pre() const { return narrow<Modifiers>(attr("premod")());}  
  TypeIdList parameters() const { return narrow<TypeIdList>(attr("parameters")());}

  virtual void accept(TypeIdVisitor *v) { v->visit_function_type_id(this);}
};

}
}

#endif
