//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_SymbolLookup_TypeEvaluator_hh_
#define Synopsis_SymbolLookup_TypeEvaluator_hh_

#include <Synopsis/PTree/Visitor.hh>
#include <Synopsis/PTree/Atoms.hh>
#include <Synopsis/PTree/Lists.hh>
#include <Synopsis/SymbolLookup/Scope.hh>

namespace Synopsis
{
namespace SymbolLookup
{

//. evaluate the type of an expression
class TypeEvaluator : private PTree::Visitor
{
public:
  TypeEvaluator(const Scope &s) : my_symbols(s) {}
  PTree::Encoding evaluate(PTree::Node *node);

private:
  virtual void visit(PTree::Literal *);
  virtual void visit(PTree::Identifier *);
  virtual void visit(PTree::FstyleCastExpr *);
  virtual void visit(PTree::InfixExpr *);
  virtual void visit(PTree::SizeofExpr *);
  virtual void visit(PTree::UnaryExpr *);
  virtual void visit(PTree::CondExpr *);
  virtual void visit(PTree::ParenExpr *);
  
  const Scope    &my_symbols;
  PTree::Encoding my_type;
};
  
}
}

#endif

