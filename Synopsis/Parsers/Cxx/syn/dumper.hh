// Synopsis C++ Parser: dumper.hh header file
// The TypeFormatter and Dumper classes

// $Id: dumper.hh,v 1.15 2003/01/27 06:53:37 chalky Exp $
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

#ifndef H_SYNOPSIS_CPP_DUMPER
#define H_SYNOPSIS_CPP_DUMPER

#include "ast.hh"
#include "type.hh"

//. Formats Types in a way suitable for output
class TypeFormatter : public Types::Visitor
{
public:
    TypeFormatter();

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
    //. types. The pointed to pointer will be set to NULL if the identifier is
    //. used
    std::string format(const Types::Type*, const std::string** id = NULL);
    virtual void visit_type(Types::Type*);
    virtual void visit_unknown(Types::Unknown*);
    virtual void visit_modifier(Types::Modifier*);
    virtual void visit_named(Types::Named*);
    virtual void visit_base(Types::Base*);
    virtual void visit_declared(Types::Declared*);
    virtual void visit_template_type(Types::Template*);
    virtual void visit_parameterized(Types::Parameterized*);
    virtual void visit_func_ptr(Types::FuncPtr*);

protected:
    //. The Type String
    std::string m_type;
    //. The current scope name
    ScopedName m_scope;
    //. Returns the given Name relative to the current scope
    std::string colonate(const ScopedName& name);
    //. A stack of previous scopes
    std::vector<ScopedName> m_scope_stack;
    //. A pointer to the identifier for function pointers
    const std::string** m_fptr_id;
};

//. Dumper displays the AST to the screen
class Dumper : public AST::Visitor, public TypeFormatter
{
public:
    Dumper();

    //. Sets to only show decls with given filename
    void onlyShow(const std::string& fname);

    std::string formatParam(AST::Parameter*);
    //
    // AST Visitor
    //
    void visit(const std::vector<AST::Declaration*>&);
    void visit(const std::vector<AST::Comment*>&);
    virtual void visit_macro(AST::Macro*);
    virtual void visit_declaration(AST::Declaration*);
    virtual void visit_scope(AST::Scope*);
    virtual void visit_namespace(AST::Namespace*);
    virtual void visit_forward(AST::Forward*);
    virtual void visit_class(AST::Class*);
    virtual void visit_function(AST::Function*);
    virtual void visit_variable(AST::Variable*);
    virtual void visit_typedef(AST::Typedef*);
    virtual void visit_enum(AST::Enum*);
    virtual void visit_enumerator(AST::Enumerator*);

private:
    //. The indent depth
    int m_indent;
    //. The indent string
    std::string m_indent_string;
    //. Increases indent
    void indent();
    //. Decreases indent
    void undent();
    //. Only show this filename, if set
    std::string m_filename;

};

#endif // header guard
// vim: set ts=8 sts=4 sw=4 et:
