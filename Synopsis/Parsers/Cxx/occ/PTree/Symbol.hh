//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _PTree_Symbol_hh
#define _PTree_Symbol_hh

#include <PTree/Encoding.hh>
#include <map>

namespace PTree
{

struct Symbol
{
  enum Type { NONE = 0, VARIABLE, CONST, TYPE};
  
  Symbol() : type(NONE), ptree(0) {}
  Symbol(Type t, Node *p) : type(t), ptree(p) {}
  
  Type  type;
  Node *ptree;
};

//. A Scope contains symbol definitions.
class Scope
{
public:
  virtual ~Scope();

  virtual const Scope *global() const { return this;}
  //. declare the given symbol in the local scope using the given encoded name.
  void declare(const Encoding &, const Symbol &);

  void declare(Declaration *);
  void declare(Typedef *);
  //. declare the enumeration as a new TYPE as well as all the enumerators as CONST
  void declare(EnumSpec *);
  //. declare the class as a new TYPE
  void declare(ClassSpec *);
  void declare(TemplateDecl *, ClassSpec *);
  void declare(TemplateDecl *, Node *);

  //. look up the encoded name and return the associated symbol, if found.
  virtual Symbol::Type lookup(const Encoding &, Symbol &) const;
  //. dump the content of the symbol table to a stream (for debugging).
  virtual void dump(std::ostream &) const;

  static PTree::Encoding get_base_name(const Encoding &enc, const Scope *&scope);

private:
  typedef std::map<Encoding, Symbol> SymbolTable;

  static int get_base_name_if_template(Encoding::iterator i, const Scope *&);
  static const Scope *lookup_typedef_name(Encoding::iterator, size_t, const Scope *);

  SymbolTable my_symbols;
};

//. a NestedScope has an outer Scope.
class NestedScope : public Scope
{
public:
  NestedScope(Scope *outer) : my_outer(outer) {}
  virtual const Scope *global() const { return my_outer->global();}
  virtual Symbol::Type lookup(const Encoding &, Symbol &) const;
  virtual void dump(std::ostream &) const;
private:
  Scope *my_outer;
};

}

#endif
