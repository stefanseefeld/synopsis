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

//. Add an operation
AST::Operation* Builder::addOperation(int line, string name, vector<string> premod, Type::Type* ret, string realname)
{
    // Generate the name
    AST::Name scope = m_scope->name();
    scope.push_back(name);
    AST::Operation* oper = new AST::Operation(m_filename, line, "method", scope, premod, ret, realname);
    add(oper);
    return oper;
}

//. Add a variable
AST::Variable* Builder::addVariable(int line, string name, Type::Type* vtype, bool constr)
{
    // Generate the name
    AST::Name scope = m_scope->name();
    scope.push_back(name);
    AST::Variable* var = new AST::Variable(m_filename, line, "variable", scope, vtype, constr);
    add(var);
    return var;
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
