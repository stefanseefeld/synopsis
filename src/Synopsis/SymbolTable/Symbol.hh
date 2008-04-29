//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_SymbolTable_Symbol_hh_
#define Synopsis_SymbolTable_Symbol_hh_

#include <Synopsis/PTree.hh>
#include <Synopsis/SymbolTable/Scope.hh>

namespace Synopsis
{
namespace SymbolTable
{

class Symbol;
class VariableName;
class ConstName;
class TypeName;
class TypedefName;
class ClassName;
class EnumName;
class ClassTemplateName;
class FunctionName;
class FunctionTemplateName;
class NamespaceName;
class DependentName;

class SymbolVisitor
{
public:
  virtual ~SymbolVisitor() {}

  virtual void visit(Symbol const *) {}
  virtual void visit(VariableName const *) {}
  virtual void visit(ConstName const *) {}
  virtual void visit(TypeName const *) {}
  virtual void visit(TypedefName const *) {}
  virtual void visit(ClassName const *) {}
  virtual void visit(EnumName const *) {}
  virtual void visit(DependentName const *) {}
  virtual void visit(ClassTemplateName const *) {}
  virtual void visit(FunctionName const *) {}
  virtual void visit(FunctionTemplateName const *) {}
  virtual void visit(NamespaceName const *) {}
};

// class Scope;
class Class;
class Namespace;
class FunctionScope;

class Symbol
{
public:
  Symbol(PTree::Encoding const &t, PTree::Node const *p, bool d, Scope *s)
    : type_(t), ptree_(p), definition_(d), scope_(s) {}
  virtual ~Symbol(){}
  virtual void accept(SymbolVisitor *v) const { v->visit(this);}
  PTree::Encoding const & type() const { return type_;}
  PTree::Encoding const & name() const { return scope_->reverse_lookup(this);}
  //  std::string qname() const { return scope_->name(this);}
  PTree::Node const * ptree() const { return ptree_;}
  bool is_definition() const { return definition_;}
  Scope * scope() const { return scope_;}
protected:
  PTree::Encoding     type_;
  PTree::Node const * ptree_;
  bool                definition_;
  Scope             * scope_;
};

class VariableName : public Symbol
{
public:
  VariableName(PTree::Encoding const &type, PTree::Node const *ptree,
	       bool def, Scope *s)
    : Symbol(type, ptree, def, s) {}
  virtual void accept(SymbolVisitor *v) const { v->visit(this);}
};

class ConstName : public VariableName
{
public:
  ConstName(PTree::Encoding const &type, long v,
	    PTree::Node const *ptree, bool def, Scope *s)
    : VariableName(type, ptree, def, s), defined_(true), value_(v) {}
  ConstName(PTree::Encoding const &type,
	    PTree::Node const *ptree, bool def, Scope *s)
    : VariableName(type, ptree, def, s), defined_(false) {}
  virtual void accept(SymbolVisitor *v) const { v->visit(this);}
  bool defined() const { return defined_;}
  long value() const { return value_;}
private:
  bool defined_;
  long value_;
};

class TypeName : public Symbol
{
public:
  TypeName(PTree::Encoding const &type, PTree::Node const *ptree,
	   bool def, Scope *s)
    : Symbol(type, ptree, def, s) {}
  virtual void accept(SymbolVisitor *v) const { v->visit(this);}
};

class TypedefName : public TypeName
{
public:
  TypedefName(PTree::Encoding const &type, PTree::Node const *ptree, Scope *scope,
	      Symbol const *aliased)
    : TypeName(type, ptree, false, scope), aliased_(aliased) {}
  virtual void accept(SymbolVisitor *v) const { v->visit(this);}
  //. The aliased symbol. This may be a TypeName, ClassTemplateName, or Dependent.
  Symbol const *aliased() const { return aliased_;}
private:
  Symbol const *aliased_;
};

class ClassName : public TypeName
{
public:
  ClassName(PTree::Encoding const &type, PTree::Node const *ptree, bool def, Scope *s)
    : TypeName(type, ptree, def, s) {}

  //. Rather than create a new symbol once a class definition is seen, this
  //. method allows to morph a class forward declaration into a class definition.
  void define(PTree::Node const *ptree) { ptree_ = ptree; definition_ = true;}

  virtual void accept(SymbolVisitor *v) const { v->visit(this);}

  //. True if `is_definition()` is false.
  bool is_forward() const { return !is_definition();}
  //. Return the class scope associated with this symbol.
  //. This will return 0 if the class definition hasn't been seen yet.
  Class *as_scope() const;
};

class EnumName : public TypeName
{
public:
  EnumName(PTree::Node const *ptree, Scope *scope)
    : TypeName(PTree::Encoding(), ptree, true, scope) {}

  //. Rather than create a new symbol once an enum definition is seen, this
  //. method allows to morph an enum forward declaration into an enum definition.
  void define(PTree::Node const *ptree) { ptree_ = ptree; definition_ = true;}

  virtual void accept(SymbolVisitor *v) const { v->visit(this);}

  //. True if `is_definition()` is false.
  bool is_forward() const { return !is_definition();}
};

class DependentName : public TypeName
{
public:
  DependentName(PTree::Node const *ptree, Scope *scope)
    : TypeName(PTree::Encoding(), ptree, false, scope) {}
  virtual void accept(SymbolVisitor *v) const { v->visit(this);}
};

class ClassTemplateName : public Symbol
{
public:
  ClassTemplateName(PTree::Encoding const &type, PTree::Node const *ptree, bool def,
		    Scope *s)
    : Symbol(type, ptree, def, s) {}
  virtual void accept(SymbolVisitor *v) const { v->visit(this);}

  //. Return the class scope associated with this symbol.
  //. This will return 0 if the class definition hasn't been seen yet.
  Class *as_scope() const;
};

class FunctionName : public Symbol
{
public:
  FunctionName(PTree::Encoding const &type, PTree::Node const *ptree,
	       size_t params, size_t default_args, bool def, Scope *s)
    : Symbol(type, ptree, def, s),
      params_(params),
      default_args_(default_args) 
  {}
  virtual void accept(SymbolVisitor *v) const { v->visit(this);}

  //. Return the function scope associated with this symbol.
  //. This will return 0 if the function definition hasn't been seen yet.
  FunctionScope *as_scope() const;

  size_t params() const { return params_;}
  size_t default_args() const { return default_args_;}

private:
  size_t params_;
  size_t default_args_;
};

class FunctionTemplateName : public Symbol
{
public:
  FunctionTemplateName(PTree::Encoding const &type, PTree::Node const *ptree,
		       size_t params, size_t default_args, bool def, Scope *s)
    : Symbol(type, ptree, def, s),
      params_(params),
      default_args_(default_args) {}
  virtual void accept(SymbolVisitor *v) const { v->visit(this);}

  //. Return the function scope associated with this symbol.
  //. This will return 0 if the function definition hasn't been seen yet.
  FunctionScope *as_scope() const;

  size_t params() const { return params_;}
  size_t default_args() const { return default_args_;}

private:
  size_t params_;
  size_t default_args_;
};

class NamespaceName : public Symbol
{
public:
  NamespaceName(PTree::Encoding const &type, PTree::Node const *ptree,
		bool def, Scope *s)
    : Symbol(type, ptree, def, s) {}
  virtual void accept(SymbolVisitor *v) const { v->visit(this);}

  //. Return the namespace scope associated with this symbol.
  //. This will return 0 if the namespace definition hasn't been seen yet.
  Namespace *as_scope() const;
};

extern TypeName const * const DEPENDENT;

}
}

#endif
