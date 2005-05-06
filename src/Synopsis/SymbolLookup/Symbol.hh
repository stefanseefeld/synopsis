//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_SymbolLookup_Symbol_hh_
#define Synopsis_SymbolLookup_Symbol_hh_

#include <Synopsis/PTree/Encoding.hh>
#include <Synopsis/PTree/Lists.hh>
#include <Synopsis/SymbolLookup/Visitor.hh>

namespace Synopsis
{
namespace SymbolLookup
{

class Scope;

class Symbol
{
public:
  Symbol(PTree::Encoding const &t, PTree::Node const *p, bool def, Scope *s)
    : my_type(t), my_ptree(p), my_definition(def), my_scope(s) {}
  virtual ~Symbol(){}
  virtual void accept(Visitor *v) const { v->visit(this);}
  PTree::Encoding const & type() const { return my_type;}
  PTree::Node const * ptree() const { return my_ptree;}
  bool is_definition() const { return my_definition;}
  Scope * scope() const { return my_scope;}
private:
  PTree::Encoding     my_type;
  PTree::Node const * my_ptree;
  bool                my_definition;
  Scope             * my_scope;
};

class VariableName : public Symbol
{
public:
  VariableName(PTree::Encoding const &type, PTree::Node const *ptree,
	       bool def, Scope *s)
    : Symbol(type, ptree, def, s) {}
  virtual void accept(Visitor *v) const { v->visit(this);}
};

class ConstName : public VariableName
{
public:
  ConstName(PTree::Encoding const &type, long v,
	    PTree::Node const *ptree, bool def, Scope *s)
    : VariableName(type, ptree, def, s), my_defined(true), my_value(v) {}
  ConstName(PTree::Encoding const &type,
	    PTree::Node const *ptree, bool def, Scope *s)
    : VariableName(type, ptree, def, s), my_defined(false) {}
  virtual void accept(Visitor *v) const { v->visit(this);}
  bool defined() const { return my_defined;}
  long value() const { return my_value;}
private:
  bool my_defined;
  long my_value;
};

class TypeName : public Symbol
{
public:
  TypeName(PTree::Encoding const &type, PTree::Node const *ptree,
	   bool def, Scope *s)
    : Symbol(type, ptree, def, s) {}
  virtual void accept(Visitor *v) const { v->visit(this);}
};

class TypedefName : public TypeName
{
public:
  TypedefName(PTree::Encoding const &type, PTree::Node const *ptree, Scope *scope)
    : TypeName(type, ptree, false, scope) {}
  virtual void accept(Visitor *v) const { v->visit(this);}
};

class ClassName : public TypeName
{
public:
  ClassName(PTree::Encoding const &type, PTree::Node const *ptree, bool def, Scope *s)
    : TypeName(type, ptree, def, s) {}
  virtual void accept(Visitor *v) const { v->visit(this);}
};

class EnumName : public TypeName
{
public:
  EnumName(PTree::Encoding const &type, PTree::Node const *ptree, Scope *scope)
    : TypeName(type, ptree, true, scope) {}
  virtual void accept(Visitor *v) const { v->visit(this);}
};

class ClassTemplateName : public Symbol
{
public:
  ClassTemplateName(PTree::Encoding const &type, PTree::Node const *ptree, bool def,
		    Scope *s)
    : Symbol(type, ptree, def, s) {}
  virtual void accept(Visitor *v) const { v->visit(this);}
};

class FunctionName : public Symbol
{
public:
  FunctionName(PTree::Encoding const &type, PTree::Node const *ptree,
	       bool def, Scope *s)
    : Symbol(type, ptree, def, s) {}
  virtual void accept(Visitor *v) const { v->visit(this);}
};

class FunctionTemplateName : public Symbol
{
public:
  FunctionTemplateName(PTree::Encoding const &type, PTree::Node const *ptree, Scope *s)
    : Symbol(type, ptree, true, s) {}
  virtual void accept(Visitor *v) const { v->visit(this);}
};

class NamespaceName : public Symbol
{
public:
  NamespaceName(PTree::Encoding const &type, PTree::Node const *ptree,
		bool def, Scope *s)
    : Symbol(type, ptree, def, s) {}
  virtual void accept(Visitor *v) const { v->visit(this);}
};

}
}

#endif
