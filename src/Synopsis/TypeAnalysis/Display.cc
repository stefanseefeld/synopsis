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
  my_os << type->name();
}

void Display::visit(Enum *)
{
}

void Display::visit(Class *)
{
}

void Display::visit(Union *)
{
}

void Display::visit(CVType *type)
{
  const_cast<Type *>(type->unqualified())->accept(this);
  my_os << ' ' << type->name();
}

void Display::visit(Pointer *type)
{
  const_cast<Type *>(type->dereference())->accept(this);
  my_os << ' ' << type->name();
}

void Display::visit(Reference *type)
{
  const_cast<Type *>(type->dereference())->accept(this);
  my_os << ' ' << type->name();
}

void Display::visit(Array *type)
{
  const_cast<Type *>(type->dereference())->accept(this);
  my_os << '[' << type->dim() << ']';
}

void Display::visit(Function *)
{
}

void Display::visit(PointerToMember *)
{
}
