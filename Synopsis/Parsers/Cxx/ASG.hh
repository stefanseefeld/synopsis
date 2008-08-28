//
// Copyright (C) 2002 Stephen Davies
// Copyright (C) 2002 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef ASG_hh_
#define ASG_hh_

#include <set>
#include <map>
#include <string>
#include "QName.hh"
#include "FakeGC.hh"

// Forward declare Dictionary
class Dictionary;

// Forward declare Types::Type, Declared and Template
namespace Types
{
class Type;
class Named;
class Declared;
class Template;
}

//. A namespace for the ASG hierarchy
namespace ASG
{
// Forward declaration of ASG::Visitor defined in this file
class Visitor;

// Forward declaration of ASG::SourceFile defined in this file
class SourceFile;

//. An enumeration of accessability specifiers
enum Access
{
    Default = 0,
    Public,
    Protected,
    Private
};

//FIXME: move to somewhere else
//. A struct to hold cross-reference info
struct Reference
{
    std::string file;
    int         line;
    QName  scope;
    std::string context;

    // Foundation
    Reference()
            : line(-1)
    { }
    Reference(const std::string& _file, int _line, const QName& _scope, const std::string& _context)
            : file(_file), line(_line), scope(_scope), context(_context)
    { }
    Reference(const Reference& other)
            : file(other.file), line(other.line), scope(other.scope), context(other.context)
    { }

    Reference& operator=(const Reference& other)
    {
        file = other.file;
        line = other.line;
        scope = other.scope;
        context = other.context;
        return *this;
    }
};

//. Encapsulation of one Comment, which may span multiple lines.
//. Each comment encapsulates one /* */ block or a block of // comments on
//. adjacent lines. If extract_tails is set, then comments will be added
//. even when they are not adjacent to a declaration - these comments will be
//. marked as "suspect". Most of these will be discarded by the Linker, unless
//. they have appropriate markings such as "//.< comment for previous decl"
class Comment : public FakeGC::LightObject
{
public:
    //. A vector of Comments
    typedef std::vector<std::string> vector;

    //. Constructor
    Comment(SourceFile* file, int line, const std::string& text, bool suspect=false);

    //
    // Attributes
    //

    //. Returns the filename of this comment
    SourceFile* file() const
    {
        return m_file;
    }

    //. Returns the line number of the start of this comment
    int line() const
    {
        return m_line;
    }

    //. Returns the text of this comment
    const std::string& text() const
    {
        return m_text;
    }

    //. Sets whether this comment is suspicious
    void set_suspect(bool suspect)
    {
        m_suspect = suspect;
    }

    //. Returns whether this comment is suspicious
    bool is_suspect() const
    {
        return m_suspect;
    }

private:
    //. The file
    SourceFile* m_file;
    //. The first line number
    int         m_line;
    //. The text
    std::string m_text;
    //. True if this comment is probably not needed. The exception is comments
    //. which will be used as "tails", eg: //.< comment for previous decl
    bool        m_suspect;
};


//. The base class of the Declaration hierarchy.
//. All declarations have a scoped Name, comments, etc. The filename and
//. type name are constant strings. This is enforced so that the strings
//. will reference the same data, saving both memory and cpu time. For this
//. to work however, you must be careful to use the same strings for
//. constructing the names from, for example from a dictionary.
class Declaration : public FakeGC::LightObject
{
public:
    //. A vector of Declaration objects
    typedef std::vector<Declaration*> vector;

    //. Constructor
    Declaration(SourceFile* file, int line, const std::string& type, const QName& name);

    //. Destructor. Recursively deletes the comments for this declaration
    virtual
    ~Declaration();

    //. Accept the given ASG::Visitor
    virtual void
    accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Returns the scoped name of this declaration
    QName& name()
    {
        return m_name;
    }

    //. Constant version of name()
    const QName& name() const
    {
        return m_name;
    }

    //. Returns the filename of this declaration
    SourceFile* file() const
    {
        return m_file;
    }

    //. Changes the filename of this declaration
    void set_file(SourceFile* file)
    {
	m_file = file;
    }

    //. Returns the line number of this declaration
    int line() const
    {
        return m_line;
    }

    //. Returns the name of the type (not class) of this declaration
    const std::string& type() const
    {
        return m_type;
    }

    //. Change the type name. Currently only used to indicate template
    //. types
    void set_type(const std::string& type)
    {
        m_type = type;
    }

    //. Returns the accessability of this declaration
    Access access() const
    {
        return m_access;
    }

    //. Sets the accessability of this declaration
    void set_access(Access axs)
    {
        m_access = axs;
    }

