// Synopsis C++ Parser: ast.cc source file
// Implementation of the AST classes

// $Id: ast.cc,v 1.18 2003/01/16 17:14:10 chalky Exp $
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

// $Log: ast.cc,v $
// Revision 1.18  2003/01/16 17:14:10  chalky
// Increase AST version number. SourceFiles store full filename. Executor/Project
// uses it to check timestamp for all included files when deciding whether to
// reparse input files.
//
// Revision 1.17  2002/12/12 17:25:33  chalky
// Implemented Include support for C++ parser. A few other minor fixes.
//
// Revision 1.16  2002/12/09 04:00:59  chalky
// Added multiple file support to parsers, changed AST datastructure to handle
// new information, added a demo to demo/C++. AST Declarations now have a
// reference to a SourceFile (which includes a filename) instead of a filename.
//
// Revision 1.15  2002/11/17 12:11:43  chalky
// Reformatted all files with astyle --style=ansi, renamed fakegc.hh
//
//

#include "strace.hh"
#include "ast.hh"
#include "type.hh"

using namespace AST;

//
// AST::SourceFile
//

SourceFile::SourceFile(const std::string& filename, const std::string& full_filename, bool is_main)
    : m_filename(filename), m_full_filename(full_filename), m_is_main(is_main)
{ }

//
// AST::Include
//

Include::Include(SourceFile* target, bool is_macro, bool is_next)
    : m_target(target), m_is_macro(is_macro), m_is_next(is_next)
{ }

//
// AST::Declaration
//

Declaration::Declaration(SourceFile* file, int line, const std::string& type, const ScopedName& name)
        : m_file(file), m_line(line), m_type(type), m_name(name), m_access(Default), m_declared(NULL)
{ }

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

Scope::Scope(SourceFile* file, int line, const std::string& type, const ScopedName& name)
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
// AST::Namespace
//

Namespace::Namespace(SourceFile* file, int line, const std::string& type, const ScopedName& name)
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
// AST::Class
//

Class::Class(SourceFile* file, int line, const std::string& type, const ScopedName& name)
        : Scope(file, line, type, name)
{
    m_template = NULL;
}

Class::~Class()
{}

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

Forward::Forward(SourceFile* file, int line, const std::string& type, const ScopedName& name)
        : Declaration(file, line, type, name), m_template(NULL)
{ }

Forward::Forward(AST::Declaration* decl)
        : Declaration(decl->file(), decl->line(), decl->type(), decl->name()), m_template(NULL)
{ }

void
Forward::accept(Visitor* visitor)
{
    visitor->visit_forward(this);
}


//
// AST::Typedef
//

Typedef::Typedef(SourceFile* file, int line, const std::string& type, const ScopedName& name, Types::Type* alias, bool constr)
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
// AST::Variable
//

Variable::Variable(SourceFile* file, int line, const std::string& type, const ScopedName& name, Types::Type* vtype, bool constr)
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
// AST::Const
//

Const::Const(SourceFile* file, int line, const std::string& type, const ScopedName& name, Types::Type* t, const std::string& v)
        : Declaration(file, line, type, name), m_ctype(t), m_value(v)
{ }

void
Const::accept(Visitor* visitor)
{
    visitor->visit_const(this);
}


//
// AST::Enum
//

Enum::Enum(SourceFile* file, int line, const std::string& type, const ScopedName& name)
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
// AST::Enumerator
//

Enumerator::Enumerator(SourceFile* file, int line, const std::string& type, const ScopedName& name, const std::string& value)
        : Declaration(file, line, type, name), m_value(value)
{ }

void
Enumerator::accept(Visitor* visitor)
{
    visitor->visit_enumerator(this);
}


//
// AST::Function
//

Function::Function(
    SourceFile* file, int line, const std::string& type, const ScopedName& name,
    const Mods& premod, Types::Type* ret, const std::string& realname
)
        : Declaration(file, line, type, name), m_pre(premod), m_ret(ret),
        m_realname(realname), m_template(NULL)
{}

Function::~Function()
{}

void
Function::accept(Visitor* visitor)
{
    visitor->visit_function(this);
}


//
// AST::Operation
//

Operation::Operation(
    SourceFile* file, int line, const std::string& type, const ScopedName& name,
    const Mods& premod, Types::Type* ret, const std::string& realname
)
        : Function(file, line, type, name, premod, ret, realname)
{ }

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
{}

void
Parameter::accept(Visitor* visitor)
{
    visitor->visit_parameter(this);
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

// vim: set ts=8 sts=4 sw=4 et:
