//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _SymbolLookup_OverloadResolver_hh
#define _SymbolLookup_OverloadResolver_hh

#include <SymbolLookup/Table.hh>

namespace SymbolLookup
{

//. Resolve a function call in the context of the current symbol lookup table.
Symbol *resolve_funcall(PTree::FuncallExpr const *funcall, Table &symbols);

}

#endif

