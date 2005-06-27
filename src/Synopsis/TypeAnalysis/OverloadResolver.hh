//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_TypeAnalysis_OverloadResolver_hh_
#define Synopsis_TypeAnalysis_OverloadResolver_hh_

#include <Synopsis/PTree.hh>
#include <Synopsis/SymbolTable.hh>
#include <Synopsis/TypeAnalysis/Type.hh>

namespace Synopsis
{
namespace TypeAnalysis
{

//. Helper class to assist in the function overload resolution.
//. It is exposed here mainly so it can be unit-tested.
class OverloadResolver
{
public:
  OverloadResolver(SymbolTable::Scope *scope) : my_scope(scope) {}
  //. Resolve a function call using the given set of functions.
  SymbolTable::Symbol const *resolve(PTree::FuncallExpr const *funcall);

private:
  static const unsigned int MATCH = 0;
  static const unsigned int MISMATCH = 1000;

  //. Determine viable functions given the set of functions found by symbol lookup
  //. (see [13.3.2]).
  SymbolTable::SymbolSet find_viable_set(SymbolTable::SymbolSet const &,
					 Function::ParameterList const &);
  //. Determine a penalty associated with a type conversion 'from' -> 'to'.
  //. Return '0' for an exact match, or '~0' for a mismatch.
  unsigned int conversion_penalty(Type const *from, Type const *to);

  SymbolTable::Scope *my_scope;
};


//. Resolve a function call using the given set of functions.
inline SymbolTable::Symbol const *resolve_funcall(PTree::FuncallExpr const *funcall,
						  SymbolTable::Scope *scope)
{
  OverloadResolver resolver(scope);
  return resolver.resolve(funcall);
}

}
}

#endif

