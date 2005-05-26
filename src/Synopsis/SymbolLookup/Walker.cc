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
#include <Synopsis/PTree/Display.hh>

using namespace Synopsis;
using namespace SymbolLookup;

Walker::Walker(Scope *scope)
{
  Trace trace("Walker::Walker", Trace::SYMBOLLOOKUP);
  my_scopes.push(scope->ref());
}

Walker::~Walker() 
{
  Trace trace("Walker::~Walker", Trace::SYMBOLLOOKUP);
  Scope *scope = my_scopes.top();
  scope->unref();
  my_scopes.pop();
}

void Walker::visit(PTree::List *node)
{
  Trace trace("Walker::visit(List)", Trace::SYMBOLLOOKUP);
  if (node->car()) node->car()->accept(this);
  if (node->cdr()) node->cdr()->accept(this);
}

void Walker::visit(PTree::Block *node)
{
  Trace trace("Walker::visit(Block)", Trace::SYMBOLLOOKUP);
  Scope *scope = my_scopes.top()->find_scope(node);
  std::cout << "blablabla" << std::endl;
  if (!scope) PTree::display(node, std::cout, false, true);
  assert(scope);
  scope->ref();
  my_scopes.push(scope);
  visit_block(node);
  leave_scope();  
}

void Walker::visit(PTree::NamespaceSpec *spec)
{
  Trace trace("Walker::visit(NamespaceSpec)", Trace::SYMBOLLOOKUP);
  traverse(spec);
}

void Walker::visit(PTree::FunctionDefinition *def)
{
  Trace trace("Walker::visit(FunctionDefinition)", Trace::SYMBOLLOOKUP);
  PTree::Node *decl = PTree::third(def);
  visit(static_cast<PTree::Declarator *>(decl)); // visit the declarator
  Scope *scope = my_scopes.top()->find_scope(def);
  assert(scope);
  scope->ref();
  my_scopes.push(scope);
  visit_block(static_cast<PTree::Block *>(PTree::nth(def, 3)));
  leave_scope();
}

void Walker::visit(PTree::ClassSpec *spec)
{
  Trace trace("Walker::visit(ClassSpec)", Trace::SYMBOLLOOKUP);
  traverse(spec);
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

void Walker::traverse(PTree::NamespaceSpec *spec)
{
  Trace trace("Walker::traverse(NamespaceSpec)", Trace::SYMBOLLOOKUP);
  Scope *scope = my_scopes.top()->find_scope(spec);
  assert(scope);
  scope->ref();
  my_scopes.push(scope);
  PTree::tail(spec, 2)->car()->accept(this);
  leave_scope();
}

void Walker::traverse(PTree::ClassSpec *spec)
{
  Trace trace("Walker::traverse(ClassSpec)", Trace::SYMBOLLOOKUP);
  if (PTree::ClassBody *body = spec->body())
  {
    Scope *scope = my_scopes.top()->find_scope(spec);
    assert(scope);
    scope->ref();
    my_scopes.push(scope);
    body->accept(this);
    leave_scope();
  }
}

void Walker::visit_block(PTree::Block *node)
{
  Trace trace("Walker::visit_block(Block)", Trace::SYMBOLLOOKUP);
  visit(static_cast<PTree::List *>(node));
}

void Walker::leave_scope()
{
  Trace trace("Walker::leave_scope", Trace::SYMBOLLOOKUP);
  Scope *top = my_scopes.top();
  my_scopes.pop();
  top->unref();
}
