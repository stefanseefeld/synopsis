//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <PTree/Display.hh>
#include <PTree/Writer.hh>
#include <SymbolLookup/Symbol.hh>

using namespace PTree;
using namespace SymbolLookup;

Scope::~Scope()
{
}

void Scope::declare(const Encoding &name, const Symbol *s)
{
  my_symbols[name] = s;
}

const Symbol *Scope::lookup(const Encoding &id) const throw()
{
  HashTable::const_iterator i = my_symbols.find(id);
  return i == my_symbols.end() ? 0 : i->second;
}

void Scope::dump(std::ostream &os) const
{
  os << "Scope::dump:" << std::endl;
  for (HashTable::const_iterator i = my_symbols.begin(); i != my_symbols.end(); ++i)
  {
    if (const VariableName *variable = dynamic_cast<const VariableName *>(i->second))
      os << "Variable: " << i->first << ' ' << variable->type() << std::endl;
    else if (const ConstName *const_ = dynamic_cast<const ConstName *>(i->second))
    {
      os << "Const:    " << i->first << ' ' << const_->type();
      if (const_->defined()) os << " (" << const_->value() << ')';
      os << std::endl;
    }
    else if (const TypeName *type = dynamic_cast<const TypeName *>(i->second))
      os << "Type: " << i->first << ' ' << type->type() << std::endl;
    else if (const ClassTemplateName *type = dynamic_cast<const ClassTemplateName *>(i->second))
      os << "Class template: " << i->first << ' ' << type->type() << std::endl;
    else if (const FunctionTemplateName *type = dynamic_cast<const FunctionTemplateName *>(i->second))
      os << "Function template: " << i->first << ' ' << type->type() << std::endl;
    else // shouldn't get here
      os << "Symbol: " << i->first << ' ' << i->second->type() << std::endl;
  }  
}
