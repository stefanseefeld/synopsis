//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/PTree/Visitor.hh>
#include <Synopsis/SymbolLookup/OverloadResolver.hh>
#include <typeinfo>

using namespace Synopsis;
using namespace SymbolLookup;

Symbol *resolve_funcall(PTree::FuncallExpr const *funcall, Table &symbols)
{
  // overload resolution is done in multiple steps:
  //
  // o determine a set of symbols that match the name of the function being called,
  //   including objects
  // o determine the types associated with the parameter expressions
  // o determine the set of viable functions (13.3.2 [over.match.viable])
  // o rank the set of viable functions and select the best viable function
  //   (13.3.3 [over.match.best])
  std::cerr << "resolve_funcall: not implemented yet" << std::endl;
  return 0;
}
