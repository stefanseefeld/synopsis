//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _SymbolTable_ConstEvaluator_hh
#define _SymbolTable_ConstEvaluator_hh

#include <PTree/Visitor.hh>
#include <PTree/Atoms.hh>
#include <PTree/Lists.hh>
#include <SymbolTable/Symbol.hh>
#include <cassert>

namespace SymbolTable
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
  
inline bool evaluate_const(const PTree::Node *node, const Scope &symbols,
			   long &value)
{
  if (!node) return false;
  ConstEvaluator e(symbols);
  return e.evaluate(const_cast<PTree::Node *>(node), value);
}

}

#endif

