//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/PTree/Writer.hh>
#include <Synopsis/SymbolLookup/Scope.hh>
#include <Synopsis/Trace.hh>
#include <functional>

using namespace Synopsis;
using namespace PTree;
using namespace SymbolLookup;

Scope::~Scope()
{
}

void Scope::declare(Encoding const &name, Symbol const *symbol)
{
  Trace trace("Scope::declare");
  trace << name;
  // Conditions under which a symbol can be bound to multiple
  // objects:
  // * overloaded functions
  // * typedefs redefining an already declared or defined type
  // * defining a type that had been forward declared and typedefed before
  //
  if (symbol->type().is_function())
  {
    SymbolTable::const_iterator l = my_symbols.lower_bound(name);
    SymbolTable::const_iterator u = my_symbols.upper_bound(name);
    // FIXME: test for functions and function templates...
    //     for (; l != u; ++l)
    //       if (!l->second->type().is_function())
    // 	throw MultiplyDefined(name);
  }
  else if (my_symbols.count(name))
  {
    // This is allowed only if one of the symbols is a typedef and the other
    // a typename.
    SymbolTable::const_iterator s = my_symbols.find(name);
    if (!(dynamic_cast<TypeName const *>(s->second) &&
	  dynamic_cast<TypedefName const *>(symbol)) &&
	!(dynamic_cast<TypedefName const *>(s->second) && 
	  dynamic_cast<TypeName const *>(symbol)))
    {
      throw MultiplyDefined(name,
			    symbol->ptree(),
			    my_symbols.lower_bound(name)->second->ptree());
    }
  }

  my_symbols.insert(std::make_pair(name, symbol));
}

void Scope::use(PTree::Using const *udecl)
{
  throw InternalError("Invalid use of 'using' declaration.");
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
  // TODO: test for ClassTemplateName ...
  if (!decl)
  {
    // the symbol was found but doesn't refer to a scope
    std::cerr << name << " neither refers to a namespace nor a type" << std::endl;
    throw TypeError(name, symbol->ptree()->encoded_type());
  }
  return find_scope(decl);
}

SymbolSet Scope::find(Encoding const &name, bool scope) const throw()
{
  Trace trace("Scope::find");
  trace << name;
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
  Trace trace("Scope::lookup");
  trace << name;
  // If the name is not qualified, start an unqualified lookup.
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
  Trace trace("Scope::qualified_lookup");
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
  if (symbols.empty()) throw Undefined(symbol_name);

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

void Scope::dump(std::ostream &os, size_t in) const
{
  for (SymbolTable::const_iterator i = my_symbols.begin(); i != my_symbols.end(); ++i)
  {
    if (VariableName const *variable = dynamic_cast<VariableName const *>(i->second))
      indent(os, in) << "Variable: " << i->first << ' ' << variable->type() << std::endl;
    else if (ConstName const *const_ = dynamic_cast<ConstName const *>(i->second))
    {
      indent(os, in) << "Const:    " << i->first << ' ' << const_->type();
      if (const_->defined()) os << " (" << const_->value() << ')';
      os << std::endl;
    }
    else if (NamespaceName const *module = dynamic_cast<NamespaceName const *>(i->second))
      indent(os, in) << "Namespace: " << i->first << ' ' << module->type() << std::endl;
    else if (TypeName const *type = dynamic_cast<TypeName const *>(i->second))
      indent(os, in) << "Type: " << i->first << ' ' << type->type() << std::endl;
    else if (ClassTemplateName const *type = dynamic_cast<ClassTemplateName const *>(i->second))
      indent(os, in) << "Class template: " << i->first << ' ' << type->type() << std::endl;
    else if (FunctionName const *type = dynamic_cast<FunctionName const *>(i->second))
      indent(os, in) << "Function: " << i->first << ' ' << type->type() << std::endl;
    else if (FunctionTemplateName const *type = dynamic_cast<FunctionTemplateName const *>(i->second))
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
