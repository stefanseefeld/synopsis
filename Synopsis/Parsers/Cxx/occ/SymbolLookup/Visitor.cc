//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <SymbolLookup/Visitor.hh>
#include <PTree/Lists.hh>
#include <PTree/TypeVisitor.hh>

using namespace SymbolLookup;

void Visitor::visit(PTree::List *node)
{
  for (PTree::Node *i = node; i; i = i->cdr())
    if (i->car()) i->car()->accept(this);
}

void Visitor::visit(PTree::NamespaceSpec *spec)
{
  my_table.enter_namespace(spec);
  visit(static_cast<PTree::List *>(spec));
  my_table.leave_scope();
}

void Visitor::visit(PTree::Declaration *decl)
{
  PTree::Node *decls = PTree::third(decl);
  if(PTree::is_a(decls, Token::ntDeclarator)) // function definition
  {
    my_table.enter_function(decl);
    visit(static_cast<PTree::List *>(decl));
    my_table.leave_scope();
  }
  else
    decls->accept(this);
}

void Visitor::visit(PTree::ClassSpec *spec)
{
  my_table.enter_class(spec);
  visit(static_cast<PTree::List *>(spec));
  my_table.leave_scope();
}

void Visitor::visit(PTree::DotMemberExpr *expr)
{
  std::cout << "Sorry: dot member expression (<postfix>.<name>) not yet supported" << std::endl;
}

void Visitor::visit(PTree::ArrowMemberExpr *expr)
{
  std::cout << "Sorry: arrow member expression (<postfix>-><name>) not yet supported" << std::endl;
}
