//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _PTree_ConstEvaluator_hh
#define _PTree_ConstEvaluator_hh

#include <PTree/Visitor.hh>
#include <PTree/Symbol.hh>
#include <PTree/Atoms.hh>
#include <PTree/Lists.hh>
#include <cassert>

namespace PTree
{

//. evaluate the value of a constant expression
class ConstEvaluator : public Visitor
{
public:
  ConstEvaluator(const Scope &s) : my_valid(false), my_symbols(s) {}
  bool evaluate(Node *node, long &value);

private:
  virtual void visit(Literal *);
  virtual void visit(Identifier *);
  virtual void visit(InfixExpr *);
  virtual void visit(UnaryExpr *);
  virtual void visit(CondExpr *);
  virtual void visit(ParenExpr *);
  
  bool         my_valid;
  long         my_value;
  const Scope &my_symbols;
};
  
inline bool evaluate_const(const Node *node, const Scope &symbols,
			   long &value)
{
  if (!node) return false;
  ConstEvaluator e(symbols);
  return e.evaluate(const_cast<Node *>(node), value);
}

}

#endif

