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
#include <map>

namespace SymbolLookup
{

struct TypeError : public std::exception
{
  TypeError(const PTree::Encoding &t) : type(t) {}
  virtual ~TypeError() throw() {}
  virtual const char* what() const throw() { return "TypeError";}
  PTree::Encoding type;
};

class Symbol
{
public:
  Symbol(const PTree::Encoding &t, PTree::Node *p) : my_type(t), my_ptree(p) {}
  virtual ~Symbol(){}
  const PTree::Encoding &type() const { return my_type;}
private:
  PTree::Encoding my_type;
  PTree::Node    *my_ptree;
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

//. A Scope contains symbol definitions.
class Scope
{
public:

  Scope() : my_refcount(1) {}
  Scope *ref() { ++my_refcount; return this;}
  const Scope *ref() const { ++my_refcount; return this;}
  void unref() const { if (!--my_refcount) delete this;}

  virtual const Scope *global() const { return this;}

  //. declare the given symbol in the local scope 
  //. using the given encoded name.
  void declare(const PTree::Encoding &name, const Symbol *symbol);

  //. look up the encoded name and return the associated symbol, if found.
  virtual const Symbol *lookup(const PTree::Encoding &) const throw();

  //. declare a nested scope
  void declare_scope(const PTree::Node *, const Scope *);

  //. look up a nested scope by associated parse tree node
  const Scope *lookup_scope(const PTree::Node *) const;

  //. same as the untyped lookup, but type safe. Throws a TypeError
  //. if the symbol exists but doesn't have the expected type.
  template <typename T>
  const T *lookup(const PTree::Encoding &name) const throw(TypeError)
  {
    const Symbol *symbol = lookup(name);
    if (!symbol) return 0;
    const T *t = dynamic_cast<const T *>(symbol);
    if (!t) throw TypeError(symbol->type());
    return t;
  }

  //. recursively dump the content of the symbol table to a stream (for debugging).
  virtual void dump(std::ostream &, size_t indent) const;

protected:
  //. Scopes are ref counted, and thus are deleted only by 'unref()'
  virtual ~Scope();

private:
  //. SymbolTable provides a mapping from (encoded) names to Symbols declared
  //. in this scope
  typedef std::map<PTree::Encoding, const Symbol *> SymbolTable;
  //. ScopeTable provides a mapping from scope nodes to Scopes,
  //. which can be used to traverse the scope tree in parallel with
  //. the associated parse tree
  typedef std::map<const PTree::Node *, const Scope *> ScopeTable;

  SymbolTable    my_symbols;
  ScopeTable     my_scopes;
  mutable size_t my_refcount;
};

inline void Scope::declare_scope(const PTree::Node *node, const Scope *scope)
{
  my_scopes[node] = scope->ref();
}

inline const Scope *Scope::lookup_scope(const PTree::Node *node) const
{
  ScopeTable::const_iterator i = my_scopes.find(node);
  return i == my_scopes.end() ? 0 : i->second;
}

}

#endif
