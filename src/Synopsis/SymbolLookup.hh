//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_SymbolLookup_hh_
#define Synopsis_SymbolLookup_hh_

#include <Synopsis/SymbolTable.hh>

namespace Synopsis
{

//. Look up the encoded name and return a set of matching symbols.
//. This process may become rather involved, as it may require
//. templates to be instantiated. For this reason it can't be part
//. of the SymbolTable module, as that doesn't have any knowledge of
//. type analysis.
SymbolTable::SymbolSet lookup(PTree::Encoding const &name,
			      SymbolTable::Scope const *scope,
			      SymbolTable::Scope::LookupContext);

inline SymbolTable::TypeName const *
resolve_type(SymbolTable::TypedefName const *symbol)
{
  SymbolTable::TypeName const *type_name = 0;
  do
  {
    SymbolTable::Scope *scope = symbol->scope();
    SymbolTable::SymbolSet symbols = lookup(symbol->type(), symbol->scope(),
					    SymbolTable::Scope::DEFAULT);
    if (symbols.size() != 1) return 0; // TODO: handle error
    type_name = dynamic_cast<SymbolTable::TypeName const *>(*symbols.begin());
    if (!type_name) return 0; // not a type
    symbol = dynamic_cast<SymbolTable::TypedefName const *>(type_name);
  }
  while (symbol);
  return type_name;
}

}

#endif
