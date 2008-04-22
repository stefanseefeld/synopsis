//
// Copyright (C) 2002 Stephen Davies
// Copyright (C) 2002 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef builder_hh_
#define builder_hh_

#include <map>
#include "ast.hh"
#include "common.hh"

// Forward declare some Types::Types
namespace Types
{
class Type;
class Base;
class Named;
class Unknown;
class TemplateType;
class FuncPtr;
class Dependent;
}

// Forward declare the SWalker class
class SWalker;

// Forward declare the Lookup class
class Lookup;

class ScopeInfo;
typedef std::vector<ScopeInfo*> ScopeSearch; // TODO: move to common

//. Enumeration of namespace types for use in Builder::start_namespace()
enum NamespaceType
{
    NamespaceNamed, //.< Normal, named, namespace. name is its given name
    NamespaceAnon, //.< An anonymous namespace. name is the filename
    NamespaceUnique, //.< A unique namespace. name is the type (for, while, etc.)
    NamespaceTemplate, //.< A template namespace. name is empty
};


//. AST Builder.
//. This class manages the building of an AST, including queries on the
//. existing AST such as name and type lookups. The building functions are
//. called by SWalker as it walks the parse tree.
class Builder
{
    friend class Lookup;
public:
    //. Constructor
    Builder(AST::SourceFile* file);

    //. Destructor. Recursively destroys all AST objects
    ~Builder();

    //. Sets the swalker
    void set_swalker(SWalker* swalker)
    {
        m_swalker = swalker;
    }

    //. Changes the current accessability for the current scope
    void set_access(AST::Access);

    //. Returns the current file
    AST::SourceFile* file() const
    {
        return m_file;
    }
    //. Changes the current file
    void set_file(AST::SourceFile*);

    //. Returns the list of builtin decls ("__null_t", "true", etc.)
    const AST::Declaration::vector& builtin_decls() const;

    //
    // State Methods
    //

    //. Returns the current scope
    AST::Scope* scope()
    {
        return m_scope;
    }

    //. Returns the current ScopeInfo for the current Scope
    ScopeInfo* scopeinfo()
    {
        return m_scopes.back();
    }

    //. Returns the global scope
    AST::Scope* global()
    {
        return m_global;
    }

    //. Returns the Lookup object for the builder
    Lookup* lookup()
    {
        return m_lookup;
    }

    //
    // AST Methods
    //

    //. Add the given Declaration to the current scope. If is_template is true,
    //. then it is added to the parent of the current scope, assuming that the
    //. current scope is the temporary template scope
    void add(AST::Declaration* declaration, bool is_template = false);

    //. Add the given non-declaration type to the current scope
    void add(Types::Named* named);

    //. Adds the given Macros to the global scope. This method should only be
    //. called once, with the macros stored in order from the preprocessing
    //. stage.
    void add_macros(const std::vector<AST::Macro*>&);

    //. Construct and open a new Namespace. The Namespace becomes the
    //. current scope, and the old one is pushed onto the stack. If name is
    //. empty then a unique name is generated of the form `ns1
    AST::Namespace* start_namespace(const std::string& name, NamespaceType type);

    //. End the current namespace and pop the previous Scope off the stack
    void end_namespace();

    //. Starts a new template namespace
    AST::Namespace* start_template();

    //. End the current template namespace
    void end_template();

    //. Construct and open a new Class. The Class becomes the current scope,
    //. and the old one is pushed onto the stack. The type argument is the
    //. type, ie: "class" or "struct". This is tested to determine the default
    //. accessability. If this is a template class, the templ_params vector must
    //. be non-null pointer
    AST::Class* start_class(int, const std::string& type, const std::string& name,
                            AST::Parameter::vector* templ_params, std::string const &primary_name);

    //. Construct and open a new Class with a qualified name
    AST::Class* start_class(int, const std::string& type, const ScopedName& names);

    //. Update the search to include base classes. Call this method after
    //. startClass(), and after filling in the parents() vector of the returned
    //. AST::Class object. After calling this method, name and type lookups
    //. will correctly search the base classes of this class.
    void update_class_base_search();

