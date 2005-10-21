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

//. Convenience wrapper around 'lookup' that only reports a namespace name 
//. if found.
inline SymbolTable::NamespaceName const *
lookup_namespace(PTree::Encoding const &name, SymbolTable::Scope const *scope)
{
  SymbolTable::SymbolSet symbols = lookup(name, scope, SymbolTable::Scope::SCOPE);
  if (symbols.size() != 1) 
    return 0;
  else
    return dynamic_cast<SymbolTable::NamespaceName const *>(*symbols.begin());
}

//. Convenience wrapper around 'lookup' that only reports a class name 
//. if found.
inline SymbolTable::ClassName const *
lookup_class(PTree::Encoding const &name, SymbolTable::Scope const *scope)
{
  SymbolTable::SymbolSet symbols = lookup(name, scope, SymbolTable::Scope::ELABORATE);
  if (symbols.size() != 1) 
    return 0;
  else
    return dynamic_cast<SymbolTable::ClassName const *>(*symbols.begin());
}

}

#endif
