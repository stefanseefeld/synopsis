//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <SymbolTable/Scopes.hh>
#include <PTree/Writer.hh>
#include <PTree/Display.hh>
#include <cassert>

using namespace PTree;
using namespace SymbolTable;

const Symbol *LocalScope::lookup(const Encoding &name) const throw()
{
  const Symbol *symbol = Scope::lookup(name);
  return symbol ? symbol : my_outer->lookup(name);
}

void LocalScope::dump(std::ostream &os) const
{
  os << "LocalScope::dump:" << std::endl;
  Scope::dump(os);
  my_outer->dump(os);
}

const Symbol *Class::lookup(const Encoding &name) const throw()
{
  const Symbol *symbol = Scope::lookup(name);
  return symbol ? symbol : my_outer->lookup(name);
}

void Class::dump(std::ostream &os) const
{
  os << "Class::dump:" << std::endl;
  Scope::dump(os);
  my_outer->dump(os);
}

std::string Class::name() const
{
  const PTree::Node *name_spec = PTree::second(my_spec);
  // FIXME: why is name_spec for anonumous classes 'list(0, 0)' ?
  //        see Parser::class_spec()...
  if (name_spec && name_spec->is_atom())
    return std::string(name_spec->position(), name_spec->length());
  return "";
}

const Symbol *Namespace::lookup(const Encoding &name) const throw()
{
  const Symbol *symbol = Scope::lookup(name);
  return symbol ? symbol : my_outer->lookup(name);
}

void Namespace::dump(std::ostream &os) const
{
  os << "Namespace::dump:" << std::endl;
  Scope::dump(os);
  my_outer->dump(os);
}

std::string Namespace::name() const
{
  const PTree::Node *name_spec = PTree::second(my_spec);
  return name_spec ? std::string(name_spec->position(), name_spec->length()) : "";
}

