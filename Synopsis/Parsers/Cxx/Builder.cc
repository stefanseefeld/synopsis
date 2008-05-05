//
// Copyright (C) 2002 Stephen Davies
// Copyright (C) 2002 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <map>
#include <typeinfo>
#include <sstream>
#include <algorithm>

#include "Builder.hh"
#include "Types.hh"
#include "Dictionary.hh"
#include "Walker.hh"
#include "ScopeInfo.hh"
#include "Lookup.hh"
#include "TypeInfo.hh"
#include "STrace.hh"

//. Simplify names. Typically only used for accessing vectors and iterators
using namespace Types;
using namespace ASG;

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
class ostream_ptr_iterator
{
    std::ostream* out;
    const char* sep;
public:
    ostream_ptr_iterator(std::ostream& o, const char* s) : out(&o), sep(s)
    {}
    ostream_ptr_iterator<T>& operator =(const T* value)
    {
        *out << *value;
        if (sep)
            *out << sep;
        return *this;
    }
    ostream_ptr_iterator<T>& operator *()
    {
        return *this;
    }
    ostream_ptr_iterator<T>& operator ++()
    {
        return *this;
    }
    ostream_ptr_iterator<T>& operator ++(int)
    {
        return *this;
    }
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
    typedef std::map<ASG::Scope*, ScopeInfo*> ScopeMap;
    ScopeMap map;

    //. A map of name references
    typedef std::map<ScopedName, std::vector<ASG::Reference> > RefMap;
    RefMap refs;

    //. A list of builtin declarations
    Declaration::vector builtin_decls;
};

//
// Class Builder
//

Builder::Builder(ASG::SourceFile* file)
{
  // m_file was originally initialized to 0. Why ??
    m_file = file;
    m_unique = 1;
    m = new Private;
    ScopedName name;
    m_scope = m_global = new ASG::Namespace(file, 0, "global", name);
    ScopeInfo* global = find_info(m_global);
    m_scopes.push_back(global);
    // Insert the global base types
    Types::Base* t_bool, *t_null;
    ASG::Declaration* decl;
    global->dict->insert(create_base("char"));
    global->dict->insert(t_bool = create_base("bool"));
    global->dict->insert(create_base("short"));
    global->dict->insert(create_base("int"));
    global->dict->insert(create_base("long"));
    global->dict->insert(create_base("unsigned"));
    global->dict->insert(create_base("unsigned long"));
    global->dict->insert(create_base("float"));
    global->dict->insert(create_base("double"));
    global->dict->insert(create_base("void"));
    global->dict->insert(create_base("..."));
    global->dict->insert(create_base("long long"));
    global->dict->insert(create_base("long double"));
    global->dict->insert(t_null = create_base("__null_t"));
    // Add a variables for true
    name.clear();
    name.push_back("true");
    decl = new ASG::Const(file, -1, "const", name, t_bool, "true");
    global->dict->insert(decl);
    m->builtin_decls.push_back(decl);
    // Add a variable for false
    name.clear();
    name.push_back("false");
    decl = new ASG::Const(file, -1, "const", name, t_bool, "false");
    global->dict->insert(decl);
    m->builtin_decls.push_back(decl);
    // Add a variable for null pointer types (g++ #defines NULL to __null)
    name.clear();
    name.push_back("__null");
    decl = new ASG::Variable(file, -1, "variable", name, t_null, false);
    global->dict->insert(decl);
    m->builtin_decls.push_back(decl);

    // Create the Lookup helper
    m_lookup = new Lookup(this);
}

Builder::~Builder()
{
    // TODO Delete all ...
    delete m_lookup;
    delete m;
}

//. Finds or creates a cached Scope
ScopeInfo* Builder::find_info(ASG::Scope* decl)
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

void Builder::set_access(ASG::Access axs)
{
    m_scopes.back()->access = axs;
}

void Builder::set_file(ASG::SourceFile* file)
{
    m_file = file;
}

const Declaration::vector& Builder::builtin_decls() const
{
    return m->builtin_decls;
}

//
// ASG Methods
//

void Builder::add(ASG::Declaration* decl, bool is_template)
{
    ScopeInfo* scopeinfo;
    ASG::Declaration::vector* decls;
    if (is_template)
    {
        scopeinfo = m_scopes[m_scopes.size()-2];
        decls = &scopeinfo->scope_decl->declarations();
    }
    else
    {
        scopeinfo = m_scopes.back();
        decls = &m_scope->declarations();
    }
    // Set access
    decl->set_access(scopeinfo->access);
    // Add to name dictionary
    scopeinfo->dict->insert(decl);

    const std::string& scope_type = scopeinfo->scope_decl->type();
    if (scope_type != "local" && scope_type != "function")
      decls->push_back(decl);
    decl->file()->declarations().push_back(decl);
}

void Builder::add(Types::Named* type)
{
    // Add to name dictionary
    m_scopes.back()->dict->insert(type);
}

void Builder::add_macros(const std::vector<ASG::Macro*>& macros)
{
    std::vector<ASG::Macro*>::const_iterator iter;
    for (iter = macros.begin(); iter != macros.end(); iter++)
	m_global->declarations().push_back(*iter);

}

ASG::Namespace* Builder::start_namespace(const std::string& n, NamespaceType nstype)
{
    std::string name = n, type_str;
    ASG::Namespace* ns = 0;
    bool generated = false;
    // Decide what to do based on the given namespace type
    switch (nstype)
    {
    case NamespaceNamed:
      type_str = "namespace";
      { // Check if namespace already exists
        Dictionary* dict = scopeinfo()->dict;
        if (dict->has_key(name))
          try
          {
            Types::Named::vector types = dict->lookupMultiple(name);
            ns = Types::declared_cast<ASG::Namespace>(*types.begin());
          }
          catch (const Types::wrong_type_cast&) {}
        }
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
            if (!name.size())
                name = "ns";
            int count = m_scopes.back()->getCount(name);
            std::ostringstream buf;
            buf << '`' << name;
            if (count > 1)
                buf << count;
            name = buf.str();
        }
        break;
    case NamespaceTemplate:
        type_str = "template";
        { // name is empty or the type. Encode it with a unique number
            if (!name.size())
                name = "template";
            int count = m_scopes.back()->getCount(name);
            std::ostringstream buf;
            buf << '`' << name << count;
            name = buf.str();
        }
        break;
    }
    // Create a new namespace object unless we already found one
    if (ns == 0)
    {
        generated = true;
        // Create the namspace
        if (nstype == NamespaceTemplate)
            ns = new ASG::Namespace(m_file, 0, type_str, m_scope->name());
        else
        {
            // Generate a nested name
            ScopedName ns_name = extend(m_scope->name(), name);
            ns = new ASG::Namespace(m_file, 0, type_str, ns_name);
            add(ns);
        }
    }
    // Create ScopeInfo object. Search is this NS plus enclosing NS's search
    ScopeInfo* ns_info = find_info(ns);
    ScopeInfo* current = m_scopes.back();
    // For anon namespaces, we insert the anon ns into the parent search
    if (nstype == NamespaceAnon && generated)
        current->search.push_back(ns_info);
    // Initialize search to same as parent scope if we generated a new NS
    if (generated)
        std::copy(current->search.begin(), current->search.end(),
                  std::back_inserter(ns_info->search));

    // Push stack
    m_scopes.push_back(ns_info);
    m_scope = ns;
    return ns;
}

void Builder::end_namespace()
{
    // TODO: Check if it is a namespace...
    m_scopes.pop_back();
    m_scope = m_scopes.back()->scope_decl;
}

// Utility class to recursively add base classes to given search
void Builder::add_class_bases(ASG::Class* clas, ScopeSearch& search)
{
    ASG::Inheritance::vector::iterator inh_iter = clas->parents().begin();
    while (inh_iter != clas->parents().end())
    {
        ASG::Inheritance* inh = *inh_iter++;
        Types::Type* type = inh->parent();
        try
        {
            ASG::Class* base = Types::declared_cast<ASG::Class>(type);
            // Found a base class, so look it up
            ScopeInfo* scope = find_info(base);
            search.push_back(scope);
            // Recursively add..
            add_class_bases(base, search);
        }
        catch (const Types::wrong_type_cast&)
        { /* TODO: handle typedefs and parameterized types */
        }
    }
}

void Builder::update_class_base_search()
{
    ScopeInfo* scope = m_scopes.back();
    ASG::Class* clas = dynamic_cast<ASG::Class*>(scope->scope_decl);
    if (!clas)
        return;
    ScopeSearch search = scope->search;
    ScopeSearch::iterator iter = search.begin();
    scope->search.clear();
    // Add the scope itself
    scope->search.push_back(*iter++);
    // Add base classes
    add_class_bases(clas, scope->search);
    // Add containing scopes, stored in search
    while (iter != search.end())
        scope->search.push_back(*iter++);
}

ASG::Class* Builder::start_class(int lineno, const std::string& type, const std::string& name,
                                 ASG::Parameter::vector* templ_params, std::string const &primary_name)
{
    // Generate the name
    ASG::Class* class_ = 0;
    bool is_template = templ_params && templ_params->size();
    bool is_specialization = *name.rbegin() == '>';
    if (is_template)
    {
      ScopedName class_name = extend(m_scopes[m_scopes.size()-2]->scope_decl->name(), name);
      ASG::ClassTemplate *ct = new ASG::ClassTemplate(m_file, lineno, type, class_name, is_specialization);
      Types::Template* templ = new Types::Template(class_name, ct, *templ_params);
      ct->set_template_id(templ);
      add(ct, true);
      class_ = ct;
    }
    else
    {
      ScopedName class_name = extend(m_scope->name(), name);
      class_ = new ASG::Class(m_file, lineno, type, class_name, is_specialization);
      add(class_);
    }
    // Push stack. Search is this Class plus base Classes plus enclosing NS's search
    ScopeInfo* class_info = find_info(class_);
    class_info->access = (type == "struct" ? ASG::Public : ASG::Private);
    std::copy(scopeinfo()->search.begin(), scopeinfo()->search.end(),
              std::back_inserter(class_info->search));
    m_scopes.push_back(class_info);
    m_scope = class_;

    return class_;
}

// Declaration of a previously forward declared class (ie: must find and
// replace previous forward declaration in the appropriate place)
ASG::Class* Builder::start_class(int lineno, const std::string& type, const ScopedName& names)
{
    // Find the forward declaration of this class
    Types::Named* named = m_lookup->lookupType(names);
    // Must be either unknown or declared->forward
    Types::Unknown* unknown = dynamic_cast<Types::Unknown*>(named);
    if (!unknown)
    {
        Types::Declared* declared = dynamic_cast<Types::Declared*>(named);
        if (!declared)
        {
            std::cerr << "Fatal: Qualified class name did not reference a declared type." << std::endl;
            exit(1);
        }
        ASG::Forward* forward = dynamic_cast<ASG::Forward*>(declared->declaration());
        if (!forward)
        {
            std::cerr << "Fatal: Qualified class name did not reference a forward declaration." << std::endl;
            exit(1);
        }
    }
    // Create the Class
    ASG::Class* ns = new ASG::Class(m_file, lineno, type, named->name(), false);
    // Add to container scope
    ScopedName scope_name = names;
    scope_name.pop_back();
    Types::Declared* scope_type = dynamic_cast<Types::Declared*>(m_lookup->lookupType(scope_name));
    if (!scope_type)
    {
        std::cerr << "Fatal: Qualified class name was not in a declaration." << std::endl;
        exit(1);
    }
    ASG::Scope* scope = dynamic_cast<ASG::Scope*>(scope_type->declaration());
    if (!scope)
    {
        std::cerr << "Fatal: Qualified class name was not in a scope." << std::endl;
        exit(1);
    }
    // Set access
    //decl->setAccess(m_scopes.back()->access);
    // Add declaration
    scope->declarations().push_back(ns);
    // Add to name dictionary
    ScopeInfo* scope_info = find_info(scope);
    scope_info->dict->insert(ns);
    // Push stack. Search is this Class plus enclosing scopes. bases added later
    ScopeInfo* ns_info = find_info(ns);
    ns_info->access = (type == "struct" ? ASG::Public : ASG::Private);
    std::copy(scope_info->search.begin(), scope_info->search.end(),
              std::back_inserter(ns_info->search));
    m_scopes.push_back(ns_info);
    m_scope = ns;
    return ns;
}

void Builder::end_class()
{
    // Check if it is a class...
    m_scopes.pop_back();
    m_scope = m_scopes.back()->scope_decl;
}

ASG::Namespace* Builder::start_template()
{
    return start_namespace("", NamespaceTemplate);
}

void Builder::end_template()
{
    end_namespace();
}


//. Start function impl scope
void Builder::start_function_impl(const ScopedName& name)
{
    STrace trace("Builder::start_function_impl");
    // Create the Namespace
    ASG::Namespace* ns = new ASG::Namespace(m_file, 0, "function", name);
    ScopeInfo* ns_info = find_info(ns);
    // if this is a function template, add the template parameter scope for
    // dependent types.
    if (m_scopes.back()->scope_decl->type() == "template")
      ns_info->search.push_back(m_scopes.back());
    ScopeInfo* scope_info;
    if (name.size() > 1)
    {
        // Find container scope
        std::vector<std::string> scope_name = name;
        scope_name.pop_back();
        scope_name.insert(scope_name.begin(), ""); // force lookup from global, not current scope, since name is fully qualified
        Types::Declared* scope_type = dynamic_cast<Types::Declared*>(m_lookup->lookupType(scope_name));
        if (!scope_type)
        {
            throw ERROR("Warning: Qualified func name was not in a declaration. Parent scope is:" << scope_name);
        }
        ASG::Scope* scope = dynamic_cast<ASG::Scope*>(scope_type->declaration());
        if (!scope)
        {
            throw ERROR("Warning: Qualified func name was not in a scope.");
        }
        scope_info = find_info(scope);
    }
    else
    {
        scope_info = find_info(m_global);
    }
    // Add to name dictionary TODO: this is for debugging only!
    scope_info->dict->insert(ns);
    // Push stack. Search is this Class plus enclosing scopes. bases added later
    std::copy(scope_info->search.begin(), scope_info->search.end(),
              std::back_inserter(ns_info->search));

    m_scopes.push_back(ns_info);
    m_scope = ns;
}

//. End function impl scope
void Builder::end_function_impl()
{
    m_scopes.pop_back();
    m_scope = m_scopes.back()->scope_decl;
}

//. Add an function
ASG::Function* Builder::add_function(int line, const std::string& name,
                                     const std::vector<std::string>& premod,
				     Types::Type* ret,
                                     const std::vector<std::string>& postmod,
                                     const std::string& realname,
				     ASG::Parameter::vector* templ_params)
{
    // Find the parent scope, depending on whether this is a template or not
    ASG::Scope* parent_scope;
    if (templ_params)
        parent_scope = m_scopes[m_scopes.size()-2]->scope_decl;
    else
        parent_scope = m_scope;

    // Determine the new scoped name
    ScopedName func_name = extend(parent_scope->name(), name);

    // Decide whether this is a member function (aka operation) or just a
    // function
    ASG::Function* func;
    if (dynamic_cast<ASG::Class*>(parent_scope))
    {
      char const *type = (templ_params && templ_params->size()) ? "member function template" : "member function";
      func = new ASG::Operation(m_file, line, type, func_name, premod, ret, postmod, realname);
    }
    else
    {
      char const *type = (templ_params && templ_params->size()) ? "function template" : "function";
      func = new ASG::Function(m_file, line, type, func_name, premod, ret, postmod, realname);
    }
    // Create template type
    if (templ_params)
    {
        Types::Template* templ = new Types::Template(func_name, func, *templ_params);
        func->set_template_id(templ);
        add(func, true);
    }
    else
        add(func);
    return func;
}

//. Add a variable
ASG::Variable* Builder::add_variable(int line, const std::string& name, Types::Type* vtype, bool constr, const std::string& type)
{
    // Generate the name
    ScopedName scope = m_scope->name();
    scope.push_back(name);
    ASG::Variable* var = new ASG::Variable(m_file, line, type, scope, vtype, constr);
    add(var);
    return var;
}

ASG::Const* Builder::add_constant(int line, const std::string& name, Types::Type* cType, const std::string& type, std::string const &value)
{
    // Generate the name
    ScopedName scope = m_scope->name();
    scope.push_back(name);
    ASG::Const* c = new ASG::Const(m_file, line, type, scope, cType, value);
    add(c);
    return c;
}

void Builder::add_this_variable()
{
    // First find out if we are in a method
    ASG::Scope* func_ns = m_scope;
    ScopedName name = func_ns->name();
    name.pop_back();
    name.insert(name.begin(), std::string());
    Types::Named* clas_named = m_lookup->lookupType(name, false);
    ASG::Class* clas;
    try
    {
        clas = Types::declared_cast<ASG::Class>(clas_named);
    }
    catch (const Types::wrong_type_cast& )
        // Not in a method -- so dont add a 'this'
    {
        return;
    }

    // clas is now the ASG::Class of the enclosing class
    Types::Type::Mods pre, post;
    post.push_back("*");
    Types::Modifier* t_this = new Types::Modifier(clas->declared(), pre, post);
    add_variable(-1, "this", t_this, false, "this");
}

//. Add a typedef
ASG::Typedef* Builder::add_typedef(int line, const std::string& name, Types::Type* alias, bool constr)
{
    // Generate the name
    ScopedName scoped_name = extend(m_scope->name(), name);
    // Create the object
    ASG::Typedef* tdef = new ASG::Typedef(m_file, line, "typedef", scoped_name, alias, constr);
    add(tdef);
    return tdef;
}

//. Add an enumerator
ASG::Enumerator* Builder::add_enumerator(int line, const std::string& name, const std::string& value)
{
    ScopedName scoped_name = extend(m_scope->name(), name);
    ASG::Enumerator* enumor = new ASG::Enumerator(m_file, line, "enumerator", scoped_name, value);
    add(enumor->declared());
    return enumor;
}

//. Add an enum
ASG::Enum* Builder::add_enum(int line, const std::string& name, const std::vector<ASG::Enumerator*>& enumors)
{
    ScopedName scoped_name = extend(m_scope->name(), name);
    ASG::Enum* theEnum = new ASG::Enum(m_file, line, "enum", scoped_name);
    theEnum->enumerators() = enumors;
    add(theEnum);
    return theEnum;
}

//. Add tail comment
ASG::Builtin *Builder::add_tail_comment(int line)
{
    ScopedName name;
    name.push_back("EOS");
    ASG::Builtin *builtin = new ASG::Builtin(m_file, line, "EOS", name);
    add(builtin);
    return builtin;
}

// A functor that adds only inheritances which are class objects to a given
// list
class InheritanceAdder
{
    std::list<ASG::Class*>& open_list;
public:
    InheritanceAdder(std::list<ASG::Class*>& l) : open_list(l)
    {}
    InheritanceAdder(const InheritanceAdder& i) : open_list(i.open_list)
    {}
    void operator() (ASG::Inheritance* i)
    {
        try
        {
            open_list.push_back(Types::declared_cast<ASG::Class>(i->parent()));
        }
        catch (const Types::wrong_type_cast&)
        { /* ?? ignore for now */
        }
    }
};


//. Maps a scoped name into a vector of scopes and the final type. Returns
//. true on success.
bool Builder::mapName(const ScopedName& names, std::vector<ASG::Scope*>& o_scopes, Types::Named*& o_type)
{
    STrace trace("Builder::mapName");
    ASG::Scope* ast_scope = m_global;
    ScopedName::const_iterator iter = names.begin();
    ScopedName::const_iterator last = names.end();
    last--;
    ScopedName scoped_name;

    // Start scope name at global level
    scoped_name.push_back("");

    // Sanity check
    if (iter == names.end())
        return false;

    // Loop through all containing scopes
    while (iter != last)
    {
        //const std::string& name = *iter++;
        scoped_name.push_back(*iter++);
        Types::Named* type = m_lookup->lookupType(scoped_name);
        if (!type)
        {
            LOG("Warning: failed to lookup " << scoped_name << " in global scope");
            return false;
        }
        try
        {
            ast_scope = Types::declared_cast<ASG::Scope>(type);
        }
        catch (const Types::wrong_type_cast&)
        {
            LOG("Warning: looked up scope wasnt a scope!" << scoped_name);
            return false;
        }
        o_scopes.push_back(ast_scope);
    }

    // iter now == last, which can be any type
    scoped_name.push_back(*iter);
    Types::Named* type = m_lookup->lookupType(scoped_name, true);
    if (!type)
    {
        //find_info(ast_scope)->dict->dump();
        LOG("\nWarning: final type lookup wasn't found!" << *iter);
        return false;
    }

    o_type = type;
    return true;
}


Types::Unknown* Builder::create_unknown(const ScopedName& name)
{
    // Generate the name
    ScopedName u_name = m_scope->name();
    for (ScopedName::const_iterator i = name.begin(); i != name.end(); ++i)
      u_name.push_back(*i);
    Types::Unknown* unknown = new Types::Unknown(u_name);
    return unknown;
}

Types::Unknown* Builder::add_unknown(const std::string& name)
{
    if (m_scopes.back()->dict->has_key(name) == false)
    {
      ScopedName u_name;
      u_name.push_back(name);
      add(create_unknown(u_name));
    }
    return 0;
}

ASG::Forward* Builder::add_forward(int lineno, const std::string& name, const std::string &type,
                                   ASG::Parameter::vector* templ_params)
{
  ScopeInfo* parent_scope = templ_params ? 
    // Must find the scope above the template scope
    m_scopes[m_scopes.size() - 2] : 
    m_scopes[m_scopes.size() - 1];
  ScopedName scoped_name = extend(parent_scope->scope_decl->name(), name);
  // The type is already known.
  if (parent_scope->dict->has_key(name) == true) return 0;
  bool is_template = templ_params && templ_params->size();
  bool is_specialization = *name.rbegin() == '>';
  ASG::Forward* forward = new ASG::Forward(m_file, lineno, type, scoped_name, is_specialization);
  if (is_template)
  {
    Types::Template* templ = new Types::Template(scoped_name, forward, *templ_params);
    forward->set_template_id(templ);
  }
  add(forward, templ_params != 0);
  return forward;
}

Types::Base* Builder::create_base(const std::string& name)
{
    return new Types::Base(extend(m_scope->name(), name));
}

Types::Dependent* Builder::create_dependent(const std::string& name)
{
    return new Types::Dependent(extend(m_scope->name(), name));
}

std::string Builder::dump_search(ScopeInfo* scope)
{
    ScopeSearch& search = scope->search;
    std::ostringstream buf;
    buf << "Search for ";
    if (scope->scope_decl->name().size() == 0)
        buf << "global";
    else
        buf << m_scope->name();
    buf << " is now: ";
    ScopeSearch::iterator iter = search.begin();
    while (iter != search.end())
    {
        buf << (iter==search.begin() ? "" : ", ");
        const ScopedName& name = (*iter)->scope_decl->name();
        if (name.size())
            if ( (*iter)->is_using )
                buf << "(" << name << ")";
            else
                buf << name;
        else
            buf << "global";
        ++iter;
    }
    //buf << std::ends;
    return buf.str();
}

// A comparator which compares declaration pointers of ScopeInfo objects
class Builder::EqualScope
{
    ASG::Scope* target;
public:
    EqualScope(ScopeInfo* t)
    {
        target = t->scope_decl;
    }
    bool operator()(ScopeInfo* s)
    {
        return s->scope_decl == target;
    }
};

void Builder::do_add_using_directive(ScopeInfo* target, ScopeInfo* scope)
{
    STrace trace("Builder::do_add_using_directive");

    // Check if 'scope' already has 'target' in its using list
    ScopeSearch& uses = scope->using_scopes;
    if (std::find_if(uses.begin(), uses.end(), EqualScope(target)) != uses.end())
        // Already using
        return;
    // Else add it
    scope->using_scopes.push_back(target);
    target->used_by.push_back(scope);

    ScopedName& target_name = target->scope_decl->name();

    // Find where to insert 'scope' into top()'s search list
    // "closest enclosing namespace that contains both using directive and
    //  target namespace" -- C++ Standard
    ScopeSearch& search = scope->search;
    LOG(dump_search(scope));
    ScopeSearch::iterator iter = search.end();
    // Skip global scope.. cant check something with no name
    --iter;
    while (iter != search.begin())
    {
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

    LOG(dump_search(scope));

    // Add target to all used_by scopes
    ScopeSearch used_by_copy = scope->used_by;
    iter = used_by_copy.begin();
    while (iter != used_by_copy.end())
        do_add_using_directive(target, *iter++);
}

// Add a namespace using declaration.
ASG::UsingDirective *Builder::add_using_directive(int line, Types::Named* type)
{
    STrace trace("Builder::using_directive");
    ASG::Scope* ast_scope = Types::declared_cast<ASG::Scope>(type);
    ScopeInfo* target = find_info(ast_scope);
    do_add_using_directive(target, m_scopes.back());
    
    ASG::UsingDirective* u = new ASG::UsingDirective(m_file, line, type->name());
    add(u);
    return u;
}


// Add a namespace alias using declaration.
void Builder::add_aliased_using_namespace(Types::Named* type, const std::string& alias)
{
    STrace trace("Builder::usingNamespace");

    // Retrieve the 'Namespace' it points to
    ASG::Namespace* ns = Types::declared_cast<ASG::Namespace>(type);

    // Create a new declared type with a different name
    ScopedName new_name = extend(m_scope->name(), alias);
    Types::Declared* declared = new Types::Declared(new_name, ns);

    // Add to current scope
    add(declared);
}

// Add a using declaration.
ASG::UsingDeclaration *Builder::add_using_declaration(int line, Types::Named* type)
{
    // Add it to the current scope
    ScopedName name = extend(m_scope->name(), type->name().back());
    ASG::UsingDeclaration* u = new ASG::UsingDeclaration(m_file, line, name, type);
    add(u);
    return u;
}
