//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/SymbolLookup/Walker.hh>
#include <Synopsis/PTree/Lists.hh>
#include <Synopsis/PTree/TypeVisitor.hh>

using namespace Synopsis;
using namespace SymbolLookup;

void Walker::visit(PTree::List *node)
{
  if (node->car()) node->car()->accept(this);
  if (node->cdr()) node->cdr()->accept(this);
}

void Walker::visit(PTree::Block *node)
{
  my_table.enter_block(node);
  visit_block(node);
  my_table.leave_scope();  
}

void Walker::visit(PTree::NamespaceSpec *spec)
{
  my_table.enter_namespace(spec);
  visit(static_cast<PTree::List *>(spec));
  my_table.leave_scope();
}

void Walker::visit(PTree::Declaration *decl)
{
  PTree::Node *decls = PTree::third(decl);
  if(PTree::is_a(decls, Token::ntDeclarator)) // function definition
  {
    visit(static_cast<PTree::Declarator *>(decls)); // visit the declarator
    my_table.enter_function(decl);
    visit_block(static_cast<PTree::Block *>(PTree::nth(decl, 3)));
    my_table.leave_scope();
  }
  else
    visit(static_cast<PTree::List *>(decl));
}

void Walker::visit(PTree::ClassSpec *spec)
{
  my_table.enter_class(spec);
  if (PTree::ClassBody *body = spec->body())
    body->accept(this);
  my_table.leave_scope();
}

void Walker::visit(PTree::DotMemberExpr *expr)
{
  std::cout << "Sorry: dot member expression (<postfix>.<name>) not yet supported" << std::endl;
}

void Walker::visit(PTree::ArrowMemberExpr *expr)
{
  std::cout << "Sorry: arrow member expression (<postfix>-><name>) not yet supported" << std::endl;
}

void Walker::visit_block(PTree::Block *node)
{
  visit(static_cast<PTree::List *>(node));
}

