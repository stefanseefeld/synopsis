//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/SymbolTable/Symbol.hh>
#include <Synopsis/SymbolTable/Scopes.hh>

using namespace Synopsis;
using namespace PTree;
using namespace SymbolTable;

Class *ClassName::as_scope() const
{
  Scope *outer = scope();
  Scope *nested = outer->find_scope(ptree());
  return dynamic_cast<Class *>(nested);
}

Class *ClassTemplateName::as_scope() const
{
  Scope *outer = scope();
  Scope *nested = outer->find_scope(ptree());
  return dynamic_cast<Class *>(nested);
}

FunctionScope *FunctionName::as_scope() const
{
  Scope *outer = scope();
  Scope *nested = outer->find_scope(ptree());
  return dynamic_cast<FunctionScope *>(nested);
}

FunctionScope *FunctionTemplateName::as_scope() const
{
  Scope *outer = scope();
  Scope *nested = outer->find_scope(ptree());
  return dynamic_cast<FunctionScope *>(nested);
}

Namespace *NamespaceName::as_scope() const
{
  Scope *outer = scope();
  Scope *nested = outer->find_scope(ptree());
  return dynamic_cast<Namespace *>(nested);
}
