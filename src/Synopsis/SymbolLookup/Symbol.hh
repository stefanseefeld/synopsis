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

namespace Synopsis
{
namespace SymbolLookup
{

class Scope;

class Symbol
{
public:
  Symbol(const PTree::Encoding &t, PTree::Node *p, Scope const *s)
    : my_type(t), my_ptree(p), my_scope(s) {}
  virtual ~Symbol(){}
  PTree::Encoding const & type() const { return my_type;}
  PTree::Node const * ptree() const { return my_ptree;}
  Scope const * scope() const { return my_scope;}
private:
  PTree::Encoding     my_type;
  PTree::Node const * my_ptree;
  Scope const       * my_scope;
};

class VariableName : public Symbol
{
public:
  VariableName(const PTree::Encoding &type, PTree::Node *ptree, Scope const *scope)
    : Symbol(type, ptree, scope) {}
};

class ConstName : public Symbol
{
public:
  ConstName(const PTree::Encoding &type, long v, PTree::Node *ptree, Scope const *s)
    : Symbol(type, ptree, s), my_defined(true), my_value(v) {}
  ConstName(const PTree::Encoding &type, PTree::Node *ptree, Scope const *s)
    : Symbol(type, ptree, s), my_defined(false) {}
  bool defined() const { return my_defined;}
  long value() const { return my_value;}
private:
  bool my_defined;
  long my_value;
};

class TypeName : public Symbol
{
public:
  TypeName(const PTree::Encoding &type, PTree::Node *ptree, Scope const *scope)
    : Symbol(type, ptree, scope) {}
};

class TypedefName : public TypeName
{
public:
  TypedefName(const PTree::Encoding &type, PTree::Node *ptree, Scope const *scope)
    : TypeName(type, ptree, scope) {}
};

class ClassTemplateName : public Symbol
{
public:
  ClassTemplateName(const PTree::Encoding &type, PTree::Node *ptree, Scope const *s)
    : Symbol(type, ptree, s) {}
};

class FunctionName : public Symbol
{
public:
  FunctionName(const PTree::Encoding &type, PTree::Node *ptree, Scope const *s)
    : Symbol(type, ptree, s) {}
};

class FunctionTemplateName : public Symbol
{
public:
  FunctionTemplateName(const PTree::Encoding &type, PTree::Node *ptree, Scope const *s)
    : Symbol(type, ptree, s) {}
};

class NamespaceName : public Symbol
{
public:
  NamespaceName(const PTree::Encoding &type, PTree::Node *ptree, Scope const *s)
    : Symbol(type, ptree, s) {}
};

}
}

#endif
