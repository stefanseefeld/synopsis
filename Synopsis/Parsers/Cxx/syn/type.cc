// vim: set ts=8 sts=2 sw=2 et:
// File: type.cc

#include "type.hh"

using namespace Types;

Type::Type() { }
Type::~Type() { }

void
Type::accept(Visitor* visitor)
{
  visitor->visit_type(this);
}

Named::Named(const ScopedName& n)
: m_name(n)
{ }

void Named::accept(Visitor* visitor)
{
  visitor->visit_named(this);
}

Base::Base(const ScopedName& n)
: Named(n)
{ }


void
Base::accept(Visitor* visitor)
{
  visitor->visit_base(this);
}

Unknown::Unknown(const ScopedName& n)
: Named(n)
{ }


void
Unknown::accept(Visitor* visitor)
{
  visitor->visit_unknown(this);
}

Dependant::Dependant(const ScopedName& n)
: Named(n)
{ }

void
Dependant::accept(Visitor* visitor)
{
  visitor->visit_dependant(this);
}

Declared::Declared(const ScopedName& n, AST::Declaration* decl)
: Named(n), m_decl(decl)
{ }


void
Declared::accept(Visitor* visitor)
{
  visitor->visit_declared(this);
}

Template::Template(const ScopedName& n, AST::Declaration* decl, const Type::vector& params)
: Declared(n, decl), m_params(params)
{ }


void
Template::accept(Visitor* visitor)
{
  visitor->visit_template_type(this);
}

Modifier::Modifier(Type* alias, const Mods& pre, const Mods& post)
: m_alias(alias), m_pre(pre), m_post(post)
{ }


void
Modifier::accept(Visitor* visitor)
{
  visitor->visit_modifier(this);
}

Array::Array(Type* alias, const Mods& sizes) : m_alias(alias), m_sizes(sizes) {}
void
Array::accept(Visitor* visitor)
{
  visitor->visit_array(this);
}

Parameterized::Parameterized(Template* t, const Type::vector& params)
: m_template(t), m_params(params)
{ }


void
Parameterized::accept(Visitor* visitor)
{
  visitor->visit_parameterized(this);
}

FuncPtr::FuncPtr(Type::Type* ret, const Mods& premods, const Type::vector& params)
: m_return(ret), m_premod(premods), m_params(params)
{ }


void
FuncPtr::accept(Visitor* visitor)
{
  visitor->visit_func_ptr(this);
}

//
// Type::Visitor
//

//Visitor::Visitor() {}
Visitor::~Visitor() {}
void Visitor::visit_type(Type*) {}
void Visitor::visit_unknown(Unknown* t) { visit_type(t); }
void Visitor::visit_base(Base* t) { visit_named(t); }
void Visitor::visit_dependant(Dependant* t) { visit_named(t); }
void Visitor::visit_declared(Declared* t) { visit_named(t); }
void Visitor::visit_modifier(Modifier* t) { visit_type(t); }
void Visitor::visit_array(Array* t) { visit_type(t); }
void Visitor::visit_named(Named* t) { visit_type(t); }
void Visitor::visit_template_type(Template* t) { visit_declared(t); }
void Visitor::visit_parameterized(Parameterized* t) { visit_type(t); }
void Visitor::visit_func_ptr(FuncPtr* t) { visit_type(t); }

// exception wrong_type_cast
const char* wrong_type_cast::what() const throw()
{
  return "Type::wrong_type_cast";
}
