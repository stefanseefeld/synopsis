// vim: set ts=8 sts=2 sw=2 et:
// File: builder.cc

#include <map>
#include <typeinfo>
#include <sstream>
#include <algorithm>

#include "builder.hh"
#include "type.hh"
#include "dict.hh"
#include "swalker.hh"
#include "scopeinfo.hh"
#include "lookup.hh"
#include "typeinfo.hh"

// for debugging
#include "dumper.hh"
#include "strace.hh"

//. Simplify names. Typically only used for accessing vectors and iterators
using namespace Types;
using namespace AST;

//. Utility method
ScopedName extend(const ScopedName& name, const std::string& str)
{
  ScopedName ret = name;
  ret.push_back(str);
  return ret;
}


/////// FIXME: wtf is this doing here??
namespace
{
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
}

//. Formats a vector<string> to the output, joining the strings with ::'s.
//. This operator is prototyped in builder.hh and can be used from other
//. modules
std::ostream& operator <<(std::ostream& out, const ScopedName& vec)
{
  return out << join(vec, "::");
}

//
// Struct Builder::Private
//

struct Builder::Private
{
  typedef std::map<AST::Scope*, ScopeInfo*> ScopeMap;
  ScopeMap map;

  //. A map of name references
  typedef std::map<ScopedName, std::vector<AST::Reference> > RefMap;
  RefMap refs;
};

//
// Class Builder
//

Builder::Builder(const std::string& basename)
{
  m_basename = basename;
  m_unique = 1;
  m = new Private;
  ScopedName name;
  m_scope = m_global = new AST::Namespace("", 0, "global", name);
  ScopeInfo* global = find_scope(m_global);
  m_scopes.push_back(global);
  // Insert the global base types
  Types::Base* t_bool, *t_null;
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

  // Create the Lookup helper
  m_lookup = new Lookup(this);
}

Builder::~Builder()
{
  // TODO Delete all ...
  delete m;
}

//. Finds or creates a cached Scope
ScopeInfo* Builder::find_scope(AST::Scope* decl)
{
  Private::ScopeMap::iterator iter = m->map.find(decl);
  if (iter == m->map.end())
    {
      ScopeInfo* scope = new ScopeInfo(decl);
      m->map.insert(Private::ScopeMap::value_type(decl, scope));
      return scope;
    }
  return iter->second;
}

// Returns the scope 'depth' deep. 0 means current scope, 1 means parent, etc
AST::Scope* Builder::scope(size_t depth)
{
  if (depth >= m_scopes.size())
    return global();
  return m_scopes[m_scopes.size() - 1 - depth]->scope_decl;
}

void Builder::set_access(AST::Access axs)
{
  m_scopes.back()->access = axs;
}

void Builder::set_filename(const std::string& filename)
{
  if (filename.substr(0, m_basename.size()) == m_basename)
    m_filename.assign(filename, m_basename.size(), std::string::npos);
  else
    m_filename = filename;
}

void Builder::add(AST::Declaration* decl)
{
  // Set access
  decl->set_access(m_scopes.back()->access);
  // Add declaration
  m_scope->declarations().push_back(decl);
  // Add to name dictionary
  m_scopes.back()->dict->insert(decl);
}

void Builder::add(Types::Named* type)
{
  // Add to name dictionary
  m_scopes.back()->dict->insert(type);
}

