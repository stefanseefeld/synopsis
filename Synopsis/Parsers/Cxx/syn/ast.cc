// File: ast.cc
// AST hierarchy

#include "ast.hh"

using namespace AST;

Declaration::Declaration(string fn, int line, string type, Name name)
    : m_name(name), m_filename(fn), m_line(line), m_type(type),
      m_access(Default)
{
}

Declaration::~Declaration()
{
}

void Declaration::accept(Visitor* visitor)
{
    visitor->visitDeclaration(this);
}

Scope::Scope(string fn, int line, string type, Name name)
    : Declaration(fn, line, type, name)
{
}

Scope::~Scope()
{
    // recursively delete...
}

Namespace::Namespace(string fn, int line, string type, Name name)
    : Scope(fn, line, type, name)
{
}

Namespace::~Namespace()
{
}

