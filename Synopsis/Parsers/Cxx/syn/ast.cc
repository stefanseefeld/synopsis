// vim: set ts=8 sts=2 sw=2 et:
// File: ast.cc
// AST hierarchy

#include "strace.hh"
#include "ast.hh"
#include "type.hh"

using namespace AST;

Declaration::Declaration(const std::string& file, int line, const std::string& type, const ScopedName& name)
: m_filename(file), m_line(line), m_type(type), m_name(name), m_access(Default), m_declared(NULL)
{ }

Declaration::~Declaration()
{
}

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
    m_declared = new Types::Declared(m_name, const_cast<AST::Declaration*>(this));
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
// AST::Scope
//

Scope::Scope(const std::string& file, int line, const std::string& type, const ScopedName& name)
: Declaration(file, line, type, name)
{ }

Scope::~Scope()
{
}

void
Scope::accept(Visitor* visitor)
{
  visitor->visit_scope(this);
}

//
// AST::Namespace
//

Namespace::Namespace(const std::string& file, int line, const std::string& type, const ScopedName& name)
: Scope(file, line, type, name)
{
}

Namespace::~Namespace()
{
}

void
Namespace::accept(Visitor* visitor)
{
  visitor->visit_namespace(this);
}

//
// AST::Class
//

Class::Class(const std::string& file, int line, const std::string& type, const ScopedName& name)
: Scope(file, line, type, name)
{
  m_template = NULL;
}

Class::~Class()
{
}

void
Class::accept(Visitor* visitor)
{
  visitor->visit_class(this);
}

//
// AST::Inheritance
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
// AST::Forward
//

Forward::Forward(const std::string& file, int line, const std::string& type, const ScopedName& name)
: Declaration(file, line, type, name), m_template(NULL)
{ }

Forward::Forward(AST::Declaration* decl)
: Declaration(decl->filename(), decl->line(), decl->type(), decl->name()), m_template(NULL)
{ }

void
Forward::accept(Visitor* visitor)
{
  visitor->visit_forward(this);
}


//
// AST::Typedef
//

Typedef::Typedef(const std::string& file, int line, const std::string& type, const ScopedName& name, Types::Type* alias, bool constr)
: Declaration(file, line, type, name), m_alias(alias), m_constr(constr)
{ }

Typedef::~Typedef()
{
}

void
Typedef::accept(Visitor* visitor)
{
  visitor->visit_typedef(this);
}


//
// AST::Variable
//

Variable::Variable(const std::string& file, int line, const std::string& type, const ScopedName& name, Types::Type* vtype, bool constr)
: Declaration(file, line, type, name), m_vtype(vtype), m_constr(constr)
{ }

Variable::~Variable()
{
}

void
Variable::accept(Visitor* visitor)
{
  visitor->visit_variable(this);
}

//
// AST::Const
//

Const::Const(const std::string& file, int line, const std::string& type, const ScopedName& name, Types::Type* t, const std::string& v)
: Declaration(file, line, type, name), m_ctype(t), m_value(v)
{ }

/*Const::~Const()
{
}*/

void
Const::accept(Visitor* visitor)
{
  visitor->visit_const(this);
}



//
// AST::Enum
//

Enum::Enum(const std::string& file, int line, const std::string& type, const ScopedName& name)
: Declaration(file, line, type, name)
{ }

Enum::~Enum()
{
}

void
Enum::accept(Visitor* visitor)
{
  visitor->visit_enum(this);
}


//
// AST::Enumerator
//

Enumerator::Enumerator(const std::string& file, int line, const std::string& type, const ScopedName& name, const std::string& value)
: Declaration(file, line, type, name), m_value(value)
{ }

/*Enumerator::~Enumerator()
{
}*/

void
Enumerator::accept(Visitor* visitor)
{
  visitor->visit_enumerator(this);
}


//
// AST::Function
//

Function::Function(
  const std::string& file, int line, const std::string& type, const ScopedName& name, 
  const Mods& premod, Types::Type* ret, const std::string& realname
)
: Declaration(file, line, type, name), m_pre(premod), m_ret(ret),
      m_realname(realname), m_template(NULL)
{
}

Function::~Function()
{
}

void
Function::accept(Visitor* visitor)
{
  visitor->visit_function(this);
}


//
// AST::Operation
//

Operation::Operation(
  const std::string& file, int line, const std::string& type, const ScopedName& name,
  const Mods& premod, Types::Type* ret, const std::string& realname
)
: Function(file, line, type, name, premod, ret, realname)
{ }

/*Operation::~Operation()
{
}*/

void
Operation::accept(Visitor* visitor)
{
  visitor->visit_operation(this);
}


//
// AST::Parameter
//

Parameter::Parameter(const Mods& pre, Types::Type* t, const Mods& post, const std::string& name, const std::string& value)
: m_pre(pre), m_post(post), m_type(t), m_name(name), m_value(value)
{ }

Parameter::~Parameter()
{
}

void
Parameter::accept(Visitor* visitor)
{
  visitor->visit_parameter(this);
}


Comment::Comment(const std::string& file, int line, const std::string& text, bool suspect)
: m_filename(file), m_line(line), m_text(text), m_suspect(suspect)
{ }

//
// AST::Visitor
//

//Visitor::Visitor() {}
Visitor::~Visitor() {}
void Visitor::visit_declaration(Declaration*) {}
void Visitor::visit_scope(Scope* d) { visit_declaration(d); }
void Visitor::visit_namespace(Namespace* d) { visit_scope(d); }
void Visitor::visit_class(Class* d) { visit_scope(d); }
void Visitor::visit_inheritance(Inheritance* d) {}
void Visitor::visit_forward(Forward* d) { visit_declaration(d); }
void Visitor::visit_typedef(Typedef* d) { visit_declaration(d); }
void Visitor::visit_variable(Variable* d) { visit_declaration(d); }
void Visitor::visit_const(Const* d) { visit_declaration(d); }
void Visitor::visit_enum(Enum* d) { visit_declaration(d); }
void Visitor::visit_enumerator(Enumerator* d) { visit_declaration(d); }
void Visitor::visit_function(Function* d) { visit_declaration(d); }
void Visitor::visit_operation(Operation* d) { visit_function(d); }
void Visitor::visit_parameter(Parameter* d) { }
