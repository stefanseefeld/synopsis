//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_SymbolLookup_ConstEvaluator_hh_
#define Synopsis_SymbolLookup_ConstEvaluator_hh_

#include <Synopsis/PTree/Visitor.hh>
#include <Synopsis/PTree/Atoms.hh>
#include <Synopsis/PTree/Lists.hh>
#include <Synopsis/SymbolLookup/Scope.hh>
#include <cassert>

namespace Synopsis
{
namespace SymbolLookup
{

//. evaluate the value of a constant expression
class ConstEvaluator : public PTree::Visitor
{
public:
  ConstEvaluator(const Scope &s) : my_valid(false), my_symbols(s) {}
  bool evaluate(PTree::Node *node, long &value);

private:
  virtual void visit(PTree::Literal *);
  virtual void visit(PTree::Identifier *);
  virtual void visit(PTree::FstyleCastExpr *);
  virtual void visit(PTree::InfixExpr *);
  virtual void visit(PTree::SizeofExpr *);
  virtual void visit(PTree::UnaryExpr *);
  virtual void visit(PTree::CondExpr *);
  virtual void visit(PTree::ParenExpr *);
  
  bool         my_valid;
  long         my_value;
  const Scope &my_symbols;
};
  
}
}

#endif

