// File: builder.cc

#include <map>
using std::map;

#include "builder.hh"
#include "type.hh"
#include "dict.hh"

//. Utility method
Name extend(Name name, string str)
{
    Name ret = name;
    ret.push_back(str);
    return ret;
}

//
// Class Builder::Scope
//
Builder::Scope::Scope(AST::Scope* s)
{
    scope_decl = s;
    search.push_back(this);
    dict = new Dictionary();
}

Builder::Scope::~Scope()
{
    delete dict;
}

//
// Struct Builder::Private
//

typedef map<AST::Scope*, Builder::Scope*> ScopeMap;
struct Builder::Private {
    ScopeMap map;
};

//
// Class Builder
//

Builder::Builder()
{
    m = new Private;
    AST::Name name;
    m_scope = m_global = new AST::Scope("", 0, "file", name);
    Scope* global = findScope(m_global);
    m_scopes.push(global);
    // Insert the global base types
    global->dict->insert(Base("char"));
    global->dict->insert(Base("bool"));
    global->dict->insert(Base("short"));
    global->dict->insert(Base("int"));
    global->dict->insert(Base("long"));
    global->dict->insert(Base("signed"));
    global->dict->insert(Base("unsigned"));
    global->dict->insert(Base("float"));
    global->dict->insert(Base("double"));
    global->dict->insert(Base("void"));
    global->dict->insert(Base("..."));
}

Builder::~Builder()
{
    // TODO Delete all ...
    delete m;
}

//. Finds or creates a cached Scope
Builder::Scope* Builder::findScope(AST::Scope* decl)
{
    ScopeMap::iterator iter = m->map.find(decl);
    if (iter == m->map.end()) {
	Scope* scope = new Scope(decl);
	m->map.insert(ScopeMap::value_type(decl, scope));
	return scope;
    }
    return iter->second;
}

void Builder::add(AST::Declaration* decl)
{
    // Add declaration
    m_scope->declarations().push_back(decl);
    // Add to name dictionary
    m_scopes.top()->dict->insert(decl);
}

AST::Namespace* Builder::startNamespace(string name)
{
    // Generate the name
    AST::Name ns_name = extend(m_scope->name(), name);
    // Create the Namespace
    AST::Namespace* ns = new AST::Namespace(m_filename, 0, "namespace", ns_name);
    add(ns);
    // Push stack. Search is this NS plus enclosing NS's search
    Scope* scope = findScope(ns);
    scope->search.insert(
	scope->search.end(),
	m_scopes.top()->search.begin(),
	m_scopes.top()->search.end()
    );
    m_scopes.push(scope);
    m_scope = ns;
    return ns;
}

void Builder::endNamespace()
{
    // Check if it is a namespace...
    m_scopes.pop();
    m_scope = m_scopes.top()->scope_decl;
}

AST::Class* Builder::startClass(string type, string name)
{
    // Generate the name
    AST::Name class_name = extend(m_scope->name(), name);
    // Create the Class
    AST::Class* ns = new AST::Class(m_filename, 0, type, class_name);
    add(ns);
    // Push stack. Search is this Class plus base Classes plus enclosing NS's search
    Scope* scope = findScope(ns);
    scope->search.insert(
	scope->search.end(),
	m_scopes.top()->search.begin(),
	m_scopes.top()->search.end()
    );
    m_scopes.push(scope);
    m_scope = ns;
    return ns;
}

void Builder::endClass()
{
    // Check if it is a class...
    m_scopes.pop();
    m_scope = m_scopes.top()->scope_decl;
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

//. Add a typedef
AST::Typedef* Builder::addTypedef(int line, string name, Type::Type* alias, bool constr)
{
    // Generate the name
    AST::Name scoped_name = extend(m_scope->name(), name);
    // Create the object
    AST::Typedef* tdef = new AST::Typedef(m_filename, line, "typedef", scoped_name, alias, constr);
    add(tdef);
    return tdef;
}



//
// Type Methods
//

// Public method to lookup a type
Type::Named* Builder::lookupType(string name)
{
    Type::Named* type = lookup(name);
    if (type) return type;
    // Not found, forward declare it
    cout << "Warning: Name "<<name<<" not found."<<endl;
    return Forward(name);
}

// Private method to lookup a type in the current scope
Type::Named* Builder::lookup(string name)
{
    const Scope::Search& search = m_scopes.top()->search;
    return lookup(name, search);
}

// Private method to lookup a type in the specified search space
Type::Named* Builder::lookup(string name, const Scope::Search& search)
{
    Scope::Search::const_iterator s_iter = search.begin();
    while (s_iter != search.end()) {
	Scope* scope = *s_iter++;
	// Check if dict has it
	if (!scope->dict->has_key(name))
	    continue;
	try { // Try to return it
	    return scope->dict->lookup(name);
	}
	catch (Dictionary::MultipleError) {
	    cout << "Warning: Found multiple declarations searching for "<<name<<endl;
	    return NULL;
	}
	catch (Dictionary::KeyError) {
	    // Continue to next search
	}
    }
    return NULL;
}

//. Public Qualified Type Lookup
Type::Named* Builder::lookupType(const vector<string>& names)
{
    Type::Named* type = NULL;
    vector<string>::const_iterator n_iter = names.begin();
    while (n_iter != names.end()) {
	string name = *n_iter++;
	Scope* scope;
	if (!type) {
	    if (!name.size()) {
		type = m_global->declared();
		continue;
	    } else {
		// Use current scope
		scope = m_scopes.top();
	    }
	} else {
	    // Find cached scope
	    Type::Declared* decl = dynamic_cast<Type::Declared*>(type);
	    if (!decl) { type = NULL; break; }
	    AST::Scope* scope_decl = dynamic_cast<AST::Scope*>(decl->declaration());
	    if (!scope_decl) { type = NULL; break; }
	    scope = findScope(scope_decl);
	}
	// Note the dynamic_cast may return NULL also!
	type = lookup(name, scope->search);
	if (!type)
	    // Abort lookup
	    break;
    }

    if (!type) {
	// Not found! Add Type.Forward of scoped name
	string name = names[0];
	for (n_iter = names.begin(); ++n_iter != names.end();)
	    name += "::" + *n_iter;
	return Forward(name);
    }
    return type;
}


Type::Forward* Builder::Forward(string name)
{
    // Generate the name
    AST::Name scope = m_scope->name();
    scope.push_back(name);
    return new Type::Forward(scope);
}

Type::Base* Builder::Base(string name)
{
    return new Type::Base(extend(m_scope->name(), name));
}


