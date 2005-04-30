//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/SymbolLookup/Scopes.hh>
#include <Synopsis/PTree/Writer.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/Trace.hh>
#include <sstream>
#include <cassert>

using namespace Synopsis;
using namespace PTree;
using namespace SymbolLookup;

SymbolSet TemplateParameterScope::unqualified_lookup(Encoding const &name,
						     bool scope) const
{
  Trace trace("TemplateParameterScope::unqualified_lookup", Trace::SYMBOLLOOKUP);
  trace << name;
  SymbolSet symbols = find(name, scope);
  return symbols;
}

void TemplateParameterScope::dump(std::ostream &os, size_t in) const
{
  indent(os, in) << "TemplateParameterScope:\n";
  Scope::dump(os, in);
}

SymbolSet LocalScope::unqualified_lookup(Encoding const &name,
					 bool scope) const
{
  Trace trace("LocalScope::unqualified_lookup", Trace::SYMBOLLOOKUP);
  trace << name;
  SymbolSet symbols = find(name, scope);
  return symbols.size() ? symbols : my_outer->unqualified_lookup(name, scope);
}

void LocalScope::dump(std::ostream &os, size_t in) const
{
  indent(os, in) << "LocalScope:\n";
  Scope::dump(os, in + 1);
}

void FunctionScope::use(PTree::UsingDirective const *udecl)
{
  if (*PTree::second(udecl) == "namespace") // using.dir
  {
    PTree::Encoding name = PTree::third(udecl)->encoded_name();
    SymbolSet symbols = lookup(name);
    // now get the namespace associated with that symbol:
    Symbol const *symbol = *symbols.begin();
    // FIXME: may this symbol-to-scope conversion be implemented
    //        by the appropriate Symbol subclass(es) ?
    Scope const *scope = symbol->scope()->find_scope(name, symbol);
    Namespace const *ns = dynamic_cast<Namespace const *>(scope);
    if (ns) my_using.push_back(ns);
  }
  else
  {
    std::cout << "sorry, using declaration not supported yet" << std::endl;
  }
}

