//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/PTree.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/PTree/Writer.hh>
#include <Synopsis/SymbolTable/Display.hh>
#include <Synopsis/TypeAnalysis/TypeEvaluator.hh>
#include <Synopsis/TypeAnalysis/TypeRepository.hh>
#include <Synopsis/TypeAnalysis/Display.hh>
#include <Synopsis/Trace.hh>
#include <Synopsis/SymbolLookup.hh> // FIXME: should that be moved into TA ?
#include "OverloadResolver.hh"

using Synopsis::Trace;
using Synopsis::lookup;
namespace PT = Synopsis::PTree;
namespace ST = Synopsis::SymbolTable;
namespace TA = Synopsis::TypeAnalysis;

ST::Symbol const *TA::OverloadResolver::resolve(PT::FuncallExpr const *funcall)
{
  Trace trace("OverloadResolver::resolve", Trace::TYPEANALYSIS);
  // Construct type list from funcall.
  TA::Function::ParameterList arguments;
  for (PT::List const *arglist = static_cast<PT::List *>(PT::nth<2>(funcall));
       arglist;
       arglist = PT::tail(arglist, 2))
  {
    PT::Node const *arg = arglist->car();
    Type const *type = TA::type_of(arg, my_scope);
    arguments.push_back(type);
  }
  PT::Encoding name;
  if (funcall->car()->is_atom())
    name.simple_name(static_cast<PT::Atom *>(funcall->car())); // PT::Identifier
  else
    name = funcall->car()->encoded_name(); // PT::Name
  ST::SymbolSet symbols = lookup(name, my_scope, ST::Scope::DEFAULT);
  ST::SymbolSet viable = find_viable_set(symbols, arguments);
  std::cout << "viable functions :" << std::endl;
  ST::display(viable, std::cout);
  // Rank functions according to type matches.
  unsigned int best_penalty = MISMATCH;
  ST::Symbol const *best_func = 0;
  for (ST::SymbolSet::iterator i = viable.begin(); i != viable.end(); ++i)
  {
    PT::Encoding const &type = (*i)->type();
    assert(type.is_function()); // FIXME: what about the call operator ?
    TypeRepository *repo = TypeRepository::instance();
    Function const *function = static_cast<Function const *>(repo->lookup(type, (*i)->scope()));
    Function::ParameterList const &params = function->params();
    Function::ParameterList::const_iterator p = params.begin();
    Function::ParameterList::const_iterator a = arguments.begin();
    unsigned int penalty = MATCH;
    for (; p != params.end(), a != arguments.end(); ++p, ++a)
      penalty += conversion_penalty(*a, *p);
    if (penalty < MISMATCH && penalty < best_penalty)
    {
      best_penalty = penalty;
      best_func = *i;
    }
  }
  if (best_func)
  {
    std::cout << "the winner is : " << PT::reify(best_func->ptree()) << std::endl;
  }
  return best_func;
}

ST::SymbolSet 
TA::OverloadResolver::find_viable_set(ST::SymbolSet const &symbols,
				      TA::Function::ParameterList const &args)
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
    ST::FunctionName const *func = static_cast<ST::FunctionName const *>(*i);
    size_t params = func->params();
    size_t default_args = func->default_args();
    // If the number of arguments match the parameters the candidate is viable.
    if (params == args.size()) viable.insert(*i);
    else if (params < args.size())
    {
      // If the number of arguments exceeds the number of parameters,
      // but we have an ellipse the candidate is viable.
      if (ellipsis) viable.insert(*i);
    }
    // If the number of arguments  matches at least the number
    // pf parameters without default argument the candidate is viable.
    else if (params - default_args <= args.size()) viable.insert(*i);
  }
  return viable;
}

unsigned int TA::OverloadResolver::conversion_penalty(TA::Type const *from,
						      TA::Type const *to)
{
  Trace trace("OverloadResolver::conversion_penalty", Trace::TYPEANALYSIS);
  std::cout << "from ";
  display(from, std::cout);
  std::cout << " to ";
  display(to, std::cout);
  std::cout << std::endl;
  if (from == to) return MATCH; // Found an exact match.
  // FIXME: TBD
  return MISMATCH;
}
