//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/TypeAnalysis/Display.hh>

using namespace Synopsis;
using namespace TypeAnalysis;

Display::Display(std::ostream &os)
  : my_os(os)
{
}

void Display::display(Type const *t)
{
  if (t) const_cast<Type *>(t)->accept(this);
  else my_os << "<no type>";
}

void Display::visit(Type *)
{
}

void Display::visit(BuiltinType *type)
{
  my_os << "builtin " << type->name();
}

void Display::visit(Enum *type)
{
  my_os << "enum " << type->name();
}

void Display::visit(Class *type)
{
  if (type->kind() == Class::STRUCT)
    my_os << "struct ";
  else
    my_os << "class ";
  my_os << type->name();
}

void Display::visit(Union *type)
{
  my_os << "union " << type->name();
}

void Display::visit(CVType *type)
{
  const_cast<Type *>(type->unqualified())->accept(this);
  CVType::Qualifier qualifier = type->qualifier();
  if (qualifier & CVType::CONST)
    my_os << " const";
  if (qualifier & CVType::VOLATILE)
    my_os << " volatile";
}

void Display::visit(Pointer *type)
{
  const_cast<Type *>(type->dereference())->accept(this);
  my_os << " *";
}

void Display::visit(Reference *type)
{
  const_cast<Type *>(type->dereference())->accept(this);
  my_os << " &";
}

void Display::visit(Array *type)
{
  const_cast<Type *>(type->dereference())->accept(this);
  my_os << '[' << type->dim() << ']';
}

void Display::visit(Function *type)
{
  const_cast<Type *>(type->return_type())->accept(this);
  my_os << "(*)(";
  Function::ParameterList const &params = type->params();
  Function::ParameterList::const_iterator i = params.begin();
  if (i != params.end())
  {
    const_cast<Type *>(*i)->accept(this);
    for (++i; i != params.end(); ++i)
    {
      my_os << ", ";
      const_cast<Type *>(*i)->accept(this);
    }
  }
  my_os << ')';
}

void Display::visit(PointerToMember *)
{
}

void Display::visit(Parameter *type)
{
  my_os << "parameter " << type->name();
}

void Display::visit(Dependent *type)
{
  my_os << "dependent " << type->name();
}
