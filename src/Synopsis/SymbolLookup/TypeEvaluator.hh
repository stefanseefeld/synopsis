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
#include <Synopsis/SymbolLookup/Type.hh>

namespace Synopsis
{
namespace SymbolLookup
{

//. evaluate the type of an expression
class TypeEvaluator : private PTree::Visitor
{
public:
  TypeEvaluator(Scope const *s) : my_scope(s) {}
  Type evaluate(PTree::Node *node);

private:
  virtual void visit(PTree::Literal *);
  virtual void visit(PTree::Identifier *);
  virtual void visit(PTree::This *);
  virtual void visit(PTree::Name *);
  virtual void visit(PTree::FstyleCastExpr *);
  virtual void visit(PTree::AssignExpr *);
  virtual void visit(PTree::CondExpr *);
  virtual void visit(PTree::InfixExpr *);
  virtual void visit(PTree::PmExpr *);
  virtual void visit(PTree::CastExpr *);
  virtual void visit(PTree::UnaryExpr *);
  virtual void visit(PTree::ThrowExpr *);
  virtual void visit(PTree::SizeofExpr *);
  virtual void visit(PTree::TypeidExpr *);
  virtual void visit(PTree::TypeofExpr *);
  virtual void visit(PTree::NewExpr *);
  virtual void visit(PTree::DeleteExpr *);
  virtual void visit(PTree::ArrayExpr *);
  virtual void visit(PTree::FuncallExpr *);
  virtual void visit(PTree::PostfixExpr *);
  virtual void visit(PTree::DotMemberExpr *);
  virtual void visit(PTree::ArrowMemberExpr *);
  virtual void visit(PTree::ParenExpr *);
  
  Scope const *my_scope;
  Type         my_type;
};
  
inline Type type_of(PTree::Node *node, Scope const *s)
{
  TypeEvaluator evaluator(s);
  return evaluator.evaluate(node);
}

}
}

#endif

