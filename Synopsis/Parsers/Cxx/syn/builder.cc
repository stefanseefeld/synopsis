// File: builder.cc

#include <map>
#include <typeinfo>
#include <strstream>

#include "builder.hh"
#include "type.hh"
#include "dict.hh"

// for debugging
#include "dumper.hh"
#include "strace.hh"

//. Utility method
Name extend(const Name &name, const std::string &str)
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
    access = AST::Default;
}

Builder::Scope::~Scope()
{
    delete dict;
}

int Builder::Scope::getCount(const std::string& name)
{
    return ++nscounts[name];
}

namespace {
//     std::ostream& operator << (std::ostream& out, const Builder::Scope& scope)
//     {
// 	const AST::Name& name = scope.scope_decl->name();
// 	if (name.size())
// 	    copy(name.begin(), name.end(), ostream_iterator<string>(out, "::"));
// 	else
// 	    out << "(global)";
// 	return out;
//     }

    //. This class is very similar to ostream_iterator, except that it works on
    //. pointers to types
    template <typename T>
    class ostream_ptr_iterator {
	std::ostream* out;
	const char* sep;
    public:
	ostream_ptr_iterator(std::ostream& o, const char* s) : out(&o), sep(s) {}
	ostream_ptr_iterator<T>& operator =(const T* value) {
	    *out << *value;
	    if (sep) *out << sep;
	    return *this;
	}
	ostream_ptr_iterator<T>& operator *() { return *this; }
	ostream_ptr_iterator<T>& operator ++() { return *this; }
	ostream_ptr_iterator<T>& operator ++(int) { return *this; }
    };

    std::string join(const std::vector<std::string>& strs, const std::string &sep = " ")
    {
	std::vector<std::string>::const_iterator iter = strs.begin();
	if (iter == strs.end()) return "";
	std::string str = *iter++;
	while (iter != strs.end())
	    str += sep + *iter++;
	return str;
    }

}

//. Formats a vector<string> to the output, joining the strings with ::'s.
//. This operator is prototyped in builder.hh and can be used from other
//. modules
std::ostream& operator <<(std::ostream& out, const std::vector<std::string>& vec) {
    return out << join(vec, "::");
}

//
// Struct Builder::Private
//

struct Builder::Private {
  typedef std::map<AST::Scope*, Builder::Scope*> ScopeMap;
  ScopeMap map;
};

//
// Class Builder
//

Builder::Builder(const std::string &basename)
{
    m_basename = basename;
    m_unique = 1;
    m = new Private;
    AST::Name name;
    m_scope = m_global = new AST::Scope("", 0, "global", name);
    Scope* global = findScope(m_global);
    m_scope = m_global;
    m_scopes.push(global);
    // Insert the global base types
    Type::Base* t_bool;
    global->dict->insert(Base("char"));
    global->dict->insert(t_bool = Base("bool"));
    global->dict->insert(Base("short"));
    global->dict->insert(Base("int"));
    global->dict->insert(Base("long"));
    global->dict->insert(Base("signed"));
    global->dict->insert(Base("unsigned"));
    global->dict->insert(Base("float"));
    global->dict->insert(Base("double"));
    global->dict->insert(Base("void"));
    global->dict->insert(Base("..."));
    global->dict->insert(Base("long long"));
    global->dict->insert(Base("long double"));
    // Add variables for true and false
    name.clear(); name.push_back("true");
    add(new AST::Variable("", -1, "variable", name, t_bool, false));
    name.clear(); name.push_back("false");
    add(new AST::Variable("", -1, "variable", name, t_bool, false));
}

Builder::~Builder()
{
    // TODO Delete all ...
    delete m;
}

//. Finds or creates a cached Scope
Builder::Scope* Builder::findScope(AST::Scope* decl)
{
    Private::ScopeMap::iterator iter = m->map.find(decl);
    if (iter == m->map.end()) {
	Scope* scope = new Scope(decl);
	m->map.insert(Private::ScopeMap::value_type(decl, scope));
	return scope;
    }
    return iter->second;
}

void Builder::setAccess(AST::Access axs)
{
    m_scopes.top()->access = axs;
}

void Builder::setFilename(const std::string &filename)
{
    if (filename.substr(0, m_basename.size()) == m_basename)
	m_filename.assign(filename, m_basename.size(), std::string::npos);
    else
	m_filename = filename;
}