AST::Namespace* Builder::startNamespace(const std::string& n, NamespaceType nstype)
{
  std::string name = n, type_str;
  AST::Namespace* ns = 0;
  bool generated = false;
  switch (nstype)
    {
    case NamespaceNamed:
      type_str = "namespace";
      // Check if namespace already exists
      try
        {
          if (m_scopes.back()->dict->has_key(name))
            {
              Types::Named* type = m_scopes.back()->dict->lookup(name);
              ns = Types::declared_cast<AST::Namespace>(type);
            }
        }
      catch (const Types::wrong_type_cast&)
        { }
      break;
    case NamespaceAnon:
      type_str = "module";
      { // name is the filename. Wrap it in {}'s. only keep after last /
        size_t slash_at = name.rfind('/');
        if (slash_at != std::string::npos)
          name.erase(0, slash_at + 1);
        name = "{" + name + "}";
      }
      break;
    case NamespaceUnique:
      type_str = "local";
      { // name is empty or the type. Encode it with a unique number
        if (!name.size()) name = "ns";
        std::ostringstream buf; buf << '`' << name;
        int count = m_scopes.back()->getCount(name);
        if (count > 1) buf << count;
        name = buf.str();
      }
      break;
    }
  // Create a new namespace object unless we already found one
  if (!ns)
    {
      generated = true;
      // Generate the name
      ScopedName ns_name = extend(m_scope->name(), name);
      // Create the Namespace
      ns = new AST::Namespace(m_filename, 0, type_str, ns_name);
      add(ns);
    }
  // Create ScopeInfo object. Search is this NS plus enclosing NS's search
  ScopeInfo* scope = find_scope(ns);
  ScopeInfo* current = m_scopes.back();
  if (nstype == NamespaceAnon && generated)
    // For anon namespaces, we insert the anon ns into the parent search
    current->search.push_back(scope);
  if (generated)
    {
      // Add current scope's search to new search
      /*scope->search.insert(
          scope->search.end(),
          m_scopes.back()->search.begin(),
          m_scopes.back()->search.end()
      );*/
      std::copy(current->search.begin(), current->search.end(),
                std::back_inserter(scope->search));
    }
  //cout << "Search: ";copy(scope->search.begin(), scope->search.end(), ostream_ptr_iterator<Scope>(cout, ", ")); cout << endl;

  storeReference(ns, m_filename, m_swalker->current_lineno(), 0, "definition");

  // Push stack
  m_scopes.push_back(scope);
  m_scope = ns;
  return ns;
}

void Builder::endNamespace()
{
  // Check if it is a namespace...
  m_scopes.pop_back();
  m_scope = m_scopes.back()->scope_decl;
}

