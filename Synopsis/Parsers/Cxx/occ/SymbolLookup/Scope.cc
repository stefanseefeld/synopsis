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
  // it is an error if the symbol was already defined in this scope
  // and either is not a function / function template
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
			  my_symbols.lower_bound(name)->second->ptree(),
			  s->ptree());

  my_symbols.insert(std::make_pair(name, s));
}

std::set<Symbol const *> Scope::lookup(const Encoding &name) const throw()
{
  SymbolTable::const_iterator l = my_symbols.lower_bound(name);
  SymbolTable::const_iterator u = my_symbols.upper_bound(name);
  std::set<Symbol const *> symbols;
  for (; l != u; ++l) symbols.insert(l->second);
  return symbols;
  
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
