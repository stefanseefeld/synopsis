//. ast.h
//. A C++ class hierarchy that more or less mirrors the AST hierarchy in
//. Python/Core.AST.

#ifndef H_SYNOPSIS_CPP_AST
#define H_SYNOPSIS_CPP_AST

// OCC uses stupid #defines, like #define Scope
#ifdef Scope
#undef Scope
#endif


#include <string>
#include <vector>

// Forward declare Dictionary
class Dictionary;

// Forward declare Type::Type
namespace Type { class Type; }

//. A namespace for the AST hierarchy
namespace AST {

    using std::vector;

    // Forward declaration of AST::Visitor defined in this file
    class Visitor;

    // Forward declaration of AST::Comment defined in this file
    class Comment;

    //. An enumeration of accessability specifiers
    enum Access {
	Default = 0,
	Public, Protected, Private
    };

    //. A scoped name type. This typedef makes it easier to use scoped name
    //. types, and also makes it clearer than using the raw vector in your
    //. code.
    typedef vector<std::string> Name;

    //. The base class of the Declaration hierarchy.
    //. All declarations have a scoped Name, comments, etc. The filename and
    //. type name are constant strings. This is enforced so that the strings
    //. will reference the same data, saving both memory and cpu time. For this
    //. to work however, you must be careful to use the same strings for
    //. constructing the names from, for example from a dictionary.
    class Declaration {
    public:
	//. Constructor
	Declaration(string filename, int line, string type, Name);
	//. Destructor. Recursively deletes the comments for this declaration
	virtual ~Declaration();

	//. Accept the given AST::Visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Constant version of name()
	const Name name() const { return m_name; }
	//. Returns the scoped name of this declaration
	Name name() { return m_name; }

	//. Returns the filename of this declaration
	string filename() const { return m_filename; }

	//. Returns the line number of this declaration
	int line() const { return m_line; }

	// TODO: move to whatever classes need it. Not needed here!
	//. Returns the string type name of this declaration
	string type() const { return m_type; }
	
	//. Returns the accessability of this declaration
	Access access() const { return m_access; }

	//. Constant version of comments()
	const vector<Comment*>& comments() const { return m_comments; }
	//. Returns the vector of comments. The vector returned is the private
	//. member vector of this Declaration, so modifications will affect the
	//. Declaration.
	vector<Comment*>& comments() { return m_comments; }
	

    private:
	//. The scoped name
	Name m_name;
	//. The filename
	string m_filename;
	//. The first line number
	int m_line;
	//. The string type name
	string m_type;
	//. The vector of Comment objects
	vector<Comment*> m_comments;
	//. The accessability spec
	Access m_access;
    }; // class Declaration


    //. Base class for scopes with contained declarations. Each scope has its
    //. own Dictionary of names so far accumulated for this scope. Each scope
    //. also as a complete vector of scopes where name lookup is to proceed if
    //. unsuccessful in this scope. Name lookup is not recursive.
    class Scope : public Declaration {
    public:
	//. A vector of Scopes for name searching from this Scope
	typedef vector<Scope*> scope_vector;

	//. Constructor
	Scope(string, int, string, Name);
	//. Destructor. 
	//. Recursively destroys contained declarations
	virtual ~Scope();

	//. Accepts the given visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Constant version of declarations()
	const vector<Declaration*>& declarations() const { return m_declarations; }
	//. Returns the vector of declarations. The vector returned is the
	//. private member vector of this Scope, so modifications will affect
	//. the Scope.
	vector<Declaration*>& declarations() { return m_declarations; }

	//. Returns the Dictionary object
	Dictionary* dict() { return m_dict; }
	//. Returns the scope_vector of search Scopes
	scope_vector& search_scopes() { return m_search_scopes; }

    private:
	//. The vector of contained declarations
	vector<Declaration*> m_declarations;
	//. The Dictionary of names in this Scope
	Dictionary* m_dict;
	//. The search vector of lookup Scopes
	scope_vector m_search_scopes;

    }; // class Scope

