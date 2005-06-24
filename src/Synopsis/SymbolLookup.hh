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
SymbolTable::SymbolSet lookup(PTree::Encoding &name,
			      SymbolTable::Scope *scope,
			      SymbolTable::Scope::LookupContext);

}

#endif