// Utility class to recursively add base classes to given search
void Builder::addClassBases(AST::Class* clas, ScopeSearch& search)
{
    std::vector<AST::Inheritance*>::iterator inh_iter = clas->parents().begin();
    while (inh_iter != clas->parents().end()) {
        AST::Inheritance* inh = *inh_iter++;
        Types::Type* type = inh->parent();
        Types::Declared* declared;
        if ((declared = dynamic_cast<Types::Declared*>(type)) != NULL) {
            AST::Class* base;
            if ((base = dynamic_cast<AST::Class*>(declared->declaration())) != NULL) {
                // Found a base class, so look it up
                ScopeInfo* scope = find_scope(base);
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
    ScopeInfo* scope = m_scopes.back();
    AST::Class* clas = dynamic_cast<AST::Class*>(scope->scope_decl);
    if (!clas) return;
    ScopeSearch search = scope->search;
    ScopeSearch::iterator iter = search.begin();
    scope->search.clear();
    // Add the scope itself
    scope->search.push_back(*iter++);
    // Add base classes
    addClassBases(clas, scope->search);
    // Add containing scopes, stored in search
    while (iter != search.end())
        scope->search.push_back(*iter++);
}

AST::Class* Builder::startClass(int lineno, const std::string& type, const std::string& name)
{
    // Generate the name
    ScopedName class_name = extend(m_scope->name(), name);
    // Create the Class
    AST::Class* ns = new AST::Class(m_filename, lineno, type, class_name);
    add(ns);
    // Push stack. Search is this Class plus base Classes plus enclosing NS's search
    ScopeInfo* scope = find_scope(ns);
    scope->access = (type == "struct" ? AST::Public : AST::Private);
    scope->search.insert(
        scope->search.end(),
        m_scopes.back()->search.begin(),
        m_scopes.back()->search.end()
    );
    storeReference(ns, m_filename, m_swalker->current_lineno(), 0, "definition");
    m_scopes.push_back(scope);
    m_scope = ns;
    return ns;
}

AST::Class* Builder::startClass(int lineno, const std::string& type, const std::vector<std::string>& names)
{
    // Find the forward declaration of this class
    Types::Unknown* unknown = dynamic_cast<Types::Unknown*>(m_lookup->lookupType(names));
    if (!unknown) {
        std::cerr << "Fatal: Qualified class name did not reference an unknown type." << std::endl; exit(1);
    }
    // Create the Class
    AST::Class* ns = new AST::Class(m_filename, lineno, type, unknown->name());
    // Add to container scope
    std::vector<std::string> scope_name = names;
    scope_name.pop_back();
    Types::Declared* scope_type = dynamic_cast<Types::Declared*>(m_lookup->lookupType(scope_name));
    if (!scope_type) {
        std::cerr << "Fatal: Qualified class name was not in a declaration." << std::endl; exit(1);
    }
    AST::Scope* scope = dynamic_cast<AST::Scope*>(scope_type->declaration());
    if (!scope) {
        std::cerr << "Fatal: Qualified class name was not in a scope." << std::endl; exit(1);
    }
    // Set access
    //decl->setAccess(m_scopes.back()->access);
    // Add declaration
    scope->declarations().push_back(ns);
    // Add to name dictionary
    ScopeInfo* scope_scope = find_scope(scope);
    scope_scope->dict->insert(ns);
    // Push stack. Search is this Class plus enclosing scopes. bases added later
    ScopeInfo* ns_scope = find_scope(ns);
    ns_scope->access = (type == "struct" ? AST::Public : AST::Private);
    ns_scope->search.insert(
        ns_scope->search.end(),
        scope_scope->search.begin(),
        scope_scope->search.end()
    );
    storeReference(ns, m_filename, m_swalker->current_lineno(), 0, "definition");
    m_scopes.push_back(ns_scope);
    m_scope = ns;
    return ns;
}

void Builder::endClass()
{
    // Check if it is a class...
    m_scopes.pop_back();
    m_scope = m_scopes.back()->scope_decl;
}

//. Start function impl scope
void Builder::startFunctionImpl(const std::vector<std::string>& name)
{
    /*{ cout << "starting function with name: ";
        Types::Name::const_iterator niter = name.begin();
        cout << name.size() <<" "<< *niter++;
        while (niter != name.end())
            cout << "::" << *niter++;
        cout << endl;
    }*/
    // Create the Namespace
    AST::Namespace* ns = new AST::Namespace(m_filename, 0, "function", name);
    ScopeInfo* ns_scope = find_scope(ns);
    ScopeInfo* scope_scope;
    if (name.size() > 1) {
        // Find container scope
        std::vector<std::string> scope_name = name;
        scope_name.pop_back();
        scope_name.insert(scope_name.begin(), ""); // force lookup from global, not current scope, since name is fully qualified
        Types::Declared* scope_type = dynamic_cast<Types::Declared*>(m_lookup->lookupType(scope_name));
        if (!scope_type) { std::cerr << "Fatal: Qualified func name was not in a declaration." << std::endl; exit(1); }
        AST::Scope* scope = dynamic_cast<AST::Scope*>(scope_type->declaration());
        if (!scope) { std::cerr << "Fatal: Qualified func name was not in a scope." << std::endl; exit(1); }
        scope_scope = find_scope(scope);
    } else {
        scope_scope = find_scope(m_global);
    }
    // Add to name dictionary TODO: this is for debugging only!
    scope_scope->dict->insert(ns);
    // Push stack. Search is this Class plus enclosing scopes. bases added later
    ns_scope->search.insert(
        ns_scope->search.end(),
        scope_scope->search.begin(),
        scope_scope->search.end()
    );

    storeReference(ns, m_filename, m_swalker->current_lineno(), 0, "implementation");
    m_scopes.push_back(ns_scope);
    m_scope = ns;
}

//. End function impl scope
void Builder::endFunctionImpl()
{
    m_scopes.pop_back();
    m_scope = m_scopes.back()->scope_decl;
}

//. Add an operation
AST::Operation* Builder::addOperation(int line, const std::string& name, const std::vector<std::string>& premod, Types::Type* ret, const std::string& realname)
{
    // Generate the name
    ScopedName scope = m_scope->name();
    scope.push_back(name);
    AST::Operation* oper = new AST::Operation(m_filename, line, "method", scope, premod, ret, realname);
    add(oper);
    storeReference(oper, m_filename, line, 0, "declaration");
    return oper;
}

//. Add a variable
AST::Variable* Builder::addVariable(int line, const std::string& name, Types::Type* vtype, bool constr, const std::string& type)
{
    // Generate the name
    ScopedName scope = m_scope->name();
    scope.push_back(name);
    AST::Variable* var = new AST::Variable(m_filename, line, type, scope, vtype, constr);
    add(var);
    storeReference(var, m_filename, line, 0, "declaration");
    return var;
}

void Builder::addThisVariable()
{
    // First find out if we are in a method
    AST::Scope* func_ns = m_scope;
    ScopedName name = func_ns->name();
    name.pop_back();
    name.insert(name.begin(), std::string());
    Types::Named* clas_named = m_lookup->lookupType(name, false);
    AST::Class* clas;
    try {
        clas = Types::declared_cast<AST::Class>(clas_named);
    } catch (const Types::wrong_type_cast& ) {
        // Not in a method -- so dont add a 'this'
        return;
    }

    // clas is now the AST::Class of the enclosing class
    Types::Type::Mods pre, post;
    post.push_back("*");
    Types::Modifier* t_this = new Types::Modifier(clas->declared(), pre, post);
    addVariable(-1, "this", t_this, false, "this");
}

//. Add a typedef
AST::Typedef* Builder::addTypedef(int line, const std::string& name, Types::Type* alias, bool constr)
{
    // Generate the name
    ScopedName scoped_name = extend(m_scope->name(), name);
    // Create the object
    AST::Typedef* tdef = new AST::Typedef(m_filename, line, "typedef", scoped_name, alias, constr);
    add(tdef);
    storeReference(tdef, m_filename, line, 0, "definition");
    return tdef;
}

//. Add an enumerator
AST::Enumerator* Builder::addEnumerator(int line, const std::string& name, const std::string& value)
{
    ScopedName scoped_name = extend(m_scope->name(), name);
    AST::Enumerator* enumor = new AST::Enumerator(m_filename, line, "enumerator", scoped_name, value);
    add(enumor->declared());
    storeReference(enumor, m_filename, line, 0, "definition");
    return enumor;
}

//. Add an enum
AST::Enum* Builder::addEnum(int line, const std::string& name, const std::vector<AST::Enumerator*>& enumors)
{
    ScopedName scoped_name = extend(m_scope->name(), name);
    AST::Enum* theEnum = new AST::Enum(m_filename, line, "enum", scoped_name);
    theEnum->enumerators() = enumors;
    add(theEnum);
    storeReference(theEnum, m_filename, line, 0, "definition");
    return theEnum;
}

//. Add tail comment
AST::Declaration* Builder::addTailComment(int line)
{
    ScopedName name; name.push_back("dummy");
    AST::Declaration* decl = new AST::Declaration(m_filename, line, "dummy", name);
    add(decl);
    return decl;
}

class InheritanceAdder {
    std::list<AST::Class*>& open_list;
public:
    InheritanceAdder(std::list<AST::Class*>& l) : open_list(l) {}
    InheritanceAdder(const InheritanceAdder& i) : open_list(i.open_list) {}
    void operator() (AST::Inheritance* i) {
        try {
            AST::Class* parent = Types::declared_cast<AST::Class>(i->parent());
            open_list.push_back(parent);
        }
        catch (const Types::wrong_type_cast&) {
            // ?? ignore for now
        }
    }
};


//. Maps a scoped name into a vector of scopes and the final type. Returns
//. true on success.
bool Builder::mapName(const ScopedName& names, std::vector<AST::Scope*>& o_scopes, Types::Named*& o_type)
{
    STrace trace("Builder::mapName");
    AST::Scope* ast_scope = m_global;
    ScopedName::const_iterator iter = names.begin();
    ScopedName::const_iterator last = names.end(); last--;
    ScopedName scoped_name;

    // Start scope name at global level
    scoped_name.push_back("");

    // Sanity check
    if (iter == names.end()) return false;

    // Loop through all containing scopes
    while (iter != last) {
        //const std::string& name = *iter++;
        scoped_name.push_back(*iter++);
        Types::Named* type = m_lookup->lookupType(scoped_name);
        if (!type) { LOG("Warning: failed to lookup " << scoped_name << " in global scope"); return false; }
        try { ast_scope = Types::declared_cast<AST::Scope>(type); }
        catch (const Types::wrong_type_cast&) { LOG("Warning: looked up scope wasnt a scope!" << scoped_name); return false; }
        o_scopes.push_back(ast_scope);
    }

    // iter now == last, which can be any type
    scoped_name.push_back(*iter);
    Types::Named* type = m_lookup->lookupType(scoped_name, true);
    if (!type) {
        //find_scope(ast_scope)->dict->dump();
        LOG("\nWarning: final type lookup wasn't found!" << *iter); return false;
    }

    o_type = type;
    return true;
}


Types::Unknown* Builder::Unknown(const std::string& name)
{
    // Generate the name
    ScopedName scope = m_scope->name();
    scope.push_back(name);
    Types::Unknown* unknown = new Types::Unknown(scope);
    return unknown;
}

Types::Unknown* Builder::addUnknown(const std::string& name)
{
    if (m_scopes.back()->dict->has_key(name))
        return 0;
    Types::Unknown* unknown = Unknown(name);
    add(unknown);
    return 0;
}

Types::Base* Builder::Base(const std::string& name)
{
    return new Types::Base(extend(m_scope->name(), name));
}

std::string Builder::dumpSearch(ScopeInfo* scope)
{
    ScopeSearch& search = scope->search;
    std::ostringstream buf;
    buf << "Search for ";
    if (scope->scope_decl->name().size() == 0) buf << "global";
    else buf << m_scope->name();
    buf << " is now: ";
    ScopeSearch::iterator iter = search.begin();
    while (iter != search.end()) {
        buf << (iter==search.begin() ? "" : ", ");
        const ScopedName& name = (*iter)->scope_decl->name();
        if (name.size()) 
            if ( (*iter)->is_using ) buf << "(" << name << ")";
            else buf << name;
        else buf << "global";
        ++iter;
    }
    //buf << std::ends;
    return buf.str();
}

class Builder::EqualScope {
    AST::Scope* target;
public:
    EqualScope(ScopeInfo* t) { target = t->scope_decl; }
    bool operator() (ScopeInfo* s) {
        return s->scope_decl == target;
    }
};

void Builder::addUsingNamespace(ScopeInfo* target, ScopeInfo* scope)
{
    STrace trace("Builder::addUsingNamespace");

    // Check if 'scope' alreayd has 'target' in its using list
    ScopeSearch& already = scope->using_scopes;
    if (std::find_if(already.begin(), already.end(), EqualScope(target))
        != already.end())
        // Already using
        return;
    // Else add it
    scope->using_scopes.push_back(target);
    target->used_by.push_back(scope);

    ScopedName& target_name = target->scope_decl->name();

    // Find where to insert 'scope' into top()'s search list
    // "closest enclosing namespace that contains both using directive and
    //  target namespace"
    ScopeSearch& search = scope->search;
    LOG(dumpSearch(scope));
    ScopeSearch::iterator iter = search.end();
    // Skip global scope.. cant check something with no name
    --iter;
    while (iter != search.begin()) {
        // Move to next scope to check
        --iter;
        ScopedName& search_name = (*iter)->scope_decl->name();
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
    
    // Create a dummy ScopeInfo which is just an alias to the original. This
    // is needed to cumulate using namespaces in the lookups
    ScopeInfo* new_scope = new ScopeInfo(target);
    search.insert(iter, new_scope);

    LOG(dumpSearch(scope));

    // Add target to all used_by scopes
    ScopeSearch used_by_copy = scope->used_by;
    iter = used_by_copy.begin();
    while (iter != used_by_copy.end())
        addUsingNamespace(target, *iter++);
}

// Add a namespace using declaration.
void Builder::usingNamespace(Types::Named* type)
{
    STrace trace("Builder::usingNamespace");
    AST::Scope* ast_scope = Types::declared_cast<AST::Scope>(type);
    ScopeInfo* target = find_scope(ast_scope);
    addUsingNamespace(target, m_scopes.back());
    storeReference(ast_scope, m_filename, m_swalker->current_lineno(), 0, "using directive");
}


// Add a namespace alias using declaration.
void Builder::usingNamespace(Types::Named* type, const std::string& alias)
{
    STrace trace("Builder::usingNamespace");

    // Retrieve the 'Namespace' it points to
    AST::Namespace* ns = Types::declared_cast<AST::Namespace>(type);

    // Create a new declared type with a different name
    ScopedName new_name = m_scope->name();
    new_name.push_back(alias);
    Types::Declared* declared = new Types::Declared(new_name, ns);

    // Add to current scope
    add(declared);

    storeReference(ns, m_filename, m_swalker->current_lineno(), 0, "using directive");
}

// Add a using declaration.
void Builder::usingDeclaration(Types::Named* type)
{
    // Add it to the current scope
    add(type);

    if (Types::Declared* declared = dynamic_cast<Types::Declared*>(type))
      storeReference(declared->declaration(), m_filename, m_swalker->current_lineno(), 0, "using declaration");
}


// Add a reference to the given decl to the reference DB
void Builder::storeReference(AST::Declaration* decl, const std::string& file, int line, int depth, const std::string& context)
{
#ifdef DEBUG
  // TODO: merge with LinkStorer into combined output format (with a real
  // output format too, :)
  AST::Scope* container = scope(depth);
  m->refs[decl->name()].push_back(AST::Reference(file, line, container->name(), context));
  std::cout << decl->name() << " - " << file << ":" << line;
  std::cout << " - " << container->name() << " - " << context << std::endl;
#endif
}
