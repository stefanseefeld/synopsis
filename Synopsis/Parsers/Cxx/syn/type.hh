//
// Copyright (C) 2002 Stephen Davies
// Copyright (C) 2002 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef type_hh_
#define type_hh_

#include <vector>
#include <string>
#include <typeinfo>

#include "common.hh"

// Forward declare AST::Declaration
namespace AST
{
class Declaration;
class Parameter;
}

//. The Type hierarchy
namespace Types
{
// Forward declaration of the Types::Visitor class defined in this file
class Visitor;

// Forward decl of Named
class Named;

//. The base class of the Type hierarchy
class Type : public cleanup
{
public:
    //. A vector of Type objects
    typedef std::vector<Type*> vector;

    //. Typedef for modifier list
    typedef std::vector<std::string> Mods;

    //. Constructor
    Type();

    //. Destructor. Note that Types::Type becomes abstract, unlike AST::Declaration
    virtual ~Type();

    //. Accept the given visitor
    virtual void accept(Visitor*);
};

//. Base class of types with Names
class Named : public Type
{
public:
    //. A vector of Types::Named objects
    typedef std::vector<Types::Named*> vector;

    //. Constructor
    Named(const ScopedName& name);

    //. Destructor
    ~Named();

    //. Accept the given visitor
    virtual void accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Constant version of name()
    const ScopedName& name() const
    {
        return m_name;
    }

    //. Return the scoped ScopedName of this type
    ScopedName& name()
    {
        return m_name;
    }

private:
    //. The scoped ScopedName of this type
    ScopedName m_name;
};


//. Base types are the (implicitly declared) builtin types such as 'int', 'bool' etc
class Base : public Named
{
public:
    //. Constructor
    Base(const ScopedName& name);

    //. Accept the given visitor
    virtual void accept(Visitor*);

};


//. Unknown type
class Unknown : public Named
{
public:
    //. Constructor
    Unknown(const ScopedName& name);
    //. Accept the given visitor
    virtual void accept(Visitor*);
};


//. Template parameter dependent types have a possibly scoped name, but no
//. known type. Member types of template-parameters will be scoped, eg:
//.
//. template<class T> class A { T::B b; }
//.
//. Here T::B is a depandant type, and so is T.
class Dependent : public Named
{
public:
    //. Constructor
    Dependent(const ScopedName& name);
    //. Accept the given visitor
    virtual void accept(Visitor*);
};



//. Declared types have a name and a reference to their declaration
class Declared : public Named
{
public:
    //. Constructor
    Declared(const ScopedName& name, AST::Declaration* decl);
    //. Accept the given visitor
    virtual void accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Const version of declaration()
    const AST::Declaration* declaration() const
    {
        return m_decl;
    }

    //. Returns the Declaration referenced by this type
    AST::Declaration* declaration()
    {
        return m_decl;
    }

private:
    //. The declaration referenced by this type
    AST::Declaration* m_decl;
};


//. Template types are declared template types. They have a name, a
//. declaration (which is an AST::Class) and a vector of parameters
//. declare this template. Each parameter (using AST::Parameter) should be
//. either the correct type for non-type parameters, or a Dependent for type
//. parameters. In either case, there may be default values.
class Template : public Declared
{
public:
    //. A vector of Types::Template objects
    typedef std::vector<Types::Template*> vector;

    //. A vector of Parameter objects
    typedef std::vector<AST::Parameter*> param_vector;

    //. Constructor
    Template(const ScopedName& name , AST::Declaration* decl, const param_vector& params);

    //. Accept the given visitor
    virtual void accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Constant version of parameters()
    const param_vector& parameters() const
    {
        return m_params;
    }

    //. Returns the vector of parameter Types
    param_vector& parameters()
    {
        return m_params;
    }

    //. Constant version of specializations()
    const Template::vector& specializations() const
    {
        return m_specs;
    }

    //. Returns the vector of specializations for this template.
    //. If this template is a specialization, the vector must be empty
    Template::vector& specializations()
    {
        return m_specs;
    }

private:
    //. The parameters
    param_vector m_params;

    //. The vector of specializations for this template
    Template::vector m_specs;
};

//. Type Modifier. This type has a nested type, and wraps it with
//. modifiers such as const, reference, etc.
class Modifier : public Type
{
public:
    //. Constructor
    Modifier(Type* alias, const Mods& pre, const Mods& post);

    //. Destructor
    ~Modifier();

    //. Accept the given visitor
    virtual void accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Returns the alias (modified) Type object
    Type* alias()
    {
        return m_alias;
    }

    //. Returns the premodifiers
    Mods& pre()
    {
        return m_pre;
    }

    //. Returns the postmodifiers
    Mods& post()
    {
        return m_post;
    }

private:
    //. The alias type
    Type* m_alias;
    //. The pre and post modifiers
    Mods  m_pre, m_post;
};

//. Type Array. This type adds array dimensions to a primary type
class Array : public Type
{
public:
    //. Constructor
    Array(Type* alias, const Mods& sizes);

    //. Destructor
    ~Array();

    //. Accept the given visitor
    virtual void accept(Visitor*);

    //. Returns the alias (modified) Type object
    Type* alias()
    {
        return m_alias;
    }

    //. Returns the sizes
    Mods& sizes()
    {
        return m_sizes;
    }
private:
    //. The alias type
    Type* m_alias;
    //. The sizes
    Mods m_sizes;
};


//. Parameterized type. A parameterized type is an instantiation of a
//. Template type with concrete Types as parameters.
class Parameterized : public Type
{
public:
    //. Constructor
    Parameterized(Named* templ, const Type::vector& params);

    //. Destructor
    ~Parameterized();

    //. Accept the given visitor
    virtual void accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Returns the Template type this is an instance of
    Named* template_id()
    {
        return m_template;
    }

    //. Constant version of parameters()
    const Type::vector& parameters() const
    {
        return m_params;
    }

    //. Returns the vector of parameter Types
    Type::vector& parameters()
    {
        return m_params;
    }

private:
    //. The Template object, either a Template or a Dependent.
    Named* m_template;
    //. The vector of parameter Types
    Type::vector m_params;
};


//. Function Pointer type. Function ptr types have a return type and a
//. list of parameter types, along with a list of premodifiers.
class FuncPtr : public Type
{
public:
    //. Constructor
    FuncPtr(Type* ret, const Mods& premods, const Type::vector& params);

    //. Destructor
    ~FuncPtr();

    //. Accept the given visitor
    virtual void accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Returns the Return Type object
    Type* return_type()
    {
        return m_return;
    }

    //. Const version of pre()
    const Mods& pre() const
    {
        return m_premod;
    }

    //. Returns the premodifier vector
    Mods& pre()
    {
        return m_premod;
    }

    //. Constant version of parameters()
    const Type::vector& parameters() const
    {
        return m_params;
    }

    //. Returns the vector of parameter Types
    Type::vector& parameters()
    {
        return m_params;
    }

private:
    //. The Return Type
    Type*        m_return;
    //. The premodifiers
    Mods         m_premod;
    //. The parameters
    Type::vector m_params;
};


//. The Visitor base class
class Visitor
{
public:
    // Virtual destructor makes abstract
    virtual ~Visitor() = 0;
    virtual void visit_type(Type*);
    virtual void visit_unknown(Unknown*);
    virtual void visit_modifier(Modifier*);
    virtual void visit_array(Array*);
    virtual void visit_named(Named*);
    virtual void visit_base(Base*);
    virtual void visit_dependent(Dependent*);
    virtual void visit_declared(Declared*);
    virtual void visit_template_type(Template*);
    virtual void visit_parameterized(Parameterized*);
    virtual void visit_func_ptr(FuncPtr*);
};

//. The exception thrown by the special cast templates
class wrong_type_cast : public std::exception
{
public:
    //. Constructor
    wrong_type_cast() throw ()
    {}
    //. Destructor
    virtual ~wrong_type_cast() throw ()
    {}
    //. Returns name of this class
    virtual const char* what() const throw();
};

//. Casts Types::Type types to derived types safely. The cast is done using
//. dynamic_cast, and wrong_type_cast is thrown upon failure.
template <typename T>
T* type_cast(Type* type_ptr) throw (wrong_type_cast)
{
    if (T* derived_ptr = dynamic_cast<T*>(type_ptr))
        return derived_ptr;
    throw wrong_type_cast();
}

//. Casts Types::Named types to derived types safely. The cast is done using
//. dynamic_cast, and wrong_type_cast is thrown upon failure.
template <typename T>
T* named_cast(Named* named_ptr) throw (wrong_type_cast)
{
    if (T* derived_ptr = dynamic_cast<T*>(named_ptr))
        return derived_ptr;
    throw wrong_type_cast();
}

//. Safely extracts typed Declarations from Named types. The type is first
//. safely cast to Types::Declared, then the declaration() safely cast to
//. the template type.
template <typename T>
T* declared_cast(Named* named_ptr) throw (wrong_type_cast)
{
    if (Declared* declared_ptr = dynamic_cast<Declared*>(named_ptr))
        if (AST::Declaration* decl = declared_ptr->declaration())
            if (T* derived_ptr = dynamic_cast<T*>(decl))
                return derived_ptr;
    throw wrong_type_cast();
}

//. Safely extracts typed Declarations from Type types. The type is first
//. safely cast to Types::Declared, then the declaration() safely cast to
//. the template type.
template <typename T>
T* declared_cast(Type* type_ptr) throw (wrong_type_cast)
{
    if (Declared* declared_ptr = dynamic_cast<Declared*>(type_ptr))
        if (AST::Declaration* decl = declared_ptr->declaration())
            if (T* derived_ptr = dynamic_cast<T*>(decl))
                return derived_ptr;
    throw wrong_type_cast();
}


} // namespace Type

#endif
// vim: set ts=8 sts=4 sw=4 et:
