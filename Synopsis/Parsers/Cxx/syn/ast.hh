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
	Declaration(const string filename, int line, const string type, Name);
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
	const string filename() const { return m_filename; }

	//. Returns the line number of this declaration
	int line() const { return m_line; }

	// TODO: move to whatever classes need it. Not needed here!
	//. Returns the string type name of this declaration
	const string type() const { return m_type; }
	
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
	const string m_filename;
	//. The first line number
	int m_line;
	//. The string type name
	const string m_type;
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
	Scope(const string, int, const string, Name);
	//. Destructor. 
	//. Recursively destroys contained declarations
	virtual ~Scope();

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
	Namespace(const string, int, const string, Name);
	//. Destructor
	virtual ~Namespace();
    };

    // Forward declare Inheritance
    class Inheritance;

    //. Class class
    class Class : public Scope {
    public:
	//. Constructor
	Class(const string, int, const string, Name);
	//. Destructor. Recursively destroys Inheritance objects
	virtual ~Class();

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
    };

} // namespace AST

#endif // header guard
