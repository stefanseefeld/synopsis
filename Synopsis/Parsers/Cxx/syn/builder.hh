// File: builder.hh

#ifndef H_SYNOPSIS_CPP_BUILDER
#define H_SYNOPSIS_CPP_BUILDER

#include "ast.hh"
#include <stack.h>

// Forward declare some Type::Types
namespace Type {
    class Type;
    class Base;
    class Named;
    class Unknown;
    class Template;
    class Function;
}

typedef vector<string> Name;

//. AST Builder.
//. This class manages the building of an AST, including queries on the
//. existing AST such as name and type lookups. The building operations are
//. called by SWalker as it walks the parse tree.
class Builder {
public:
    //. Constructor
    Builder(string basename);
    //. Destructor. Recursively destroys all AST objects
    ~Builder();

    //. Changes the current accessability for the current scope
    void setAccess(AST::Access);

    //. Returns the current filename
    string filename() { return m_filename; }
    //. Changes the current filename
    void setFilename(string filename);
    
    //
    // AST Methods
    //

    //. Returns the current scope
    AST::Scope* scope() { return m_scope; }

    //. Add the given Declaration to the current scope
    void add(AST::Declaration*);

    //. Add the given non-declaration type to the current scope
    void add(Type::Named*);

    //. Enumeration of types of namespaces
    enum NamespaceType {
	NamespaceNamed, //.< Normal, named, namespace. name is its given name
	NamespaceAnon, //.< An anonymous namespace. name is the filename
	NamespaceUnique, //.< A unique namespace. name is the type (for, while, etc.)
    };

    //. Construct and open a new Namespace. The Namespace becomes the
    //. current scope, and the old one is pushed onto the stack. If name is
    //. empty then a unique name is generated of the form `ns1
    AST::Namespace* startNamespace(string name, NamespaceType type);

    //. End the current namespace and pop the previous Scope off the stack
    void endNamespace();

    //. Construct and open a new Class. The Class becomes the current scope,
    //. and the old one is pushed onto the stack. The type argument is the
    //. type, ie: "class" or "struct". This is tested to determine the default
    //. accessability.
    AST::Class* startClass(int, string type, string name);
    //. Construct and open a new Class with a qualified name
    AST::Class* startClass(int, string type, const vector<string>& names);
    //. Update the search to include base classes. Call this method after
    //. startClass(), and after filling in the parents() vector of the returned
    //. AST::Class object. After calling this method, name and type lookups
    //. will correctly search the base classes of this class.
    void updateBaseSearch();

    //. End the current class and pop the previous Scope off the stack
    void endClass();

    //. Start function impl scope
    void startFunctionImpl(const vector<string>& name);

    //. End function impl scope
    void endFunctionImpl();

    //. Add an operation
    AST::Operation* addOperation(int, string name, vector<string> premod, Type::Type* ret, string realname);

    //. Add a variable
    AST::Variable* addVariable(int, string name, Type::Type* vtype, bool constr);

    //. Add a variable to represent 'this', iff we are in a method
    void addThisVariable();

    //. Add a typedef
    AST::Typedef* addTypedef(int, string name, Type::Type* alias, bool constr);

    //. Add an enumerator
    AST::Enumerator* addEnumerator(int, string name, string value);

    //. Add an enum
    AST::Enum* addEnum(int, string name, const vector<AST::Enumerator*>&);

    //. Add a tail comment. This will be a dummy declaration with an empty
    //. and type "dummy"
    AST::Declaration* addTailComment(int line);

    //
    // Type Methods
    //

    //. Looks up the name in the current scope. This method always succeeds --
    //. if the name is not found it forward declares it.
    Type::Named* lookupType(string name);

    //. Looks up the qualified name in the current scope. This method always
    //. succeeds -- if the name is not found it forwards declares it.
    Type::Named* lookupType(const vector<string>& names, bool func_okay=false);

    //. Looks up the name in the scope of the given scope. This method may
    //. return a NULL ptr if the lookup failed.
    Type::Named* lookupType(string name, AST::Scope* scope);

    //. Looks up the function in the given scope with the given args. 
    AST::Function* lookupFunc(string, AST::Scope*, const vector<AST::Parameter*>&);

    //. Resolves the final type of the given type. If the given type is an
    //. Unknown, it checks to see if the type has been defined yet and returns
    //. that instead.
    Type::Named* resolveType(Type::Named* maybe_unknown);

    //. Create a Base type for the given name in the current scope
    Type::Base* Base(string name);

    //. Create an Unknown type for the given name in the current scope
    Type::Unknown* Unknown(string name);

    //. Create a Template type for the given name in the current scope
    Type::Template* Template(string name, const vector<Type::Type*>&); 

    //. Add an Unknown decl for given name if it doesnt already exist
    Type::Unknown* addUnknown(string name);

private:
    //. Base filename to strip from the start of all filenames
    string m_basename;

    //. Current filename
    string m_filename;

    //. The global scope object
    AST::Scope* m_global;

    //. Current scope object
    AST::Scope* m_scope;

    //. A counter used to generate unique namespace names
    int m_unique;

    //
    // Scope Class
    //

    //. This class encapsulates a Scope and its dictionary of names
    struct Scope {
	//. Constructor
	Scope(AST::Scope* s);
	//. Destructor
	~Scope();
	//. Dictionary for this scope
	class Dictionary* dict;
	//. The declaration for this scope
	AST::Scope* scope_decl;
	//. Typedef for a Scope Search
	typedef vector<Scope*> Search;
	//. The list of scopes to search for this scope, including this
	Search search;
	//. Current accessability
	AST::Access access;
    };
    //. The stack of Builder::Scopes
    stack<Scope*> m_scopes;

    //. Looks up the name in the current scope. This method may fail and
    //. return a NULL ptr.
    Type::Named* lookup(string name);

    //. Searches for name in the list of Scopes. This method may return NULL
    //. if the name is not found.
    Type::Named* lookup(string name, const Scope::Search&, bool func_okay = false) throw ();

    //. Private data which uses map
    struct Private;
    //. Private data which uses map instance
    Private* m;

    //. Return a Scope* for the given Declaration. This method first looks for
    //. an existing Scope* in the Private map.
    Scope* findScope(AST::Scope*);

    //. Utility class to recursively add base classes to given search
    void addClassBases(AST::Class* clas, Scope::Search& search);

}; // class Builder

#endif
