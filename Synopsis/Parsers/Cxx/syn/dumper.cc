// File: dumper.cc


#include "dumper.hh"
#include <iostream>

//
//
// TypeFormatter
//
//

TypeFormatter::TypeFormatter()
{
}

void TypeFormatter::setScope(const AST::Name& scope) {
    m_scope = scope;
}

std::string TypeFormatter::colonate(const AST::Name& name)
{
    std::string str;
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

std::string TypeFormatter::format(const Type::Type* type)
{
    if (!type) return "(unknown)";
    const_cast<Type::Type*>(type)->accept(this);
    return m_type;
}

void TypeFormatter::visitType(Type::Type* type)
{
    m_type = "(unknown)";
}

void TypeFormatter::visitUnknown(Type::Unknown* type)
{
    m_type = colonate(type->name());
}

void TypeFormatter::visitModifier(Type::Modifier* type)
{
    // Premods
    std::string pre = "";
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

void TypeFormatter::visitNamed(Type::Named* type)
{
    m_type = colonate(type->name());
}

void TypeFormatter::visitBase(Type::Base* type)
{
    m_type = colonate(type->name());
}

void TypeFormatter::visitDeclared(Type::Declared* type)
{
    m_type = colonate(type->name());
}

void TypeFormatter::visitTemplateType(Type::Template* type)
{
    m_type = colonate(type->name());
}

void TypeFormatter::visitParameterized(Type::Parameterized* type)
{
    std::string str;
    if (type->templateType())
	str = colonate(type->templateType()->name()) + "<";
    else
	str = "(unknown)<";
    if (type->parameters().size()) {
	str += format(type->parameters().front());
	Type::Type::vector_t::iterator iter = type->parameters().begin();
	while (++iter != type->parameters().end())
	    str += "," + format(*iter);
    }
    m_type = str + ">";
}

void TypeFormatter::visitFuncPtr(Type::FuncPtr* type)
{
    std::string str = "(*)(";
    if (type->parameters().size()) {
	str += format(type->parameters().front());
	Type::Type::vector_t::iterator iter = type->parameters().begin();
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

static std::string join(const std::vector<std::string>& strs, std::string sep = " ")
{
    std::vector<std::string>::const_iterator iter = strs.begin();
    if (iter == strs.end()) return "";
    std::string str = *iter++;
    while (iter != strs.end())
	str += sep + *iter++;
    return str;
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
// string colonate(const Type::Name& name)
// {
//     if (!name.size()) return "";
//     string str = name[0];
//     Type::Name::const_iterator iter = name.begin();
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

namespace {
    std::ostream& operator << (std::ostream& os, const AST::Name& name)
    {
	AST::Name::const_iterator iter = name.begin(), end = name.end();
	if (iter == end) { os << "<global>"; return os; }
	os << (*iter++);
	while (iter != end)
	    os << "::" << (*iter++);
	return os;
    }
}

void Dumper::visit(const std::vector<AST::Comment*>& comms)
{
    std::vector<AST::Comment*>::const_iterator iter = comms.begin();
    while (iter != comms.end()) {
	std::cout << m_indent_string << (*iter++)->text() << std::endl;
    }
}

// Format a Parameter
std::string Dumper::formatParam(AST::Parameter* param)
{
    std::string str = param->premodifier();
    if (str.size()) str += " ";
    str += format(param->type());
    if (param->name().size()) str += " " + param->name();
    if (param->value().size()) str += " = " + param->value();
    if (param->postmodifier().size()) str += " ";
    str += param->postmodifier();
    return str;
}

void Dumper::visitDeclaration(AST::Declaration* decl)
{
    visit(decl->comments());
    if (decl->type() == "dummy") return;
    std::cout << m_indent_string << "DECL " << decl->name() << std::endl;
}

void Dumper::visitScope(AST::Scope* scope)
{
    // Generally the File scope..
    visit(scope->declarations());
}

void Dumper::visitNamespace(AST::Namespace* ns)
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

void Dumper::visitClass(AST::Class* clas)
{
    visit(clas->comments());
    if (clas->templateType()) {
	m_scope.push_back(clas->name().back());
	Type::Template* templ = clas->templateType();
	std::cout << m_indent_string << "template<";
	std::vector<std::string> names;
	Type::Type::vector_t::iterator iter = templ->parameters().begin();
	while (iter != templ->parameters().end())
	    names.push_back(format(*iter++));
	std::cout << join(names, ", ") << ">" << std::endl;
	m_scope.pop_back();
    }
    std::cout << m_indent_string << clas->type() << " " << clas->name();
    if (clas->parents().size()) {
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
    const AST::Name& name = func->name();
    if (name.size() < 2) return false;
    std::string realname = func->realname();
    if (realname[0] == '~') return true;
    AST::Name::const_iterator second_last;
    second_last = name.end() - 2;
    return (realname == *second_last);
}

void Dumper::visitOperation(AST::Operation* oper)
{
    visit(oper->comments());
    std::cout << m_indent_string;
    if (!isStructor(oper)) std::cout << format(oper->returnType()) + " ";
    std::cout << oper->realname() << "(";
    if (oper->parameters().size()) {
	std::cout << formatParam(oper->parameters().front());
	std::vector<AST::Parameter*>::iterator iter = oper->parameters().begin();
	while (++iter != oper->parameters().end())
	    std::cout << "," << formatParam(*iter);
    }
    std::cout << ");" << std::endl;
}

void Dumper::visitVariable(AST::Variable* var)
{
    visit(var->comments());
    std::cout << m_indent_string << format(var->vtype()) << " " << var->name().back() << ";" << std::endl;
}

void Dumper::visitTypedef(AST::Typedef* tdef)
{
    visit(tdef->comments());
    std::cout << m_indent_string << "typedef " << format(tdef->alias()) << " " << tdef->name().back() << ";" << std::endl;
}

void Dumper::visitEnum(AST::Enum* decl)
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

void Dumper::visitEnumerator(AST::Enumerator* enumor)
{
    visit(enumor->comments());
    if (enumor->type() == "dummy") return;
    std::cout << m_indent_string << enumor->name().back();
    if (enumor->value().size())
	std::cout << " = " << enumor->value();
    std::cout << "," << std::endl;
}