    //. Constant version of comments()
    const Comment::vector& comments() const
    {
        return m_comments;
    }

    //. Returns the vector of comments. The vector returned is the private
    //. member vector of this Declaration, so modifications will affect the
    //. Declaration.
    Comment::vector& comments()
    {
        return m_comments;
    }

    //. Return a cached Types::Declared for this Declaration. It is created
    //. on demand and returned every time you call the method on this
    //. object.
    Types::Declared* declared();

    //. Return a cached Types::Declared for this Declaration. It is created
    //. on demand and returned every time you call the method on this
    //. object. This is the const version.
    const Types::Declared* declared() const;

private:
    //. The filename
    SourceFile*     m_file;
    //. The first line number
    int             m_line;
    //. The string type name
    std::string     m_type;
    //. The scoped name
    QName      m_name;
    //. The vector of Comment objects
    Comment::vector m_comments;
    //. The accessability spec
    Access          m_access;
    //. The Types::Declared cache
    mutable Types::Declared* m_declared;
};

//. Information about an #include or #include_next directive.
//. This object is a thin wrapper around the target SourceFile, with some
//. attributes to indicate whether the directive included a macro expansion,
//. and whether it was an #include_next or not.
//.
//. A macro expansion is flagged because often you don't want to include those
//. in an include graph. For example, some headers in the Boost PP library
//. will include other files multiple times, where the file included is given
//. by a macro. It is rare that you actually want to show this macro-dependent
//. include in the documentation or in a graph.
class Include : public FakeGC::LightObject
{
public:
    //. A vector of Includes
    typedef std::vector<Include*> vector;

    //. Constructor
    Include(SourceFile* target, bool is_macro, bool is_next);

    //. Returns the target of this include
    SourceFile* target() const
    {
	return m_target;
    }

    //. Returns whether the include filename was a macro expansion
    bool is_macro() const
    {
	return m_is_macro;
    }

    //. Returns whether the include was an #include_next directive
    bool is_next() const
    {
	return m_is_next;
    }

private:
    //. The target file of the include or include_next
    SourceFile* m_target;

    //. Whether the include filename was a macro expansion
    bool m_is_macro;

    //. Whether the include was an #include_next directive
    bool m_is_next;
};

//. Information about a source file used to generate the ASG.
//.
//. Generally an ASG will include SourceFile objects for *all* files,
//. including headers, that were used. The difference is that the main files
//. (those named to the parser) are flagged as "main", and others will not
//. have lists of declarations.
class SourceFile : public FakeGC::LightObject
{
public:
  //. A vector of SourceFiles
  typedef std::vector<SourceFile*> vector;

  //. Constructor
  SourceFile(std::string const &name, std::string const &abs_name, bool is_primary)
    : name_(name), abs_name_(abs_name), is_primary_(is_primary) {}

  std::string const &name() const { return name_;}
  const std::string& abs_name() const { return abs_name_;}
  bool is_primary() const { return is_primary_;}
  Declaration::vector& declarations() { return declarations_;}
  const Declaration::vector& declarations() const { return declarations_;}
  Include::vector& includes() { return includes_;}
  const Include::vector& includes() const { return includes_;}

  void add_macro_call(char const *name, int line, int start_col, int end_col, int offset);

  //. Map a position in the preprocessed file to a position in the original file.
  //. If the position happens to fall right into an expanded macro, -1 is returned.
  int map_column(int line, int col);
  
private:
  struct MacroCall
  {
    MacroCall(char const *n, int s, int e, int o)
      : name(n), start(s), end(e), offset(o) {}
    std::string name;
    // start column of the expanded macro
    long start;
    // end column of the expanded macro
    long end;
    // offset of all subsequent tokens in the stream, caused by this expansion.
    long offset;
    bool operator <(const MacroCall &o) const { return start < o.start;}
  };
  typedef std::set<MacroCall> Line;
  typedef std::map<long, Line> Lines;

  std::string name_;
  std::string abs_name_;
  bool is_primary_;
  Declaration::vector declarations_;
  Include::vector includes_;
  Lines macro_calls_;
};

//. A Builtin is a node to be used internally.
//. Right now it's being used to capture comments
//. at the end of a scope.
class Builtin : public Declaration
{
public:
    Builtin(SourceFile* file, int line, const std::string &type, const QName& name);

    //. Destructor
    virtual ~Builtin();

    //. Accepts the given visitor
    virtual void accept(Visitor*);
};

//. Encapsulates a preprocessor macro. Macros are stored in the ASG, but since
//. they are not regular C++ syntax they are treated specially: They will be
//. in order compared to other macros, but not to the rest of the ASG since
//. the preprocessing stage occurs first. They will always be in the global
//. scope. Note that the parameters is a pointer to a vector - if the pointer
//. is null, then the macro is not function-like. If the pointer is non-null
//. then it points to a vector of parameter names. If the macro is
//. function-like but with no parameters, it is a pointer to an empty vector.
class Macro : public Declaration
{
public:
    //. The type of the parameters
    typedef std::vector<std::string> Parameters;

    //. Constructor. Assumes ownership of the Parameters vector if it is not a
    //. null pointer.
    Macro(SourceFile* file, int line, const QName& name, Parameters* params, const std::string& text);

    //. Destructor
    virtual ~Macro();

    //. The parameters of the macro. May be a null pointer if the macro is not
    //. function-like
    const Parameters* parameters() const
    {
	return m_parameters;
    }

    //. The expansion text of the macro.
    const std::string& text() const
    {
	return m_text;
    }

    //. Accepts the given visitor
    virtual void accept(Visitor*);

private:
    //. The parameters
    Parameters* m_parameters;

    //. The expansion text
    std::string m_text;
};

//. Base class for scopes with contained declarations. Each scope has its
//. own Dictionary of names so far accumulated for this scope. Each scope
//. also as a complete vector of scopes where name lookup is to proceed if
//. unsuccessful in this scope. Name lookup is not recursive.
class Scope : public Declaration
{
public:
    //. Constructor
    Scope(SourceFile* file, int line, const std::string& type, const QName& name);

    //. Destructor.
    //. Recursively destroys contained declarations
    virtual ~Scope();

    //. Accepts the given visitor
    virtual void accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Constant version of declarations()
    const Declaration::vector& declarations() const
    {
        return m_declarations;
    }

    //. Returns the vector of declarations. The vector returned is the
    //. private member vector of this Scope, so modifications will affect
    //. the Scope.
    Declaration::vector& declarations()
    {
        return m_declarations;
    }

private:
    //. The vector of contained declarations
    Declaration::vector m_declarations;
};


//. Namespace class
class Namespace : public Scope
{
public:
    //. Constructor
    Namespace(SourceFile* file, int line, const std::string& type, const QName& name);
    //. Destructor
    virtual
    ~Namespace();

    //. Accepts the given ASG::Visitor
    virtual void
    accept(Visitor*);
}
; // class Namespace


//. Inheritance class. This class encapsulates the information about an
//. inheritance, namely its accessability. Note that classes inherit from
//. types, not class declarations. As such it's possible to inherit from a
//. parameterized type, or a declared typedef or class/struct.
class Inheritance
{
public:
    //. A vector of Inheritance objects
    typedef std::vector<Inheritance*> vector;

    //. A typedef of the Attributes type
    typedef std::vector<std::string> Attributes;

    //. Constructor
    Inheritance(Types::Type* parent, const Attributes& attributes);

    //. Accepts the given ASG::Visitor
    void accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Returns the Class object this inheritance refers to. The method
    //. returns a Type since typedefs to classes are preserved to
    //. enhance readability of the generated docs. Note that the parent
    //. may also be a non-declaration type, such as vector<int>
    Types::Type* parent()
    {
        return m_parent;
    }

    //. Returns the attributes of this inheritance
    const Attributes& attributes() const
    {
        return m_attrs;
    }

private:
    //. The parent class or typedef to class
    Types::Type* m_parent;
    //. The attributes
    Attributes  m_attrs;
};

//. Forward declaration. Currently this has no extra attributes.
class Forward : public Declaration
{
public:
  //. Constructor
  Forward(SourceFile* file, int line, const std::string& type, const QName& name,
          bool is_template_specialization);

  //. Accepts the given ASG::Visitor
  virtual void accept(Visitor*);

  Types::Template* template_id() { return template_;}
  void set_template_id(Types::Template* id) { template_ = id;}
  bool is_template_specialization() const { return is_template_specialization_;}

private:
  //. The Template Type for this forward if it's a template
  Types::Template* template_;
  bool is_template_specialization_;
};


class ClassTemplate;

//. Class class
class Class : public Scope
{
public:
  Class(SourceFile* file, int line, const std::string& type, const QName& name,
        bool is_specialization);

  virtual ~Class();

  virtual void accept(Visitor*);

  //. Returns the vector of parent Inheritance objects. The vector
  //. returned is the private member vector of this Class, so
  //. modifications will affect the Class.
  const Inheritance::vector& parents() const { return parents_;}
  Inheritance::vector& parents() { return parents_;}

