//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/PTree/Writer.hh>
#include <Synopsis/SymbolTable/Scope.hh>
#include <Synopsis/Trace.hh>
#include <functional>

using namespace Synopsis;
using namespace PTree;
using namespace SymbolTable;

Scope::~Scope()
{
}

void Scope::declare(Encoding const &name, Symbol const *symbol)
{
  Trace trace("Scope::declare", Trace::SYMBOLLOOKUP);
  trace << name;
  my_symbols.insert(std::make_pair(name, symbol));
}

void Scope::use(PTree::UsingDirective const *)
{
  throw InternalError("Invalid use of using directive in this scope.");
}

Scope *Scope::find_scope(PTree::Encoding const &name, Symbol const *symbol) const
{
  PTree::Node const *decl = 0;
  if (NamespaceName const *ns = dynamic_cast<NamespaceName const *>(symbol))
    decl = ns->ptree();
  else if (TypeName const *tn = dynamic_cast<TypeName const *>(symbol))
  {
    decl = tn->ptree();
    // test that 'decl' is a ClassSpec
  }
  else if (ClassTemplateName const *ctn = dynamic_cast<ClassTemplateName const *>(symbol))
  {
    decl = ctn->ptree();
  }
  if (!decl)
  {
    // the symbol was found but doesn't refer to a scope
    std::cerr << name << " neither refers to a namespace nor a type" << std::endl;
    throw TypeError(name, symbol->ptree()->encoded_type());
  }
  return find_scope(decl);
}

void Scope::remove_scope(PTree::Node const *decl)
{
  ScopeTable::iterator i = my_scopes.find(decl);
  if (i == my_scopes.end()) throw InternalError("Attempt to remove unknown scope !");
  my_scopes.erase(i);
}

SymbolSet Scope::find(Encoding const &name, LookupContext context) const
{
  Trace trace("Scope::find", Trace::SYMBOLLOOKUP);
  trace << name << ' ' << context;
  SymbolTable::const_iterator l = my_symbols.lower_bound(name);
  SymbolTable::const_iterator u = my_symbols.upper_bound(name);
  SymbolSet symbols;
  // [basic.lookup.qual]
  // During the lookup for a name preceding the :: scope resolution operator, 
  // object, function, and enumerator names are ignored.
  if (context & SCOPE)
    for (; l != u; ++l)
    {
      if ((!dynamic_cast<VariableName const *>(l->second)) &&
	  (!dynamic_cast<FunctionName const *>(l->second)))
	symbols.insert(l->second);
    }
  // [basic.lookup.elab]
  else if (context & ELABORATED)
    for (; l != u; ++l)
    {
      if ((dynamic_cast<ClassName const *>(l->second)) ||
	  (dynamic_cast<EnumName const *>(l->second)))
	symbols.insert(l->second);
    }
  else if (context & TYPE)
    for (; l != u; ++l)
    {
      if (dynamic_cast<TypeName const *>(l->second))
	symbols.insert(l->second);
    }
  // [basic.scope.hiding]
  else
  {
    // There is at most one type-name, which needs to be
    // hidden if any other symbol was found.
    TypeName const *type_name = 0;
    for (; l != u; ++l)
    {
      trace << "considering " << typeid(*l->second).name();
      TypeName const *type = dynamic_cast<TypeName const *>(l->second);
      if (!type) symbols.insert(l->second);
      else type_name = type;
    }
    if (!symbols.size() && type_name) symbols.insert(type_name);
  }
  trace << "found " << symbols.size() << " matches";
  return symbols;
}

void Scope::remove(Symbol const *symbol)
{
  Trace trace("Scope::remove", Trace::SYMBOLLOOKUP);
  for (SymbolTable::iterator i = my_symbols.begin(); i != my_symbols.end(); ++i)
    if (i->second == symbol)
    {
      my_symbols.erase(i);
      delete symbol;
      return;
    }
  // FIXME: Calling 'remove' with an unknown symbol is an error.
  // Should we throw here ? 
}

SymbolSet 
Scope::qualified_lookup(PTree::Encoding const &name,
			LookupContext context,
			Scopes &scopes) const
{
  Trace trace("Scope::qualified_lookup", Trace::SYMBOLLOOKUP, name);
  return find(name, context);
}
