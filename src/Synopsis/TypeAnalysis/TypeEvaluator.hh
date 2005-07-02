//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_TypeAnalysis_TypeEvaluator_hh_
#define Synopsis_TypeAnalysis_TypeEvaluator_hh_

#include <Synopsis/PTree.hh>
#include <Synopsis/SymbolTable.hh>
#include <Synopsis/TypeAnalysis/Type.hh>
#include <map>

namespace Synopsis
{
namespace TypeAnalysis
{

//. A dictionary storing return types for
//. binary operators. These are only known
//. for builtin types, so whenever for a given
//. pair of types no entry is found we look for
//. a user-defined binary operator.
class BinaryPromotion
{
  typedef std::pair<Type const *, Type const *> Key;
  typedef std::map<Key, Type const *> ReturnTypes;
public:
  BinaryPromotion();
  //. Find the return type for the given pair of types.
  //. This only returns non-zero if both arguments are
  //. builtin types.
  Type const *find(Type const *, Type const *);

private:
  ReturnTypes my_return_types;
};

//. evaluate the type of an expression
class TypeEvaluator : private PTree::Visitor
{
public:
  TypeEvaluator(SymbolTable::Scope const *s) : my_scope(s) {}
  Type const *evaluate(PTree::Node const *node);

private:
  class BinaryPromotion;

  virtual void visit(PTree::Literal *);
  virtual void visit(PTree::Identifier *);
  virtual void visit(PTree::Kwd::This *);
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
  
  SymbolTable::Scope const * my_scope;
  Type const *               my_type;
};
  
inline Type const *type_of(PTree::Node const *node,
			   SymbolTable::Scope const *s)
{
  TypeEvaluator evaluator(s);
  return evaluator.evaluate(node);
}

}
}

#endif

