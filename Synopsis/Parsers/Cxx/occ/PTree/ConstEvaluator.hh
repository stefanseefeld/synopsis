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
  ConstEvaluator() : my_type(Symbol::NONE) {}
  bool evaluate(Node *node, unsigned long &value);

private:
  virtual void visit(Literal *);
  virtual void visit(InfixExpr *);
  
  Symbol::Type  my_type;
  unsigned long my_value;
};
  
inline bool evaluate_const(const Node *node, unsigned long &value)
{
  if (!node) return false;
  ConstEvaluator e;
  return e.evaluate(const_cast<Node *>(node), value);
}

}

#endif