SymbolSet FunctionScope::unqualified_lookup(Encoding const &name,
					    bool scope) const
{
  Trace trace("FunctionScope::unqualified_lookup", Trace::SYMBOLLOOKUP);
  trace << name;
  SymbolSet symbols = find(name, scope);
  if (symbols.size()) return symbols;

  // see 7.3.4 [namespace.udir]
  for (Using::const_iterator i = my_using.begin(); i != my_using.end(); ++i)
  {
    SymbolSet more = (*i)->unqualified_lookup(name, scope);
    symbols.insert(more.begin(), more.end());
  }
  if (symbols.size() || !my_outer)
    return symbols;
  else
    return my_outer->unqualified_lookup(name, scope);
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

SymbolSet PrototypeScope::unqualified_lookup(PTree::Encoding const &name,
					     bool scope) const
{
  Trace trace("PrototypeScope::unqualified_lookup", Trace::SYMBOLLOOKUP);
  return SymbolSet();
}

SymbolSet FunctionScope::qualified_lookup(PTree::Encoding const &name) const
{
  Trace trace("FunctionScope::qualified_lookup", Trace::SYMBOLLOOKUP);
  trace << name;
  PTree::Encoding symbol_name = name.get_scope();
  PTree::Encoding remainder = name.get_symbol();

  if (symbol_name.empty())
  {
    symbol_name = name;
    remainder.clear();
  }

  // find symbol locally
  SymbolSet symbols = find(symbol_name);
  // see 7.3.4 [namespace.udir]
  if (symbols.empty())
    for (Using::const_iterator i = my_using.begin(); i != my_using.end(); ++i)
    {
      SymbolSet more = (*i)->qualified_lookup(name);
      symbols.insert(more.begin(), more.end());
    }
  if (symbols.empty()) return symbols; // nothing found

  // If the remainder is empty, just return the found symbol(s).
  else if (remainder.empty()) return symbols;

  // Having multiple symbols implies they are all overloaded functions.
  // That's a type error if the reminder is non-empty, as we are looking
  // for a scope.
  else if (symbols.size() > 1)
    throw TypeError(symbol_name, (*symbols.begin())->ptree()->encoded_type());

  // Find the scope the symbol refers to 
  // and look up the remainder there.

  // move into inner scope and start over the lookup
  Scope const *nested = find_scope(symbol_name, *symbols.begin());
  if (!nested) throw InternalError("undeclared scope !");

  return nested->qualified_lookup(remainder);
}

void PrototypeScope::dump(std::ostream &os, size_t in) const
{
  indent(os, in) << "Prototype:\n";
  Scope::dump(os, in + 1);
}

SymbolSet Class::unqualified_lookup(Encoding const &name,
				    bool scope) const
{
  Trace trace("Class::unqualified_lookup", Trace::SYMBOLLOOKUP);
  trace << name;
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

Namespace *Namespace::find_namespace(std::string const &name) const
{
  for (ScopeTable::const_iterator i = my_scopes.begin();
       i != my_scopes.end();
       ++i)
  {
    Namespace *ns = dynamic_cast<Namespace *>(i->second);
    if (ns && name == ns->name())
      return ns;
  }
  return 0;
}

void Namespace::use(PTree::UsingDirective const *udecl)
{
  if (*PTree::second(udecl) == "namespace") // using.dir
  {
    PTree::Encoding name = PTree::third(udecl)->encoded_name();
    SymbolSet symbols = lookup(name);
    // now get the namespace associated with that symbol:
    Symbol const *symbol = *symbols.begin();
    // FIXME: may this symbol-to-scope conversion be implemented
    //        by the appropriate Symbol subclass(es) ?
    Scope const *scope = symbol->scope()->find_scope(name, symbol);
    Namespace const *ns = dynamic_cast<Namespace const *>(scope);
    if (ns) my_using.push_back(ns);
  }
  else
  {
    std::cout << "sorry, using declaration not supported yet" << std::endl;
  }
}

SymbolSet Namespace::unqualified_lookup(Encoding const &name,
					bool scope) const
{
  Trace trace("Namespace::unqualified_lookup", Trace::SYMBOLLOOKUP);
  trace << name;
  SymbolSet symbols = find(name, scope);
  if (symbols.size()) return symbols;

  // see 7.3.4 [namespace.udir]
  for (Using::const_iterator i = my_using.begin(); i != my_using.end(); ++i)
  {
    SymbolSet more = (*i)->unqualified_lookup(name, scope);
    symbols.insert(more.begin(), more.end());
  }
  if (symbols.size() || !my_outer)
    return symbols;
  else
    return my_outer->unqualified_lookup(name, scope);
}

SymbolSet Namespace::qualified_lookup(PTree::Encoding const &name) const
{
  Trace trace("Namespace::qualified_lookup", Trace::SYMBOLLOOKUP, this->name());
  trace << name;
  PTree::Encoding symbol_name = name.get_scope();
  PTree::Encoding remainder = name.get_symbol();

  if (symbol_name.empty())
  {
    symbol_name = name;
    remainder.clear();
  }

  // find symbol locally
  SymbolSet symbols = find(symbol_name);
  // see 7.3.4 [namespace.udir]
  if (symbols.empty())
    for (Using::const_iterator i = my_using.begin(); i != my_using.end(); ++i)
    {
      SymbolSet more = (*i)->qualified_lookup(name);
      symbols.insert(more.begin(), more.end());
    }
  if (symbols.empty()) return symbols; // nothing found

  // If the remainder is empty, just return the found symbol(s).
  else if (remainder.empty()) return symbols;

  // Having multiple symbols implies they are all overloaded functions.
  // That's a type error if the reminder is non-empty, as we are looking
  // for a scope.
  else if (symbols.size() > 1)
    throw TypeError(symbol_name, (*symbols.begin())->ptree()->encoded_type());

  // Find the scope the symbol refers to 
  // and look up the remainder there.

  // move into inner scope and start over the lookup
  Scope const *nested = find_scope(symbol_name, *symbols.begin());
  if (!nested) throw InternalError("undeclared scope !");

  return nested->qualified_lookup(remainder);
}

std::string Namespace::name() const
{
  if (!my_spec) return "<global>";
  PTree::Node const *name_spec = PTree::second(my_spec);
  if (name_spec)
    return std::string(name_spec->position(), name_spec->length());
  else
    return "<anonymous>";
}

void Namespace::dump(std::ostream &os, size_t in) const
{
  indent(os, in) << "Namespace'" << this->name() << "':\n";
  Scope::dump(os, in + 1);
}
