// $Id: Visitor.hh,v 1.1 2004/01/25 21:21:54 stefan Exp $
//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_AST_Visitor_hh
#define _Synopsis_AST_Visitor_hh

#include <Synopsis/AST/Declaration.hh>

namespace Synopsis
{
namespace AST
{

class Declaration;
class Builtin;
class Macro;
class Scope;
class Module;
class Class;
class Inheritance;
class Forward;
class Typedef;
class Variable;
class Const;
class Enumerator;
class Enum;
class Parameter;
class Function;

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
  virtual void visit_builtin(Builtin*);
  virtual void visit_macro(Macro*);
  virtual void visit_scope(Scope*);
  virtual void visit_module(Module*);
  virtual void visit_class(Class*);
  virtual void visit_inheritance(Inheritance*);
  virtual void visit_forward(Forward*);
  virtual void visit_typedef(Typedef*);
  virtual void visit_variable(Variable*);
  virtual void visit_const(Const*);
  virtual void visit_enum(Enum*);
  virtual void visit_enumerator(Enumerator*);
  virtual void visit_parameter(Parameter*);
  virtual void visit_function(Function*);
//   virtual void visit_operation(Operation*);
};

}
}

#endif
