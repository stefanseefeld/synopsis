//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _SymbolLookup_Scope_hh
#define _SymbolLookup_Scope_hh

#include <SymbolLookup/Symbol.hh>
#include <map>
#include <set>

namespace SymbolLookup
{
struct TypeError : std::exception
{
  TypeError(PTree::Encoding const &n, PTree::Encoding const &t)
    : name(n), type(t) {}
  virtual ~TypeError() throw() {}
  virtual char const * what() const throw() { return "TypeError";}
  PTree::Encoding name;
  PTree::Encoding type;
};

struct Undefined : std::exception
{
  Undefined(PTree::Encoding const &n) : name(n) {}
  virtual ~Undefined() throw() {}
  virtual char const * what() const throw() { return "Undefined";}
  PTree::Encoding name;
};

struct MultiplyDefined : std::exception
{
  MultiplyDefined(PTree::Encoding const &n, const PTree::Node *decl, const PTree::Node *orig)
    : name(n), declaration(decl), original(orig) {}
  virtual ~MultiplyDefined() throw() {}
  virtual char const * what() const throw() { return "MultiplyDefined";}
  PTree::Encoding name;
  PTree::Node const * declaration;
  PTree::Node const * original;
};

class InternalError : public std::exception
{
public:
  InternalError(std::string const &what) : my_what(what) {}
  virtual ~InternalError() throw() {}
  virtual char const * what() const throw() { return my_what.c_str();}
private:
  std::string my_what;
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

  //. declare a nested scope
  void declare_scope(const PTree::Node *, const Scope *);

  //. look up a nested scope by associated parse tree node
  const Scope *lookup_scope(const PTree::Node *) const;

  //. look up the encoded name and return a set of matching symbols.
  virtual std::set<const Symbol *> lookup(const PTree::Encoding &) const throw();

  //. same as the untyped lookup, but type safe. Return either a set of function names
  //. (if T is of type FunctionName) or a single matching symbol.
  //. Throws a TypeError if the symbol exists but doesn't have the expected type.
  template <typename T>
  const T *lookup(const PTree::Encoding &name) const throw(TypeError);

  //. lookup specialization for the case where the Symbol looked for is a FunctionName,
  //. as in this case a set of possibly overloaded functions is returned.
  std::set<FunctionName const *> lookup_function(const PTree::Encoding &name) const throw(TypeError);

  //. recursively dump the content of the symbol table to a stream (for debugging).
  virtual void dump(std::ostream &, size_t indent) const;

protected:
  //. Scopes are ref counted, and thus are deleted only by 'unref()'
  virtual ~Scope();

  //. little helper function used in the implementation of 'dump()'
  static std::ostream &indent(std::ostream &os, size_t i);

private:
  //. SymbolTable provides a mapping from (encoded) names to Symbols declared
  //. in this scope.
  typedef std::multimap<PTree::Encoding, const Symbol *> SymbolTable;
  //. ScopeTable provides a mapping from scope nodes to Scopes,
  //. which can be used to traverse the scope tree in parallel with
  //. the associated parse tree.
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

template <typename T>
inline T const *Scope::lookup(const PTree::Encoding &name) const throw(TypeError)
{
  SymbolTable::const_iterator symbol = my_symbols.lower_bound(name);
  if (symbol == my_symbols.upper_bound(name)) return 0; // no such symbol
  else if (symbol->second->type().is_function())
    throw TypeError(name, symbol->second->type()); // function
  T const * t = dynamic_cast<T const *>(symbol->second);
  if (!t) throw TypeError(name, symbol->second->type());
  return t;
}

inline std::set<FunctionName const *> 
Scope::lookup_function(const PTree::Encoding &name) const throw(TypeError)
{
  SymbolTable::const_iterator symbol = my_symbols.lower_bound(name);
  std::set<FunctionName const *> symbols;
  for (; symbol != my_symbols.upper_bound(name); ++symbol)
  {
    FunctionName const * t = dynamic_cast<FunctionName const *>(symbol->second);
    if (!t) throw TypeError(name, symbol->second->type());
    else symbols.insert(t);
  }
  return symbols;
}

}

#endif
