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

class Symbol
{
public:
  Symbol(const Encoding &t, Node *p) : my_type(t), my_ptree(p) {}
  virtual ~Symbol(){}
  const Encoding &type() const { return my_type;}
private:
  Encoding my_type;
  Node    *my_ptree;
};

class VariableName : public Symbol
{
public:
  VariableName(const Encoding &type, Node *ptree) : Symbol(type, ptree) {}
};

class ConstName : public Symbol
{
public:
  ConstName(const Encoding &type, long v, Node *ptree)
    : Symbol(type, ptree), my_value(v) {}
  long value() const { return my_value;}
private:
  long my_value;
};

class TypeName : public Symbol
{
public:
  TypeName(const Encoding &type, Node *ptree) : Symbol(type, ptree) {}
};

//. A Scope contains symbol definitions.
class Scope
{
public:
  struct TypeError
  {
    TypeError(const Encoding &t) : type(t) {}
    Encoding type;
  };

  virtual ~Scope();

  virtual const Scope *global() const { return this;}

  void declare(Declaration *);
  void declare(Typedef *);
  //. declare the enumeration as a new TYPE as well as all the enumerators as CONST
  void declare(EnumSpec *);
  //. declare the class as a new TYPE
  void declare(ClassSpec *);
  void declare(TemplateDecl *, ClassSpec *);
  void declare(TemplateDecl *, Node *);

  //. look up the encoded name and return the associated symbol, if found.
  virtual const Symbol *lookup(const Encoding &) const throw();

  //. same as the untyped lookup, but type safe. Throws a TypeError
  //. if the symbol exists but doesn't have the expected type.
  template <typename T>
  const T *lookup(const Encoding &name) const throw(TypeError)
  {
    const Symbol *symbol = lookup(name);
    if (!symbol) return 0;
    const T *t = dynamic_cast<const T *>(symbol);
    if (!t) throw TypeError(symbol->type());
    return t;
  }

  //. dump the content of the symbol table to a stream (for debugging).
  virtual void dump(std::ostream &) const;

  static PTree::Encoding get_base_name(const Encoding &enc, const Scope *&scope);

private:
  typedef std::map<Encoding, const Symbol *> SymbolTable;

  static int get_base_name_if_template(Encoding::iterator i, const Scope *&);
  static const Scope *lookup_typedef_name(Encoding::iterator, size_t, const Scope *);

  //. declare the given symbol in the local scope 
  //. using the given encoded name.
  void declare(const Encoding &name, const Symbol *symbol);

  SymbolTable my_symbols;
};

//. a NestedScope has an outer Scope.
class NestedScope : public Scope
{
public:
  NestedScope(Scope *outer) : my_outer(outer) {}
  virtual const Scope *global() const { return my_outer->global();}
  virtual const Symbol *lookup(const Encoding &) const throw();
  virtual void dump(std::ostream &) const;
private:
  Scope *my_outer;
};

}

#endif
