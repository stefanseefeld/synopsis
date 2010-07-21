//
// Copyright (C) 2002 Stephen Davies
// Copyright (C) 2002 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "STrace.hh"
#include "ASG.hh"
#include "Types.hh"

using namespace ASG;

void SourceFile::add_macro_call(char const *name, long l, long col,
                                long sl, long sc, long el, long ec, long o, bool c)
{
  Line &line = macro_calls_[l];
  line.insert(MacroCall(name, col, sl, sc, el, ec, o, c));
}

int SourceFile::map_column(int l, int col)
{
  Lines::iterator i = macro_calls_.find(l);
  if (i == macro_calls_.end()) return col;
  Line &line = i->second;
  int offset = 0;
  for (Line::iterator j = line.begin(), end = line.end(); j != end && j->start_column <= col; ++j)
  {
    if (j->end_column == -1 || col <= j->end_column) return -1;
    offset += j->offset;
  }
  return col - offset;
}

//
// ASG::Include
//

Include::Include(SourceFile* target, bool is_macro, bool is_next)
    : m_target(target), m_is_macro(is_macro), m_is_next(is_next)
{ }

//
// ASG::Declaration
//

Declaration::Declaration(SourceFile* file, int line, const std::string& type, const QName& name)
        : m_file(file), m_line(line), m_type(type), m_name(name), m_access(Default), m_declared(0)
{}

Declaration::~Declaration()
{ }

void
Declaration::accept(Visitor* visitor)
{
    visitor->visit_declaration(this);
}

const Types::Declared*
Declaration::declared() const
{
    if (!m_declared)
        // Constness of 'this' is preserved through const return type
        m_declared = new Types::Declared(m_name, const_cast<ASG::Declaration*>(this));
    return m_declared;
}

Types::Declared*
Declaration::declared()
{
    if (!m_declared)
        m_declared = new Types::Declared(m_name, this);
    return m_declared;
}

//
// ASG::Builtin
//

Builtin::Builtin(SourceFile* file, int line, const std::string &type, const QName& name)
    : Declaration(file, line, type, name)
{ }

Builtin::~Builtin()
{ }

void
Builtin::accept(Visitor* visitor)
{
    visitor->visit_builtin(this);
}

//
// ASG::Macro
//

Macro::Macro(SourceFile* file, int line, const QName& name, Parameters* params, const std::string& text)
    : Declaration(file, line, "macro", name),
    m_parameters(params), m_text(text)
{ }

Macro::~Macro()
{ }

void
Macro::accept(Visitor* visitor)
{
    visitor->visit_macro(this);
}

//
// ASG::Scope
//

Scope::Scope(SourceFile* file, int line, const std::string& type, const QName& name)
        : Declaration(file, line, type, name)
{ }

Scope::~Scope()
{}

void
Scope::accept(Visitor* visitor)
{
    visitor->visit_scope(this);
}

//
// ASG::Namespace
//

Namespace::Namespace(SourceFile* file, int line, const std::string& type, const QName& name)
        : Scope(file, line, type, name)
{}

Namespace::~Namespace()
{}

void
Namespace::accept(Visitor* visitor)
{
    visitor->visit_namespace(this);
}

//
// ASG::Class
//

Class::Class(SourceFile* file, int line, const std::string& type, const QName& name,
             bool is_template_specialization)
  : Scope(file, line, type, name), is_template_specialization_(is_template_specialization)
{}

Class::~Class() {}
void Class::accept(Visitor* visitor) { visitor->visit_class(this);}

//
// ASG::ClassTemplate
//

ClassTemplate::ClassTemplate(SourceFile* file, int line, const std::string& type, const QName& name,
                             bool is_specialization)
  : Class(file, line, type, name, is_specialization),
    template_(0)
{}

ClassTemplate::~ClassTemplate() {}
void ClassTemplate::accept(Visitor* visitor) { visitor->visit_class_template(this);}

//
// ASG::Inheritance
//

Inheritance::Inheritance(Types::Type* type, const Attributes& attrs)
        : m_parent(type), m_attrs(attrs)
{ }

void
Inheritance::accept(Visitor* visitor)
{
    visitor->visit_inheritance(this);
}


//
// ASG::Forward
//

Forward::Forward(SourceFile* file, int line, const std::string& type, const QName& name,
                 bool is_template_specialization)
  : Declaration(file, line, type, name), template_(0),
    is_template_specialization_(is_template_specialization)
{}

void
Forward::accept(Visitor* visitor)
{
  visitor->visit_forward(this);
}


//
// ASG::Typedef
//

Typedef::Typedef(SourceFile* file, int line, const std::string& type, const QName& name, Types::Type* alias, bool constr)
        : Declaration(file, line, type, name), m_alias(alias), m_constr(constr)
{ }

Typedef::~Typedef()
{}

void
Typedef::accept(Visitor* visitor)
{
    visitor->visit_typedef(this);
}


//
// ASG::Variable
//

Variable::Variable(SourceFile* file, int line, const std::string& type, const QName& name, Types::Type* vtype, bool constr)
        : Declaration(file, line, type, name), m_vtype(vtype), m_constr(constr)
{ }

Variable::~Variable()
{}

void
Variable::accept(Visitor* visitor)
{
    visitor->visit_variable(this);
}

//
// ASG::Const
//

Const::Const(SourceFile* file, int line, const std::string& type, const QName& name, Types::Type* t, const std::string& v)
        : Declaration(file, line, type, name), m_ctype(t), m_value(v)
{ }

void
Const::accept(Visitor* visitor)
{
    visitor->visit_const(this);
}


//
// ASG::Enum
//

Enum::Enum(SourceFile* file, int line, const std::string& type, const QName& name)
        : Declaration(file, line, type, name)
{ }

Enum::~Enum()
{}

void
Enum::accept(Visitor* visitor)
{
    visitor->visit_enum(this);
}


//
// ASG::Enumerator
//

Enumerator::Enumerator(SourceFile* file, int line, const std::string& type, const QName& name, const std::string& value)
        : Declaration(file, line, type, name), m_value(value)
{ }

void
Enumerator::accept(Visitor* visitor)
{
    visitor->visit_enumerator(this);
}


//
// ASG::Function
//

Function::Function(
    SourceFile* file, int line, const std::string& type, const QName& name,
    const Mods& premod, Types::Type* ret, const Mods& postmod, const std::string& realname
)
  : Declaration(file, line, type, name), m_pre(premod), m_ret(ret), m_post(postmod),
    m_realname(realname), m_template(0)
{}

Function::~Function()
{}

void
Function::accept(Visitor* visitor)
{
    visitor->visit_function(this);
}


//
// ASG::Operation
//

Operation::Operation(
    SourceFile* file, int line, const std::string& type, const QName& name,
    const Mods& premod, Types::Type* ret, const Mods& postmod, const std::string& realname
)
  : Function(file, line, type, name, premod, ret, postmod, realname)
{ }

void
Operation::accept(Visitor* visitor)
{
    visitor->visit_operation(this);
}


//
// ASG::Parameter
//

Parameter::Parameter(const Mods& pre, Types::Type* t, const Mods& post, const std::string& name, const std::string& value)
        : m_pre(pre), m_post(post), m_type(t), m_name(name), m_value(value)
{ }

Parameter::~Parameter()
{}

void
Parameter::accept(Visitor* visitor)
{
    visitor->visit_parameter(this);
}

void UsingDirective::accept(Visitor* visitor)
{
    visitor->visit_using_directive(this);
}

UsingDeclaration::UsingDeclaration(SourceFile* file, int line, QName const& name, Types::Named *d)
  : Declaration(file, line, "using", name), m_target(d) {}


void UsingDeclaration::accept(Visitor* visitor)
{
    visitor->visit_using_declaration(this);
}


Comment::Comment(SourceFile* file, int line, const std::string& text, bool suspect)
        : m_file(file), m_line(line), m_text(text), m_suspect(suspect)
{ }

//
// AST::Visitor
//

Visitor::~Visitor()
{}
void Visitor::visit_declaration(Declaration*)
{}

void Visitor::visit_builtin(Builtin*)
{}

void Visitor::visit_macro(Macro* d)
{
    visit_declaration(d);
}
void Visitor::visit_scope(Scope* d)
{
    visit_declaration(d);
}
void Visitor::visit_namespace(Namespace* d)
{
    visit_scope(d);
}
void Visitor::visit_class(Class* d)
{
    visit_scope(d);
}
void Visitor::visit_class_template(ClassTemplate* d)
{
    visit_class(d);
}
void Visitor::visit_inheritance(Inheritance* d)
{}
void Visitor::visit_forward(Forward* d)
{
    visit_declaration(d);
}
void Visitor::visit_typedef(Typedef* d)
{
    visit_declaration(d);
}
void Visitor::visit_variable(Variable* d)
{
    visit_declaration(d);
}
void Visitor::visit_const(Const* d)
{
    visit_declaration(d);
}
void Visitor::visit_enum(Enum* d)
{
    visit_declaration(d);
}
void Visitor::visit_enumerator(Enumerator* d)
{
    visit_declaration(d);
}
void Visitor::visit_function(Function* d)
{
    visit_declaration(d);
}
void Visitor::visit_operation(Operation* d)
{
    visit_function(d);
}
void Visitor::visit_parameter(Parameter* d)
{ }

void Visitor::visit_using_directive(UsingDirective* u)
{ }

void Visitor::visit_using_declaration(UsingDeclaration* u)
{ }

// vim: set ts=8 sts=4 sw=4 et:
