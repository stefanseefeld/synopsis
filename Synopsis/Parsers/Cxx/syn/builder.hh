// File: builder.hh

#ifndef H_SYNOPSIS_CPP_BUILDER
#define H_SYNOPSIS_CPP_BUILDER

#include "ast.hh"
#include <stack>
#include <map>

// Forward declare some Type::Types
namespace Type {
    class Type;
    class Base;
    class Named;
    class Unknown;
    class Template;
    class Function;
}

typedef std::vector<std::string> Name;

//. Prototype for scoped name output operator
std::ostream& operator <<(std::ostream& out, const std::vector<std::string>& vec);

//. AST Builder.
//. This class manages the building of an AST, including queries on the
//. existing AST such as name and type lookups. The building operations are
//. called by SWalker as it walks the parse tree.
class Builder {
public:
    //. Constructor
    Builder(const std::string &basename);
    //. Destructor. Recursively destroys all AST objects
    ~Builder();

    //. Changes the current accessability for the current scope
    void setAccess(AST::Access);

    //. Returns the current filename
    const std::string &filename() const { return m_filename; }
    //. Changes the current filename
    void setFilename(const std::string &filename);
    
    //
    // AST Methods
    //

    //. Returns the current scope
    AST::Scope* scope() { return m_scope; }

    //. Returns the global scope
    AST::Scope* global() { return m_global; }

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
    AST::Namespace* startNamespace(const std::string &name, NamespaceType type);

    //. End the current namespace and pop the previous Scope off the stack
    void endNamespace();

    //. Construct and open a new Class. The Class becomes the current scope,
    //. and the old one is pushed onto the stack. The type argument is the
    //. type, ie: "class" or "struct". This is tested to determine the default
    //. accessability.
    AST::Class* startClass(int, const std::string &type, const std::string &name);
    //. Construct and open a new Class with a qualified name
    AST::Class* startClass(int, const std::string &type, const std::vector<std::string> &names);
    //. Update the search to include base classes. Call this method after
    //. startClass(), and after filling in the parents() vector of the returned
    //. AST::Class object. After calling this method, name and type lookups
    //. will correctly search the base classes of this class.
    void updateBaseSearch();

    //. End the current class and pop the previous Scope off the stack
    void endClass();

    //. Start function impl scope
    void startFunctionImpl(const std::vector<std::string> &name);

    //. End function impl scope
    void endFunctionImpl();

    //. Add an operation
    AST::Operation* addOperation(int, const std::string &name, const std::vector<std::string> &premod, Type::Type* ret, const std::string &realname);

    //. Add a variable
    AST::Variable* addVariable(int, const std::string &name, Type::Type* vtype, bool constr, const std::string& type);

    //. Add a variable to represent 'this', iff we are in a method
    void addThisVariable();

    //. Add a typedef
    AST::Typedef* addTypedef(int, const std::string &name, Type::Type* alias, bool constr);

    //. Add an enumerator
    AST::Enumerator* addEnumerator(int, const std::string &name, const std::string &value);

    //. Add an enum
    AST::Enum* addEnum(int, const std::string &name, const std::vector<AST::Enumerator*>&);

    //. Add a tail comment. This will be a dummy declaration with an empty
    //. and type "dummy"
    AST::Declaration* addTailComment(int line);

    //
    // Using methods
    //
    
    //. Add a namespace using declaration.
    void usingNamespace(Type::Named* type);

    //. Add a namespace alias using declaration.
    void usingNamespace(Type::Named* type, const std::string& alias);

    //. Add a using declaration.
    void usingDeclaration(Type::Named* type);


    //
    // Type Methods
    //

    //. Looks up the name in the current scope. This method always succeeds --
    //. if the name is not found it forward declares it.
    Type::Named* lookupType(const std::string &name);

    //. Looks up the qualified name in the current scope. This method always
    //. succeeds -- if the name is not found it forwards declares it.
    //. @param names The list of identifiers given
    //. @param fuc_okay If true, multiple declarations will not cause an error (needs fixing)
    //. @param scope If set determines the scope to start lookup from, else the
    //. current scope is used
    Type::Named* lookupType(const std::vector<std::string> &names, bool func_okay=false, AST::Scope* scope=NULL);

    //. Looks up the name in the scope of the given scope. This method may
    //. return a NULL ptr if the lookup failed.
    Type::Named* lookupType(const std::string &name, AST::Scope* scope);

