//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _SymbolLookup_Symbol_hh
#define _SymbolLookup_Symbol_hh

#include <PTree/Encoding.hh>
#include <PTree/Lists.hh>

namespace SymbolLookup
{

class Symbol
{
public:
  Symbol(const PTree::Encoding &t, PTree::Node *p) : my_type(t), my_ptree(p) {}
  virtual ~Symbol(){}
  PTree::Encoding const & type() const { return my_type;}
  PTree::Node const * ptree() const { return my_ptree;}
private:
  PTree::Encoding my_type;
  PTree::Node const * my_ptree;
};

class VariableName : public Symbol
{
public:
  VariableName(const PTree::Encoding &type, PTree::Node *ptree) : Symbol(type, ptree) {}
};

class ConstName : public Symbol
{
public:
  ConstName(const PTree::Encoding &type, long v, PTree::Node *ptree)
    : Symbol(type, ptree), my_defined(true), my_value(v) {}
  ConstName(const PTree::Encoding &type, PTree::Node *ptree)
    : Symbol(type, ptree), my_defined(false) {}
  bool defined() const { return my_defined;}
  long value() const { return my_value;}
private:
  bool my_defined;
  long my_value;
};

class TypeName : public Symbol
{
public:
  TypeName(const PTree::Encoding &type, PTree::Node *ptree) : Symbol(type, ptree) {}
};

class ClassTemplateName : public Symbol
{
public:
  ClassTemplateName(const PTree::Encoding &type, PTree::Node *ptree)
    : Symbol(type, ptree) {}
};

class FunctionName : public Symbol
{
public:
  FunctionName(const PTree::Encoding &type, PTree::Node *ptree)
    : Symbol(type, ptree) {}
};

class FunctionTemplateName : public Symbol
{
public:
  FunctionTemplateName(const PTree::Encoding &type, PTree::Node *ptree)
    : Symbol(type, ptree) {}
};

class NamespaceName : public Symbol
{
public:
  NamespaceName(const PTree::Encoding &type, PTree::Node *ptree)
    : Symbol(type, ptree) {}
};

}

#endif
