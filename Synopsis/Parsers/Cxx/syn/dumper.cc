// File: dumper.cc


#include "dumper.hh"

Dumper::Dumper()
{
    m_indent = 0;
    m_indent_string = "";
}

void Dumper::onlyShow(string fname)
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

string join(const vector<string>& strs, string sep = " ")
{
    vector<string>::const_iterator iter = strs.begin();
    if (iter == strs.end()) return "";
    string str = *iter++;
    while (iter != strs.end())
	str += sep + *iter++;
    return str;
}

string append(const vector<string>& strs, string sep = " ")
{
    vector<string>::const_iterator iter = strs.begin();
    string str = "";
    while (iter != strs.end())
	str += *iter++ + sep;
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

void Dumper::visitUnknown(Type::Unknown* type)
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
    string str;
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
	if (!m_filename.size() || (*iter)->filename() == m_filename)
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

void Dumper::visit(const vector<AST::Comment*>& comms)
{
    vector<AST::Comment*>::const_iterator iter = comms.begin();
    while (iter != comms.end()) {
	cout << m_indent_string << (*iter++)->text() << endl;
    }
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
    visit(decl->comments());
    if (decl->type() == "dummy") return;
    cout << m_indent_string << "DECL " << decl->name() << endl;
}

void Dumper::visitScope(AST::Scope* scope)
{
    // Generally the File scope..
    visit(scope->declarations());
}

void Dumper::visitNamespace(AST::Namespace* ns)
{
    visit(ns->comments());
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
    visit(clas->comments());
    if (clas->templateType()) {
	m_scope.push_back(clas->name().back());
	Type::Template* templ = clas->templateType();
	cout << m_indent_string << "template<";
	vector<string> names;
	Type::Type::vector_t::iterator iter = templ->parameters().begin();
	while (iter != templ->parameters().end())
	    names.push_back(format(*iter++));
	cout << join(names, ", ") << ">" << endl;
	m_scope.pop_back();
    }
    cout << m_indent_string << clas->type() << " " << clas->name();
    if (clas->parents().size()) {
	cout << ": ";
	vector<string> inherits;
	vector<AST::Inheritance*>::iterator iter = clas->parents().begin();
	for (;iter != clas->parents().end(); ++iter)
	    inherits.push_back(append((*iter)->attributes()) + format((*iter)->parent()));
	cout << join(inherits, ", ");
    }
    cout << " {" << endl;
    indent();
    m_scope.push_back(clas->name().back());
    visit(clas->declarations());
    m_scope.pop_back();
    undent();
    cout << m_indent_string << "};" << endl;
}


// Utility func that returns true if name[-1] == name[-2]
// ie: constructor or destructor
bool isStructor(const AST::Function* func)
{
    const AST::Name& name = func->name();
    if (name.size() < 2) return false;
    string realname = func->realname();
    if (realname[0] == '~') return true;
    AST::Name::const_iterator second_last;
    second_last = name.end() - 2;
    return (realname == *second_last);
}

void Dumper::visitOperation(AST::Operation* oper)
{
    visit(oper->comments());
    cout << m_indent_string;
    if (!isStructor(oper)) cout << format(oper->returnType()) + " ";
    cout << oper->realname() << "(";
    if (oper->parameters().size()) {
	cout << format(oper->parameters().front());
	vector<AST::Parameter*>::iterator iter = oper->parameters().begin();
	while (++iter != oper->parameters().end())
	    cout << "," << format(*iter);
    }
    cout << ");" << endl;
}

void Dumper::visitVariable(AST::Variable* var)
{
    visit(var->comments());
    cout << m_indent_string << format(var->vtype()) << " " << var->name().back() << ";" << endl;
}

void Dumper::visitTypedef(AST::Typedef* tdef)
{
    visit(tdef->comments());
    cout << m_indent_string << "typedef " << format(tdef->alias()) << " " << tdef->name().back() << ";" << endl;
}

void Dumper::visitEnum(AST::Enum* decl)
{
    visit(decl->comments());
    cout << m_indent_string << "enum " << decl->name().back() << "{" << endl;
    indent();
    vector<AST::Enumerator*>::iterator iter = decl->enumerators().begin();
    while (iter != decl->enumerators().end())
	(*iter++)->accept(this);
    undent();
    cout << m_indent_string << "};" << endl;
}

void Dumper::visitEnumerator(AST::Enumerator* enumor)
{
    visit(enumor->comments());
    if (enumor->type() == "dummy") return;
    cout << m_indent_string << enumor->name().back();
    if (enumor->value().size())
	cout << " = " << enumor->value();
    cout << "," << endl;
}


