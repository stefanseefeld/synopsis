// File: dumper.cc


#include "dumper.hh"

Dumper::Dumper()
{
    m_indent = 0;
    m_indent_string = "";
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

void Dumper::visit(const vector<AST::Declaration*>& decls)
{
    vector<AST::Declaration*>::const_iterator iter, end;
    iter = decls.begin(), end = decls.end();
    for (; iter != end; ++iter)
	(*iter)->accept(this);
}

ostream& operator << (ostream& os, const AST::Name& name)
{
    AST::Name::const_iterator iter = name.begin(), end = name.end();
    if (iter == end) { os << "<global>"; return os; }
    os << (*iter++);
    while (iter != end)
	os << "::" << (*iter++);
    return os;
}


void Dumper::visitDeclaration(AST::Declaration* decl)
{
    cout << m_indent_string << "DECL " << decl->name() << endl;
}

void Dumper::visitScope(AST::Scope* scope)
{
    // Generally the File scope..
    visit(scope->declarations());
}

void Dumper::visitNamespace(AST::Namespace* ns)
{
    cout << m_indent_string << "namespace " << ns->name() << " {" << endl;
    indent();
    visit(ns->declarations());
    undent();
    cout << m_indent_string << "}" << endl;
}

void Dumper::visitClass(AST::Class* clas)
{
    cout << m_indent_string << clas->type() << " " << clas->name() << " {" << endl;
    indent();
    visit(clas->declarations());
    undent();
    cout << m_indent_string << "};" << endl;
}

void Dumper::visitOperation(AST::Operation* oper)
{
    cout << m_indent_string << oper->realname() << "(";
    cout << ")" << endl;
}