    //. Namespace class
    class Namespace : public Scope {
    public:
	//. Constructor
	Namespace(string, int, string, Name);
	//. Destructor
	virtual ~Namespace();

	//. Accepts the given AST::Visitor
	virtual void accept(Visitor*);
    };

    // Forward declare Inheritance
    class Inheritance;

    //. Class class
    class Class : public Scope {
    public:
	//. Constructor
	Class(string, int, string, Name);
	//. Destructor. Recursively destroys Inheritance objects
	virtual ~Class();

	//. Accepts the given AST::Visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Constant version of parents()
	const vector<Inheritance*>& parents() const { return m_parents; }
	//. Returns the vector of parent Inheritance objects. The vector
	//. returned is the private member vector of this Class, so
	//. modifications will affect the Class.
	vector<Inheritance*>& parents() { return m_parents; }

    private:
	//. The vector of parent Inheritance objects
	vector<Inheritance*> m_parents;
    };

    //. Inheritance class. This class encapsulates the information about an
    //. inheritance, namely its accessability.
    class Inheritance {
    public:
	//. Constructor
	Inheritance(Access, Class*);

	//. Accepts the given AST::Visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Returns the Class object this inheritance refers to
	Class* parent() { return m_parent; }

	//. Returns the accessability of this inheritance
	Access access() { return m_access; }
    private:
	//. The parent class
	Class* m_parent;
	//. The accessability
	Access m_access;
    };

    //. Forward declaration. Currently this has no extra attributes.
    class Forward : public Declaration {
    public:
	//. Constructor
	Forward(string, int, string, Name);

	//. Accepts the given AST::Visitor
	virtual void accept(Visitor*);
    };

    //. Typedef declaration
    class Typedef : public Declaration {
    public:
	//. Constructor
	Typedef(string, int, string, Name, Type::Type* alias, bool constr);

	//. Accepts the given AST::Visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Returns the Type object this typedef aliases
	Type::Type* alias() { return m_alias; }

	//. Returns true if the Type object was constructed inside the typedef
	bool constructed() { return m_constr; }
    private:
	//. The alias Type
	Type::Type* m_alias;

	//. True if constructed
	bool m_constr;
    };

    //. Variable declaration
    class Variable : public Declaration {
    public:
	//. Constructor
	Variable(string, int, string, Name, Type::Type* vtype, bool constr);

	//. Accepts the given AST::Visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Returns the Type object of this variable
	Type::Type* vtype() { return m_vtype; }

	//. Returns true if the Type object was constructed inside the variable
	bool constructed() { return m_constr; }

	//. Returns the array sizes vector
	vector<int>& sizes() { return m_sizes; }
    private:
	//. The variable Type
	Type::Type* m_vtype;

	//. True if constructed
	bool m_constr;

	//. Vector of array sizes. zero length indicates not an array.
	vector<int> m_sizes;
    };

    //. Enumerator declaration. This is a name with a value in the containing
    //. scope. Enumerators only appear inside Enums via their enumerators()
    //. attribute.
    class Enumerator : public Declaration {
    public:
	//. Constructor
	Enumerator(string, int, string, Name, string value);
	
	//. Accept the given AST::Visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Returns the value of this enumerator
	string value() const { return m_value; }
    private:
	//. The value of this enumerator
	string m_value;
    };

    //. Enum declaration. An enum contains multiple enumerators.
    class Enum : public Declaration {
    public:
	//. Constructor
	Enum(string, int, string, Name);
	//. Destructor. Recursively destroys Enumerators
	~Enum();

	//. Accepts the given AST::Visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Returns the vector of Enumerators
	vector<Enumerator*>& enumerators() { return m_enums; }
    private:
	//. The vector of Enumerators
	vector<Enumerator*> m_enums;
    };

    //. A const is a name with a value and declared type.
    class Const : public Declaration {
    public:
	//. Constructor
	Const(string, int, string, Name, Type::Type* ctype, string value);
	
	//. Accept the given AST::Visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Returns the Type object of this const
	Type::Type* ctype() { return m_ctype; }

	//. Returns the value of this enumerator
	string value() const { return m_value; }
    private:
	//. The const Type
	Type::Type* m_ctype;

	//. The value of this enumerator
	string m_value;
    };


    //. Parameter encapsulates one parameter to a function
    class Parameter {
    public:
	//. The type of modifiers such as 'in', 'out'
	typedef string Mods;

	//. Constructor
	Parameter(Mods& pre, Type::Type*, Mods& post, string name, string value);

	//. Accept the given AST::Visitor. Note this is not derived from
	//. Declaration so it is not a virtual method.
	void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Returns the premodifier
	Mods& premodifier() { return m_pre; }
	//. Returns the postmodifier
	Mods& postmodifier() { return m_post; }
	//. Returns the type of the parameter
	Type::Type* type() { return m_type; }
	//. Returns the name of the parameter
	string name() { return m_name; }
	//. Returns the value of the parameter
	string value() { return m_value; }
    private:
	Mods m_pre, m_post;
	Type::Type* m_type;
	string m_name, m_value;
    };

    //. Function encapsulates a function declaration. Note that names may be
    //. stored in mangled form, and formatters should use realname() to get
    //. the unmangled version.
    class Function : public Declaration {
    public:
	//. The type of premodifiers
	typedef vector<string> Mods;

	//. Constructor
	Function(string, int, string, Name, Mods premod, Type::Type* ret, string realname);

	//. Accept the given visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Returns the premodifier vector
	Mods& premodifier() { return m_pre; }
	//. Returns the return Type
	Type::Type* returnType() { return m_ret; }
	//. Returns the real name of this function
	string realname() { return m_realname; }

	//. Returns the vector of parameters
	vector<Parameter*>& parameters() { return m_params; }
    private:
	//. The premodifier vector
	Mods m_pre;
	//. The return type
	Type::Type* m_ret;
	//. The real name
	string m_realname;
	//. The vector of parameters
	vector<Parameter*> m_params;
    };

    //. Operations are similar to functions but Not Quite Right
    class Operation : public Function {
    public:
	//. Constructor
	Operation(string, int, string, Name, Mods, Type::Type*, string);

	//. Accept the given visitor
	virtual void accept(Visitor*);
    };

    //. Encapsulate one Comment
    class Comment {
    public:
	//. Constructor
	Comment(string file, int line, string text);

	//. Returns the filename of this declaration
	string filename() const { return m_filename; }

	//. Returns the line number of this declaration
	int line() const { return m_line; }

    private:
	//. The filename
	string m_filename;
	//. The first line number
	int m_line;
    };

    //. The Visitor for the AST hierarchy. This class is just an interface
    //. really. It is abstract, and you must reimplement any methods you want.
    //. The default implementations of the methods call the visit methods for
    //. the subclasses of the visited type, eg visitNamespace calls visitScope
    //. which calls visitDeclaration.
    class Visitor {
    public:
	// Abstract destructor makes the class abstract
	virtual ~Visitor() = 0;
	virtual void visitDeclaration(Declaration*);
	virtual void visitScope(Scope*);
	virtual void visitNamespace(Namespace*);
	virtual void visitClass(Class*);
	virtual void visitInheritance(Inheritance*);
	virtual void visitForward(Forward*);
	virtual void visitTypedef(Typedef*);
	virtual void visitVariable(Variable*);
	virtual void visitEnum(Enum*);
	virtual void visitEnumerator(Enumerator*);
	virtual void visitFunction(Function*);
	virtual void visitOperation(Operation*);
	virtual void visitParameter(Parameter*);
    };

} // namespace AST

#endif // header guard