    //. Looks up the function in the given scope with the given args. 
    AST::Function* lookupFunc(const std::string &, AST::Scope*, const std::vector<Type::Type*>&);

    //. Looks up the function operator in the current scope with the given
    //. types. May return NULL if builtin operator or no operator is found.
    AST::Function* lookupOperator(const std::string& oper, Type::Type* left_type, Type::Type* right_type);

    //. Maps a scoped name into a vector of scopes and the final type. Returns
    //. true on success.
    bool mapName(const AST::Name& name, std::vector<AST::Scope*>&, Type::Named*&);

    //. Returns the types for an array operator on the given type with an
    //. argument of the given type. If a function is used then it is stored in
    //. the function ptr ref given, else the ptr is set to NULL.
    Type::Type* arrayOperator(Type::Type* object, Type::Type* arg, AST::Function*&);

    //. Resolves the final type of the given type. If the given type is an
    //. Unknown, it checks to see if the type has been defined yet and returns
    //. that instead.
    Type::Named* resolveType(Type::Named* maybe_unknown);

    //. Create a Base type for the given name in the current scope
    Type::Base* Base(const std::string &name);

    //. Create an Unknown type for the given name in the current scope
    Type::Unknown* Unknown(const std::string &name);

    //. Create a Template type for the given name in the current scope
    Type::Template* Template(const std::string &name, const std::vector<Type::Type*>&); 

    //. Add an Unknown decl for given name if it doesnt already exist
    Type::Unknown* addUnknown(const std::string &name);

private:
    //. Base filename to strip from the start of all filenames
    std::string m_basename;

    //. Current filename
    std::string m_filename;

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
	//. Constructor that creates a Dummy 'using' scope referencing 's'
	Scope(Scope* s);
	//. Destructor
	~Scope();
	//. Dictionary for this scope
	class Dictionary* dict;
	//. The declaration for this scope
	AST::Scope* scope_decl;
	//. Typedef for a Scope Search
	typedef std::vector<Scope*> Search;
	//. The list of scopes to search for this scope, including this
	Search search;
	//. The list of scopes in the search because they are 'using'd
	Search using_scopes;
	//. The list of scopes 'using' this one
	Search used_by;
	//. True only if this is a dummy Scope representing the use of
	//. another. If this is the case, then only 'dict' is set
	bool is_using;
	//. Current accessability
	AST::Access access;
	//. Counts of named sub-namespaces
	std::map<std::string, int> nscounts;
	int getCount(const std::string& name);
    };
    //. The stack of Builder::Scopes
    std::stack<Scope*> m_scopes;

    //. Looks up the name in the current scope. This method may fail and
    //. return a NULL ptr.
    Type::Named* lookup(const std::string &name);

    //. Searches for name in the list of Scopes. This method may return NULL
    //. if the name is not found.
    Type::Named* lookup(const std::string &name, const Scope::Search&, bool func_okay = false) throw ();

    //. Searches for name in the given qualified scope. This method may return
    //. NULL if the name is not found. Lookup proceeds according to the spec:
    //. if 'scope' is a Class scope, then scope and all base classes are
    //. searched, else if it's a 'namespace' scope then all usings are checked.
    Type::Named* lookupQual(const std::string& name, const Scope*, bool func_okay = false);

    //. Private data which uses map
    struct Private;
    //. Private data which uses map instance
    Private* m;

    //. Return a Scope* for the given Declaration. This method first looks for
    //. an existing Scope* in the Private map.
    Scope* findScope(AST::Scope*);

    //. Utility class to recursively add base classes to given search
    void addClassBases(AST::Class* clas, Scope::Search& search);

    //. Utility class to add all functions with the given name in the given
    //. Scope's dictionary to the given vector. May throw an error if the
    //. types looked up are not functions.
    void findFunctions(const std::string&, Scope*, std::vector<AST::Function*>&);

    //. Determines the best function from the given list for the given
    //. arguments using heuristics. Returns the function and stores the cost
    AST::Function* bestFunction(const std::vector<AST::Function*>&, const std::vector<Type::Type*>&, int& cost);

    //. Formats the search of the given Scope for logging
    std::string dumpSearch(Builder::Scope* scope);

    //. Recursively adds 'target' as using in 'scope'
    void addUsingNamespace(Scope* target, Scope* scope);
}; // class Builder

#endif
