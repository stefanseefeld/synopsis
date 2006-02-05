//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/SymbolTable/Walker.hh>
#include <Synopsis/PTree/Lists.hh>
#include <Synopsis/Trace.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/SymbolTable/Scopes.hh>
#include <Synopsis/SymbolTable/Symbol.hh>

using namespace Synopsis;
namespace PT = Synopsis::PTree;
using namespace SymbolTable;

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

void Walker::visit(PT::List *node)
{
  Trace trace("Walker::visit(List)", Trace::SYMBOLLOOKUP);
  if (node->car()) node->car()->accept(this);
  if (node->cdr()) node->cdr()->accept(this);
}

void Walker::visit(PT::Block *node)
{
  Trace trace("Walker::visit(Block)", Trace::SYMBOLLOOKUP);
  Scope *scope = my_scopes.top()->find_scope(node);
  if (!scope)
  {
    // Not all Blocks represent a scope...
    visit_block(node);
  }
  else
  {
    scope->ref();
    my_scopes.push(scope);
    visit_block(node);
    leave_scope();
  }  
}

void Walker::visit(PT::TemplateDeclaration *tdecl)
{
  Trace trace("Walker::visit(TemplateDeclaration)", Trace::SYMBOLLOOKUP);
  traverse_parameters(tdecl);
  // If we are in a template template parameter, the following
  // is just the 'class' keyword.
  // Else it is a Declaration, which we want to traverse.
  PT::Node *decl = PT::nth(tdecl, 4);
  if (!decl->is_atom()) decl->accept(this);
  else
  {
    std::cout << "length " << PT::length(tdecl) << std::endl;
  }
}

void Walker::visit(PT::NamespaceSpec *spec)
{
  Trace trace("Walker::visit(NamespaceSpec)", Trace::SYMBOLLOOKUP);
  traverse_body(spec);
}

void Walker::visit(PT::FunctionDefinition *def)
{
  Trace trace("Walker::visit(FunctionDefinition)", Trace::SYMBOLLOOKUP);
  PT::Node *decl = PT::nth<1>(def);
  visit(static_cast<PT::Declarator *>(decl)); // visit the declarator
  traverse_body(def);
}

void Walker::visit(PT::ClassSpec *spec)
{
  Trace trace("Walker::visit(ClassSpec)", Trace::SYMBOLLOOKUP);
  traverse_body(spec);
}

void Walker::visit(PT::DotMemberExpr *)
{
  Trace trace("Walker::visit(DotMemberExpr)", Trace::SYMBOLLOOKUP);
  std::cout << "Sorry: dot member expression (<postfix>.<name>) not yet supported" << std::endl;
}

void Walker::visit(PT::ArrowMemberExpr *)
{
  Trace trace("Walker::visit(ArrowMemberExpr)", Trace::SYMBOLLOOKUP);
  std::cout << "Sorry: arrow member expression (<postfix>-><name>) not yet supported" << std::endl;
}

void Walker::traverse_body(PT::NamespaceSpec *spec)
{
  Trace trace("Walker::traverse_body(NamespaceSpec)", Trace::SYMBOLLOOKUP);
  Scope *scope = my_scopes.top()->find_scope(spec);
  assert(scope);
  scope->ref();
  my_scopes.push(scope);
  PT::tail(spec, 2)->car()->accept(this);
  leave_scope();
}

void Walker::traverse_body(PT::ClassSpec *spec)
{
  Trace trace("Walker::traverse_body(ClassSpec)", Trace::SYMBOLLOOKUP);
  if (PT::ClassBody *body = spec->body())
  {
    Scope *scope = my_scopes.top()->find_scope(spec);
    assert(scope);
    scope->ref();
    my_scopes.push(scope);
    body->accept(this);
    leave_scope();
  }
}

