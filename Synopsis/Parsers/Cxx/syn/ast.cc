// File: ast.cc
// AST hierarchy

#include "ast.hh"
#include "type.hh"

using namespace AST;

Declaration::Declaration(const std::string &fn, int line, const std::string &type, const Name &name)
    : m_name(name), m_filename(fn), m_line(line), m_type(type), m_access(Default), m_declared(0)
{}

Declaration::~Declaration()
{
}

void Declaration::accept(Visitor* visitor)
{
    visitor->visitDeclaration(this);
}

Type::Declared* Declaration::declared()
{
    if (!m_declared) {
	m_declared = new Type::Declared(m_name, this);
    }
    return m_declared;
}

//
// AST::Scope
//

Scope::Scope(const std::string &fn, int line, const std::string &type, const Name &name)
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

Namespace::Namespace(const std::string &fn, int line, const std::string &type, const Name &name)
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

Class::Class(const std::string &fn, int line, const std::string &type, const Name &name)
    : Scope(fn, line, type, name)
{
    m_template = NULL;
}

Class::~Class()
{
}

void Class::accept(Visitor* visitor)
{
    visitor->visitClass(this);
}

//
// AST::Inheritance
//

Inheritance::Inheritance(Type::Type* type, const Attrs& attrs)
    : m_attrs(attrs), m_parent(type)
{
}

/*Inheritance::~Inheritance()
{
}*/

void Inheritance::accept(Visitor* visitor)
{
    visitor->visitInheritance(this);
}


//
// AST::Forward
//

Forward::Forward(const std::string &fn, int line, const std::string &type, const Name &name)
    : Declaration(fn, line, type, name)
{
}

Forward::Forward(AST::Declaration* decl)
    : Declaration(decl->filename(), decl->line(), decl->type(), decl->name())
{
}

/*Forward::~Forward()
{
}*/

void Forward::accept(Visitor* visitor)
{
    visitor->visitForward(this);
}


//
// AST::Typedef
//

Typedef::Typedef(const std::string &fn, int line, const std::string &type, const Name &name, Type::Type* alias, bool constr)
    : Declaration(fn, line, type, name), m_alias(alias), m_constr(constr)
{
}

/*Typedef::~Typedef()
{
}*/

void Typedef::accept(Visitor* visitor)
{
    visitor->visitTypedef(this);
}


//
// AST::Variable
//

Variable::Variable(const std::string &fn, int line, const std::string &type, const Name &name, Type::Type* vtype, bool constr)
    : Declaration(fn, line, type, name), m_vtype(vtype), m_constr(constr)
{
}

/*Variable::~Variable()
{
}*/

void Variable::accept(Visitor* visitor)
{
    visitor->visitVariable(this);
}

//
// AST::Const
//

Const::Const(const std::string &fn, int line, const std::string &type, const Name &name, Type::Type* t, const std::string &v)
    : Declaration(fn, line, type, name), m_ctype(t), m_value(v)
{
}

/*Const::~Const()
{
}*/

void Const::accept(Visitor* visitor)
{
    visitor->visitConst(this);
}



//
// AST::Enum
//

Enum::Enum(const std::string &fn, int line, const std::string &type, const Name &name)
    : Declaration(fn, line, type, name)
{
}

Enum::~Enum()
{
}

void Enum::accept(Visitor* visitor)
{
    visitor->visitEnum(this);
}


//
// AST::Enumerator
//

Enumerator::Enumerator(const std::string &fn, int line, const std::string &type, const Name &name, const std::string &value)
    : Declaration(fn, line, type, name), m_value(value)
{
}

/*Enumerator::~Enumerator()
{
}*/

void Enumerator::accept(Visitor* visitor)
{
    visitor->visitEnumerator(this);
}


//
// AST::Function
//

Function::Function(const std::string &fn, int line, const std::string &type, const Name &name, const Mods &premod, Type::Type* ret, const std::string &realname)
    : Declaration(fn, line, type, name), m_pre(premod), m_ret(ret),
      m_realname(realname)
{
}

Function::~Function()
{
    // Recursively delete parameters..
}

void Function::accept(Visitor* visitor)
{
    visitor->visitFunction(this);
}


//
// AST::Operation
//

Operation::Operation(const std::string &fn, int line, const std::string &type, const Name &name, const Mods &premod, Type::Type* ret, const std::string &realname)
    : Function(fn, line, type, name, premod, ret, realname)
{
}

/*Operation::~Operation()
{
}*/

void Operation::accept(Visitor* visitor)
{
    visitor->visitOperation(this);
}


//
// AST::Parameter
//

Parameter::Parameter(const Mods &pre, Type::Type* t, const Mods &post, const std::string &name, const std::string &value)
    : m_pre(pre), m_post(post), m_type(t), m_name(name), m_value(value)
{
}

/*Parameter::~Parameter()
{
}*/

void Parameter::accept(Visitor* visitor)
{
    visitor->visitParameter(this);
}


Comment::Comment(const std::string &file, int line, const std::string &text)
    : m_filename(file), m_line(line), m_text(text)
{
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
void Visitor::visitInheritance(Inheritance* d) {}
void Visitor::visitForward(Forward* d) { visitDeclaration(d); }
void Visitor::visitTypedef(Typedef* d) { visitDeclaration(d); }
void Visitor::visitVariable(Variable* d) { visitDeclaration(d); }
void Visitor::visitConst(Const* d) { visitDeclaration(d); }
void Visitor::visitEnum(Enum* d) { visitDeclaration(d); }
void Visitor::visitEnumerator(Enumerator* d) { visitDeclaration(d); }
void Visitor::visitFunction(Function* d) { visitDeclaration(d); }
void Visitor::visitOperation(Operation* d) { visitFunction(d); }
void Visitor::visitParameter(Parameter* d) { }
