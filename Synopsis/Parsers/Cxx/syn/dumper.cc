// File: dumper.cc


#include "common.hh"
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

void TypeFormatter::setScope(const ScopedName& scope) {
    m_scope = scope;
}

std::string TypeFormatter::colonate(const ScopedName& name)
{
    std::string str;
    ScopedName::const_iterator n_iter = name.begin();
    ScopedName::const_iterator s_iter = m_scope.begin();
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

std::string TypeFormatter::format(const Types::Type* type)
{
    if (!type) return "(unknown)";
    const_cast<Types::Type*>(type)->accept(this);
    return m_type;
}

void TypeFormatter::visit_type(Types::Type* type)
{
    m_type = "(unknown)";
}

void TypeFormatter::visit_unknown(Types::Unknown* type)
{
    m_type = colonate(type->name());
}

void TypeFormatter::visit_modifier(Types::Modifier* type)
{
    // Premods
    std::string pre = "";
    Types::Type::Mods::iterator iter = type->pre().begin();
    while (iter != type->pre().end())
	pre += *iter++ + " ";
    // Alias
    m_type = pre + format(type->alias());
    // Postmods
    iter = type->post().begin();
    while (iter != type->post().end())
	m_type += " " + *iter++;
}

void TypeFormatter::visit_named(Types::Named* type)
{
    m_type = colonate(type->name());
}

void TypeFormatter::visit_base(Types::Base* type)
{
    m_type = colonate(type->name());
}

void TypeFormatter::visit_declared(Types::Declared* type)
{
    m_type = colonate(type->name());
}

void TypeFormatter::visit_template_type(Types::Template* type)
{
    m_type = colonate(type->name());
}

void TypeFormatter::visit_parameterized(Types::Parameterized* type)
{
    std::string str;
    if (type->template_type())
	str = colonate(type->template_type()->name()) + "<";
    else
	str = "(unknown)<";
    if (type->parameters().size()) {
	str += format(type->parameters().front());
	Types::Type::vector::iterator iter = type->parameters().begin();
	while (++iter != type->parameters().end())
	    str += "," + format(*iter);
    }
    m_type = str + ">";
}

void TypeFormatter::visit_func_ptr(Types::FuncPtr* type)
{
    std::string str = "(*)(";
    if (type->parameters().size()) {
	str += format(type->parameters().front());
	Types::Type::vector::iterator iter = type->parameters().begin();
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

std::string append(const std::vector<std::string>& strs, const std::string &sep = " ")
{
    std::vector<std::string>::const_iterator iter = strs.begin();
    std::string str = "";
    while (iter != strs.end())
	str += *iter++ + sep;
    return str;
}

//. Utility method
// string colonate(const Types::Name& name)
// {
//     if (!name.size()) return "";
//     string str = name[0];
//     Types::Name::const_iterator iter = name.begin();
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

void Dumper::visit_declaration(AST::Declaration* decl)
{
    visit(decl->comments());
    if (decl->type() == "dummy") return;
    std::cout << m_indent_string << "DECL " << decl->name() << std::endl;
}

void Dumper::visit_scope(AST::Scope* scope)
{
    // Generally the File scope..
    visit(scope->declarations());
}

void Dumper::visit_namespace(AST::Namespace* ns)
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

void Dumper::visit_class(AST::Class* clas)
{
    visit(clas->comments());
    if (clas->template_type()) {
	m_scope.push_back(clas->name().back());
	Types::Template* templ = clas->template_type();
	std::cout << m_indent_string << "template<";
	std::vector<std::string> names;
	Types::Type::vector::iterator iter = templ->parameters().begin();
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
    const ScopedName& name = func->name();
    if (name.size() < 2) return false;
    std::string realname = func->realname();
    if (realname[0] == '~') return true;
    ScopedName::const_iterator second_last;
    second_last = name.end() - 2;
    return (realname == *second_last);
}

void Dumper::visit_operation(AST::Operation* oper)
{
    visit(oper->comments());
    std::cout << m_indent_string;
    if (!isStructor(oper) && oper->return_type()) std::cout << format(oper->return_type()) + " ";
    std::cout << oper->realname() << "(";
    if (oper->parameters().size()) {
	std::cout << formatParam(oper->parameters().front());
	std::vector<AST::Parameter*>::iterator iter = oper->parameters().begin();
	while (++iter != oper->parameters().end())
	    std::cout << "," << formatParam(*iter);
    }
    std::cout << ");" << std::endl;
}

void Dumper::visit_variable(AST::Variable* var)
{
    visit(var->comments());
    std::cout << m_indent_string << format(var->vtype()) << " " << var->name().back() << ";" << std::endl;
}

void Dumper::visit_typedef(AST::Typedef* tdef)
{
    visit(tdef->comments());
    std::cout << m_indent_string << "typedef " << format(tdef->alias()) << " " << tdef->name().back() << ";" << std::endl;
}

void Dumper::visit_enum(AST::Enum* decl)
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

void Dumper::visit_enumerator(AST::Enumerator* enumor)
{
    visit(enumor->comments());
    if (enumor->type() == "dummy") return;
    std::cout << m_indent_string << enumor->name().back();
    if (enumor->value().size())
	std::cout << " = " << enumor->value();
    std::cout << "," << std::endl;
}


