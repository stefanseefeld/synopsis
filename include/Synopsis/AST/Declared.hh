// $Id: Declared.hh,v 1.1 2004/01/25 21:21:54 stefan Exp $
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
};

//. Template types are declared template types. They have a name, a
//. declaration (which is an AST::Class) and a vector of parameters
//. declare this template. Each parameter (using AST::Parameter) should be
//. either the correct type for non-type parameters, or a Dependent for type
//. parameters. In either case, there may be default values.
class Template : public Declared
{
public:
  typedef TypedList<Parameter> Parameters;

  Template() {}
  Template(const Object &o, bool check = true)
    : Declared(o, false) { if (check) assert_type("Template");}

  Parameters parameters() { return narrow<Parameters>(attr("parameters")());}
  // specializations() ???
};

class Parametrized : public Type
{
public:
  Parametrized() {}
  Parametrized(const Object &o, bool check = true)
    : Type(o, false) { if (check) assert_type("Parametrized");}

  Template _template() const { return narrow<Template>(attr("template")());}  
  TypeList parameters() const { return narrow<TypeList>(attr("parameters")());}
};

}
}

#endif
