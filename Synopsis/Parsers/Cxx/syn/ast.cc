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

//
// AST::Scope
//

Scope::Scope(string fn, int line, string type, Name name)
    : Declaration(fn, line, type, name)
{
}

Scope::~Scope()
{
    // recursively delete...
}

void Scope::accept(Visitor* visitor)
{
    visitor->visitScope(this);
}

//
// AST::Namespace
//

Namespace::Namespace(string fn, int line, string type, Name name)
    : Scope(fn, line, type, name)
{
}

Namespace::~Namespace()
{
}

void Namespace::accept(Visitor* visitor)
{
    visitor->visitNamespace(this);
}

//
// AST::Class
//

Class::Class(string fn, int line, string type, Name name)
    : Scope(fn, line, type, name)
{
}

Class::~Class()
{
}

void Class::accept(Visitor* visitor)
{
    visitor->visitClass(this);
}

//
// AST::Visitor
//

//Visitor::Visitor() {}
Visitor::~Visitor() {}
void Visitor::visitDeclaration(Declaration*) {}
void Visitor::visitScope(Scope* d) { visitDeclaration(d); }
void Visitor::visitNamespace(Namespace* d) { visitScope(d); }
void Visitor::visitClass(Class* d) { visitScope(d); }
void Visitor::visitInheritance(Inheritance*) {}