void Walker::traverse_parameters(PT::TemplateDeclaration *decl)
{
  Trace trace("Walker::traverse_body(TemplateDeclaration)", Trace::SYMBOLLOOKUP);
  Scope *scope = my_scopes.top()->find_scope(decl);
  scope->ref();
  my_scopes.push(scope);
  // list of template parameters (TypeParameter or ParameterDeclaration)
  PT::nth<2>(decl)->accept(this);
  leave_scope();
}

void Walker::traverse_body(PT::FunctionDefinition *def)
{
  Trace trace("Walker::traverse_body(FunctionDefinition)", Trace::SYMBOLLOOKUP);
  PT::Node *decl = PT::nth<1>(def);

  Scope *scope = my_scopes.top();
  PT::Encoding name = decl->encoded_name();
  if (name.is_qualified())
  {
    SymbolSet symbols = lookup(name);
    assert(!symbols.empty());
    // FIXME: We need type analysis / overload resolution
    //        here to take the right symbol / scope.
    FunctionName const *symbol = dynamic_cast<FunctionName const *>(*symbols.begin());
    assert(symbol);
    scope = symbol->as_scope();
  }
  else
    scope = my_scopes.top()->find_scope(def);
  assert(scope);
  scope->ref();
  my_scopes.push(scope);
  visit_block(static_cast<PT::Block *>(PT::nth(def, 2)));
  leave_scope();
}

SymbolSet Walker::lookup(PT::Encoding const &name, Scope::LookupContext context)
{
  Scope *scope = my_scopes.top();
  // This is a verbatim copy from lookup() defined in SymbolLookup.cc.
  // However, the above will evolve to use type analysis to implicitely
  // instantiate class / function templates, while here we consider
  // the symbol table non-modifiable.
  if (!name.is_qualified())
    return scope->unqualified_lookup(name, context);

  PT::Encoding::name_iterator next = name.begin_name();
  // Figure out which scope to start the qualified lookup in.
  if (next->is_global_scope())
  {
    // If the scope is the global scope, do a qualified lookup there.
    scope = scope->global_scope();
  }
  else
  {
    // Else do an unqualified lookup for the scope.
    SymbolSet symbols = scope->unqualified_lookup(*next, context | Scope::SCOPE);
    if (symbols.empty())
      throw Undefined(name, scope);
    else if (symbols.size() > 1)
      // If the name was found multiple times, it must refer to a function,
      // so throw a TypeError.
      throw TypeError(name, (*symbols.begin())->ptree()->encoded_type());

    Symbol const *symbol = *symbols.begin();
    scope = symbol->scope()->find_scope(symbol->ptree());
  }

  PT::Encoding component = *++next;
  SymbolSet symbols = scope->qualified_lookup(component, context);


  // Now iterate over all name components in the qualified name and
  // look them up in their respective (nested) scopes.
  while (++next != name.end_name())
  {
    if (symbols.empty())
      throw Undefined(component, scope);
    else if (symbols.size() > 1)
      // If the name was found multiple times, it must refer to a function,
      // so throw a TypeError.
      throw TypeError(component, (*symbols.begin())->ptree()->encoded_type());

    // As scopes contain a table of nested scopes, accessible through their 
    // respective declaration objects, we find the scope using the symbol's 
    // declaration as key within its scope.

    Symbol const *symbol = *symbols.begin();
    scope = symbol->scope()->find_scope(symbol->ptree());
    if (!scope) 
      throw InternalError("undeclared scope !");

    component = *next;
    symbols = scope->qualified_lookup(component, context);
  }
  return symbols;
}

void Walker::visit_block(PT::Block *node)
{
  Trace trace("Walker::visit_block(Block)", Trace::SYMBOLLOOKUP);
  visit(static_cast<PT::List *>(node));
}

void Walker::leave_scope()
{
  Trace trace("Walker::leave_scope", Trace::SYMBOLLOOKUP);
  Scope *top = my_scopes.top();
  my_scopes.pop();
  top->unref();
}
