// $Id: Declaration.hh,v 1.3 2004/01/25 21:21:54 stefan Exp $
//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_AST_Declaration_hh
#define _Synopsis_AST_Declaration_hh

#include <Synopsis/Object.hh>
#include <Synopsis/AST/Visitor.hh>
#include <Synopsis/AST/Type.hh>

namespace Synopsis
{
namespace AST
{

enum Access
{
  DEFAULT = 0,
  PUBLIC = 1,
  PROTECTED = 2,
  PRIVATE = 3
};

class Declaration;
typedef TypedList<Declaration> Declarations;

class SourceFile : public Object
{
public:
  SourceFile() {}
  SourceFile(const Object &o) : Object(o) {}
  std::string name() const { return narrow<std::string>(attr("filename")());}
  std::string long_name() const { return narrow<std::string>(attr("full_filename")());}
  bool is_main() const { return narrow<bool>(attr("is_main")());}
  void is_main(bool flag) { attr("set_is_main")(Tuple(flag));}
  List includes() { return attr("includes")();}
  Dict macro_calls() { return attr("macro_calls")();}
  Declarations declarations();
};

//. Encapsulation of one Comment, which may span multiple lines.
//. Each comment encapsulates one /* */ block or a block of // comments on
//. adjacent lines. If extract_tails is set, then comments will be added
//. even when they are not adjacent to a declaration - these comments will be
//. marked as "suspect". Most of these will be discarded by the Linker, unless
//. they have appropriate markings such as "//.< comment for previous decl"
class Comment : public Object
{
public:
  Comment() {}
  Comment(const Object &o, bool check = true)
    : Object(o) { if (check) assert_type("Synopsis.AST", "Comment");}

  SourceFile file() const { return narrow<SourceFile>(attr("file")());}
  long line() const { return narrow<long>(attr("line")());}
  std::string text() const { return narrow<std::string>(attr("text")());}
  void suspect(bool flag) { attr("set_suspect")(Tuple(flag));}
  bool suspect() const { return narrow<bool>(attr("is_suspect")());}
};

class Declaration : public Object
{
public:
  Declaration() {}
  Declaration(const Object &o, bool check = true)
    : Object(o) { if (check) assert_type("Declaration");}

  SourceFile file() const { return narrow<SourceFile>(attr("file")());}
  long line() const { return narrow<long>(attr("line")());}
  std::string language() const { return narrow<std::string>(attr("language")());}
  std::string type() const { return narrow<std::string>(attr("type")());}
  ScopedName name() const { return attr("name")();}
  List comments() { return attr("comments")();}
  Access accessibility() const 
  { return static_cast<Access>(narrow<long>(attr("accessibility")()));}
  void accessibility(Access a) const 
  { attr("accessibility")(Tuple(static_cast<long>(a)));}

  virtual void accept(Visitor *v) { v->visit_declaration(this);}

  void assert_type(const char *type) { Object::assert_type("Synopsis.AST", type);}
};

inline Declarations SourceFile::declarations()
{ return narrow<Declarations>(attr("declarations")());}

//. A Builtin is a node to be used internally.
//. Right now it's being used to capture comments
//. at the end of a scope.
class Builtin : public Declaration
{
public:
  Builtin() {}
  Builtin(const Object &o, bool check = true)
    : Declaration(o, false) { if (check) assert_type("Builtin");}

  virtual void accept(Visitor *v) { v->visit_builtin(this);}
};

class Macro : public Declaration
{
public:
  Macro() {}
  Macro(const Object &o, bool check = true)
    : Declaration(o, false) { if (check) assert_type("Macro");}

  List parameters() { return attr("parameters")();}
  std::string text() { return narrow<std::string>(attr("text")());}

  virtual void accept(Visitor *v) { v->visit_macro(this);}
};

//. Forward declaration. Currently this has no extra attributes.
class Forward : public Declaration
{
public:
  Forward() {}
  Forward(const Object &o, bool check = true)
    : Declaration(o, false) { if (check) assert_type("Forward");}
  
//   //. Returns the Template object if this is a template
//   Template template_type() {}
  
//   //. Sets the Template object for this class. NULL means not a template
//   void set_template_type(Types::Template* type);

  virtual void accept(Visitor *v) { v->visit_forward(this);}
};

//. Base class for scopes with contained declarations. Each scope has its
//. own Dictionary of names so far accumulated for this scope. Each scope
//. also as a complete vector of scopes where name lookup is to proceed if
//. unsuccessful in this scope. Name lookup is not recursive.
class Scope : public Declaration
{
public:
  Scope() {}
  Scope(const Object &o, bool check = true)
    : Declaration(o, false) { if (check) assert_type("Scope");}

  List declarations() const { return attr("declarations")();}

  virtual void accept(Visitor *v) { v->visit_scope(this);}
};

//. Module class
class Module : public Scope
{
public:
  Module() {}
  Module(const Object &o, bool check = true)
    : Scope(o, false) { if (check) assert_type("Module");}

  virtual void accept(Visitor *v) { v->visit_module(this);}
};

//. Inheritance class. This class encapsulates the information about an
//. inheritance, namely its accessability. Note that classes inherit from
//. types, not class declarations. As such it's possible to inherit from a
//. parameterized type, or a declared typedef or class/struct.
class Inheritance : public Object
{
public:
  Inheritance() {}
  Inheritance(const Object &o, bool check = true)
    : Object(o) { if (check) assert_type("Synopsis.AST", "Inheritance");}

  //. Returns the Class object this inheritance refers to. The method
  //. returns a Type since typedefs to classes are preserved to
  //. enhance readability of the generated docs. Note that the parent
  //. may also be a non-declaration type, such as vector<int>
  Type parent() const { return attr("parent")();}

  //. Returns the attributes of this inheritance
  List attributes() const { return attr("attributes")();}

  void accept(Visitor *v) { v->visit_inheritance(this);}
};

//. Class class
class Class : public Scope
{
public:
  Class() {}
  Class(const Object &o, bool check = true)
    : Scope(o, false) { if (check) assert_type("Class");}

  //. Constant version of parents()
  List parents() const { return attr("parents")();}

//   //. Returns the Template object if this is a template
//   Template template_type() { return attr("template_type")();}

//   //. Sets the Template object for this class. NULL means not a template
//   void set_template_type(Types::Template* type)
//   {
//     m_template = type;
//   }

  virtual void accept(Visitor *v) { v->visit_class(this);}
};

//. Typedef declaration
class Typedef : public Declaration
{
public:
  Typedef() {}
  Typedef(const Object &o, bool check = true)
    : Declaration(o, false) { if (check) assert_type("Typedef");}

  //. Returns the Type object this typedef aliases
  Type alias() const { return attr("alias")();}
  //. Returns true if the Type object was constructed inside the typedef
  bool constructed() const { return narrow<bool>(attr("constr")());}

  virtual void accept(Visitor *v) { v->visit_typedef(this);}
};

//. Enumerator declaration. This is a name with a value in the containing
//. scope. Enumerators only appear inside Enums via their enumerators()
//. attribute.
class Enumerator : public Declaration
{
public:
  Enumerator() {}
  Enumerator(const Object &o, bool check = true)
    : Declaration(o, false) { if (check) assert_type("Enumerator");}

  //. Returns the value of this enumerator
  std::string value() const { return narrow<std::string>(attr("value")());}

  virtual void accept(Visitor *v) { v->visit_enumerator(this);}
};

typedef TypedList<Enumerator> Enumerators;

//. Enum declaration. An enum contains multiple enumerators.
class Enum : public Declaration
{
public:
  Enum() {}
  Enum(const Object &o, bool check = true)
    : Declaration(o, false) { if (check) assert_type("Enum");}

  //. Returns the vector of Enumerators
  List enumerators() { return attr("enumerators")();}

  virtual void accept(Visitor *v) { v->visit_enum(this);}
};

//. Variable declaration
class Variable : public Declaration
{
public:
  Variable() {}
  Variable(const Object &o, bool check = true)
    : Declaration(o, false) { if (check) assert_type("Variable");}

  //. Returns the Type object of this variable
  Type vtype() const { return attr("vtype")();}
  //. Returns true if the Type object was constructed inside the variable
  bool constructed() const { return attr("constr")();}
  //. Returns the array sizes vector
  List sizes() const { return attr("sizes")();}

  virtual void accept(Visitor *v) { v->visit_variable(this);}
};

//. A const is a name with a value and declared type.
class Const : public Declaration
{
public:
  Const() {}
  Const(const Object &o, bool check = true)
    : Declaration(o, false) { if (check) assert_type("Const");}

  //. Returns the Type object of this const
  Type ctype() const { return attr("ctype")();}
  //. Returns the value of this enumerator
  std::string value() const { return narrow<std::string>(attr("value")());}

  virtual void accept(Visitor *v) { v->visit_const(this);}
};

class Parameter : public Object
{
public:
  Parameter() {}
  Parameter(const Object &o, bool check = true)
    : Object(o) { if (check) assert_type("Synopsis.AST", "Parameter");}

  Modifiers premodifiers() const { return narrow<Modifiers>(attr("premodifiers")());}
  Modifiers postmodifiers() const { return narrow<Modifiers>(attr("postmodifiers")());}
  Type type() const { return narrow<Type>(attr("type")());}
  std::string name() const { return narrow<std::string>(attr("identifier")());}
  std::string value() const { return narrow<std::string>(attr("value")());}

  virtual void accept(Visitor *v) { v->visit_parameter(this);}
};

//. Function encapsulates a function declaration. Note that names may be
//. stored in mangled form, and formatters should use real_name() to get
//. the unmangled version. If this is a function template, use the
//. template_type() method to get at the template type
class Function : public Declaration
{
public:
  typedef TypedList<Parameter> Parameters;

  Function() {}
  Function(const Object &o, bool check = true)
    : Declaration(o, false) { if (check) assert_type("Function");}

//   //. The type of premodifiers
//   typedef std::vector<std::string> Mods;

  //. Returns the return Type
  Type return_type() const { return attr("returnType")();}

  //. Returns the real name of this function
  ScopedName real_name() const { return attr("realname")();}

  //. Returns the vector of parameters
  Parameters parameters() const { return attr("parameters")();}

  //. Returns the Template object if this is a template
//   Types::Template* template_type()
//   {
//     return m_template;
//   }

  //. Sets the Template object for this class. NULL means not a template
//   void set_template_type(Types::Template* type)
//   {
//     m_template = type;
//   }

  //. Accept the given visitor
  virtual void accept(Visitor *v) { v->visit_function(this);}
};

//. Operations are similar to functions but Not Quite Right
class Operation : public Function
{
public:
  Operation() {}
  Operation(const Object &o, bool check = true)
    : Function(o, false) { if (check) assert_type("Operation");}

  virtual void accept(Visitor *v) { v->visit_operation(this);}
};

}
}

#endif