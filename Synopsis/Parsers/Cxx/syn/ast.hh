// vim: set ts=8 sts=2 sw=2 et:
// File: ast.h
// A C++ class hierarchy that more or less mirrors the AST hierarchy in
// Python/Core.AST.

#ifndef H_SYNOPSIS_CPP_AST
#define H_SYNOPSIS_CPP_AST

#include "common.hh"

// Forward declare Dictionary
class Dictionary;

// Forward declare Types::Type, Declared and Template
namespace Types
{ class Type; class Declared; class Template; }

//. A namespace for the AST hierarchy
namespace AST
{
  // Forward declaration of AST::Visitor defined in this file
  class Visitor;

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
    ScopedName  scope;
    std::string context;

    // Foundation
    Reference()
    : line(-1)
    { }
    Reference(const std::string& _file, int _line, const ScopedName& _scope, const std::string& _context)
    : file(_file), line(_line), scope(_scope), context(_context)
    { }
    Reference(const Reference& other)
    : file(other.file), line(other.line), scope(other.scope), context(other.context)
    { }
    Reference&
    operator=(const Reference& other)
    {
      file = other.file; line = other.line; scope = other.scope; context = other.context;
      return *this;
    }
  };

  //. Encapsulation of one Comment, which may span multiple lines.
  //. Each comment encapsulates one /* */ block or a block of // comments on
  //. adjacent lines. If extract_tails is set, then comments will be added
  //. even when they are not adjacent to a declaration - these comments will be
  //. marked as "suspect". Most of these will be discarded by the Linker, unless
  //. they have appropriate markings such as "//.< comment for previous decl"
  class Comment : public cleanup
  {
  public:
    //. A vector of Comments
    typedef std::vector<Comment*> vector;

    //. Constructor
    Comment(const std::string& file, int line, const std::string& text, bool suspect=false);

    //
    // Attributes
    //

    //. Returns the filename of this comment
    const std::string&
    filename() const { return m_filename; }

    //. Returns the line number of the start of this comment
    int
    line() const { return m_line; }

    //. Returns the text of this comment
    const std::string&
    text() const { return m_text; }

    //. Sets whether this comment is suspicious
    void set_suspect(bool suspect) { m_suspect = suspect; }
    
    //. Returns whether this comment is suspicious
    bool is_suspect() const { return m_suspect; }

  private:
    //. The filename
    std::string m_filename;
    //. The first line number
    int         m_line;
    //. The text
    std::string m_text;
    //. True if this comment is probably not needed. The exception is comments
    //. which will be used as "tails", eg: //.< comment for previous decl
    bool        m_suspect;
  }; // class Comment


  //. The base class of the Declaration hierarchy.
  //. All declarations have a scoped Name, comments, etc. The filename and
  //. type name are constant strings. This is enforced so that the strings
  //. will reference the same data, saving both memory and cpu time. For this
  //. to work however, you must be careful to use the same strings for
  //. constructing the names from, for example from a dictionary.
  class Declaration : public cleanup
  {
  public:
    //. A vector of Declaration objects
    typedef std::vector<Declaration*> vector;

    //. Constructor
    Declaration(const std::string& filename, int line, const std::string& type, const ScopedName& name);
    
    //. Destructor. Recursively deletes the comments for this declaration
    virtual
    ~Declaration();

    //. Accept the given AST::Visitor
    virtual void
    accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Returns the scoped name of this declaration
    ScopedName&
    name() { return m_name; }
    
    //. Constant version of name()
    const ScopedName&
    name() const { return m_name; }

    //. Returns the filename of this declaration
    const std::string&
    filename() const { return m_filename; }

    //. Returns the line number of this declaration
    int
    line() const { return m_line; }

    //. Returns the name of the type (not class) of this declaration
    const std::string&
    type() const { return m_type; }

    //. Change the type name. Currently only used to indicate template
    //. types
    void
    set_type(const std::string& type) { m_type = type; }
    
    //. Returns the accessability of this declaration
    Access
    access() const { return m_access; }

    //. Sets the accessability of this declaration
    void
    set_access(Access axs) { m_access = axs; }

    //. Constant version of comments()
    const Comment::vector&
    comments() const { return m_comments; }

    //. Returns the vector of comments. The vector returned is the private
    //. member vector of this Declaration, so modifications will affect the
    //. Declaration.
    Comment::vector&
    comments() { return m_comments; }
    
    //. Return a cached Types::Declared for this Declaration. It is created
    //. on demand and returned every time you call the method on this
    //. object.
    Types::Declared*
    declared();

    //. Return a cached Types::Declared for this Declaration. It is created
    //. on demand and returned every time you call the method on this
    //. object. This is the const version.
    const Types::Declared*
    declared() const;

  private:
    //. The filename
    std::string     m_filename;
    //. The first line number
    int             m_line;
    //. The string type name
    std::string     m_type;
    //. The scoped name
    ScopedName      m_name;
    //. The vector of Comment objects
    Comment::vector m_comments;
    //. The accessability spec
    Access          m_access;
    //. The Types::Declared cache
    mutable Types::Declared* m_declared;
  }; // class Declaration


  //. Base class for scopes with contained declarations. Each scope has its
  //. own Dictionary of names so far accumulated for this scope. Each scope
  //. also as a complete vector of scopes where name lookup is to proceed if
  //. unsuccessful in this scope. Name lookup is not recursive.
  class Scope : public Declaration
  {
  public:
    //. Constructor
    Scope(const std::string& file, int line, const std::string& type, const ScopedName& name);

    //. Destructor. 
    //. Recursively destroys contained declarations
    virtual
    ~Scope();

    //. Accepts the given visitor
    virtual void
    accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Constant version of declarations()
    const Declaration::vector&
    declarations() const { return m_declarations; }

    //. Returns the vector of declarations. The vector returned is the
    //. private member vector of this Scope, so modifications will affect
    //. the Scope.
    Declaration::vector&
    declarations() { return m_declarations; }

  private:
    //. The vector of contained declarations
    Declaration::vector m_declarations;
  }; // class Scope

  
  //. Namespace class
  class Namespace : public Scope
  {
  public:
    //. Constructor
    Namespace(const std::string& file, int line, const std::string& type, const ScopedName& name);
    //. Destructor
    virtual
    ~Namespace();

    //. Accepts the given AST::Visitor
    virtual void
    accept(Visitor*);
  }; // class Namespace

  
  //. Inheritance class. This class encapsulates the information about an
  //. inheritance, namely its accessability. Note that classes inherit from
  //. types, not class declarations. As such it's possible to inherit from a
  //. parameterized type, or a declared typedef or class/struct.
  class Inheritance {
  public:
    //. A vector of Inheritance objects
    typedef std::vector<Inheritance*> vector;

    //. A typedef of the Attributes type
    typedef std::vector<std::string> Attributes;

    //. Constructor
    Inheritance(Types::Type* parent, const Attributes& attributes);

    //. Accepts the given AST::Visitor
    void
    accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Returns the Class object this inheritance refers to. The method
    //. returns a Type since typedefs to classes are preserved to
    //. enhance readability of the generated docs. Note that the parent
    //. may also be a non-declaration type, such as vector<int>
    Types::Type*
    parent() { return m_parent; }

    //. Returns the attributes of this inheritance
    const Attributes&
    attributes() const { return m_attrs; }

  private:
    //. The parent class or typedef to class
    Types::Type* m_parent;
    //. The attributes
    Attributes  m_attrs;
  }; // class Inheritance


  //. Class class
  class Class : public Scope
  {
  public:
    //. Constructor
    Class(const std::string& file, int line, const std::string& type, const ScopedName& name);

    //. Destructor. Recursively destroys Inheritance objects
    virtual
    ~Class();

    //. Accepts the given AST::Visitor
    virtual void
    accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Constant version of parents()
    const Inheritance::vector&
    parents() const { return m_parents; }

    //. Returns the vector of parent Inheritance objects. The vector
    //. returned is the private member vector of this Class, so
    //. modifications will affect the Class.
    Inheritance::vector&
    parents() { return m_parents; }

    //. Returns the Template object if this is a template
    Types::Template*
    template_type() { return m_template; }

    //. Sets the Template object for this class. NULL means not a template
    void
    set_template_type(Types::Template* type) { m_template = type; }

  private:
    //. The vector of parent Inheritance objects
    Inheritance::vector m_parents;
    //. The Template Type for this class if it's a template
    Types::Template*     m_template;
  }; // class Class

  
  //. Forward declaration. Currently this has no extra attributes.
  class Forward : public Declaration
  {
  public:
    //. Constructor
    Forward(const std::string& file, int line, const std::string& type, const ScopedName& name);
    //. Constructor that copies an existing declaration
    Forward(AST::Declaration* decl);

    //. Accepts the given AST::Visitor
    virtual void
    accept(Visitor*);
  }; // class Forward

  
  //. Typedef declaration
  class Typedef : public Declaration
  {
  public:
    //. Constructor
    Typedef(const std::string& file, int line, const std::string& type, const ScopedName& name, Types::Type* alias, bool constr);

    //. Destructor
    ~Typedef();

    //. Accepts the given AST::Visitor
    virtual void
    accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Returns the Type object this typedef aliases
    Types::Type*
    alias() { return m_alias; }

    //. Returns true if the Type object was constructed inside the typedef
    bool
    constructed() { return m_constr; }

  private:
    //. The alias Type
    Types::Type* m_alias;

    //. True if constructed
    bool        m_constr;
  }; // class Typedef

  
  //. Variable declaration
  class Variable : public Declaration {
  public:
    //. The type of the vector of sizes
    typedef std::vector<size_t> Sizes;

    //. Constructor
    Variable(const std::string& file, int line, const std::string& type, const ScopedName& name, Types::Type* vtype, bool constr);

    //. Destructor
    ~Variable();

    //. Accepts the given AST::Visitor
    virtual void
    accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Returns the Type object of this variable
    Types::Type*
    vtype() const { return m_vtype; }

    //. Returns true if the Type object was constructed inside the variable
    bool
    constructed() const { return m_constr; }

    //. Returns the array sizes vector
    Sizes& sizes() { return m_sizes; }

  private:
    //. The variable Type
    Types::Type* m_vtype;

    //. True if constructed
    bool        m_constr;

    //. Vector of array sizes. zero length indicates not an array.
    Sizes       m_sizes;
  }; // class Variable

  
  //. Enumerator declaration. This is a name with a value in the containing
  //. scope. Enumerators only appear inside Enums via their enumerators()
  //. attribute.
  class Enumerator : public Declaration
  {
  public:
    //. Type of a vector of Enumerator objects
    typedef std::vector<Enumerator*> vector;

    //. Constructor
    Enumerator(const std::string& file, int line, const std::string& type, const ScopedName& name, const std::string& value);
    
    //. Accept the given AST::Visitor
    virtual void
    accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Returns the value of this enumerator
    const std::string&
    value() const { return m_value; }

  private:
    //. The value of this enumerator
    std::string m_value;
  }; // class Enumerator
  

  //. Enum declaration. An enum contains multiple enumerators.
  class Enum : public Declaration
  {
  public:
    //. Constructor
    Enum(const std::string& file, int line, const std::string& type, const ScopedName& name);
    //. Destructor. Recursively destroys Enumerators
    ~Enum();

    //. Accepts the given AST::Visitor
    virtual void
    accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Returns the vector of Enumerators
    Enumerator::vector&
    enumerators() { return m_enums; }

  private:
    //. The vector of Enumerators
    Enumerator::vector m_enums;
  }; // class Enum


  //. A const is a name with a value and declared type.
  class Const : public Declaration
  {
  public:
    //. Constructor
    Const(const std::string& file, int line, const std::string& type, const ScopedName& name, Types::Type* ctype, const std::string& value);
    
    //. Accept the given AST::Visitor
    virtual void
    accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Returns the Type object of this const
    Types::Type*
    ctype() { return m_ctype; }

    //. Returns the value of this enumerator
    const std::string&
    value() const { return m_value; }

  private:
    //. The const Type
    Types::Type* m_ctype;

    //. The value of this enumerator
    std::string m_value;
  }; // class Const


  //. Parameter encapsulates one parameter to a function
  class Parameter : public cleanup
  {
  public:
    //. The type of modifiers such as 'in', 'out'
    typedef std::string Mods;

    //. A vector of Parameter objects
    typedef std::vector<Parameter*> vector;

    //. Constructor
    Parameter(const Mods& pre, Types::Type* type, const Mods& post, const std::string& name, const std::string& value);

    //. Destructor
    ~Parameter();

    //. Accept the given AST::Visitor. Note this is not derived from
    //. Declaration so it is not a virtual method.
    void
    accept(Visitor*);

    //
    // Attribute Methods
    //

    //. Returns the premodifier
    Mods& 
    premodifier() { return m_pre; }

    //. Returns the postmodifier
    Mods&
    postmodifier() { return m_post; }
    
    //. Returns the type of the parameter
    Types::Type*
    type() { return m_type; }

    //. Const version of type()
    const Types::Type*
    type() const { return m_type; }

    //. Returns the name of the parameter
    const std::string&
    name() const { return m_name; }

    //. Returns the value of the parameter
    const std::string&
    value() const { return m_value; }

    //. Sets the name of the parameter
    void
    set_name(const std::string& name) { m_name = name; }
  private:
    Mods        m_pre, m_post;
    Types::Type* m_type;
    std::string m_name, m_value;
  }; // class Parameter


  //. Function encapsulates a function declaration. Note that names may be
  //. stored in mangled form, and formatters should use realname() to get
  //. the unmangled version.
  class Function : public Declaration
  {
  public:
      //. The type of premodifiers
      typedef std::vector<std::string> Mods;

      //. A vector of Function objects
      typedef std::vector<Function*> vector;

      //. Constructor
      Function(
        const std::string& file, int line, const std::string& type, const ScopedName& name,
        const Mods& premod, Types::Type* ret, const std::string& realname
      );

      //. Destructor. Recursively destroys parameters
      ~Function();

      //. Accept the given visitor
      virtual void
      accept(Visitor*);

      //
      // Attribute Methods
      //

      //. Returns the premodifier vector
      Mods&
      premodifier() { return m_pre; }

      //. Returns the return Type
      Types::Type*
      return_type() { return m_ret; }

      //. Returns the real name of this function
      const std::string&
      realname() const { return m_realname; }

      //. Returns the vector of parameters
      Parameter::vector&
      parameters() { return m_params; }
  private:
      //. The premodifier vector
      Mods              m_pre;
      //. The return type
      Types::Type*       m_ret;
      //. The real (unmangled) name
      std::string       m_realname;
      //. The vector of parameters
      Parameter::vector m_params;
  }; // class Function
  

  //. Operations are similar to functions but Not Quite Right
  class Operation : public Function
  {
  public:
    //. Constructor
    Operation(const std::string& file, int line, const std::string& type, const ScopedName& name, const Mods& modifiers, Types::Type* ret, const std::string& realname);

    //. Accept the given visitor
    virtual void
    accept(Visitor*);
  }; // class Operation

  
  //. The Visitor for the AST hierarchy. This class is just an interface
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
    virtual void visit_scope(Scope*);
    virtual void visit_namespace(Namespace*);
    virtual void visit_class(Class*);
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
  }; // class Visitor

} // namespace AST

#endif // header guard
