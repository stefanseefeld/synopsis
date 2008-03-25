//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include "Display.hh"

using namespace Synopsis;
using namespace SymbolTable;

void SymbolDisplay::display(PTree::Encoding const &name,
			    SymbolTable::Symbol const *symbol)
{
  name_ = name.unmangled();
  symbol->accept(this);
  os_ << std::endl;
}

void SymbolDisplay::visit(SymbolTable::Symbol const *) {}

void SymbolDisplay::visit(SymbolTable::VariableName const *name)
{
  prefix("Variable:          ") << name_ << ' ' << name->type().unmangled();
}

void SymbolDisplay::visit(SymbolTable::ConstName const *name)
{
  prefix("Const:             ") << name_ << ' ' << name->type().unmangled();
  if (name->defined()) os_ << " (" << name->value() << ')';
}

void SymbolDisplay::visit(SymbolTable::TypeName const *name)
{
  prefix("Type:              ") << name_ << ' ' << name->type().unmangled();
}

void SymbolDisplay::visit(SymbolTable::TypedefName const *name)
{
  prefix("Typedef:           ") << name_ << ' ' << name->type().unmangled();
}

void SymbolDisplay::visit(SymbolTable::ClassName const *name)
{
  prefix("Class:             ") << name_ << ' ' << name->type().unmangled();
}

void SymbolDisplay::visit(SymbolTable::EnumName const *name)
{
  prefix("Enum:              ") << name_ << ' ' << name->type().unmangled();
}

void SymbolDisplay::visit(SymbolTable::DependentName const *name)
{
  prefix("Dependent Type:    ") << name_ << ' ' << name->type().unmangled();
}

void SymbolDisplay::visit(SymbolTable::ClassTemplateName const *name)
{
  prefix("Class template:    ") << name_ << ' ' << name->type().unmangled();
}

void SymbolDisplay::visit(SymbolTable::FunctionName const *name)
{
  prefix("Function:          ") << name_ << ' ' << name->type().unmangled();
}

void SymbolDisplay::visit(SymbolTable::FunctionTemplateName const *name)
{
  prefix("Function template: ") << name_ << ' ' << name->type().unmangled();
}

void SymbolDisplay::visit(SymbolTable::NamespaceName const *name)
{
  prefix("Namespace:         ") << name_ << ' ' << name->type().unmangled();
}

void ScopeDisplay::visit(SymbolTable::TemplateParameterScope *s)
{
  indent() << "TemplateParameterScope:\n";
  dump(s);
}

void ScopeDisplay::visit(SymbolTable::LocalScope *s)
{
  indent() << "LocalScope:\n";
  dump(s);
}

void ScopeDisplay::visit(SymbolTable::PrototypeScope *s)
{
  indent() << "PrototypeScope '" << s->name() << "':\n";
  dump(s);
}

void ScopeDisplay::visit(SymbolTable::FunctionScope *s)
{
  indent() << "FunctionScope '" << s->name() << "':\n";
  dump(s);
}

void ScopeDisplay::visit(SymbolTable::Class *s)
{
  indent() << "Class '" << s->name() << "':\n";
  dump(s);
}

void ScopeDisplay::visit(SymbolTable::Namespace *s)
{
  indent() << "Namespace '" << s->name() << "':\n";
  dump(s);
}

void ScopeDisplay::dump(SymbolTable::Scope const *s)
{
  ++indent_;
  for (SymbolTable::Scope::symbol_iterator i = s->symbols_begin();
       i != s->symbols_end();
       ++i)
  {
    SymbolDisplay display(os_, indent_);
    display.display(i->first, i->second);
  }
  for (SymbolTable::Scope::scope_iterator i = s->scopes_begin();
       i != s->scopes_end();
       ++i)
    i->second->accept(this);
  --indent_;
}

std::ostream &ScopeDisplay::indent()
{
  size_t i = indent_;
  while (i--) os_.put(' ');
  return os_;
}
