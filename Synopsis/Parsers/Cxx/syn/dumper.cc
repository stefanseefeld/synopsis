// Synopsis C++ Parser: dumper.cc source file
// Implementation of the TypeFormatter and Dumper classes

// $Id: dumper.cc,v 1.21 2002/11/17 12:11:43 chalky Exp $
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

// $Log: dumper.cc,v $
// Revision 1.21  2002/11/17 12:11:43  chalky
// Reformatted all files with astyle --style=ansi, renamed fakegc.hh
//
//


#include "common.hh"
#include "dumper.hh"
#include <iostream>

//
//
// TypeFormatter
//
//

TypeFormatter::TypeFormatter()
        : m_fptr_id(NULL)
{
    m_scope_stack.push_back(ScopedName());
}

void TypeFormatter::push_scope(const ScopedName& scope)
{
    m_scope_stack.push_back(m_scope);
    m_scope = scope;
}

void TypeFormatter::pop_scope()
{
    m_scope = m_scope_stack.back();
    m_scope_stack.pop_back();
}

std::string TypeFormatter::colonate(const ScopedName& name)
{
    std::string str;
    ScopedName::const_iterator n_iter = name.begin();
    ScopedName::const_iterator s_iter = m_scope.begin();
    // Skip identical scopes
    while ((n_iter != name.end()) && (s_iter != m_scope.end()) && (*n_iter == *s_iter))
        ++n_iter, ++s_iter;
    // If name == scope, just return last eg: struct S { S* ptr; };
    if (n_iter == name.end())
        return name.back();
    // Join the rest in name with colons
    str = *n_iter++;
    while (n_iter != name.end())
        str += "::" + *n_iter++;
    return str;
}



//
// Type Visitor
//

std::string TypeFormatter::format(const Types::Type* type, const std::string** id)
{
    if (!type)
        return "(unknown)";
    const std::string** save = NULL;
    if (id)
    {
        save = m_fptr_id;
        m_fptr_id = id;
    }
    const_cast<Types::Type*>(type)->accept(this);
    if (id)
        m_fptr_id = save;
    return m_type;
}

void TypeFormatter::visit_type(Types::Type* type)
{
    m_type = "(unknown)";
}

void TypeFormatter::visit_unknown(Types::Unknown* type)
{
    m_type = colonate(type->name());
}

void TypeFormatter::visit_modifier(Types::Modifier* type)
{
    // Premods
    std::string pre = "";
    Types::Type::Mods::iterator iter = type->pre().begin();
    while (iter != type->pre().end())
        if (*iter == "*" || *iter == "&")
            pre += *iter++;
        else
            pre += *iter++ + " ";
    // Alias
    m_type = pre + format(type->alias());
    // Postmods
    iter = type->post().begin();
    while (iter != type->post().end())
        if (*iter == "*" || *iter == "&")
            m_type += *iter++;
        else
            m_type += " " + *iter++;
}

void TypeFormatter::visit_named(Types::Named* type)
{
    m_type = colonate(type->name());
}

void TypeFormatter::visit_base(Types::Base* type)
{
    m_type = colonate(type->name());
}

void TypeFormatter::visit_declared(Types::Declared* type)
{
    m_type = colonate(type->name());
}

void TypeFormatter::visit_template_type(Types::Template* type)
{
    m_type = colonate(type->name());
}

void TypeFormatter::visit_parameterized(Types::Parameterized* type)
{
    std::string str;
    if (type->template_type())
        str = colonate(type->template_type()->name()) + "<";
    else
        str = "(unknown)<";
    if (type->parameters().size())
    {
        str += format(type->parameters().front());
        Types::Type::vector::iterator iter = type->parameters().begin();
        while (++iter != type->parameters().end())
            str += "," + format(*iter);
    }
    m_type = str + ">";
}

void TypeFormatter::visit_func_ptr(Types::FuncPtr* type)
{
    std::string str = format(type->return_type()) + "(";
    Types::Type::Mods::iterator i_pre = type->pre().begin();
    while (i_pre != type->pre().end())
        str += *i_pre++;
    if (m_fptr_id)
    {
        str += **m_fptr_id;
        *m_fptr_id = NULL;
    }
    str += ")(";
    if (type->parameters().size())
    {
        str += format(type->parameters().front());
        Types::Type::vector::iterator iter = type->parameters().begin();
        while (++iter != type->parameters().end())
            str += "," + format(*iter);
    }
    m_type = str + ")";

}





