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

// Forward declare Type::Type and Declared
namespace Type { class Type; class Declared; class Template; }

//. A namespace for the AST hierarchy
namespace AST {

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
    typedef std::vector<std::string> Name;

    //. The base class of the Declaration hierarchy.
    //. All declarations have a scoped Name, comments, etc. The filename and
    //. type name are constant strings. This is enforced so that the strings
    //. will reference the same data, saving both memory and cpu time. For this
    //. to work however, you must be careful to use the same strings for
    //. constructing the names from, for example from a dictionary.
    class Declaration {
    public:
	//. Constructor
	Declaration(const std::string &filename, int line, const std::string &type, const Name &);
	//. Destructor. Recursively deletes the comments for this declaration
	virtual ~Declaration();

	//. Accept the given AST::Visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Constant version of name()
	const Name &name() const { return m_name;}
	//. Returns the scoped name of this declaration
	Name &name() { return m_name;}

	//. Returns the filename of this declaration
	const std::string &filename() const { return m_filename; }

	//. Returns the line number of this declaration
	int line() const { return m_line; }

	// TODO: move to whatever classes need it. Not needed here!
	//. Returns the string type name of this declaration
	const std::string &type() const { return m_type; }

	//. Change the type string. Currently only used to indicate template
	//. types
	void setType(const std::string &type) { m_type = type; }
	
	//. Returns the accessability of this declaration
	Access access() const { return m_access; }

	//. Sets the accessability of this declaration
	void setAccess(Access axs) { m_access = axs; }

	//. Constant version of comments()
	const std::vector<Comment*>& comments() const { return m_comments; }
	//. Returns the vector of comments. The vector returned is the private
	//. member vector of this Declaration, so modifications will affect the
	//. Declaration.
	std::vector<Comment*>& comments() { return m_comments; }
	
	//. Return a cached Type::Declared for this Declaration. It is created
	//. on demand and returned every time you call the method on this
	//. object.
	Type::Declared* declared();

    private:
	//. The scoped name
	Name m_name;
	//. The filename
	std::string m_filename;
	//. The first line number
	int m_line;
	//. The string type name
	std::string m_type;
	//. The vector of Comment objects
	std::vector<Comment*> m_comments;
	//. The accessability spec
	Access m_access;
	//. The Type::Declared cache
	Type::Declared* m_declared;
    }; // class Declaration


    //. Base class for scopes with contained declarations. Each scope has its
    //. own Dictionary of names so far accumulated for this scope. Each scope
    //. also as a complete vector of scopes where name lookup is to proceed if
    //. unsuccessful in this scope. Name lookup is not recursive.
    class Scope : public Declaration {
    public:
	//. Constructor
	Scope(const std::string &, int, const std::string &, const Name &);
	//. Destructor. 
	//. Recursively destroys contained declarations
	virtual ~Scope();

	//. Accepts the given visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Constant version of declarations()
	const std::vector<Declaration*>& declarations() const { return m_declarations; }
	//. Returns the vector of declarations. The vector returned is the
	//. private member vector of this Scope, so modifications will affect
	//. the Scope.
	std::vector<Declaration*>& declarations() { return m_declarations; }

    private:
	//. The vector of contained declarations
	std::vector<Declaration*> m_declarations;
    }; // class Scope

    //. Namespace class
    class Namespace : public Scope {
    public:
	//. Constructor
	Namespace(const std::string &, int, const std::string &, const Name &);
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
	Class(const std::string &, int, const std::string &, const Name &);
	//. Destructor. Recursively destroys Inheritance objects
	virtual ~Class();

	//. Accepts the given AST::Visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Constant version of parents()
	const std::vector<Inheritance*>& parents() const { return m_parents; }
	//. Returns the vector of parent Inheritance objects. The vector
	//. returned is the private member vector of this Class, so
	//. modifications will affect the Class.
	std::vector<Inheritance*>& parents() { return m_parents; }

	//. Returns the Template object if this is a template
	Type::Template* templateType() { return m_template; }
	//. Sets the Template object for this class. NULL means not a template
	void setTemplateType(Type::Template* t) { m_template = t; }
    private:
	//. The vector of parent Inheritance objects
	std::vector<Inheritance*> m_parents;
	//. The Template Type for this class if it's a template
	Type::Template* m_template;
    };

    //. Inheritance class. This class encapsulates the information about an
    //. inheritance, namely its accessability. Note that classes inherit from
    //. types, not class declarations. As such it's possible to inherit from a
    //. parameterized type, or a declared typedef or class/struct.
    class Inheritance {
    public:
	//. A typedef of the Attributes type
	typedef std::vector<std::string> Attrs;

	//. Constructor
	Inheritance(Type::Type*, const Attrs&);

	//. Accepts the given AST::Visitor
	void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Returns the Class object this inheritance refers to. The method
	//. returns a Type since typedefs to classes are preserved to
	//. enhance readability of the generated docs. Note that the parent
	//. may also be a non-declaration type, such as vector<int>
	Type::Type* parent() { return m_parent; }

	//. Returns the attributes of this inheritance
	const Attrs& attributes() const { return m_attrs; }
    private:
	//. The attributes
	Attrs m_attrs;
	//. The parent class or typedef to class
	Type::Type* m_parent;
    };

    //. Forward declaration. Currently this has no extra attributes.
    class Forward : public Declaration {
    public:
	//. Constructor
	Forward(const std::string &, int, const std::string &, const Name &);
	//. Constructor that copies an existing declaration
	Forward(AST::Declaration*);

	//. Accepts the given AST::Visitor
	virtual void accept(Visitor*);
    };

    //. Typedef declaration
    class Typedef : public Declaration {
    public:
	//. Constructor
	Typedef(const std::string &, int, const std::string &, const Name &, Type::Type* alias, bool constr);

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
	Variable(const std::string &, int, const std::string &, const Name &, Type::Type* vtype, bool constr);

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
	std::vector<int>& sizes() { return m_sizes; }
    private:
	//. The variable Type
	Type::Type* m_vtype;

	//. True if constructed
	bool m_constr;

	//. Vector of array sizes. zero length indicates not an array.
	std::vector<int> m_sizes;
    };

    //. Enumerator declaration. This is a name with a value in the containing
    //. scope. Enumerators only appear inside Enums via their enumerators()
    //. attribute.
    class Enumerator : public Declaration {
    public:
	//. Constructor
	Enumerator(const std::string &, int, const std::string &, const Name &, const std::string &value);
	
	//. Accept the given AST::Visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Returns the value of this enumerator
	const std::string &value() const { return m_value; }
    private:
	//. The value of this enumerator
        std::string m_value;
    };

    //. Enum declaration. An enum contains multiple enumerators.
    class Enum : public Declaration {
    public:
	//. Constructor
	Enum(const std::string &, int, const std::string &, const Name &);
	//. Destructor. Recursively destroys Enumerators
	~Enum();

	//. Accepts the given AST::Visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Returns the vector of Enumerators
	std::vector<Enumerator*>& enumerators() { return m_enums; }
    private:
	//. The vector of Enumerators
	std::vector<Enumerator*> m_enums;
    };

    //. A const is a name with a value and declared type.
    class Const : public Declaration {
    public:
	//. Constructor
	Const(const std::string &, int, const std::string &, const Name &, Type::Type* ctype, const std::string &value);
	
	//. Accept the given AST::Visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Returns the Type object of this const
	Type::Type* ctype() { return m_ctype; }

	//. Returns the value of this enumerator
	const std::string &value() const { return m_value; }
    private:
	//. The const Type
	Type::Type* m_ctype;

	//. The value of this enumerator
        std::string m_value;
    };


    //. Parameter encapsulates one parameter to a function
    class Parameter {
    public:
	//. The type of modifiers such as 'in', 'out'
	typedef std::string Mods;

	//. Constructor
	Parameter(const Mods &pre, Type::Type*, const Mods &post, const std::string &name, const std::string &value);

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
	//. Const version of type()
	const Type::Type* type() const { return m_type; }
	//. Returns the name of the parameter
	const std::string &name() const { return m_name; }
	//. Returns the value of the parameter
	const std::string &value() const { return m_value; }

	//. Sets the name of the parameter
	void setName(const std::string &name) { m_name = name; }
    private:
	Mods m_pre, m_post;
	Type::Type* m_type;
	std::string m_name, m_value;
    };

    //. Function encapsulates a function declaration. Note that names may be
    //. stored in mangled form, and formatters should use realname() to get
    //. the unmangled version.
    class Function : public Declaration {
    public:
	//. The type of premodifiers
	typedef std::vector<std::string> Mods;

	//. Constructor
	Function(const std::string &, int, const std::string &, const Name &, const Mods &premod, Type::Type* ret, const std::string &realname);
	//. Destructor. Recursively destroys parameters
	~Function();

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
        const std::string &realname() const { return m_realname; }

	//. Returns the vector of parameters
	std::vector<Parameter*>& parameters() { return m_params; }
    private:
	//. The premodifier vector
	Mods m_pre;
	//. The return type
	Type::Type* m_ret;
	//. The real name
	std::string m_realname;
	//. The vector of parameters
	std::vector<Parameter*> m_params;
    };

    //. Operations are similar to functions but Not Quite Right
    class Operation : public Function {
    public:
	//. Constructor
	Operation(const std::string &, int, const std::string &, const Name &, const Mods &, Type::Type*, const std::string &);

	//. Accept the given visitor
	virtual void accept(Visitor*);
    };

    //. Encapsulate one Comment
    class Comment {
    public:
	//. Constructor
	Comment(const std::string &file, int line, const std::string &text);

	//. Returns the filename of this declaration
	const std::string &filename() const { return m_filename; }

	//. Returns the line number of this declaration
	int line() const { return m_line; }

	//. Returns the text of this comment
        const std::string &text() const { return m_text; }

    private:
	//. The filename
	std::string m_filename;
	//. The first line number
	int m_line;
	//. The test
	std::string m_text;
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
	virtual void visitConst(Const*);
	virtual void visitEnum(Enum*);
	virtual void visitEnumerator(Enumerator*);
	virtual void visitFunction(Function*);
	virtual void visitOperation(Operation*);
	virtual void visitParameter(Parameter*);
    };

} // namespace AST

#endif // header guard
