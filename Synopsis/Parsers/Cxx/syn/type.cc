// Synopsis C++ Parser: type.cc source file
// Implementations of the Type heirarchy classes

// $Id: type.cc,v 1.12 2002/11/17 12:11:44 chalky Exp $
//
// This file is a part of Synopsis.
// Copyright (C) 2002 Stephen Davies
//
// Synopsis is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.

// $Log: type.cc,v $
// Revision 1.12  2002/11/17 12:11:44  chalky
// Reformatted all files with astyle --style=ansi, renamed fakegc.hh
//
//

#include "type.hh"

using namespace Types;

//
// Class Types::Type
//

Type::Type()
{}
Type::~Type()
{}

void
Type::accept(Visitor* visitor)
{
    visitor->visit_type(this);
}

//
// Class Types::Named
//

Named::Named(const ScopedName& n)
        : m_name(n)
{}

Named::~Named()
{}

void Named::accept(Visitor* visitor)
{
    visitor->visit_named(this);
}

//
// Class Types::Base
//

Base::Base(const ScopedName& n)
        : Named(n)
{}


void
Base::accept(Visitor* visitor)
{
    visitor->visit_base(this);
}

//
// Class Types::Unknown
//

Unknown::Unknown(const ScopedName& n)
        : Named(n)
{}


void
Unknown::accept(Visitor* visitor)
{
    visitor->visit_unknown(this);
}

//
// Class Types::Dependent
//

Dependent::Dependent(const ScopedName& n)
        : Named(n)
{}

void
Dependent::accept(Visitor* visitor)
{
    visitor->visit_dependent(this);
}

//
// Class Types::Declared
//

Declared::Declared(const ScopedName& n, AST::Declaration* decl)
        : Named(n), m_decl(decl)
{}


void
Declared::accept(Visitor* visitor)
{
    visitor->visit_declared(this);
}

//
// Class Types::Template
//

Template::Template(const ScopedName& n, AST::Declaration* decl, const param_vector& params)
        : Declared(n, decl), m_params(params)
{}


void
Template::accept(Visitor* visitor)
{
    visitor->visit_template_type(this);
}

//
// Class Types::Modifier
//

Modifier::Modifier(Type* alias, const Mods& pre, const Mods& post)
        : m_alias(alias), m_pre(pre), m_post(post)
{}

Modifier::~Modifier()
{}

void
Modifier::accept(Visitor* visitor)
{
    visitor->visit_modifier(this);
}

//
// Class Types::Array
//

Array::Array(Type* alias, const Mods& sizes) : m_alias(alias), m_sizes(sizes)
{}

Array::~Array()
{}

void
Array::accept(Visitor* visitor)
{
    visitor->visit_array(this);
}

//
// Class Types::Parameterized
//

Parameterized::Parameterized(Template* t, const Type::vector& params)
        : m_template(t), m_params(params)
{}

Parameterized::~Parameterized()
{}

void
Parameterized::accept(Visitor* visitor)
{
    visitor->visit_parameterized(this);
}

//
// Class Types::FuncPtr
//

FuncPtr::FuncPtr(Type::Type* ret, const Mods& premods, const Type::vector& params)
        : m_return(ret), m_premod(premods), m_params(params)
{}

FuncPtr::~FuncPtr()
{}

void
FuncPtr::accept(Visitor* visitor)
{
    visitor->visit_func_ptr(this);
}

//
// Type::Visitor
//

Visitor::~Visitor()
{}
void Visitor::visit_type(Type*)
{}
void Visitor::visit_unknown(Unknown* t)
{
    visit_type(t);
}
void Visitor::visit_base(Base* t)
{
    visit_named(t);
}
void Visitor::visit_dependent(Dependent* t)
{
    visit_named(t);
}
void Visitor::visit_declared(Declared* t)
{
    visit_named(t);
}
void Visitor::visit_modifier(Modifier* t)
{
    visit_type(t);
}
void Visitor::visit_array(Array* t)
{
    visit_type(t);
}
void Visitor::visit_named(Named* t)
{
    visit_type(t);
}
void Visitor::visit_template_type(Template* t)
{
    visit_declared(t);
}
void Visitor::visit_parameterized(Parameterized* t)
{
    visit_type(t);
}
void Visitor::visit_func_ptr(FuncPtr* t)
{
    visit_type(t);
}

// exception wrong_type_cast
const char* wrong_type_cast::what() const throw()
{
    return "Type::wrong_type_cast";
}
// vim: set ts=8 sts=4 sw=4 et:
