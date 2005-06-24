//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/SymbolLookup.hh>
#include <Synopsis/Trace.hh>

namespace Synopsis
{

namespace PT = PTree;
namespace ST = SymbolTable;

ST::SymbolSet lookup(PT::Encoding &name,
		     ST::Scope *scope,
		     ST::Scope::LookupContext context)
{
  Trace trace("lookup", Trace::SYMBOLLOOKUP, name);
  // If the name is not qualified, start an unqualified lookup.
  if (!name.is_qualified())
    return scope->unqualified_lookup(name, context);

  PT::Encoding::name_iterator next = name.begin_name();
  // Figure out which scope to start the qualified lookup in.
  if (next->is_global_scope())
  {
    // If the scope is the global scope, do a qualified lookup there.
    scope = scope->global_scope();
  }
  else
  {
    // Else do an unqualified lookup for the scope.
    ST::SymbolSet symbols = scope->unqualified_lookup(*next,
						      context | ST::Scope::SCOPE);
    if (symbols.empty())
      throw ST::Undefined(name);
    else if (symbols.size() > 1)
      // If the name was found multiple times, it must refer to a function,
      // so throw a TypeError.
      throw ST::TypeError(name, (*symbols.begin())->ptree()->encoded_type());

    ST::Symbol const *symbol = *symbols.begin();
    scope = symbol->scope()->find_scope(symbol->ptree());
  }

  PT::Encoding component = *++next;
  ST::SymbolSet symbols = scope->qualified_lookup(component, context);


  // Now iterate over all name components in the qualified name and
  // look them up in their respective (nested) scopes.
  while (++next != name.end_name())
  {
    if (symbols.empty())
      throw ST::Undefined(component);
    else if (symbols.size() > 1)
      // If the name was found multiple times, it must refer to a function,
      // so throw a TypeError.
      throw ST::TypeError(component, (*symbols.begin())->ptree()->encoded_type());

    // As scopes contain a table of nested scopes, accessible through their 
    // respective declaration objects, we find the scope using the symbol's 
    // declaration as key within its scope.

    ST::Symbol const *symbol = *symbols.begin();
    scope = symbol->scope()->find_scope(symbol->ptree());
    if (!scope) 
      throw ST::InternalError("undeclared scope !");

    component = *next;
    symbols = scope->qualified_lookup(component, context);
  }
  return symbols;
}

}
