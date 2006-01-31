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
  my_name = name.unmangled();
  symbol->accept(this);
  my_os << std::endl;
}

void SymbolDisplay::visit(SymbolTable::Symbol const *) {}

void SymbolDisplay::visit(SymbolTable::VariableName const *name)
{
  prefix("Variable:          ") << my_name << ' ' << name->type().unmangled();
}

void SymbolDisplay::visit(SymbolTable::ConstName const *name)
{
  prefix("Const:             ") << my_name << ' ' << name->type().unmangled();
  if (name->defined()) my_os << " (" << name->value() << ')';
}

void SymbolDisplay::visit(SymbolTable::TypeName const *name)
{
  prefix("Type:              ") << my_name << ' ' << name->type().unmangled();
}

void SymbolDisplay::visit(SymbolTable::TypedefName const *name)
{
  prefix("Typedef:           ") << my_name << ' ' << name->type().unmangled();
}

void SymbolDisplay::visit(SymbolTable::ClassName const *name)
{
  prefix("Class:             ") << my_name << ' ' << name->type().unmangled();
}

void SymbolDisplay::visit(SymbolTable::EnumName const *name)
{
  prefix("Enum:              ") << my_name << ' ' << name->type().unmangled();
}

void SymbolDisplay::visit(SymbolTable::DependentName const *name)
{
  prefix("Dependent Type:    ") << my_name << ' ' << name->type().unmangled();
}

void SymbolDisplay::visit(SymbolTable::ClassTemplateName const *name)
{
  prefix("Class template:    ") << my_name << ' ' << name->type().unmangled();
}

void SymbolDisplay::visit(SymbolTable::FunctionName const *name)
{
  prefix("Function:          ") << my_name << ' ' << name->type().unmangled();
}

void SymbolDisplay::visit(SymbolTable::FunctionTemplateName const *name)
{
  prefix("Function template: ") << my_name << ' ' << name->type().unmangled();
}

void SymbolDisplay::visit(SymbolTable::NamespaceName const *name)
{
  prefix("Namespace:         ") << my_name << ' ' << name->type().unmangled();
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
  ++my_indent;
  for (SymbolTable::Scope::symbol_iterator i = s->symbols_begin();
       i != s->symbols_end();
       ++i)
  {
    SymbolDisplay display(my_os, my_indent);
    display.display(i->first, i->second);
  }
  for (SymbolTable::Scope::scope_iterator i = s->scopes_begin();
       i != s->scopes_end();
       ++i)
    i->second->accept(this);
  --my_indent;
}

std::ostream &ScopeDisplay::indent()
{
  size_t i = my_indent;
  while (i--) my_os.put(' ');
  return my_os;
}
