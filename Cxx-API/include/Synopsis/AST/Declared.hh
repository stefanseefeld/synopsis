//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_AST_Declared_hh
#define _Synopsis_AST_Declared_hh

#include <Synopsis/AST/Type.hh>
#include <Synopsis/AST/Declaration.hh>

namespace Synopsis
{
namespace AST
{

class Declared : public Named
{
public:
  Declared() {}
  Declared(const Object &o, bool check = true)
    : Named(o, false) { if (check) assert_type("Declared");}

  Declaration declaration() const { return attr("declaration")();}

  virtual void accept(TypeVisitor *v) { v->visit_declared(this);}
};

//. Template types are declared template types. They have a name, a
//. declaration (which is an AST::Class) and a vector of parameters
//. declare this template. Each parameter (using AST::Parameter) should be
//. either the correct type for non-type parameters, or a Dependent for type
//. parameters. In either case, there may be default values.
class Template : public Declared
{
public:
  typedef Python::TypedList<Parameter> Parameters;

  Template() {}
  Template(const Python::Object &o, bool check = true)
    : Declared(o, false) { if (check) assert_type("Template");}

  Parameters parameters() { return narrow<Parameters>(attr("parameters")());}
  // specializations() ???

  virtual void accept(TypeVisitor *v) { v->visit_template(this);}
};

class Parametrized : public Type
{
public:
  Parametrized() {}
  Parametrized(const Python::Object &o, bool check = true)
    : Type(o, false) { if (check) assert_type("Parametrized");}

  Template _template() const { return narrow<Template>(attr("template")());}  
  TypeList parameters() const { return narrow<TypeList>(attr("parameters")());}

  virtual void accept(TypeVisitor *v) { v->visit_parametrized(this);}
};

//. Safely extracts typed Declarations from Named types. The type is first
//. safely cast to Declared, then the declaration() safely cast to
//. the template type.
template <typename T>
T declared_cast(const Type &type) throw (Python::Object::TypeError)
{
  if (Declared declared = Python::Object::try_narrow<Declared>(type))
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
