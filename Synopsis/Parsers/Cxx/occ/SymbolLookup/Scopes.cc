//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <SymbolLookup/Scopes.hh>
#include <PTree/Writer.hh>
#include <PTree/Display.hh>
#include <sstream>
#include <cassert>

using namespace PTree;
using namespace SymbolLookup;

void TemplateParameterScope::dump(std::ostream &os, size_t in) const
{
  indent(os, in) << "TemplateParameterScope:\n";
  Scope::dump(os, in);
}

std::set<Symbol const *> LocalScope::lookup(const Encoding &name) const throw()
{
  std::set<Symbol const *> symbols = Scope::lookup(name);
  return symbols.size() ? symbols : my_outer->lookup(name);
}

void LocalScope::dump(std::ostream &os, size_t in) const
{
  indent(os, in) << "LocalScope:\n";
  Scope::dump(os, in + 1);
}

std::set<Symbol const *> FunctionScope::lookup(const Encoding &name) const throw()
{
  std::set<Symbol const *> symbols = Scope::lookup(name);
  return symbols.size() ? symbols : my_outer->lookup(name);
}

void FunctionScope::dump(std::ostream &os, size_t in) const
{
  indent(os, in) << "FunctionScope '" << this->name() << "':\n";
  Scope::dump(os, in + 1);
}

std::string FunctionScope::name() const
{
  std::ostringstream oss;
  oss << PTree::reify(PTree::third(my_decl));
  return oss.str();
}

std::set<Symbol const *> Class::lookup(const Encoding &name) const throw()
{
  std::set<Symbol const *> symbols = Scope::lookup(name);
  return symbols.size() ? symbols : my_outer->lookup(name);
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

void Class::dump(std::ostream &os, size_t in) const
{
  indent(os, in) << "Class '" << this->name() << "':\n";
  Scope::dump(os, in + 1);
}

std::set<Symbol const *> Namespace::lookup(const Encoding &name) const throw()
{
  std::set<Symbol const *> symbols = Scope::lookup(name);
  return symbols.size() ? symbols : my_outer->lookup(name);
}

std::string Namespace::name() const
{
  const PTree::Node *name_spec = PTree::second(my_spec);
  return name_spec ? std::string(name_spec->position(), name_spec->length()) : "";
}

void Namespace::dump(std::ostream &os, size_t in) const
{
  indent(os, in) << "Namespace'" << this->name() << "':\n";
  Scope::dump(os, in + 1);
}
