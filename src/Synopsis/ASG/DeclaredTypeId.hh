//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_ASG_DeclaredTypeId_hh
#define _Synopsis_ASG_DeclaredTypeId_hh

#include <Synopsis/ASG/TypeId.hh>
#include <Synopsis/ASG/Declaration.hh>

namespace Synopsis
{
namespace ASG
{

class DeclaredTypeId : public NamedTypeId
{
public:
  DeclaredTypeId() {}
  DeclaredTypeId(const Object &o, bool check = true)
    : NamedTypeId(o, false) { if (check) assert_type("DeclaredTypeId");}

  Declaration declaration() const { return attr("declaration")();}

  virtual void accept(TypeIdVisitor *v) { v->visit_declared_type_id(this);}
};

//. Template types are declared template types. They have a name, a
//. declaration (which is an AST::Class) and a vector of parameters
//. declare this template. Each parameter (using AST::Parameter) should be
//. either the correct type for non-type parameters, or a Dependent for type
//. parameters. In either case, there may be default values.
class TemplateId : public DeclaredTypeId
{
public:
  typedef Python::TypedList<Parameter> Parameters;

  TemplateId() {}
  TemplateId(const Python::Object &o, bool check = true)
    : DeclaredTypeId(o, false) { if (check) assert_type("TemplateId");}

  Parameters parameters() { return narrow<Parameters>(attr("parameters")());}
  // specializations() ???

  virtual void accept(TypeIdVisitor *v) { v->visit_template_id(this);}
};

class ParametrizedTypeId : public TypeId
{
public:
  ParametrizedTypeId() {}
  ParametrizedTypeId(const Python::Object &o, bool check = true)
    : TypeId(o, false) { if (check) assert_type("ParametrizedTypeId");}

  TemplateId template_() const { return narrow<TemplateId>(attr("template")());}  
  TypeIdList parameters() const { return narrow<TypeIdList>(attr("parameters")());}

  virtual void accept(TypeIdVisitor *v) { v->visit_parametrized_type_id(this);}
};

//. Safely extracts typed Declarations from Named types. The type is first
//. safely cast to Declared, then the declaration() safely cast to
//. the template type.
template <typename T>
T declared_type_id_cast(const TypeId &type) throw (Python::Object::TypeError)
{
  if (DeclaredTypeId declared = Python::Object::try_narrow<DeclaredTypeId>(type))
    if (Declaration decl = declared.declaration())
      if (T derived = Python::Object::try_narrow<T>(decl))
	return derived;
  throw Python::Object::TypeError();
}

// //. Safely extracts typed Declarations from Type types. The type is first
// //. safely cast to Declared, then the declaration() safely cast to
// //. the template type.
// template <typename T>
// T declared_cast(const Type &type) throw (Object::TypeError)
// {
//   if (Declared declared = Object::try_narrow<Declared>(type))
//     if (Declaration decl = declared.declaration())
//       if (T derived = Object::try_narrow<T>(decl))
// 	return derived;
//   throw Object::TypeError();
// }

}
}

#endif
