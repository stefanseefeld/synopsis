//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <TypeInfoVisitor.hh>
#include <Environment.hh>

using namespace Synopsis;

TypeInfoVisitor::TypeInfoVisitor(TypeInfo &info, Environment *env)
  : my_info(info),
    my_env(env)
{
  my_info.unknown();
}

void TypeInfoVisitor::visit(PTree::Identifier *node)
{
  bool is_type_name;
  if(my_env->Lookup(node, is_type_name, my_info))
    if(is_type_name)	   // if exp is a class name
      my_info.reference(); // see visit(FuncallExpr *)
}

void TypeInfoVisitor::visit(PTree::This *)
{
  my_info.set(my_env->LookupThis());
}

void TypeInfoVisitor::visit(PTree::Name *node)
{
  bool is_type_name;
  if(my_env->Lookup(node, is_type_name, my_info))
    if(is_type_name)	   // if exp is a class name
      my_info.reference(); // see visit(FuncallExpr *)
}

void TypeInfoVisitor::visit(PTree::FstyleCastExpr *node)
{
  my_info.set(node->encoded_type(), my_env);
}

void TypeInfoVisitor::visit(PTree::AssignExpr *node)
{
  type_of(PTree::first(node));
}

void TypeInfoVisitor::visit(PTree::CondExpr *node)
{
  type_of(PTree::third(node));
}

void TypeInfoVisitor::visit(PTree::InfixExpr *node)
{
  type_of(PTree::first(node));
}

void TypeInfoVisitor::visit(PTree::PmExpr *node)
{
  type_of(PTree::third(node));
  my_info.dereference();
}

void TypeInfoVisitor::visit(PTree::CastExpr *node) 
{
  my_info.set(PTree::second(PTree::second(node))->encoded_type(), my_env);
}

void TypeInfoVisitor::visit(PTree::UnaryExpr *node)
{
  type_of(PTree::second(node));
  PTree::Node *op = PTree::first(node);
  if(*op == '*') my_info.dereference();
  else if(*op == '&') my_info.reference();
}

void TypeInfoVisitor::visit(PTree::ThrowExpr *)
{
  my_info.set_void();
}

void TypeInfoVisitor::visit(PTree::SizeofExpr *)
{
  my_info.set_int();
}

void TypeInfoVisitor::visit(PTree::TypeidExpr *)
{
  // FIXME: Should be the type_info type (node->third()->second()->encoded_type(), my_env);
  my_info.set_int();
}

void TypeInfoVisitor::visit(PTree::TypeofExpr *) 
{
  // FIXME: Should be the type_info type (node->third()->second()->encoded_type(), my_env);
  my_info.set_int();
}

void TypeInfoVisitor::visit(PTree::NewExpr *node)
{
  PTree::Node *p = node;
  PTree::Node *userkey = p->car();
  if(!userkey || !userkey->is_atom()) p = node->cdr(); // user keyword
  if(*p->car() == "::") p = p->cdr();
  PTree::Node *type = PTree::third(p);
  if(*type->car() == '(')
    my_info.set(PTree::second(PTree::second(type))->encoded_type(), my_env);
  else
    my_info.set(PTree::second(type)->encoded_type(), my_env);
  my_info.reference();
}

void TypeInfoVisitor::visit(PTree::DeleteExpr *)
{
  my_info.set_void();
}

void TypeInfoVisitor::visit(PTree::ArrayExpr *node)
{
  type_of(node->car());
  my_info.dereference();
}

void TypeInfoVisitor::visit(PTree::FuncallExpr *node)
{
  type_of(node->car());
  if(!my_info.is_function())
    my_info.dereference(); // maybe a pointer to a function
  my_info.dereference();
}

void TypeInfoVisitor::visit(PTree::PostfixExpr *node)
{
  type_of(node->car());
}

void TypeInfoVisitor::visit(PTree::DotMemberExpr *node)
{
  type_of(node->car());
  my_info.set_member(PTree::third(node));
}

void TypeInfoVisitor::visit(PTree::ArrowMemberExpr *node)
{
  type_of(node->car());
  my_info.dereference();
  my_info.set_member(PTree::third(node));
}

void TypeInfoVisitor::visit(PTree::ParenExpr *node)
{
  type_of(PTree::second(node));
}
