//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _TypeInfoVisitor_hh
#define _TypeInfoVisitor_hh

#include <Synopsis/PTree.hh>
#include <TypeInfo.hh>
#include <cassert>

using namespace Synopsis;

class Environment;

class TypeInfoVisitor : public PTree::Visitor
{
public:
  TypeInfoVisitor(TypeInfo &info, Environment *env);

  void type_of(PTree::Node *node) { node->accept(this);}

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
private:
  TypeInfo &my_info;
  Environment *my_env;
};

inline void type_of(const PTree::Node *node, Environment *env, TypeInfo &info)
{
  assert(node);
  TypeInfoVisitor v(info, env);
  v.type_of(const_cast<PTree::Node *>(node));
}

#endif
