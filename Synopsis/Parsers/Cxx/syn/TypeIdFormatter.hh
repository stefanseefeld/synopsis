//
// Copyright (C) 2008 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef TypeIdFormatter_hh_
#define TypeIdFormatter_hh_

#include "ast.hh"
#include "type.hh"
#include <string>
#include <vector>

class TypeIdFormatter : public Types::Visitor
{
public:
  TypeIdFormatter();

  //. Sets the current scope, pushing the previous onto a stack
  void push_scope(const ScopedName& scope);
  //. Pops the previous scope from the stack
  void pop_scope();

  //
  // Type Visitor
  //
  //. Returns a formatter string for given type.
  //. The option string pointer refers to the parameter name (where
  //. applicable) so that it can be put in the right place for function pointer
  //. types. The pointed to pointer will be set to 0 if the identifier is
  //. used
  std::string format(const Types::Type*, const std::string** id = 0);
  virtual void visit_type(Types::Type*);
  virtual void visit_unknown(Types::Unknown*);
  virtual void visit_modifier(Types::Modifier*);
  virtual void visit_named(Types::Named*);
  virtual void visit_base(Types::Base*);
  virtual void visit_declared(Types::Declared*);
  virtual void visit_template_type(Types::Template*);
  virtual void visit_parameterized(Types::Parameterized*);
  virtual void visit_func_ptr(Types::FuncPtr*);

private:
  //. The Type String
  std::string type_;
  //. The current scope name
  ScopedName scope_;
  //. Returns the given Name relative to the current scope
  std::string colonate(const ScopedName& name);
  //. A stack of previous scopes
  std::vector<ScopedName> scope_stack_;
  //. A pointer to the identifier for function pointers
  const std::string** fptr_id_;
};

#endif
