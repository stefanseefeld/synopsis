//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <PTree/Display.hh>
#include <Symbol.hh>
#include <cassert>

Scope::~Scope()
{
  std::cout << "Scope::~Scope()" << std::endl;
  for (SymbolTable::iterator i = my_symbols.begin(); i != my_symbols.end(); ++i)
  {
    switch (i->second.type)
    {
      case Symbol::VARIABLE:
	std::cout << "Variable : ";
	break;
      case Symbol::CONST:
	std::cout << "Const    : ";
	break;
      case Symbol::TYPE:
	std::cout << "Type     : ";
	break;
    }
    PTree::display(i->second.ptree, std::cout, false);
  }
}

void Scope::declare(const PTree::Encoding &id, const Symbol &s)
{
  assert(my_symbols.find(id) == my_symbols.end());
  my_symbols[id] = s;
}

Symbol::Type Scope::lookup(const PTree::Encoding &id, Symbol &s) const
{
  SymbolTable::const_iterator i = my_symbols.find(id);
  if (i != my_symbols.end())
  {
    s = i->second;
    return s.type;
  }
  else return Symbol::NONE;
}

Symbol::Type NestedScope::lookup(const PTree::Encoding &id, Symbol &s) const
{
  if (!Scope::lookup(id, s)) return my_outer->lookup(id, s);
  else return s.type;
}

