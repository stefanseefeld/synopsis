// File: type.hh
// This is a mirror of the Core.Type hierarchy, more or less

#ifndef H_SYNOPSIS_CPP_TYPE
#define H_SYNOPSIS_CPP_TYPE

#include <vector>
#include <string>
#include <typeinfo>

// Forward declare AST::Declaration
namespace AST { class Declaration; }

//. The Type hierarchy
namespace Type {

    // Import Name type
    typedef std::vector<std::string> Name;

    // Forward declaration of the Visitor class defined in this file
    class Visitor;

    //. The base class of the Type hierarchy
    class Type {
    public:
	//. A std::vector of Type pointers
	typedef std::vector<Type*> vector_t;

	//. Typedef for modifier list
	typedef std::vector<std::string> Mods;

	//. Constructor
	Type();
	//. Destructor
	virtual ~Type() = 0;
	//. Accept the given visitor
	virtual void accept(Visitor*);
    };

    //. Base class of types with Names
    class Named : public Type {
    public:
	//. Constructor
	Named(const Name &);
	//. Accept the given visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Constant version of name()
	const Name& name() const { return m_name; }
	//. Return the scoped Name of this type
	Name& name() { return m_name; }

    private:
	//. The scoped Name of this type
	Name m_name;
    };

    //. Base types are simple named types
    class Base : public Named {
    public:
	//. Constructor
	Base(const Name &);
	//. Accept the given visitor
	virtual void accept(Visitor*);
    };

    //. Unknown type
    class Unknown : public Named {
    public:
	//. Constructor
	Unknown(const Name &);
	//. Accept the given visitor
	virtual void accept(Visitor*);
    };

    //. Declared types have a name and a reference to their declaration
    class Declared : public Named {
    public:
	//. Constructor
	Declared(const Name &, AST::Declaration*);
	//. Accept the given visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Returns the Declaration referenced by this type
	AST::Declaration* declaration() { return m_decl; }
    private:
	//. The declaration referenced by this type
	AST::Declaration* m_decl;
    };

    //. Template types are declared template types. They have a name, a
    //. declaration (which is an AST::Class) and a vector of Types used to
    //. declare this template. Currently these must be Base types, but a
    //. future version may implement a special type for this purpose.
    class Template : public Declared {
    public:
	//. Constructor
	Template(const Name &, AST::Declaration*, const Type::vector_t& params);
	//. Accept the given visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Constant version of parameters()
	const Type::vector_t& parameters() const { return m_params; }
	//. Returns the vector of parameter Types
	Type::vector_t& parameters() { return m_params; }
    private:
	Type::vector_t m_params;
    };

    //. Type Modifier. This type has a nested type, and wraps it with
    //. modifiers such as const, reference, etc.
    class Modifier : public Type {
    public:
	//. Constructor
	Modifier(Type* alias, const Mods &pre, const Mods &post);
	//. Accept the given visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Returns the alias (modified) Type object
	Type* alias() { return m_alias; }

	//. Returns the premodifiers
	Mods &pre() { return m_pre; }

	//. Returns the postmodifiers
	Mods &post() { return m_post; }
    private:
	//. The alias type
	Type* m_alias;
	//. The pre and post modifiers
	Mods m_pre, m_post;
    };

    //. Type Array. This type adds array dimensions to a primary type
    class Array : public Type {
    public:
	//. Constructor
	Array(Type* alias, const Mods &sizes);
	//. Accept the given visitor
	virtual void accept(Visitor*);
	//. Returns the alias (modified) Type object
	Type *alias() { return m_alias; }
	//. Returns the sizes
	Mods &sizes() { return m_sizes;}
    private:
	//. The alias type
	Type* m_alias;
	//. The sizes
	Mods m_sizes;
    };

    //. Parameterized type. A parameterized type is an instantiation of a
    //. Template type with concrete Types as parameters. 
    class Parameterized : public Type {
    public:
	//. Constructor
	Parameterized(Template*, const Type::vector_t& params);
	//. Accept the given visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Returns the Template type this is an instance of
	Template* templateType() { return m_template; }

	//. Constant version of parameters()
	const Type::vector_t& parameters() const { return m_params; }
	//. Returns the vector of parameter Types
	Type::vector_t& parameters() { return m_params; }
    private:
	//. The Template object
	Template* m_template;
	//. The vector of parameter Types
	Type::vector_t m_params;
    };

    //. Function Pointer type. Function ptr types have a return type and a
    //. list of parameter types, along with a list of premodifiers.
    class FuncPtr : public Type {
    public:
	//. Constructor
	FuncPtr(Type* ret, const Mods &premods, const Type::vector_t& params);
	//. Accept the given visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Returns the Return Type object
	Type* returnType() { return m_return; }
	
	//. Const version of pre()
	const Mods& pre() const { return m_premod; }
	//. Returns the premodifier vector
	Mods& pre() { return m_premod; }
	
	//. Constant version of parameters()
	const Type::vector_t& parameters() const { return m_params; }
	//. Returns the vector of parameter Types
	Type::vector_t& parameters() { return m_params; }

    private:
	//. The Return Type
	Type* m_return;
	//. The premodifiers
	Mods m_premod;
	//. The parameters
	Type::vector_t m_params;
    };


    //. The Visitor base class
    class Visitor {
    public:
	// Virtual destructor makes abstract
	virtual ~Visitor() = 0;
	virtual void visitType(Type*);
	virtual void visitUnknown(Unknown*);
	virtual void visitModifier(Modifier*);
	virtual void visitArray(Array*);
	virtual void visitNamed(Named*);
	virtual void visitBase(Base*);
	virtual void visitDeclared(Declared*);
	virtual void visitTemplateType(Template*);
	virtual void visitParameterized(Parameterized*);
	virtual void visitFuncPtr(FuncPtr*);
    };

    //. The exception thrown by the special cast templates
    class wrong_type_cast : public std::exception {
    public:
	//. Constructor
	wrong_type_cast() throw () {}
	//. Destructor
	virtual ~wrong_type_cast() throw () {}
	//. Returns name of this class
	virtual const char* what() const throw();
    };

    //. Casts Type::Type types to derived types safely. The cast is done using
    //. dynamic_cast, and wrong_type_cast is thrown upon failure.
    template <class T>
    T* type_cast(Type* x) throw (wrong_type_cast) {
	if (T* ptr = dynamic_cast<T*>(x)) return ptr;
	throw wrong_type_cast();
    }

    //. Casts Type::Named types to derived types safely. The cast is done using
    //. dynamic_cast, and wrong_type_cast is thrown upon failure.
    template <class T>
    T* named_cast(Named* x) throw (wrong_type_cast) {
	if (T* ptr = dynamic_cast<T*>(x)) return ptr;
	throw wrong_type_cast();
    }

    //. Safely extracts typed Declarations from Named types. The type is first
    //. safely cast to Type::Declared, then the declaration() safely cast to
    //. the template type.
    template <class T>
    T* declared_cast(Named* x) throw (wrong_type_cast) {
	if (Declared* declared = dynamic_cast<Declared*>(x))
	    if (AST::Declaration* decl = declared->declaration())
		if (T* ptr = dynamic_cast<T*>(decl))
		    return ptr;
	throw wrong_type_cast();
    }

    //. Safely extracts typed Declarations from Type types. The type is first
    //. safely cast to Type::Declared, then the declaration() safely cast to
    //. the template type.
    template <class T>
    T* declared_cast(Type* x) throw (wrong_type_cast) {
	if (Declared* declared = dynamic_cast<Declared*>(x))
	    if (AST::Declaration* decl = declared->declaration())
		if (T* ptr = dynamic_cast<T*>(decl))
		    return ptr;
	throw wrong_type_cast();
    }


} // namespace Type

#endif
