// File: type.cc

#include "type.hh"

/*
using Type::Named;
using Type::Base;
using Type::Unknown;
using Type::Declared;
using Type::Template;
using Type::Modifier;
using Type::Visitor;
using Type::Parameterized;
using Type::FuncPtr;
*/

Type::Type::Type() {}
Type::Type::~Type() {}
void Type::Type::accept(Visitor* visitor)
{
    visitor->visitType(this);
}

using namespace Type;

Named::Named(Name n)
    : m_name(n)
{
}

void Named::accept(Visitor* visitor)
{
    visitor->visitNamed(this);
}

Base::Base(Name n)
    : Named(n)
{
}


void Base::accept(Visitor* visitor)
{
    visitor->visitBase(this);
}

Unknown::Unknown(Name n)
    : Named(n)
{
}


void Unknown::accept(Visitor* visitor)
{
    visitor->visitUnknown(this);
}

Declared::Declared(Name n, AST::Declaration* decl)
    : Named(n), m_decl(decl)
{
}


void Declared::accept(Visitor* visitor)
{
    visitor->visitDeclared(this);
}

Template::Template(Name n, AST::Declaration* decl, Type::vector_t& params)
    : Declared(n, decl), m_params(params)
{
}


void Template::accept(Visitor* visitor)
{
    visitor->visitTemplateType(this);
}

Modifier::Modifier(Type* alias, Mods& pre, Mods& post)
    : m_alias(alias), m_pre(pre), m_post(post)
{
}


void Modifier::accept(Visitor* visitor)
{
    visitor->visitModifier(this);
}

Parameterized::Parameterized(Template* t, Type::vector_t& params)
    : m_template(t), m_params(params)
{
}


void Parameterized::accept(Visitor* visitor)
{
    visitor->visitParameterized(this);
}

FuncPtr::FuncPtr(Type::Type* ret, Mods& premods, Type::vector_t& params)
    : m_return(ret), m_premod(premods), m_params(params)
{
}


void FuncPtr::accept(Visitor* visitor)
{
    visitor->visitFuncPtr(this);
}

//
// Type::Visitor
//

//Visitor::Visitor() {}
Visitor::~Visitor() {}
void Visitor::visitType(Type*) {}
void Visitor::visitUnknown(Unknown *t) { visitType(t); }
void Visitor::visitBase(Base *t) { visitNamed(t); }
void Visitor::visitDeclared(Declared *t) { visitNamed(t); }
void Visitor::visitModifier(Modifier *t) { visitType(t); }
void Visitor::visitNamed(Named *t) { visitType(t); }
void Visitor::visitTemplateType(Template *t) { visitDeclared(t); }
void Visitor::visitParameterized(Parameterized *t) { visitType(t); }
void Visitor::visitFuncPtr(FuncPtr *t) { visitType(t); }

