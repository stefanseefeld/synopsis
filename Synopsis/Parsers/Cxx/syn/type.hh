// File: type.hh
// This is a mirror of the Core.Type hierarchy, more or less

#ifndef H_SYNOPSIS_CPP_TYPE
#define H_SYNOPSIS_CPP_TYPE

#include <vector.h>
#include <string>
using std::string;

// Forward declare AST::Declaration
namespace AST { class Declaration; }

//. The Type hierarchy
namespace Type {

    // Import Name type
    typedef vector<string> Name;

    // Forward declaration of the Visitor class defined in this file
    class Visitor;

    //. The base class of the Type hierarchy
    class Type {
    public:
	//. A vector of Type pointers
	typedef vector<Type*> vector_t;

	//. Typedef for modifier list
	typedef vector<string> Mods;

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
	Named(Name);
	//. Accept the given visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Constant version of name()
	const Name name() const { return m_name; }
	//. Return the scoped Name of this type
	Name name() { return m_name; }

    private:
	//. The scoped Name of this type
	Name m_name;
    };

    //. Base types are simple named types
    class Base : public Named {
    public:
	//. Constructor
	Base(Name);
	//. Accept the given visitor
	virtual void accept(Visitor*);
    };

    //. Unknown type
    class Unknown : public Named {
    public:
	//. Constructor
	Unknown(Name);
	//. Accept the given visitor
	virtual void accept(Visitor*);
    };

    //. Declared types have a name and a reference to their declaration
    class Declared : public Named {
    public:
	//. Constructor
	Declared(Name, AST::Declaration*);
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
	Template(Name, AST::Declaration*, Type::vector_t& params);
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
	Modifier(Type* alias, Mods& pre, Mods& post);
	//. Accept the given visitor
	virtual void accept(Visitor*);

	//
	// Attribute Methods
	//

	//. Returns the alias (modified) Type object
	Type* alias() { return m_alias; }

	//. Returns the premodifiers
	Mods& pre() { return m_pre; }

	//. Returns the postmodifiers
	Mods& post() { return m_post; }
    private:
	//. The alias type
	Type* m_alias;
	//. The pre and post modifiers
	Mods m_pre, m_post;
    };

    //. Parameterized type. A parameterized type is an instantiation of a
    //. Template type with concrete Types as parameters. 
    class Parameterized : public Type {
    public:
	//. Constructor
	Parameterized(Template*, Type::vector_t& params);
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
	FuncPtr(Type* ret, Mods& premods, Type::vector_t& params);
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
	virtual void visitNamed(Named*);
	virtual void visitBase(Base*);
	virtual void visitDeclared(Declared*);
	virtual void visitTemplateType(Template*);
	virtual void visitParameterized(Parameterized*);
	virtual void visitFuncPtr(FuncPtr*);
    };

} // namespace Type

#endif
