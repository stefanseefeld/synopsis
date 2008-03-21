//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/SymbolLookup.hh>
#include <Synopsis/SymbolTable/Display.hh>
#include <Synopsis/Trace.hh>

namespace Synopsis
{

namespace PT = PTree;
namespace ST = SymbolTable;

ST::SymbolSet lookup(PT::Encoding const &name,
		     ST::Scope const *scope,
		     ST::Scope::LookupContext context)
{
  Trace trace("lookup", Trace::SYMBOLLOOKUP, name);
  // If the name is not qualified, start an unqualified lookup.
  if (!name.is_qualified())
    return scope->unqualified_lookup(name, context);

  PT::Encoding::name_iterator next = name.begin_name();
  trace << "first component " << *next << ' ' << next->unmangled();
  // Figure out which scope to start the qualified lookup in.
  if (next->is_global_scope())
  {
    // If the scope is the global scope, do a qualified lookup there.
    scope = const_cast<ST::Scope *>(scope)->global_scope();
  }
  else
  {
    // Else do an unqualified lookup for the scope.
    PT::Encoding component = *next;
    if (component.is_template_id()) component = component.get_template_name();
    ST::SymbolSet symbols = scope->unqualified_lookup(component,
						      context | ST::Scope::SCOPE);
    if (symbols.empty())
      throw ST::Undefined(component, scope);
//     else if (symbols.size() > 1)
//       // If the name was found multiple times, it must refer to a function,
//       // so throw a TypeError.
//       {
// 	trace << "xxx";
// 	ST::display(symbols, std::cout);
//       throw ST::TypeError(component, (*symbols.begin())->ptree()->encoded_type());
//       }
    ST::Symbol const *symbol = *symbols.begin();
    // If we found a type name (i.e. *not* a class name), it is a dependent
    // type, in which case we stop the lookup and return DEPENDENT.
    if (!dynamic_cast<ST::ClassName const *>(symbol) &&
	dynamic_cast<ST::TypeName const *>(symbol))
    {
      ST::SymbolSet symbols;
      symbols.insert(ST::DEPENDENT);
      return symbols;
    }
    scope = symbol->scope()->find_scope(symbol->ptree());
    if (!scope) throw InternalError("Undeclared scope for " + name.unmangled());
  }

  PT::Encoding component = *++next;
  if (component.is_template_id()) component = component.get_template_name();
  trace << "next component " << component;
  ST::SymbolSet symbols = scope->qualified_lookup(component, context);

  // Now iterate over all name components in the qualified name and
  // look them up in their respective (nested) scopes.
  while (++next != name.end_name())
  {
    trace << "next component " << *next << ' ' << next->unmangled();
    if (symbols.empty())
      throw ST::Undefined(component, scope);
    else if (symbols.size() > 1)
      // If the name was found multiple times, it must refer to a function,
      // so throw a TypeError.
      throw ST::TypeError(component, (*symbols.begin())->ptree()->encoded_type());

    // As scopes contain a table of nested scopes, accessible through their 
    // respective declaration objects, we find the scope using the symbol's 
    // declaration as key within its scope.

    ST::Symbol const *symbol = *symbols.begin();

    if (!dynamic_cast<ST::ClassName const *>(symbol) &&
	dynamic_cast<ST::TypeName const *>(symbol))
    {
      ST::SymbolSet symbols;
      symbols.insert(ST::DEPENDENT);
      return symbols;
    }

    scope = symbol->scope()->find_scope(symbol->ptree());
    if (!scope) throw InternalError("Undeclared scope for " + name.unmangled());

    component = *next;
    if (component.is_template_id()) component = component.get_template_name();
    symbols = scope->qualified_lookup(component, context);
  }
  return symbols;
}

class TypedefResolver : private ST::SymbolVisitor
{
public:
  TypedefResolver(PT::Encoding &encoding) : symbol_(0), encoding_(encoding) {}
  ST::Symbol const *resolve(ST::Symbol const *symbol)
  {
    symbol->accept(this);
    return symbol_;
  }
private:
  virtual void visit(ST::TypedefName const *s)
  {
    encoding_ = s->type();
    if (encoding_.is_simple_name() || encoding_.is_qualified())
    {
      ST::Symbol const *a = s->aliased();
      if (a) a->accept(this);
    }
    else if (encoding_.is_template_id())
    {
      encoding_ = encoding_.get_template_name();
      ST::Symbol const *a = s->aliased();
      if (a) a->accept(this);      
    }
    else
    {
//       std::cout << "can't resolve " << encoding_ << ' ' << typeid(*s).name() << std::endl;
      symbol_ = s;
    }
  }
  virtual void visit(ST::ClassName const *s)
  {
    symbol_ = s;
    encoding_ = s->type();
  }
  virtual void visit(ST::EnumName const *s)
  {
    symbol_ = s;
    encoding_ = s->type();
  }
  virtual void visit(ST::DependentName const *s)
  {
    symbol_ = s;
    encoding_ = s->type();
  }
  virtual void visit(ST::ClassTemplateName const *s)
  {
    symbol_ = s;
    encoding_ = s->type();
  }

  ST::Symbol const *symbol_;
  PT::Encoding &    encoding_;
};

ST::Symbol const *resolve_typedef(ST::Symbol const *symbol,
				  PT::Encoding &type)
{
  TypedefResolver resolver(type);
  return resolver.resolve(symbol);
}



}
