//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_ASG_Visitor_hh
#define _Synopsis_ASG_Visitor_hh

namespace Synopsis
{
namespace ASG
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
class Operation;

//. The Visitor for the AST hierarchy. This class is just an interface
//. really. It is abstract, and you must reimplement any methods you want.
//. The default implementations of the methods call the visit methods for
//. the subclasses of the visited type, eg visit_namespace calls visit_scope
//. which calls visit_declaration.
class Visitor
{
public:
  // Abstract destructor makes the class abstract
  virtual ~Visitor() {}
  virtual void visit_declaration(const Declaration*) = 0;
  virtual void visit_builtin(const Builtin*) = 0;
  virtual void visit_macro(const Macro*) = 0;
  virtual void visit_scope(const Scope*) = 0;
  virtual void visit_module(const Module*) = 0;
  virtual void visit_class(const Class*) = 0;
  virtual void visit_inheritance(const Inheritance*) = 0;
  virtual void visit_forward(const Forward*) = 0;
  virtual void visit_typedef(const Typedef*) = 0;
  virtual void visit_variable(const Variable*) = 0;
  virtual void visit_const(const Const*) = 0;
  virtual void visit_enum(const Enum*) = 0;
  virtual void visit_enumerator(const Enumerator*) = 0;
  virtual void visit_parameter(const Parameter*) = 0;
  virtual void visit_function(const Function*) = 0;
  virtual void visit_operation(const Operation*) = 0;
};

class TypeId;
class UnknownTypeId;
class ModifierTypeId;
class ArrayTypeId;
class NamedTypeId;
class BuiltinTypeId;
class DependentTypeId;
class DeclaredTypeId;
class TemplateId;
class ParametrizedTypeId;
class FunctionTypeId;

//. The Type Visitor base class
class TypeIdVisitor
{
public:
  // Virtual destructor makes abstract
  virtual ~TypeIdVisitor() {}
  virtual void visit_type_id(const TypeId*) = 0;
  virtual void visit_named_type_id(const NamedTypeId*) = 0;
  virtual void visit_builtin_type_id(const BuiltinTypeId*) = 0;
  virtual void visit_dependent_type_id(const DependentTypeId*) = 0;
  virtual void visit_unknown_type_id(const UnknownTypeId*) = 0;
  virtual void visit_modifier_type_id(const ModifierTypeId*) = 0;
  virtual void visit_array_type_id(const ArrayTypeId*) = 0;
  virtual void visit_declared_type_id(const DeclaredTypeId*) = 0;
  virtual void visit_template_id(const TemplateId*) = 0;
  virtual void visit_parametrized_type_id(const ParametrizedTypeId*) = 0;
  virtual void visit_function_type_id(const FunctionTypeId*) = 0;
};

}
}

#endif
