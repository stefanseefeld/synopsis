// File: builder.cc

#include "builder.hh"

Builder::Builder()
{
    m_scope = new AST::Scope("", 0, "file", AST::Name());
}

Builder::~Builder()
{
    // Delete all ...
}

AST::Namespace* Builder::startNamespace(string name)
{
    AST::Name scope = m_scope->name();
    scope.push_back(name);
    m_scope_stack.push(m_scope);
    AST::Namespace* ns = new AST::Namespace(m_filename, 0, "namespace", scope);
    m_scope = ns;
    return ns;
}

void Builder::endNamespace()
{
    // Check if it is a namespace...
    m_scope = m_scope_stack.top();
    m_scope_stack.pop();
}

