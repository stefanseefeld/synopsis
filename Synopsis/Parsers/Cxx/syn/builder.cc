// File: builder.cc

#include <map>
#include <typeinfo>
#include <sstream>
#include <algorithm>

#include "builder.hh"
#include "type.hh"
#include "dict.hh"

// for debugging
#include "dumper.hh"
#include "strace.hh"

//. Useful typedefs
typedef std::vector<AST::Function*> v_Function;
typedef v_Function::const_iterator vi_Function;

typedef std::vector<Type::Named*> v_Named;
typedef v_Named::iterator vi_Named;


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
    : is_using(false)
{
    scope_decl = s;
    search.push_back(this);
    dict = new Dictionary();
    access = AST::Default;
}

Builder::Scope::Scope(Scope* s)
    : is_using(true)
{
    scope_decl = s->scope_decl;
    dict = s->dict;
}

Builder::Scope::~Scope()
{
    if (is_using == false)
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
    m_scope = m_global = new AST::Namespace("", 0, "global", name);
    Scope* global = findScope(m_global);
    m_scope = m_global;
    m_scopes.push(global);
    // Insert the global base types
    Type::Base* t_bool, *t_null;
    global->dict->insert(Base("char"));
    global->dict->insert(t_bool = Base("bool"));
    global->dict->insert(Base("short"));
    global->dict->insert(Base("int"));
    global->dict->insert(Base("long"));
    global->dict->insert(Base("unsigned"));
    global->dict->insert(Base("unsigned long"));
    global->dict->insert(Base("float"));
    global->dict->insert(Base("double"));
    global->dict->insert(Base("void"));
    global->dict->insert(Base("..."));
    global->dict->insert(Base("long long"));
    global->dict->insert(Base("long double"));
    global->dict->insert(t_null = Base("__null_t"));
    // Add variables for true and false
    name.clear(); name.push_back("true");
    add(new AST::Variable("", -1, "variable", name, t_bool, false));
    name.clear(); name.push_back("false");
    add(new AST::Variable("", -1, "variable", name, t_bool, false));
    // Add a variable for null pointer types (g++ #defines NULL to __null)
    name.clear(); name.push_back("__null");
    add(new AST::Variable("", -1, "variable", name, t_null, false));
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
		std::ostringstream x; x << '`' << name;
		int count = m_scopes.top()->getCount(name);
		if (count > 1) x << count;
		name = x.str();
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
    if (generated) {
	// Add current scope's search to new search
	scope->search.insert(
	    scope->search.end(),
	    m_scopes.top()->search.begin(),
	    m_scopes.top()->search.end()
	);
    }
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

class TypeInfo : public Type::Visitor {
public:
    Type::Type* type;
    bool is_const;
    bool is_volatile;
    bool is_null;
    size_t deref;

    //. Constructor
    TypeInfo(Type::Type* t) {
	type = t; is_const = is_volatile = is_null = false; deref = 0;
	set(t);
    }
    //. Set to the given type
    void set(Type::Type* t) {
	type = t;
	t->accept(this);
    }
    //. Base -- null is flagged since it is special
    void visitBase(Type::Base* base) {
	if (base->name().back() == "__null_t")
	    is_null = true;
    }
    //. Modifiers -- recurse on the alias type
    void visitModifier(Type::Modifier* mod) {
	Type::Type::Mods::const_iterator iter;
	// Check for const
	for (iter = mod->pre().begin(); iter != mod->pre().end(); iter++)
	    if (*iter == "const")
		is_const = true;
	    else if (*iter == "volatile")
		is_volatile = true;
	// Check for derefs
	for (iter = mod->post().begin(); iter != mod->post().end(); iter++)
	    if (*iter == "*")
		deref++;
	    else if (*iter == "[]")
		deref++;
	
	set(mod->alias());
    }
    //. Declared -- check for typedef
    void visitDeclared(Type::Declared* t) {
	try {
	    AST::Typedef* tdef = Type::declared_cast<AST::Typedef>(t);
	    // Recurse on typedef alias
	    set(tdef->alias());
	} catch (const Type::wrong_type_cast&) {
	    // Ignore -- just means not a typedef
	}
    }
};

//. Output operator for debugging
std::ostream& operator << (std::ostream& o, TypeInfo& i) {
    TypeFormatter tf;
    o << "[" << tf.format(i.type);
    if (i.is_const) o << " (const)";
    if (i.is_volatile) o << " (volatile)";
    if (i.deref) o << " " << i.deref << "*";
    o << "]";
    return o;
}

class FunctionHeuristic {
    typedef std::vector<Type::Type*> v_Type;
    typedef v_Type::iterator vi_Type;
    typedef std::vector<AST::Parameter*> v_Param;
    typedef v_Param::iterator vi_Param;

    v_Type m_args;
    int cost;
#ifdef DEBUG
    STrace trace;
#endif
public:
    //. Constructor - takes arguments to match functions against
    FunctionHeuristic(const v_Type& v) : m_args(v)
#ifdef DEBUG
	    , trace("FunctionHeuristic")
    {
	TypeFormatter tf;
	std::ostringstream buf;
	for (size_t index = 0; index < v.size(); index++) {
	    if (index) buf << ", ";
	    buf << tf.format(v[index]);
	}
	//buf << std::ends;
	LOG("Function arguments: " << buf.str());
#else
    {
#endif
    }

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
	if (!func_ellipsis && num_args > num_params) cost = 1000;
	if (num_args < num_params - num_default) cost = 1000;
	
	if (cost < 1000) {
	    // Now calc cost of each argument in turn
	    int max_arg = num_args > num_params ? num_params : num_args;
	    for (int index = 0; index < max_arg; index++)
		calcCost(m_args[index], (*params)[index]->type());
	}

#ifdef DEBUG
	LOG("Function: " << func->name() << " -- Cost is " << cost);
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

    //. Calculate the cost of converting 'arg' into 'param'. The cost is
    //. accumulated on the 'cost' member variable.
    void calcCost(Type::Type* arg_t, Type::Type* param_t) {
	TypeFormatter tf;
	if (!arg_t) return;
	TypeInfo arg(arg_t), param(param_t);
#ifdef DEBUG
	// std::cout << arg << param << std::endl;
	// // std::cout << tf.format(type) <<","<<tf.format(param_type) << std::endl;
#endif
	// Null types convert to any ptr with no cost
	if (arg.is_null && param.deref > 0) return;
	// Different types is bad
	if (arg.type != param.type) cost += 10;
	// Different * levels is also bad
	if (arg.deref != param.deref) cost += 10;
	// Worse constness is bad
	if (arg.is_const > param.is_const) cost += 5;
    }
};

//. Looks up the function in the given scope with the given args. 
AST::Function* Builder::lookupFunc(const std::string &name, AST::Scope* decl, const std::vector<Type::Type*>& args)
{
    STrace trace("Builder::lookupFunc");
    TypeFormatter tf;
    // Now loop over the search scopes
    const Scope::Search& search = findScope(decl)->search;
    Scope::Search::const_iterator s_iter = search.begin();
    typedef std::vector<AST::Function*> v_Function;
    v_Function functions;

    // Loop over precalculated search list
    while (s_iter != search.end()) {
	Scope* scope = *s_iter++;
	
	// Check if dict has it
	if (scope->dict->has_key(name)) {
	    findFunctions(name, scope, functions);
	}
	// If not a dummy scope, resolve the set
	if (scope->is_using == false && !functions.empty()) {
	    // Return best function (or throw error)
	    int cost;
	    AST::Function* func = bestFunction(functions, args, cost);
	    if (cost < 1000) return func;
	    throw ERROR("No appropriate function found.");
	}
    }
    
    throw ERROR("No matching functions found.");
}

// Operator lookup
AST::Function* Builder::lookupOperator(const std::string& oper, Type::Type* left_type, Type::Type* right_type)
{
    STrace trace("Builder::lookupOperator("+oper+",left,right)");
    // Find some info about the two types
    TypeInfo left(left_type), right(right_type);
    bool left_user = !!dynamic_cast<Type::Declared*>(left_type) && !left.deref;
    bool right_user = !!dynamic_cast<Type::Declared*>(right_type) && !right.deref;

    // First check if the types are user-def or enum
    if (!left_user && !right_user) return NULL;

    std::vector<AST::Function*> functions;
    std::vector<Type::Type*> args;
    AST::Function* best_method = NULL, *best_func = NULL;
    int best_method_cost, best_func_cost;

    // Member methods of left_type
    try {
	AST::Class* clas = Type::declared_cast<AST::Class>(left.type);
	// Construct the argument list
	args.push_back(right_type);

	try {
	    findFunctions(oper, findScope(clas), functions);

	    best_method = bestFunction(functions, args, best_method_cost);
	} catch (const Dictionary::KeyError&) {
	    best_method = NULL;
	}

	// Clear functions and args for use below
	functions.clear();
	args.clear();
    } catch (const Type::wrong_type_cast&) { /* ignore: not a class */ }

    // Non-member functions
    // Loop over the search scopes
    const Scope::Search& search = m_scopes.top()->search;
    Scope::Search::const_iterator s_iter = search.begin();
    while (s_iter != search.end()) {
	Scope* scope = *s_iter++;
	// Check if dict has any names that match
	if (!scope->dict->has_key(oper))
	    continue;

	// Get the matching functions from the dictionary
	findFunctions(oper, scope, functions);

	// Scope rules say once you find any results: stop
	break;
    }

    // Koenig Rule: add operators from namespaces of arguments
    // void findKoenigFunctions(oper, functions, args);
    if (left_user) try {
	AST::Name enclosing_name = Type::type_cast<Type::Named>(left.type)->name();
	enclosing_name.pop_back();
	if (enclosing_name.size()) {
	    Scope* scope = findScope( Type::declared_cast<AST::Scope>(lookupType(enclosing_name, false, m_global)) );
	    findFunctions(oper, scope, functions);
	}
    } catch (const Type::wrong_type_cast& e) {}

    if (right_user) try {
	AST::Name enclosing_name = Type::type_cast<Type::Named>(right.type)->name();
	enclosing_name.pop_back();
	if (enclosing_name.size()) {
	    Scope* scope = findScope( Type::declared_cast<AST::Scope>(lookupType(enclosing_name, false, m_global)) );
	    findFunctions(oper, scope, functions);
	}
    } catch (const Type::wrong_type_cast& e) {}

    // Add builtin operators to aide in best-function resolution
    // NYI
    
    // Puts left and right types into args
    args.push_back(left_type);
    args.push_back(right_type);
    // Find best non-method function
    best_func = bestFunction(functions, args, best_func_cost);
    
    // Return best method or function
    if (best_method) {
	if (best_func) {
	    if (best_func_cost < best_method_cost)
		return best_func;
	    else
		return best_method;
	} else {
	    return best_method;
	}
    } else {
	if (best_func)
	    return best_func;
	else
	    return NULL;
    }
}

void Builder::findFunctions(const std::string& name, Scope* scope, std::vector<AST::Function*>& functions)
{
    STrace trace("Builder::findFunctions");

    // Get the matching names from the dictionary
    try {
	v_Named types = scope->dict->lookupMultiple(name);
	
	// Put only the AST::Functions into 'functions'
	for (vi_Named iter = types.begin(); iter != types.end();)
	    try {
		functions.push_back( Type::declared_cast<AST::Function>(*iter++) );
	    } catch (const Type::wrong_type_cast &) { throw ERROR("looked up func '"<<name<<"'wasnt a func!"); }
    } catch (Dictionary::KeyError) {}
}

AST::Function* Builder::bestFunction(const std::vector<AST::Function*>& functions, const std::vector<Type::Type*>& args, int& cost)
{
    // Quick sanity check
    if (!functions.size()) return NULL;
    // Find best function using a heuristic
    FunctionHeuristic heuristic(args);
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
    cost = best;
    return best_func;
}

// Private method to lookup a type in the specified search space
Type::Named* Builder::lookup(const std::string &name, const Scope::Search& search, bool func_okay) throw ()
{
    STrace trace("Builder::lookup(name,search,func_okay)");
    Scope::Search::const_iterator s_iter = search.begin();
    Dictionary::Types results;
    while (s_iter != search.end()) {
	Scope* scope = *s_iter++;
	
	// Check if dict has it
	if (scope->dict->has_key(name)) {
	    if (results.empty())
		results = scope->dict->lookupMultiple(name);
	    else {
		Dictionary::Types temp_result = scope->dict->lookupMultiple(name);
		std::copy(temp_result.begin(), temp_result.end(),
		    std::back_insert_iterator<Dictionary::Types>(results));
	    }
	}
	// If not a dummy scope, resolve the set
	if (scope->is_using == false && !results.empty()) {
	    if (results.size() == 1 && (isType(results[0]) || func_okay)) {
		// Exactly one match! return it
		return results[0];
	    }
	    // Store in class var?
	    LOG("Multiple candidates!");
	    for (Dictionary::Types::iterator iter = results.begin(); 
		    iter != results.end(); iter++)
		LOG(" - " << (*iter)->name());
	    return NULL;
	}
    }
    return NULL;
}

class InheritanceAdder {
    std::list<AST::Class*>& open_list;
public:
    InheritanceAdder(std::list<AST::Class*>& l) : open_list(l) {}
    InheritanceAdder(const InheritanceAdder& i) : open_list(i.open_list) {}
    void operator() (AST::Inheritance* i) {
	try {
	    AST::Class* parent = Type::declared_cast<AST::Class>(i->parent());
	    open_list.push_back(parent);
	}
	catch (const Type::wrong_type_cast&) {
	    // ?? ignore for now
	}
    }
};

//. Private Qualified type lookup
Type::Named* Builder::lookupQual(const std::string& name, const Scope* scope, bool func_okay = false)
{
    STrace trace("Builder::lookupQual");
    //LOG("name: " << name << " in: " << scope->scope_decl->name());
    // First determine: class or namespace
    if (AST::Class* the_class = dynamic_cast<AST::Class*>(scope->scope_decl)) {
	// A class: search recursively, in order, through base classes
	// FIXME: read up about overriding, hiding, virtual bases and funcs,
	// etc
	std::list<AST::Class*> open_list;
	open_list.push_back(the_class);
	while (!open_list.empty()) {
	    AST::Class* clas = open_list.front();
	    open_list.pop_front();
	    Scope* scope = findScope(clas);
	    if (scope->dict->has_key(name)) {
		try {
		    Type::Named* named = scope->dict->lookup(name);
		    if (func_okay || isType(named)) {
			return named;
		    }
		    // Else it's a function and a type was wanted: keep looking
		}
		catch (Dictionary::MultipleError e) {
		    // FIXME: check for duplicates etc etc
		}
	    }
	    // Add base classes to open list
	    std::for_each(clas->parents().begin(), clas->parents().end(),
		    InheritanceAdder(open_list));
	}
    } else if (dynamic_cast<AST::Namespace*>(scope->scope_decl)) {
	// A namespace: search recursively through using declarations
	// constructing a conflict set - dont traverse using declarations of
	// any namespace which has an entry for 'name'. Each NS only once
	std::list<const Scope*> open, closed;
	open.push_back(scope);
	std::vector<Type::Named*> results;
	while (!open.empty()) {
	    const Scope* ns = open.front();
	    open.pop_front();
	    // Check if 'ns' is on closed list
	    if (std::find(closed.begin(), closed.end(), ns) != closed.end())
		continue;
	    // Add to closed list
	    closed.push_back(ns);
	    // Check if 'ns' has 'name'
	    if (ns->dict->has_key(name)) {
		// Add all results to results list
		if (results.empty()) results = ns->dict->lookupMultiple(name);
		else {
		    std::vector<Type::Named*> temp = ns->dict->lookupMultiple(name);
		    std::copy(temp.begin(), temp.end(),
			    back_inserter(results));
		}
	    } else {
		// Add 'using' Scopes to open list
		std::copy(ns->using_scopes.begin(), ns->using_scopes.end(),
			back_inserter(open));
	    }
	}
	// Now we have a set of results
	if (!results.size()) {
	    LOG("No results!");
	    return NULL;
	}
	// FIXME: figure out what to do about multiple
	Type::Named* best = NULL;
	int best_score = -1;
	for (std::vector<Type::Named*>::iterator iter = results.begin();
		iter != results.end(); iter++) {
	    // Fixme.. create a Score visitor
	    int score = 0;
	    Type::Named* type = *iter;
	    if (Type::Declared* declared = dynamic_cast<Type::Declared*>(type)) {
		score++;
		if (AST::Declaration* decl = declared->declaration()) {
		    score++;
		    if (dynamic_cast<AST::Forward*>(decl))
			score--;
		}
	    }
	    if (score > best_score) {
		best_score = score;
		best = type;
	    }
	}
	    
	return best;
    }
    // Not class or NS - which is illegal for a qualified (but coulda been
    // template etc?:)
    LOG("Not class or namespace");
    return NULL;
}

//. Public Qualified Type Lookup
Type::Named* Builder::lookupType(const std::vector<std::string>& names, bool func_okay, AST::Scope* start_scope)
{
    STrace trace("Builder::lookupType(vector names,search,func_okay)");
    //LOG("looking up '" << names << "' in " << (start_scope?start_scope->name() : m_scope->name()));
    Type::Named* type = NULL;
    Scope* scope = NULL;
    std::vector<std::string>::const_iterator n_iter = names.begin();
    // Setup the initial scope
    std::string name = *n_iter;
    if (!name.size()) {
	// Qualified name starts with :: so always start at global scope
	type = m_global->declared();
    } else { 
	// Lookup first name as usual
	if (start_scope) type = lookupType(name, start_scope);
	else type = lookupType(name);
    }
    ++n_iter;
    
    // Look over every identifier in the qualified name
    while (n_iter != names.end()) {
	name = *n_iter++;
	try {
	    // FIXME: this should use some sort of visitor
	    AST::Declaration* decl = Type::declared_cast<AST::Declaration>(type);
	    if (AST::Typedef* tdef = dynamic_cast<AST::Typedef*>(decl)) {
		type = Type::type_cast<Type::Named>(tdef->alias());
	    }
	    // Find cached scope from 'type'
	    scope = findScope( Type::declared_cast<AST::Scope>(type) );
	} catch (const Type::wrong_type_cast &) {
	    // Abort lookup
	    throw ERROR("qualified lookup found a type (" << type->name() << ") that wasn't a scope finding: " << names);
	}
	// Find the named type in the current scope
	type = lookupQual(name, scope, func_okay && n_iter == names.end());
	if (!type)
	    // Abort lookup
	    break;
    }

    if (!type) {
	LOG("Not found -> creating Unknown");
	// Not found! Add Type.Unknown of scoped name
	std::string name = names[0];
	for (n_iter = names.begin(); ++n_iter != names.end();)
	    name += "::" + *n_iter;
	return Unknown(name);
    }
    return type;
}

//. Maps a scoped name into a vector of scopes and the final type. Returns
//. true on success.
bool Builder::mapName(const AST::Name& names, std::vector<AST::Scope*>& o_scopes, Type::Named*& o_type)
{
    STrace trace("Builder::mapName");
    AST::Scope* ast_scope = m_global;
    AST::Name::const_iterator iter = names.begin();
    AST::Name::const_iterator last = names.end(); last--;
    AST::Name scoped_name;

    // Start scope name at global level
    scoped_name.push_back("");

    // Sanity check
    if (iter == names.end()) return false;

    // Loop through all containing scopes
    while (iter != last) {
	//const std::string& name = *iter++;
	scoped_name.push_back(*iter++);
	Type::Named* type = lookupType(scoped_name);
	if (!type) { LOG("Warning: failed to lookup " << scoped_name << " in global scope"); return false; }
	try { ast_scope = Type::declared_cast<AST::Scope>(type); }
	catch (const Type::wrong_type_cast&) { LOG("Warning: looked up scope wasnt a scope!" << scoped_name); return false; }
	o_scopes.push_back(ast_scope);
    }

    // iter now == last, which can be any type
    scoped_name.push_back(*iter);
    Type::Named* type = lookupType(scoped_name, true);
    if (!type) {
	//findScope(ast_scope)->dict->dump();
	LOG("\nWarning: final type lookup wasn't found!" << *iter); return false;
    }

    o_type = type;
    return true;
}

Type::Type* Builder::arrayOperator(Type::Type* object, Type::Type* arg, AST::Function*& func_oper)
{
    STrace trace("Builder::arrayOperator");
    func_oper = NULL;
    // First decide if should use derefence or methods
    TypeInfo info(object);
    if (info.deref) {
	// object is of pointer type, so just deref it
	// Check for typedef
	try { object = Type::declared_cast<AST::Typedef>(object)->alias(); }
	catch (const Type::wrong_type_cast&) { /* ignore -- not a typedef */ }
	// Check for modifier
	if (Type::Modifier* mod = dynamic_cast<Type::Modifier*>(object)) {
	    typedef Type::Type::Mods Mods;
	    Type::Modifier* newmod = new Type::Modifier(mod->alias(), mod->pre(), mod->post());
	    for (Mods::iterator iter = newmod->post().begin(); iter != newmod->post().end(); iter++) {
		if (*iter == "*" || *iter == "[]") {
		    newmod->post().erase(iter);
		    return newmod;
		}
	    }
	    delete newmod;
	    throw ERROR("Unable to dereference type for array operator!");
	}
	throw ERROR("Unknown type for array operator!");
    }

    // Hmm object type -- try for [] method
    AST::Class* clas;
    try { clas = Type::declared_cast<AST::Class>(info.type); }
    catch (const Type::wrong_type_cast&) {
	TypeFormatter tf;
	throw ERROR("Not deref and not class type for array operator! " << tf.format(info.type) << " <-- " << tf.format(object));
    }

    v_Function functions;
    try { findFunctions("[]", findScope(clas), functions); }
    catch (const Dictionary::KeyError&) { throw ERROR("No array operator for class " << clas->name()); }
    
    // Make args list
    std::vector<Type::Type*> args;
    args.push_back(arg);

    // Find best function
    int cost;
    AST::Function* func = bestFunction(functions, args, cost);
    if (func && cost < 1000) {
	func_oper = func;
	return func->returnType();
    }
    throw ERROR("No best function found for array operator.");
}

Type::Named* Builder::resolveType(Type::Named* type)
{
    STrace trace("Builder::resolveType(named)");
    try {
	Type::Name& name = type->name();
	LOG("Resolving " << name);
	
	Type::Name::iterator iter = name.begin(), end = name.end() - 1;
	AST::Scope* scope = m_global;
	while (iter != end) {
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

std::string Builder::dumpSearch(Scope* scope)
{
    Scope::Search& search = scope->search;
    std::ostringstream buf;
    buf << "Search for ";
    if (scope->scope_decl->name().size() == 0) buf << "global";
    else buf << m_scope->name();
    buf << " is now: ";
    Scope::Search::iterator iter = search.begin();
    while (iter != search.end()) {
	buf << (iter==search.begin() ? "" : ", ");
	const AST::Name& name = (*iter)->scope_decl->name();
	if (name.size()) 
	    if ( (*iter)->is_using ) buf << "(" << name << ")";
	    else buf << name;
	else buf << "global";
	++iter;
    }
    //buf << std::ends;
    return buf.str();
}

class EqualScope {
    AST::Scope* target;
public:
    EqualScope(Builder::Scope* t) { target = t->scope_decl; }
    bool operator() (Builder::Scope* s) {
	return s->scope_decl == target;
    }
};

void Builder::addUsingNamespace(Scope* target, Scope* scope)
{
    STrace trace("Builder::addUsingNamespace");

    // Check if 'scope' alreayd has 'target' in its using list
    Scope::Search& already = scope->using_scopes;
    if (std::find_if(already.begin(), already.end(), EqualScope(target))
	!= already.end())
	// Already using
	return;
    // Else add it
    scope->using_scopes.push_back(target);
    target->used_by.push_back(scope);

    AST::Name& target_name = target->scope_decl->name();

    // Find where to insert 'scope' into top()'s search list
    // "closest enclosing namespace that contains both using directive and
    //  target namespace"
    Scope::Search& search = scope->search;
    LOG(dumpSearch(scope));
    Scope::Search::iterator iter = search.end();
    // Skip global scope.. cant check something with no name
    --iter;
    while (iter != search.begin()) {
	// Move to next scope to check
	--iter;
	AST::Name& search_name = (*iter)->scope_decl->name();
	if (target_name.size() < search_name.size())
	    // Search is more nested than the target
	    break;
	if (search_name.size() < 1)
	    // Global NS..
	    continue;
	if (target_name[search_name.size()-1] != search_name.back())
	    // Different scope path taken
	    break;
    }
    // Move back to last which was common, so we can insert before it
    if (*iter != search.back() && iter != search.begin())
	iter++;
    
    Scope* new_scope = new Scope(target);
    search.insert(iter, new_scope);

    LOG(dumpSearch(scope));

    // Add target to all used_by scopes
    Scope::Search used_by_copy = scope->used_by;
    iter = used_by_copy.begin();
    while (iter != used_by_copy.end())
	addUsingNamespace(target, *iter++);
}

// Add a namespace using declaration.
void Builder::usingNamespace(Type::Named* type)
{
    STrace trace("Builder::usingNamespace");
    AST::Scope* ast_scope = Type::declared_cast<AST::Scope>(type);
    Scope* target = findScope(ast_scope);
    addUsingNamespace(target, m_scopes.top());
}


// Add a namespace alias using declaration.
void Builder::usingNamespace(Type::Named* type, const std::string& alias)
{
    STrace trace("Builder::usingNamespace");

    // Retrieve the 'Namespace' it points to
    AST::Namespace* ns = Type::declared_cast<AST::Namespace>(type);

    // Create a new declared type with a different name
    AST::Name new_name = m_scope->name();
    new_name.push_back(alias);
    Type::Declared* declared = new Type::Declared(new_name, ns);

    // Add to current scope
    add(declared);
}

// Add a using declaration.
void Builder::usingDeclaration(Type::Named* type)
{
    // Add it to the current scope
    add(type);
}

