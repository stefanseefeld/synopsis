//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/PTree.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/PTree/Writer.hh>
#include <Synopsis/TypeAnalysis/TypeEvaluator.hh>
#include <Synopsis/TypeAnalysis/Display.hh>
#include <Synopsis/Trace.hh>
#include <Synopsis/SymbolLookup.hh> // FIXME: should that be moved into TA ?
#include "OverloadResolver.hh"
#include <vector>
#include <typeinfo>

using Synopsis::Trace;
using Synopsis::lookup;
namespace PT = Synopsis::PTree;
namespace ST = Synopsis::SymbolTable;
namespace TA = Synopsis::TypeAnalysis;

ST::Symbol const *TA::OverloadResolver::resolve(PT::FuncallExpr const *funcall)
{
  Trace trace("OverloadResolver::resolve", Trace::TYPEANALYSIS);
  // Construct type list from funcall.
  TypeList arguments;
  for (PT::Node const *arglist = PT::third(funcall);
       arglist;
       arglist = PT::rest(PT::rest(arglist)))
  {
    PT::Node const *arg = arglist->car();
    Type const *type = TA::type_of(arg, my_scope);
    arguments.push_back(type);
  }
  PT::Encoding name;
  if (funcall->car()->is_atom())
    name.simple_name(funcall->car());      // PT::Identifier
  else
    name = funcall->car()->encoded_name(); // PT::Name
  ST::SymbolSet symbols = lookup(name, my_scope, ST::Scope::DEFAULT);
  ST::SymbolSet viable = find_viable_set(symbols, arguments);
  // Determine best match.
//   for (ST::SymbolSet::iterator i = viable.begin(); i != viable.end(); ++i)
//     std::cout << "potential match : " << PT::reify((*i)->ptree()) << std::endl;
  return *viable.begin();
}

ST::SymbolSet 
TA::OverloadResolver::find_viable_set(ST::SymbolSet const &symbols,
				      TA::OverloadResolver::TypeList const &args)
{
  Trace trace("OverloadResolver::find_viable_set", Trace::TYPEANALYSIS);
  ST::SymbolSet viable;
  for (ST::SymbolSet::const_iterator i = symbols.begin(); i != symbols.end(); ++i)
  {
    PT::Encoding const &type = (*i)->type();
    assert(type.is_function());
    // Count number of parameters and detect ellipsis.
    PT::Encoding::iterator b = type.begin() + 1; // skip 'F'
    PT::Encoding::iterator e = b;
    while (*e != '_') ++e;
    bool ellipsis = *(e - 2) == 'e';
    size_t count = 0;
    {
      PT::Encoding params(b, e);
      for (PT::Encoding::name_iterator j = params.begin_name();
	   j != params.end_name();
	   ++j)
	if (*j->begin() != 'v') ++count;
    }
    if (count == args.size() ||
	(count < args.size() && ellipsis))
      viable.insert(*i);
    else
    {
      // Determine how many of the parameters take default values.
      size_t min = 0;
      {
	PT::Node const *decl = (*i)->ptree();
	for (PT::Node const *params = PT::third(decl);
	     params;
	     params = PT::rest(PT::rest(params)))
	{
	  if (PT::length(PT::third(params->car())) == 3) break;
	  ++min;
	}
      }
      if (min <= args.size() && count > args.size())
	viable.insert(*i);
    }
  }
  return viable;
}
