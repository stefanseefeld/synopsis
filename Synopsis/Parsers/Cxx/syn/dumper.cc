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

string Dumper::colonate(const AST::Name& name)
{
    string str;
    AST::Name::const_iterator n_iter = name.begin();
    AST::Name::const_iterator s_iter = m_scope.begin();
    // Skip identical scopes
    while ((n_iter != name.end()) && (s_iter != m_scope.end()) && (*n_iter == *s_iter))
	++n_iter, ++s_iter;
    // If name == scope, just return last eg: struct S { S* ptr; };
    if (n_iter == name.end()) return name.back();
    // Join the rest in name with colons
    str = *n_iter++;
    while (n_iter != name.end())
	str += "::" + *n_iter++;
    return str;
}

//
// Type Visitor
//

//. Utility method
string colonate(const Type::Name& name)
{
    if (!name.size()) return "";
    string str = name[0];
    Type::Name::const_iterator iter = name.begin();
    while (++iter != name.end())
	str += "::" + *iter;
    return str;
}

string Dumper::format(Type::Type* type)
{
    if (!type) return "(unknown)";
    type->accept(this);
    return m_type;
}

void Dumper::visitType(Type::Type* type)
{
    m_type = "(unknown)";
}

void Dumper::visitForward(Type::Forward* type)
{
    m_type = "{"+colonate(type->name())+"}";
}

void Dumper::visitModifier(Type::Modifier* type)
{
    // Premods
    string pre = "";
    Type::Type::Mods::iterator iter = type->pre().begin();
    while (iter != type->pre().end())
	pre += *iter++ + " ";
    // Alias
    m_type = pre + format(type->alias());
    // Postmods
    iter = type->post().begin();
    while (iter != type->post().end())
	m_type += " " + *iter++;
}

void Dumper::visitNamed(Type::Named* type)
{
    m_type = colonate(type->name());
}

void Dumper::visitBase(Type::Base* type)
{
    m_type = colonate(type->name());
}

void Dumper::visitDeclared(Type::Declared* type)
{
    m_type = colonate(type->name());
}

void Dumper::visitTemplateType(Type::Template* type)
{
    m_type = colonate(type->name());
}

void Dumper::visitParameterized(Type::Parameterized* type)
{
    m_type = colonate(type->templateType()->name()) + "<";
    if (type->parameters().size()) {
	m_type += format(type->parameters().front());
	Type::Type::vector_t::iterator iter = type->parameters().begin();
	while (++iter != type->parameters().end())
	    m_type += "," + format(*iter);
    }
    m_type += ">";
}

void Dumper::visitFuncPtr(Type::FuncPtr* type)
{
    string str = "(*)(";
    if (type->parameters().size()) {
	str += format(type->parameters().front());
	Type::Type::vector_t::iterator iter = type->parameters().begin();
	while (++iter != type->parameters().end())
	    str += "," + format(*iter);
    }
    m_type = str + ")";

}




//
// AST Visitor
//

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

// Format a Parameter
string Dumper::format(AST::Parameter* param)
{
    string str = param->premodifier();
    str += format(param->type());
    if (param->name().size()) str += " " + param->name();
    if (param->value().size()) str += " = " + param->value();
    str += param->postmodifier();
    return str;
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
    m_scope.push_back(ns->name().back());
    visit(ns->declarations());
    m_scope.pop_back();
    undent();
    cout << m_indent_string << "}" << endl;
}

void Dumper::visitClass(AST::Class* clas)
{
    cout << m_indent_string << clas->type() << " " << clas->name() << " {" << endl;
    indent();
    m_scope.push_back(clas->name().back());
    visit(clas->declarations());
    m_scope.pop_back();
    undent();
    cout << m_indent_string << "};" << endl;
}

void Dumper::visitOperation(AST::Operation* oper)
{
    cout << m_indent_string;
    cout << format(oper->returnType()) + " ";
    cout << oper->realname() << "(";
    if (oper->parameters().size()) {
	cout << format(oper->parameters().front());
	vector<AST::Parameter*>::iterator iter = oper->parameters().begin();
	while (++iter != oper->parameters().end())
	    cout << "," << format(*iter);
    }
    cout << ")" << endl;
}

void Dumper::visitVariable(AST::Variable* var)
{
    cout << m_indent_string << format(var->vtype()) << " " << var->name().back() << ";" << endl;
}

void Dumper::visitTypedef(AST::Typedef* tdef)
{
    cout << m_indent_string << "typedef " << format(tdef->alias()) << " " << tdef->name().back() << ";" << endl;
}