void Builder::add(AST::Declaration* decl)
{
    //cout << "adding decl " << decl->name().back() << endl;
    // Set access
    decl->setAccess(m_scopes.top()->access);
    // Add declaration
    m_scope->declarations().push_back(decl);
    // Add to name dictionary
    m_scopes.top()->dict->insert(decl);
}

void Builder::add(Type::Named* type)
{
    //cout << "adding type " << type->name().back() << endl;
    // Add to name dictionary
    m_scopes.top()->dict->insert(type);
}

AST::Namespace* Builder::startNamespace(const std::string &n, NamespaceType nstype)
{
    std::string name = n, type_str;
    AST::Namespace* ns = 0;
    bool generated = false;
    switch (nstype) {
	case NamespaceNamed:
	    type_str = "namespace";
	    // Check if namespace already exists
	    try {
		if (m_scopes.top()->dict->has_key(name)) {
		    Type::Named* type = m_scopes.top()->dict->lookup(name);
		    ns = Type::declared_cast<AST::Namespace>(type);
		}
	    }
	    catch (const Type::wrong_type_cast &) {}
	    break;
	case NamespaceAnon:
	    type_str = "module";
	    {	// name is the filename. Wrap it in {}'s
		// only keep after last /
		size_t slash_at = name.rfind('/');
		if (slash_at != std::string::npos)
		    name.erase(0, slash_at+1);
		name = "{"+name+"}";
	    }
	    break;
	case NamespaceUnique:
	    type_str = "local";
	    { // name is empty or the type. Encode it with a unique number
		if (!name.size()) name = "ns";
		std::ostrstream x; x << '`' << name;
		int count = m_scopes.top()->getCount(name);
		if (count > 1) x << count;
		x << std::ends; name = x.str();
	    }
	    break;
    }
    // Create a new namespace object unless we already found one
    if (!ns) {
	generated = true;
	// Generate the name
	AST::Name ns_name = extend(m_scope->name(), name);
	// Create the Namespace
	ns = new AST::Namespace(m_filename, 0, type_str, ns_name);
	add(ns);
    }
    // Create Builder::Scope object. Search is this NS plus enclosing NS's search
    Scope* scope = findScope(ns);
    // For anon namespaces, we insert the anon ns into the parent scope
    if (nstype == NamespaceAnon && generated) {
	m_scopes.top()->search.push_back(scope);
	//Scope::Search& search = m_scopes.top()->search;
	//cout << "Top's: "<<*m_scopes.top()<<endl;copy(search.begin(), search.end(), ostream_ptr_iterator<Scope>(cout, "\n "));
    }
    scope->search.insert(
	scope->search.end(),
	m_scopes.top()->search.begin(),
	m_scopes.top()->search.end()
    );
    //cout << "Search: ";copy(scope->search.begin(), scope->search.end(), ostream_ptr_iterator<Scope>(cout, ", ")); cout << endl;
    // Push stack
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

// Utility class to recursively add base classes to given search
void Builder::addClassBases(AST::Class* clas, Scope::Search& search)
{
    std::vector<AST::Inheritance*>::iterator inh_iter = clas->parents().begin();
    while (inh_iter != clas->parents().end()) {
	AST::Inheritance* inh = *inh_iter++;
	Type::Type* type = inh->parent();
	Type::Declared* declared;
	if ((declared = dynamic_cast<Type::Declared*>(type)) != NULL) {
	    AST::Class* base;
	    if ((base = dynamic_cast<AST::Class*>(declared->declaration())) != NULL) {
		// Found a base class, so look it up
		Scope* scope = findScope(base);
		search.push_back(scope);
		// Recursively add..
		addClassBases(base, search);
	    }
	}
	// TODO: handle typedefs and parameterized types
    }
}

void Builder::updateBaseSearch()
{
    Scope* scope = m_scopes.top();
    AST::Class* clas = dynamic_cast<AST::Class*>(scope->scope_decl);
    if (!clas) return;
    Scope::Search search = scope->search;
    Scope::Search::iterator iter = search.begin();
    scope->search.clear();
    // Add the scope itself
    scope->search.push_back(*iter++);
    // Add base classes
    addClassBases(clas, scope->search);
    // Add containing scopes, stored in search
    while (iter != search.end())
	scope->search.push_back(*iter++);
}

AST::Class* Builder::startClass(int lineno, const std::string &type, const std::string &name)
{
    // Generate the name
    AST::Name class_name = extend(m_scope->name(), name);
    // Create the Class
    AST::Class* ns = new AST::Class(m_filename, lineno, type, class_name);
    add(ns);
    // Push stack. Search is this Class plus base Classes plus enclosing NS's search
    Scope* scope = findScope(ns);
    scope->access = (type == "struct" ? AST::Public : AST::Private);
    scope->search.insert(
	scope->search.end(),
	m_scopes.top()->search.begin(),
	m_scopes.top()->search.end()
    );
    m_scopes.push(scope);
    m_scope = ns;
    return ns;
}

AST::Class* Builder::startClass(int lineno, const std::string &type, const std::vector<std::string>& names)
{
    // Find the forward declaration of this class
    Type::Unknown* unknown = dynamic_cast<Type::Unknown*>(lookupType(names));
    if (!unknown) {
	std::cerr << "Fatal: Qualified class name did not reference an unknown type." << std::endl; exit(1);
    }
    // Create the Class
    AST::Class* ns = new AST::Class(m_filename, lineno, type, unknown->name());
    // Add to container scope
    std::vector<std::string> scope_name = names;
    scope_name.pop_back();
    Type::Declared* scope_type = dynamic_cast<Type::Declared*>(lookupType(scope_name));
    if (!scope_type) {
	std::cerr << "Fatal: Qualified class name was not in a declaration." << std::endl; exit(1);
    }
    AST::Scope* scope = dynamic_cast<AST::Scope*>(scope_type->declaration());
    if (!scope) {
	std::cerr << "Fatal: Qualified class name was not in a scope." << std::endl; exit(1);
    }
    // Set access
    //decl->setAccess(m_scopes.top()->access);
    // Add declaration
    scope->declarations().push_back(ns);
    // Add to name dictionary
    Builder::Scope* scope_scope = findScope(scope);
    scope_scope->dict->insert(ns);
    // Push stack. Search is this Class plus enclosing scopes. bases added later
    Scope* ns_scope = findScope(ns);
    ns_scope->access = (type == "struct" ? AST::Public : AST::Private);
    ns_scope->search.insert(
	ns_scope->search.end(),
	scope_scope->search.begin(),
	scope_scope->search.end()
    );
    m_scopes.push(ns_scope);
    m_scope = ns;
    return ns;
}

void Builder::endClass()
{
    // Check if it is a class...
    m_scopes.pop();
    m_scope = m_scopes.top()->scope_decl;
}

//. Start function impl scope
void Builder::startFunctionImpl(const std::vector<std::string>& name)
{
    /*{ cout << "starting function with name: ";
	Type::Name::const_iterator niter = name.begin();
	cout << name.size() <<" "<< *niter++;
	while (niter != name.end())
	    cout << "::" << *niter++;
	cout << endl;
    }*/
    // Create the Namespace
    AST::Namespace* ns = new AST::Namespace(m_filename, 0, "function", name);
    Scope* ns_scope = findScope(ns);
    Builder::Scope* scope_scope;
    if (name.size() > 1) {
	// Find container scope
	std::vector<std::string> scope_name = name;
	scope_name.pop_back();
	scope_name.insert(scope_name.begin(), ""); // force lookup from global, not current scope, since name is fully qualified
	Type::Declared* scope_type = dynamic_cast<Type::Declared*>(lookupType(scope_name));
	if (!scope_type) { std::cerr << "Fatal: Qualified func name was not in a declaration." << std::endl; exit(1); }
	AST::Scope* scope = dynamic_cast<AST::Scope*>(scope_type->declaration());
	if (!scope) { std::cerr << "Fatal: Qualified func name was not in a scope." << std::endl; exit(1); }
	scope_scope = findScope(scope);
    } else {
	scope_scope = findScope(m_global);
    }
    // Add to name dictionary TODO: this is for debugging only!
    scope_scope->dict->insert(ns);
    // Push stack. Search is this Class plus enclosing scopes. bases added later
    ns_scope->search.insert(
	ns_scope->search.end(),
	scope_scope->search.begin(),
	scope_scope->search.end()
    );

    m_scopes.push(ns_scope);
    m_scope = ns;
}

//. End function impl scope
void Builder::endFunctionImpl()
{
    m_scopes.pop();
    m_scope = m_scopes.top()->scope_decl;
}

//. Add an operation
AST::Operation* Builder::addOperation(int line, const std::string &name, const std::vector<std::string> &premod, Type::Type* ret, const std::string &realname)
{
    // Generate the name
    AST::Name scope = m_scope->name();
    scope.push_back(name);
    AST::Operation* oper = new AST::Operation(m_filename, line, "method", scope, premod, ret, realname);
    add(oper);
    return oper;
}

//. Add a variable
AST::Variable* Builder::addVariable(int line, const std::string &name, Type::Type* vtype, bool constr, const std::string& type)
{
    // Generate the name
    AST::Name scope = m_scope->name();
    scope.push_back(name);
    AST::Variable* var = new AST::Variable(m_filename, line, type, scope, vtype, constr);
    add(var);
    return var;
}

void Builder::addThisVariable()
{
    // First find out if we are in a method
    AST::Scope* func_ns = m_scope;
    AST::Name name = func_ns->name();
    name.pop_back();
    name.insert(name.begin(), std::string());
    Type::Named* clas_named = lookupType(name, false);
    AST::Class* clas;
    try {
	clas = Type::declared_cast<AST::Class>(clas_named);
    } catch (const Type::wrong_type_cast &) {
	// Not in a method -- so dont add a 'this'
	return;
    }

    // clas is now the AST::Class of the enclosing class
    Type::Type::Mods pre, post;
    post.push_back("*");
    Type::Modifier* t_this = new Type::Modifier(clas->declared(), pre, post);
    addVariable(-1, "this", t_this, false, "this");
}

//. Add a typedef
AST::Typedef* Builder::addTypedef(int line, const std::string &name, Type::Type* alias, bool constr)
{
    // Generate the name
    AST::Name scoped_name = extend(m_scope->name(), name);
    // Create the object
    AST::Typedef* tdef = new AST::Typedef(m_filename, line, "typedef", scoped_name, alias, constr);
    add(tdef);
    return tdef;
}

//. Add an enumerator
AST::Enumerator* Builder::addEnumerator(int line, const std::string &name, const std::string &value)
{
    AST::Name scoped_name = extend(m_scope->name(), name);
    AST::Enumerator* enumor = new AST::Enumerator(m_filename, line, "enumerator", scoped_name, value);
    add(enumor->declared());
    return enumor;
}

//. Add an enum
AST::Enum* Builder::addEnum(int line, const std::string &name, const std::vector<AST::Enumerator*>& enumors)
{
    AST::Name scoped_name = extend(m_scope->name(), name);
    AST::Enum* theEnum = new AST::Enum(m_filename, line, "enum", scoped_name);
    theEnum->enumerators() = enumors;
    add(theEnum);
    return theEnum;
}

//. Add tail comment
AST::Declaration* Builder::addTailComment(int line)
{
    AST::Name name; name.push_back("dummy");
    AST::Declaration* decl = new AST::Declaration(m_filename, line, "dummy", name);
    add(decl);
    return decl;
}

//
// Type Methods
//

//. Predicate class that is true if the object passed to the constructor is a
//. type, as opposed to a modifier or a parametrized, etc
class isType : public Type::Visitor {
    bool m_value;
public:
    //. constructor. Visits the given type thereby setting the value
    isType(Type::Named* type) : m_value(false) { type->accept(this); }
    //. bool operator, returns the value determined by visitation during
    //. construction
    operator bool() { return m_value; }
    //. Okay
    void visitBase(Type::Base*) { m_value = true; }
    //. Okay
    void visitUnknown(Type::Unknown*) { m_value = true; }
    //. Okay if not a function declaration
    void visitDeclared(Type::Declared* type) {
	// Depends on what was declared: Everything but function is okay
	if (dynamic_cast<AST::Function*>(type->declaration()))
	    m_value = false;
	else
	    m_value = true;
    }
    //. Fallback: Not okay
    void visitType(Type::Type*) { m_value = false; }
};

// Public method to lookup a type
Type::Named* Builder::lookupType(const std::string &name)
{
    STrace trace("Builder::lookupType(name)");
    Type::Named* type = lookup(name);
    if (type) return type;
    // Not found, declare it unknown
    //cout << "Warning: Name "<<name<<" not found in "<<m_filename<<endl;
    return Unknown(name);
}

// Private method to lookup a type in the current scope
Type::Named* Builder::lookup(const std::string &name)
{
    STrace trace("Builder::lookup(name)");
    const Scope::Search& search = m_scopes.top()->search;
    return lookup(name, search);
}

//. Looks up the name in the scope of the given scope
Type::Named* Builder::lookupType(const std::string &name, AST::Scope* decl)
{
    STrace trace("Builder::lookupType(name,scope)");
    Private::ScopeMap::iterator iter = m->map.find(decl);
    if (iter == m->map.end()) return NULL;
    Builder::Scope* scope = iter->second;
    return lookup(name, scope->search);
}

class FunctionHeuristic {
    typedef std::vector<Type::Type*> v_Type;
    typedef v_Type::iterator vi_Type;
    typedef std::vector<AST::Parameter*> v_Param;
    typedef v_Param::iterator vi_Param;

    v_Type m_args;
    int cost;
public:
    //. Constructor - takes arguments to match functions against
    FunctionHeuristic(const v_Type& v) : m_args(v) {}

    //. Heuristic operator, returns 'cost' of given function - higher is
    //. worse, 1000 means no match
    int operator ()(AST::Function* func) {
	cost = 0;
	int num_args = m_args.size();
	v_Param* params = &func->parameters();
	bool func_ellipsis = hasEllipsis(params);
	int num_params = params->size() - func_ellipsis;
	int num_default = countDefault(params);

	// Check number of params first
	if (!func_ellipsis && num_args > num_params) return 1000;
	if (num_args < num_params - num_default) return 1000;
	
	// Now calc cost of each argument in turn
	int max_arg = num_args > num_params ? num_params : num_args;
	for (int index = 0; index < max_arg; index++)
	    calcCost(m_args[index], (*params)[index]->type());

#ifdef DEBUG
	std::cout << "--Cost is " << cost << std::endl;
#endif
	return cost;
    }

private:
    //. Find an ellipsis as the last arg
    bool hasEllipsis(v_Param* params) {
	if (params->size() == 0) return false;
	Type::Type* back = params->back()->type();
	if (Type::Base* base = dynamic_cast<Type::Base*>(back))
	    if (base->name().size() == 1 && base->name().front() == "...")
		return true;
	return false;
    }

    //. Returns the number of parameters with default values. Counts from the
    //. back and stops when it finds one without a default.
    int countDefault(v_Param* params) {
	v_Param::reverse_iterator iter = params->rbegin(), end = params->rend();
	int count = 0;
	while (iter != end) {
	    AST::Parameter* param = *iter++;
	    if (!param->value().size()) break;
	    count++;
	}
	return count;
    }

    //. Calculate the cost of converting 'type' into 'param_type'. The cost is
    //. accumulated on the 'cost' member variable.
    void calcCost(Type::Type* type, Type::Type* param_type) {
	TypeFormatter tf;
#ifdef DEBUG
	std::cout << tf.format(type) <<","<<tf.format(param_type) << std::endl;
#endif
	if (type != param_type) cost += 1;
    }
};

//. Looks up the function in the given scope with the given args. 
AST::Function* Builder::lookupFunc(const std::string &name, AST::Scope* decl, const std::vector<AST::Parameter*>& params)
{
    STrace trace("Builder::lookupFunc");
    TypeFormatter tf;
    // Change params to param_types
    std::vector<Type::Type*> param_types;
    for (std::vector<AST::Parameter*>::const_iterator iter = params.begin(); iter != params.end(); iter++)
	param_types.push_back((*iter)->type());
    // First find Builder::Scope object
    Private::ScopeMap::iterator iter = m->map.find(decl);
    if (iter == m->map.end()) return NULL;
    const Scope::Search& search = iter->second->search;
    Scope::Search::const_iterator s_iter = search.begin();
    while (s_iter != search.end()) {
	Scope* scope = *s_iter++;
	// Check if dict has any names that match
	if (!scope->dict->has_key(name))
	    continue;

	typedef std::vector<Type::Named*> v_Named;
	typedef v_Named::iterator vi_Named;
	typedef std::vector<AST::Function*> v_Function;
	typedef v_Function::iterator vi_Function;

	// Get the matching names from the dictionary
	v_Named types = scope->dict->lookupMultiple(name);
	
	// Put only the AST::Functions into 'functions'
	v_Function functions;
	for (vi_Named iter = types.begin(); iter != types.end();)
	    try {
		functions.push_back( Type::declared_cast<AST::Function>(*iter++) );
	    } catch (const Type::wrong_type_cast &) { throw ERROR("looked up func '"<<name<<"'wasnt a func!"); }
	
	// If no looked up names were functions, program is ill-formed (?)
	if (!functions.size()) return NULL;

	// Find best function using a heuristic
	FunctionHeuristic heuristic(param_types);
	vi_Function iter = functions.begin(), end = functions.end();
	AST::Function* best_func = *iter++;
	int best = heuristic(best_func);
	while (iter != end) {
	    AST::Function* func = *iter++;
	    int cost = heuristic(func);
	    if (cost < best) {
		best = cost;
		best_func = func;
	    }
	}
	if (best < 1000) return best_func;
	throw ERROR("No appropriate function found.");
    }
    throw ERROR("No matching functions found.");
}

// Private method to lookup a type in the specified search space
Type::Named* Builder::lookup(const std::string &name, const Scope::Search& search, bool func_okay) throw ()
{
    STrace trace("Builder::lookup(name,search,func_okay)");
    Scope::Search::const_iterator s_iter = search.begin();
    while (s_iter != search.end()) {
	Scope* scope = *s_iter++;
	
	// Check if dict has it
	if (!scope->dict->has_key(name))
	    continue;
	try { // Try to return it
	    Type::Named* named = scope->dict->lookup(name);
	    if (func_okay || isType(named)) {
		//cout << "Builder::lookup(): Found " << named->name() << endl;
		return named;
	    }
	}
	catch (Dictionary::MultipleError e) {
	    // This is only an error if they are all not types
	    if (!func_okay && !isType(e.types.front())) continue; // TODO: eh? check all?
	    return NULL;
	}
	catch (Dictionary::KeyError) {
	    // Continue to next search
	}
    }
    return NULL;
}

//. Public Qualified Type Lookup
Type::Named* Builder::lookupType(const std::vector<std::string>& names, bool func_okay)
{
    STrace trace("Builder::lookupType(type::name,search,func_okay)");
    Type::Named* type = NULL;
    std::vector<std::string>::const_iterator n_iter = names.begin();
    while (n_iter != names.end()) {
	std::string name = *n_iter++;
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
	    try {
		// Find cached scope from 'type'
		scope = findScope( Type::declared_cast<AST::Scope>(type) );
	    } catch (const Type::wrong_type_cast &) {
		// Abort lookup
		throw ERROR("qualified lookup found a scope that wasn't a scope finding: " << names);
	    }
	}
	// Note the dynamic_cast may return NULL also!
	type = lookup(name, scope->search, func_okay && n_iter == names.end());
	if (!type)
	    // Abort lookup
	    break;
    }

    if (!type) {
	// Not found! Add Type.Unknown of scoped name
	std::string name = names[0];
	for (n_iter = names.begin(); ++n_iter != names.end();)
	    name += "::" + *n_iter;
	return Unknown(name);
    }
    return type;
}

