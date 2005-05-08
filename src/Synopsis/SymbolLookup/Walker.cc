//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/SymbolLookup/Walker.hh>
#include <Synopsis/PTree/Lists.hh>
#include <Synopsis/PTree/TypeVisitor.hh>
#include <Synopsis/Trace.hh>

using namespace Synopsis;
using namespace SymbolLookup;

void Walker::visit(PTree::List *node)
{
  Trace trace("Walker::visit(List)", Trace::SYMBOLLOOKUP);
  if (node->car()) node->car()->accept(this);
  if (node->cdr()) node->cdr()->accept(this);
}

void Walker::visit(PTree::Block *node)
{
  Trace trace("Walker::visit(Block)", Trace::SYMBOLLOOKUP);
  my_table.enter_block(node);
  visit_block(node);
  my_table.leave_scope();  
}

void Walker::visit(PTree::NamespaceSpec *spec)
{
  Trace trace("Walker::visit(NamespaceSpec)", Trace::SYMBOLLOOKUP);
  my_table.enter_namespace(spec);
  visit(static_cast<PTree::List *>(spec));
  my_table.leave_scope();
}

void Walker::visit(PTree::FunctionDefinition *def)
{
  Trace trace("Walker::visit(FunctionDefinition)", Trace::SYMBOLLOOKUP);
  PTree::Node *decl = PTree::third(def);
  visit(static_cast<PTree::Declarator *>(decl)); // visit the declarator
  my_table.enter_function_definition(def);
  visit_block(static_cast<PTree::Block *>(PTree::nth(def, 3)));
  my_table.leave_scope();
}

void Walker::visit(PTree::ClassSpec *spec)
{
  Trace trace("Walker::visit(ClassSpec)", Trace::SYMBOLLOOKUP);
  my_table.enter_class(spec);
  if (PTree::ClassBody *body = spec->body())
    body->accept(this);
  my_table.leave_scope();
}

void Walker::visit(PTree::DotMemberExpr *expr)
{
  Trace trace("Walker::visit(DotMemberExpr)", Trace::SYMBOLLOOKUP);
  std::cout << "Sorry: dot member expression (<postfix>.<name>) not yet supported" << std::endl;
}

void Walker::visit(PTree::ArrowMemberExpr *expr)
{
  Trace trace("Walker::visit(ArrowMemberExpr)", Trace::SYMBOLLOOKUP);
  std::cout << "Sorry: arrow member expression (<postfix>-><name>) not yet supported" << std::endl;
}

void Walker::visit_block(PTree::Block *node)
{
  Trace trace("Walker::visit_block(Block)", Trace::SYMBOLLOOKUP);
  visit(static_cast<PTree::List *>(node));
}

