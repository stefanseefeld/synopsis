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

SymbolSet LocalScope::unqualified_lookup(Encoding const &name,
					 bool scope) const
{
  SymbolSet symbols = find(name, scope);
  return symbols.size() ? symbols : my_outer->unqualified_lookup(name, scope);
}

void LocalScope::dump(std::ostream &os, size_t in) const
{
  indent(os, in) << "LocalScope:\n";
  Scope::dump(os, in + 1);
}

SymbolSet FunctionScope::unqualified_lookup(Encoding const &name,
					    bool scope) const
{
  SymbolSet symbols = find(name, scope);
  return symbols.size() ? symbols : my_outer->unqualified_lookup(name, scope);
}

void FunctionScope::dump(std::ostream &os, size_t in) const
{
  indent(os, in) << "FunctionScope '" << this->name() << "':\n";
  Scope::dump(os, in + 1);
}

std::string FunctionScope::name() const
{
  std::ostringstream oss;
  oss << PTree::reify(my_decl);
  return oss.str();
}

SymbolSet Class::unqualified_lookup(Encoding const &name,
				    bool scope) const
{
  SymbolSet symbols = find(name, scope);
  return symbols.size() ? symbols : my_outer->unqualified_lookup(name, scope);
}

std::string Class::name() const
{
  PTree::Node const *name_spec = PTree::second(my_spec);
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

SymbolSet Namespace::unqualified_lookup(Encoding const &name,
					bool scope) const
{
  SymbolSet symbols = find(name, scope);
  return symbols.size() ? symbols : my_outer->unqualified_lookup(name, scope);
}

std::string Namespace::name() const
{
  PTree::Node const *name_spec = PTree::second(my_spec);
  return name_spec ? std::string(name_spec->position(), name_spec->length()) : "";
}

void Namespace::dump(std::ostream &os, size_t in) const
{
  indent(os, in) << "Namespace'" << this->name() << "':\n";
  Scope::dump(os, in + 1);
}

SymbolSet GlobalScope::unqualified_lookup(Encoding const &name,
					  bool scope) const
{
  SymbolSet symbols = find(name, scope);
  return symbols;
}

void GlobalScope::dump(std::ostream &os, size_t in) const
{
  indent(os, in) << "GlobalScope:\n";
  Scope::dump(os, in + 1);
}
