//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <PTree/Display.hh>
#include <PTree/Writer.hh>
#include <SymbolLookup/Scope.hh>
#include <functional>

using namespace PTree;
using namespace SymbolLookup;

Scope::~Scope()
{
}

void Scope::declare(const Encoding &name, const Symbol *s)
{
  // it is an error to declare a symbol with conflicting
  // types unless all are functions / function templates
  if (s->type().is_function())
  {
    SymbolTable::const_iterator l = my_symbols.lower_bound(name);
    SymbolTable::const_iterator u = my_symbols.upper_bound(name);
    // FIXME: test for functions and function templates...
    //     for (; l != u; ++l)
    //       if (!l->second->type().is_function())
    // 	throw MultiplyDefined(name);
  }
  else if (my_symbols.count(name))
    throw MultiplyDefined(name,
			  s->ptree(),
			  my_symbols.lower_bound(name)->second->ptree());

  my_symbols.insert(std::make_pair(name, s));
}

SymbolSet Scope::find(const Encoding &name, bool scope) const throw()
{
  SymbolTable::const_iterator l = my_symbols.lower_bound(name);
  SymbolTable::const_iterator u = my_symbols.upper_bound(name);
  SymbolSet symbols;
  // [basic.lookup.qual]
  // During the lookup for a name preceding the :: scope resolution operator, 
  // object, function, and enumerator names are ignored.
  if (scope)
    for (; l != u; ++l)
    {
      if ((!dynamic_cast<VariableName const *>(l->second)) &&
	  (!dynamic_cast<ConstName const *>(l->second)) &&
	  (!dynamic_cast<FunctionName const *>(l->second)))
	symbols.insert(l->second);
    }
  else
    for (; l != u; ++l) symbols.insert(l->second);
  return symbols;
}

SymbolSet Scope::lookup(PTree::Encoding const &name) const
{
  // If the name is qualified, start a qualified lookup.
  if (!name.is_qualified())
    return unqualified_lookup(name, false);

  PTree::Encoding symbol_name = name.get_scope();
  PTree::Encoding remainder = name.get_symbol();
  
  // If the scope is the global scope, do a qualified lookup there.
  if (symbol_name.is_global_scope())
    return global()->qualified_lookup(remainder);

  // Else do an unqualified lookup for the scope, followed by a
  // qualified lookup of the remainder in that scope.
  SymbolSet symbols = unqualified_lookup(symbol_name, true);
  if (symbols.empty())
    throw Undefined(symbol_name);
  else if (symbols.size() > 1)
    // If the name was found multiple times, it must refer to a function,
    // so throw a TypeError.
    throw TypeError(symbol_name, (*symbols.begin())->ptree()->encoded_type());

  // As scopes contain a table of nested scopes, accessible through their respective
  // declaration objects, we find the scope using the symbol's declaration as key
  // within its scope.
  Symbol const *symbol = *symbols.begin();
  Scope const *scope = symbol->scope()->find_scope(symbol->ptree());
  if (!scope) 
    throw InternalError("undeclared scope !");

  // Now do a qualified lookup of the remainder in the given scope.
  return scope->qualified_lookup(remainder);
}

SymbolSet Scope::qualified_lookup(PTree::Encoding const &name) const
{
  PTree::Encoding symbol_name = name.get_scope();
  PTree::Encoding remainder = name.get_symbol();

  if (symbol_name.empty())
  {
    symbol_name = name;
    remainder.clear();
  }

  // find symbol locally
  SymbolSet symbols = find(symbol_name);
  if (symbols.empty()) throw Undefined(symbol_name);
  else if (symbols.size() > 1)
  {
    // can we assume here that all symbols refer to functions ?
    std::cerr << "didn't expect multiple symbols when looking for " << name << std::endl;
    dump(std::cerr, 0);
    throw TypeError(symbol_name, (*symbols.begin())->ptree()->encoded_type());
  }

  // If the remainder is empty, just return the found symbol(s).
  if (remainder.empty()) return symbols;

  // Else take the found symbol to refer to a scope and look up
  // the remainder there.

  // now make sure the symbol indeed refers to a scope
  PTree::Node const *decl = 0;
  if (NamespaceName const *ns = dynamic_cast<NamespaceName const *>(*symbols.begin()))
    decl = ns->ptree();
  else if (TypeName const *tn = dynamic_cast<TypeName const *>(*symbols.begin()))
  {
    decl = tn->ptree();
    // test that 'decl' is a ClassSpec
  }
  // TODO: test for ClassTemplateName ...
  if (!decl)
  {
    // the symbol was found but doesn't refer to a scope
    std::cerr << name << " neither refers to a namespace nore a type" << std::endl;
    dump(std::cerr, 0);
    throw TypeError(symbol_name, (*symbols.begin())->ptree()->encoded_type());
  }
  // move into inner scope and start over the lookup
  Scope const *nested = find_scope(decl);
  if (!nested)
    throw InternalError("undeclared scope !");
  return nested->qualified_lookup(remainder);
}

void Scope::dump(std::ostream &os, size_t in) const
{
  for (SymbolTable::const_iterator i = my_symbols.begin(); i != my_symbols.end(); ++i)
  {
    if (const VariableName *variable = dynamic_cast<const VariableName *>(i->second))
      indent(os, in) << "Variable: " << i->first << ' ' << variable->type() << std::endl;
    else if (const ConstName *const_ = dynamic_cast<const ConstName *>(i->second))
    {
      indent(os, in) << "Const:    " << i->first << ' ' << const_->type();
      if (const_->defined()) os << " (" << const_->value() << ')';
      os << std::endl;
    }
    else if (const NamespaceName *module = dynamic_cast<const NamespaceName *>(i->second))
      indent(os, in) << "Namespace: " << i->first << ' ' << module->type() << std::endl;
    else if (const TypeName *type = dynamic_cast<const TypeName *>(i->second))
      indent(os, in) << "Type: " << i->first << ' ' << type->type() << std::endl;
    else if (const ClassTemplateName *type = dynamic_cast<const ClassTemplateName *>(i->second))
      indent(os, in) << "Class template: " << i->first << ' ' << type->type() << std::endl;
    else if (const FunctionName *type = dynamic_cast<const FunctionName *>(i->second))
      indent(os, in) << "Function: " << i->first << ' ' << type->type() << std::endl;
    else if (const FunctionTemplateName *type = dynamic_cast<const FunctionTemplateName *>(i->second))
      indent(os, in) << "Function template: " << i->first << ' ' << type->type() << std::endl;
    else // shouldn't get here
      indent(os, in) << "Symbol: " << i->first << ' ' << i->second->type() << std::endl;
  }
  for (ScopeTable::const_iterator i = my_scopes.begin(); i != my_scopes.end(); ++i)
    i->second->dump(os, in + 1);
}

std::ostream &Scope::indent(std::ostream &os, size_t i)
{
  while (i--) os.put(' ');
  return os;
}