//
//
// Dumper
//
//

Dumper::Dumper()
{
    m_indent = 0;
    m_indent_string = "";
}

void Dumper::onlyShow(const std::string &fname)
{
    m_filename = fname;
}

void Dumper::indent()
{
    ++m_indent;
    m_indent_string.assign(m_indent, ' ');
}

void Dumper::undent()
{
    --m_indent;
    m_indent_string.assign(m_indent, ' ');
}

std::string append(const std::vector<std::string>& strs, const std::string &sep = " ")
{
    std::vector<std::string>::const_iterator iter = strs.begin();
    std::string str = "";
    while (iter != strs.end())
        str += *iter++ + sep;
    return str;
}

//. Utility method
// string colonate(const Types::Name& name)
// {
//     if (!name.size()) return "";
//     string str = name[0];
//     Types::Name::const_iterator iter = name.begin();
//     while (++iter != name.end())
// 	str += "::" + *iter;
//     return str;
// }

//
// AST Visitor
//

void Dumper::visit(const std::vector<AST::Declaration*>& decls)
{
    std::vector<AST::Declaration*>::const_iterator iter, end;
    iter = decls.begin(), end = decls.end();
    for (; iter != end; ++iter)
        if (!m_filename.size() || (*iter)->filename() == m_filename)
            (*iter)->accept(this);
}

void Dumper::visit(const std::vector<AST::Comment*>& comms)
{
    std::vector<AST::Comment*>::const_iterator iter = comms.begin();
    while (iter != comms.end())
    {
        std::cout << m_indent_string << (*iter++)->text() << std::endl;
    }
}

// Format a Parameter
std::string Dumper::formatParam(AST::Parameter* param)
{
    std::string str;
    AST::Parameter::Mods::iterator imod = param->premodifier().begin();
    while (imod != param->premodifier().end())
        str += " " + *imod++;
    const std::string* name = &param->name();
    if (param->type())
        str += " " + format(param->type(), &name);
    if (name && param->name().size())
        str += " " + param->name();
    if (param->value().size())
        str += " = " + param->value();
    imod = param->postmodifier().begin();
    while (imod != param->postmodifier().end())
        str += " " + *imod++;
    return str;
}

void Dumper::visit_declaration(AST::Declaration* decl)
{
    visit(decl->comments());
    if (decl->type() == "dummy")
        return;
    std::cout << m_indent_string << "DECL " << decl->name() << std::endl;
}

void Dumper::visit_scope(AST::Scope* scope)
{
    // Generally the File scope..
    visit(scope->declarations());
}

void Dumper::visit_namespace(AST::Namespace* ns)
{
    visit(ns->comments());
    std::cout << m_indent_string << "namespace " << ns->name() << " {" << std::endl;
    indent();
    m_scope.push_back(ns->name().back());
    visit(ns->declarations());
    m_scope.pop_back();
    undent();
    std::cout << m_indent_string << "}" << std::endl;
}

void Dumper::visit_forward(AST::Forward* forward)
{
    visit(forward->comments());
    if (forward->template_type())
    {
        m_scope.push_back(forward->name().back());
        Types::Template* templ = forward->template_type();
        std::cout << m_indent_string << "template<";
        std::vector<std::string> names;
        AST::Parameter::vector::iterator iter = templ->parameters().begin();
        while (iter != templ->parameters().end())
            names.push_back(formatParam(*iter++));
        std::cout << join(names, ", ") << ">" << std::endl;
        m_scope.pop_back();
        if (forward->type().substr(0, 9) == "template ")
            std::cout << m_indent_string << (forward->type().c_str()+9) << " " << forward->name();
        else
            std::cout << m_indent_string << forward->type() << " " << forward->name() << ";" << std::endl;
    }
    else
        std::cout << m_indent_string << forward->name() << ";" << std::endl;
}