  bool is_template_specialization() const { return is_template_specialization_;}

private:
  //. The vector of parent Inheritance objects
  Inheritance::vector parents_;
  bool is_template_specialization_;
};

//. Class class
class ClassTemplate : public Class
{
public:
  ClassTemplate(SourceFile* file, int line, const std::string& type, const QName& name,
                bool is_specialization);

  virtual ~ClassTemplate();

  virtual void accept(Visitor*);

  Types::Template* template_id() { return template_;}
  void set_template_id(Types::Template* type) { template_ = type;}

private:
  Types::Template* template_;
};


//. Typedef declaration
class Typedef : public Declaration
{
public:
    //. Constructor
    Typedef(SourceFile* file, int line, const std::string& type, const QName& name, Types::Type* alias, bool constr);

    //. Destructor
    ~Typedef();

    //. Accepts the given ASG::Visitor
    virtual void accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Returns the Type object this typedef aliases
    Types::Type* alias()
    {
        return m_alias;
    }

    //. Returns true if the Type object was constructed inside the typedef
    bool constructed()
    {
        return m_constr;
    }

private:
    //. The alias Type
    Types::Type* m_alias;

    //. True if constructed
    bool         m_constr;
};


//. Variable declaration
class Variable : public Declaration
{
public:
    //. The type of the vector of sizes
    typedef std::vector<size_t> Sizes;

    //. Constructor
    Variable(SourceFile* file, int line, const std::string& type, const QName& name, Types::Type* vtype, bool constr);

    //. Destructor
    ~Variable();

    //. Accepts the given ASG::Visitor
    virtual void accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Returns the Type object of this variable
    Types::Type* vtype() const
    {
        return m_vtype;
    }

    //. Returns true if the Type object was constructed inside the variable
    bool constructed() const
    {
        return m_constr;
    }

    //. Returns the array sizes vector
    Sizes& sizes()
    {
        return m_sizes;
    }

private:
    //. The variable Type
    Types::Type* m_vtype;

    //. True if constructed
    bool         m_constr;

    //. Vector of array sizes. zero length indicates not an array.
    Sizes        m_sizes;
};


//. Enumerator declaration. This is a name with a value in the containing
//. scope. Enumerators only appear inside Enums via their enumerators()
//. attribute.
class Enumerator : public Declaration
{
public:
    //. Type of a vector of Enumerator objects
    typedef std::vector<Enumerator*> vector;

    //. Constructor
    Enumerator(SourceFile* file, int line, const std::string& type, const QName& name, const std::string& value);

    //. Accept the given ASG::Visitor
    virtual void accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Returns the value of this enumerator
    const std::string& value() const
    {
        return m_value;
    }

private:
    //. The value of this enumerator
    std::string m_value;
};


//. Enum declaration. An enum contains multiple enumerators.
class Enum : public Declaration
{
public:
    //. Constructor
    Enum(SourceFile* file, int line, const std::string& type, const QName& name);
    //. Destructor. Recursively destroys Enumerators
    ~Enum();

    //. Accepts the given ASG::Visitor
    virtual void
    accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Returns the vector of Enumerators
    Enumerator::vector& enumerators()
    {
        return m_enums;
    }

private:
    //. The vector of Enumerators
    Enumerator::vector m_enums;
};


//. A const is a name with a value and declared type.
class Const : public Declaration
{
public:
    //. Constructor
    Const(SourceFile* file, int line, const std::string& type, const QName& name, Types::Type* ctype, const std::string& value);

    //. Accept the given ASG::Visitor
    virtual void accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Returns the Type object of this const
    Types::Type* ctype()
    {
        return m_ctype;
    }

    //. Returns the value of this enumerator
    const std::string& value() const
    {
        return m_value;
    }

private:
    //. The const Type
    Types::Type* m_ctype;

    //. The value of this enumerator
    std::string m_value;
};


//. Parameter encapsulates one parameter to a function
class Parameter : public FakeGC::LightObject
{
public:
    //. The type of modifiers such as 'in', 'out'
    typedef std::vector<std::string> Mods;

    //. A vector of Parameter objects
    typedef std::vector<Parameter*> vector;

    //. Constructor
    Parameter(const Mods& pre, Types::Type* type, const Mods& post, const std::string& name, const std::string& value);

    //. Destructor
    ~Parameter();

    //. Accept the given ASG::Visitor. Note this is not derived from
    //. Declaration so it is not a virtual method.
    void accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Returns the premodifier
    Mods& premodifier()
    {
        return m_pre;
    }

    //. Returns the postmodifier
    Mods& postmodifier()
    {
        return m_post;
    }

    //. Returns the type of the parameter
    Types::Type* type()
    {
        return m_type;
    }

    //. Const version of type()
    const Types::Type* type() const
    {
        return m_type;
    }

    //. Returns the name of the parameter
    const std::string& name() const
    {
        return m_name;
    }

    //. Returns the value of the parameter
    const std::string& value() const
    {
        return m_value;
    }

    //. Sets the name of the parameter
    void set_name(const std::string& name)
    {
        m_name = name;
    }
private:
    Mods        m_pre, m_post;
    Types::Type* m_type;
    std::string m_name, m_value;
};


//. Function encapsulates a function declaration. Note that names may be
//. stored in mangled form, and formatters should use realname() to get
//. the unmangled version. If this is a function template, use the
//. template_type() method to get at the template type
class Function : public Declaration
{
public:
    //. The type of premodifiers
    typedef std::vector<std::string> Mods;

    //. A vector of Function objects
    typedef std::vector<Function*> vector;

    //. Constructor
    Function(
        SourceFile* file, int line, const std::string& type, const QName& name,
        const Mods& premod, Types::Type* ret, const Mods& postmod, const std::string& realname
    );

    //. Destructor. Recursively destroys parameters
    ~Function();

    //. Accept the given visitor
    virtual void accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Returns the premodifier vector
    Mods& premodifier()
    {
        return m_pre;
    }

    //. Returns the postmodifier vector
    Mods& postmodifier()
    {
        return m_post;
    }

    //. Returns the return Type
    Types::Type* return_type()
    {
        return m_ret;
    }

    //. Returns the real name of this function
    const std::string& realname() const
    {
        return m_realname;
    }

    //. Returns the vector of parameters
    Parameter::vector& parameters()
    {
        return m_params;
    }

    //. Returns the Template object if this is a template
    Types::Template* template_id()
    {
        return m_template;
    }

    //. Sets the Template object for this class. 0 means not a template
    void set_template_id(Types::Template* type)
    {
        m_template = type;
    }
private:
    //. The premodifier vector
    Mods              m_pre;
    //. The return type
    Types::Type*      m_ret;
    //. The postmodifier vector
    Mods              m_post;
    //. The real (unmangled) name
    std::string       m_realname;
    //. The vector of parameters
    Parameter::vector m_params;
    //. The Template Type for this class if it's a template
    Types::Template*  m_template;
};


//. Operations are similar to functions but Not Quite Right
class Operation : public Function
{
public:
    //. Constructor
    Operation(SourceFile* file, int line, const std::string& type, const QName& name, const Mods& premod, Types::Type* ret, const Mods& postmod, const std::string& realname);

    //. Accept the given visitor
    virtual void
    accept(Visitor*);
};

class UsingDirective : public Declaration
{
public:
  UsingDirective(SourceFile* file, int line, const QName& name)
    : Declaration(file, line, "using namespace", name) {}
  virtual void accept(Visitor*);
};

class UsingDeclaration : public Declaration
{
public:
  UsingDeclaration(SourceFile* file, int line, QName const& name, Types::Named *d);

  Types::Named* target() { return m_target;}
  virtual void accept(Visitor*);
private:
  Types::Named* m_target;
};

//. The Visitor for the ASG hierarchy. This class is just an interface
//. really. It is abstract, and you must reimplement any methods you want.
//. The default implementations of the methods call the visit methods for
//. the subclasses of the visited type, eg visit_namespace calls visit_scope
//. which calls visit_declaration.
class Visitor
{
public:
    // Abstract destructor makes the class abstract
    virtual ~Visitor() = 0;
    virtual void visit_declaration(Declaration*);
    virtual void visit_builtin(Builtin*);
    virtual void visit_macro(Macro*);
    virtual void visit_scope(Scope*);
    virtual void visit_namespace(Namespace*);
    virtual void visit_class(Class*);
    virtual void visit_class_template(ClassTemplate*);
    virtual void visit_inheritance(Inheritance*);
    virtual void visit_forward(Forward*);
    virtual void visit_typedef(Typedef*);
    virtual void visit_variable(Variable*);
    virtual void visit_const(Const*);
    virtual void visit_enum(Enum*);
    virtual void visit_enumerator(Enumerator*);
    virtual void visit_function(Function*);
    virtual void visit_operation(Operation*);
    virtual void visit_parameter(Parameter*);
    virtual void visit_using_directive(UsingDirective*);
    virtual void visit_using_declaration(UsingDeclaration*);
};

} // namespace ASG

#endif // header guard
