//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_SymbolLookup_OverloadResolver_hh_
#define Synopsis_SymbolLookup_OverloadResolver_hh_

#include <Synopsis/SymbolLookup/Table.hh>

namespace Synopsis
{
namespace SymbolLookup
{

//. Resolve a function call in the context of the given scope.
Symbol const *resolve_funcall(PTree::FuncallExpr const *funcall, Scope const *);

}
}

#endif