Type::Named* Builder::resolveType(Type::Named* type)
{
    STrace trace("Builder::resolveType(named)");
    try {
	Type::Name& name = type->name();
	
	Type::Name::iterator iter = name.begin(), end = name.end() - 1;
	AST::Scope* scope = m_global;
	while (iter < end) {
	    // Find *iter in scope
	    Type::Named* scope_type = findScope(scope)->dict->lookup(*iter++);
	    scope = Type::declared_cast<AST::Scope>(scope_type);
	}
	LOG("Looking up "<<(*iter)<<" in "<<((scope==m_global)?"global":scope->name().back()));
	// Scope is now the containing scope of the type we are checking
	return findScope(scope)->dict->lookup(*iter);
    }
    catch (const Type::wrong_type_cast &) {
	LOG("resolveType failed! bad cast.");
    }
    catch (Dictionary::KeyError e) {
	LOG("resolveType failed! key error: '"<<e.name<<"'");
    }
    catch (Dictionary::MultipleError e) {
	LOG("resolveType failed! multiple:" << e.types.size());
	std::vector<Type::Named*>::iterator iter = e.types.begin();
	while (iter != e.types.end()) {
	    LOG(" +" << (*iter)->name());
	    iter++;
	}
    }
    catch (...) {
	// There shouldn't be any errors, but just in case...
	throw ERROR("resolveType failed with unknown error!");
    }
    return type;
}

Type::Unknown* Builder::Unknown(const std::string &name)
{
    // Generate the name
    AST::Name scope = m_scope->name();
    scope.push_back(name);
    return new Type::Unknown(scope);
}

Type::Unknown* Builder::addUnknown(const std::string &name)
{
    if (m_scopes.top()->dict->has_key(name))
	return 0;
    Type::Unknown* unknown = Unknown(name);
    add(unknown);
    return 0;
}

Type::Base* Builder::Base(const std::string &name)
{
    return new Type::Base(extend(m_scope->name(), name));
}


