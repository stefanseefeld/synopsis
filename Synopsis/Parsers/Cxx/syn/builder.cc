// File: builder.cc

#include "builder.hh"
#include "type.hh"

Builder::Builder()
{
    AST::Name name;
    m_scope = new AST::Scope("", 0, "file", name);
}

Builder::~Builder()
{
    // Delete all ...
}

void Builder::add(AST::Declaration* decl)
{
    m_scope->declarations().push_back(decl);
}

AST::Namespace* Builder::startNamespace(string name)
{
    // Generate the name
    AST::Name scope = m_scope->name();
    scope.push_back(name);
    // Create the Namespace
    AST::Namespace* ns = new AST::Namespace(m_filename, 0, "namespace", scope);
    add(ns);
    // Push stack
    m_scope_stack.push(m_scope);
    m_scope = ns;
    return ns;
}

void Builder::endNamespace()
{
    // Check if it is a namespace...
    m_scope = m_scope_stack.top();
    m_scope_stack.pop();
}

AST::Class* Builder::startClass(string type, string name)
{
    // Generate the name
    AST::Name scope = m_scope->name();
    scope.push_back(name);
    // Create the Class
    AST::Class* ns = new AST::Class(m_filename, 0, type, scope);
    add(ns);
    // Push stack
    m_scope_stack.push(m_scope);
    m_scope = ns;
    return ns;
}

void Builder::endClass()
{
    // Check if it is a class...
    m_scope = m_scope_stack.top();
    m_scope_stack.pop();
}


//
// Type Methods
//

Type::Forward* Builder::Forward(string name)
{
    // Generate the name
    AST::Name scope = m_scope->name();
    scope.push_back(name);
    return new Type::Forward(scope);
}