    //. End the current class and pop the previous Scope off the stack
    void end_class();

    //. Start function impl scope
    void start_function_impl(const ScopedName& name);

    //. End function impl scope
    void end_function_impl();

    //. Add an function
    AST::Function* add_function(int, const std::string& name,
                                const std::vector<std::string>& premod,
				Types::Type* ret,
                                const std::vector<std::string>& postmod,
                                const std::string& realname,
				AST::Parameter::vector* templ_params);

    //. Add a variable
    AST::Variable* add_variable(int, const std::string& name, Types::Type* vtype, bool constr, const std::string& type);

    //. Add a const
    AST::Const* add_constant(int, const std::string& name, Types::Type* ctype, std::string const &type, std::string const &value = "");

    //. Add a variable to represent 'this', iff we are in a method
    void add_this_variable();

    //. Add a typedef
    AST::Typedef* add_typedef(int, const std::string& name, Types::Type* alias, bool constr);

    //. Add an enumerator
    AST::Enumerator* add_enumerator(int, const std::string& name, const std::string& value);

    //. Add an enum
    AST::Enum* add_enum(int, const std::string& name, const AST::Enumerator::vector &);

    //. Add a tail comment. This will be a builtin with name 'EOS'
    AST::Builtin *add_tail_comment(int line);

    //
    // Using methods
    //

    //. Add a using directive.
    AST::UsingDirective *add_using_directive(int, Types::Named* type);

    //. Add a namespace alias using declaration.
    void add_aliased_using_namespace(Types::Named* type, const std::string& alias);

    //. Add a using declaration.
    AST::UsingDeclaration *add_using_declaration(int, Types::Named* type);


    //. Maps a scoped name into a vector of scopes and the final type. Returns
    //. true on success.
    bool mapName(const ScopedName& name, std::vector<AST::Scope*>&, Types::Named*&);

    //. Create a Base type for the given name in the current scope
    Types::Base* create_base(const std::string& name);

    //. Create a Dependent type for the given name in the current scope
    Types::Dependent* create_dependent(const std::string& name);

    //. Create an Unknown type for the given name in the current scope
    Types::Unknown* create_unknown(const ScopedName& name);

    //. Create a Template type for the given name in the current scope
    Types::Template* create_template(const std::string& name, const std::vector<Types::Type*>&);

    //. Add an Unknown decl for given name if it doesnt already exist
    Types::Unknown* add_unknown(const std::string& name);

    //. Add an Templated Forward decl for given name if it doesnt already exist
    AST::Forward* add_forward(int lineno, const std::string& name, const std::string &type,
                              AST::Parameter::vector* templ_params);

private:
    //. Current file
    AST::SourceFile* m_file;

    //. The global scope object
    AST::Scope* m_global;

    //. Current scope object
    AST::Scope* m_scope;

    //. A counter used to generate unique namespace names
    int m_unique;

    //. The stack of Builder::Scopes
    std::vector<ScopeInfo*> m_scopes;

    //. Private data which uses map
    struct Private;
    //. Private data which uses map instance
    Private* m;

    //. Return a ScopeInfo* for the given Declaration. This method first looks for
    //. an existing Scope* in the Private map.
    ScopeInfo* find_info(AST::Scope*);

    //. Utility method to recursively add base classes to given search
    void add_class_bases(AST::Class* clas, ScopeSearch& search);

    //. Formats the search of the given Scope for logging
    std::string dump_search(ScopeInfo* scope);

    //. Recursively adds 'target' as using in 'scope'
    void do_add_using_directive(ScopeInfo* target, ScopeInfo* scope);

    //. A class that compares Scopes
    class EqualScope;

    //. A pointer to the SWalker. This is set explicitly by the SWalker during
    //. its constructor (which takes a Builder).
    SWalker* m_swalker;

    //. A pointer to the Lookup
    Lookup* m_lookup;

};

#endif
// vim: set ts=8 sts=4 sw=4 et:
