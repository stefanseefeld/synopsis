//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _Symbol_hh
#define _Symbol_hh

#include <PTree/Encoding.hh>
#include <map>

struct Symbol
{
  enum Type { NONE = 0, VARIABLE, CONST, TYPE};
  
  Symbol() : type(NONE), ptree(0) {}
  Symbol(Type t, PTree::Node *p) : type(t), ptree(p) {}
  
  Type         type;
  PTree::Node *ptree;
};

//. A Scope contains symbol definitions.
class Scope
{
public:
  virtual ~Scope();
  //. declare the given symbol in the local scope using the given encoded name.
  void declare(const PTree::Encoding &, const Symbol &);
  //. look up the encoded name and return the associated symbol, if found.
  virtual Symbol::Type lookup(const PTree::Encoding &, Symbol &) const;
private:
  typedef std::map<PTree::Encoding, Symbol> SymbolTable;
  SymbolTable my_symbols;
};

//. a NestedScope has an outer Scope.
class NestedScope : public Scope
{
public:
  NestedScope(Scope *outer) : my_outer(outer) {}
  virtual Symbol::Type lookup(const PTree::Encoding &, Symbol &) const;
private:
  Scope *my_outer;
};

#endif