void Dumper::visit_class(AST::Class* clas)
{
    visit(clas->comments());
    if (clas->template_type())
    {
        m_scope.push_back(clas->name().back());
        Types::Template* templ = clas->template_type();
        std::cout << m_indent_string << "template<";
        std::vector<std::string> names;
        AST::Parameter::vector::iterator iter = templ->parameters().begin();
        while (iter != templ->parameters().end())
            names.push_back(formatParam(*iter++));
        std::cout << join(names, ", ") << ">" << std::endl;
        m_scope.pop_back();
        if (clas->type().substr(0, 9) == "template ")
            std::cout << m_indent_string << (clas->type().c_str()+9) << " " << clas->name();
        else
            std::cout << m_indent_string << clas->type() << " " << clas->name();
    }
    else
        std::cout << m_indent_string << clas->type() << " " << clas->name();
    if (clas->parents().size())
    {
        std::cout << ": ";
        std::vector<std::string> inherits;
        std::vector<AST::Inheritance*>::iterator iter = clas->parents().begin();
        for (;iter != clas->parents().end(); ++iter)
            inherits.push_back(append((*iter)->attributes()) + format((*iter)->parent()));
        std::cout << join(inherits, ", ");
    }
    std::cout << " {" << std::endl;
    indent();
    m_scope.push_back(clas->name().back());
    visit(clas->declarations());
    m_scope.pop_back();
    undent();
    std::cout << m_indent_string << "};" << std::endl;
}


// Utility func that returns true if name[-1] == name[-2]
// ie: constructor or destructor
bool isStructor(const AST::Function* func)
{
    const ScopedName& name = func->name();
    if (name.size() < 2)
        return false;
    std::string realname = func->realname();
    if (realname[0] == '~')
        return true;
    ScopedName::const_iterator second_last;
    second_last = name.end() - 2;
    return (realname == *second_last);
}

void Dumper::visit_function(AST::Function* func)
{
    visit(func->comments());
    std::cout << m_indent_string;
    if (func->template_type())
    {
        m_scope.push_back(func->name().back());
        Types::Template* templ = func->template_type();
        std::cout << m_indent_string << "template<";
        std::vector<std::string> names;
        AST::Parameter::vector::iterator iter = templ->parameters().begin();
        while (iter != templ->parameters().end())
            names.push_back(formatParam(*iter++));
        std::cout << join(names, ", ") << ">" << std::endl;
        m_scope.pop_back();
    }
    if (!isStructor(func) && func->return_type())
        std::cout << format(func->return_type()) + " ";
    std::cout << func->realname() << "(";
    if (func->parameters().size())
    {
        std::cout << formatParam(func->parameters().front());
        std::vector<AST::Parameter*>::iterator iter = func->parameters().begin();
        while (++iter != func->parameters().end())
            std::cout << "," << formatParam(*iter);
    }
    std::cout << ");" << std::endl;
}

void Dumper::visit_variable(AST::Variable* var)
{
    visit(var->comments());
    std::cout << m_indent_string << format(var->vtype()) << " " << var->name().back() << ";" << std::endl;
}

void Dumper::visit_typedef(AST::Typedef* tdef)
{
    visit(tdef->comments());
    std::cout << m_indent_string << "typedef " << format(tdef->alias()) << " " << tdef->name().back() << ";" << std::endl;
}

void Dumper::visit_enum(AST::Enum* decl)
{
    visit(decl->comments());
    std::cout << m_indent_string << "enum " << decl->name().back() << "{" << std::endl;
    indent();
    std::vector<AST::Enumerator*>::iterator iter = decl->enumerators().begin();
    while (iter != decl->enumerators().end())
        (*iter++)->accept(this);
    undent();
    std::cout << m_indent_string << "};" << std::endl;
}

void Dumper::visit_enumerator(AST::Enumerator* enumor)
{
    visit(enumor->comments());
    if (enumor->type() == "dummy")
        return;
    std::cout << m_indent_string << enumor->name().back();
    if (enumor->value().size())
        std::cout << " = " << enumor->value();
    std::cout << "," << std::endl;
}


// vim: set ts=8 sts=4 sw=4 et:
