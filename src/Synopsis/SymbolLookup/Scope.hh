//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_SymbolLookup_Scope_hh_
#define Synopsis_SymbolLookup_Scope_hh_

#include <Synopsis/SymbolLookup/Symbol.hh>
#include <map>
#include <set>

namespace Synopsis
{
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
  MultiplyDefined(PTree::Encoding const &n,
		  PTree::Node const *decl,
		  PTree::Node const *orig)
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

typedef std::set<Symbol const *> SymbolSet;
typedef std::set<FunctionName const *> FunctionNameSet;

//. A Scope contains symbol definitions.
class Scope
{
public:

  Scope() : my_refcount(1) {}
  Scope *ref() { ++my_refcount; return this;}
  Scope const *ref() const { ++my_refcount; return this;}
  void unref() const { if (!--my_refcount) delete this;}

  virtual Scope const *global() const = 0;

  //. declare the given symbol in the local scope 
  //. using the given encoded name.
  void declare(PTree::Encoding const &name, Symbol const *symbol);

  //. declare a nested scope
  void declare_scope(PTree::Node const *, Scope *);

  //. declare a 'using' directive.
  //. The default implementation raises an exception,
  //. as it is only well-formed when the current scope
  //. is a function scope or a namespace.
  virtual void use(PTree::UsingDirective const *);

  //. find a nested scope by declaration
  Scope *find_scope(PTree::Node const *) const;
  //. find a nested scope by symbol.
  //. The encoded name is provided for diagnostic purposes only.
  Scope *find_scope(PTree::Encoding const &, Symbol const *) const;
  //. find a nested scope by name
  //Scope *find_scope(PTree::Encoding const &) const;

  //. find the encoded name declared in this scope and 
  //. return a set of matching symbols.
  SymbolSet find(PTree::Encoding const &, bool scope = false) const throw();

  //. look up the encoded name and return a set of matching symbols.
  SymbolSet lookup(PTree::Encoding const &) const;

  //. same as the untyped lookup, but type safe. 
  // Return either a set of function names
  //. (if T is of type FunctionName) or a single matching symbol.
  //. Throws a TypeError if the symbol exists but doesn't have the expected type.
  template <typename T>
  T const *lookup(PTree::Encoding const &name) const throw(TypeError);

  //. lookup specialization for the case where the Symbol looked for 
  //. is a FunctionName,
  //. as in this case a set of possibly overloaded functions is returned.
  FunctionNameSet lookup_function(PTree::Encoding const &name) const throw(TypeError);

  virtual SymbolSet unqualified_lookup(PTree::Encoding const &,
				       bool scope) const = 0;
  virtual SymbolSet qualified_lookup(PTree::Encoding const &) const;

  //. recursively dump the content of the symbol table to a stream (for debugging).
  virtual void dump(std::ostream &, size_t indent) const;

protected:
  //. Scopes are ref counted, and thus are deleted only by 'unref()'
  virtual ~Scope();

  //. SymbolTable provides a mapping from (encoded) names to Symbols declared
  //. in this scope.
  typedef std::multimap<PTree::Encoding, Symbol const *> SymbolTable;
  //. ScopeTable provides a mapping from scope nodes to Scopes,
  //. which can be used to traverse the scope tree in parallel with
  //. the associated parse tree. As this traversal is also done
  //. during the parsing, the scopes can not be const.
  typedef std::map<PTree::Node const *, Scope *> ScopeTable;

  //. little helper function used in the implementation of 'dump()'
  static std::ostream &indent(std::ostream &os, size_t i);

  SymbolTable    my_symbols;
  ScopeTable     my_scopes;
  mutable size_t my_refcount;
};

inline void Scope::declare_scope(PTree::Node const *node, Scope *scope)
{
  my_scopes[node] = scope->ref();
}

inline Scope *Scope::find_scope(PTree::Node const *node) const
{
  ScopeTable::const_iterator i = my_scopes.find(node);
  return i == my_scopes.end() ? 0 : i->second;
}

template <typename T>
inline T const *Scope::lookup(PTree::Encoding const &name) const throw(TypeError)
{
  SymbolSet symbols = lookup(name);
  if (symbols.empty()) return 0;
  Symbol const *symbol = *symbols.begin();
  if (symbols.size() > 1) // must be a set of overloaded functions
    throw TypeError(name, symbol->type()); // function

  T const * t = dynamic_cast<T const *>(symbol);
  if (!t)
    throw TypeError(name, symbol->type());
  return t;
}

inline FunctionNameSet
Scope::lookup_function(PTree::Encoding const &name) const throw(TypeError)
{
  SymbolSet symbols = lookup(name);
  FunctionNameSet funcs;
  for (SymbolSet::iterator i = symbols.begin(); i != symbols.end(); ++i)
  {
    FunctionName const * t = dynamic_cast<FunctionName const *>(*i);
    if (!t) throw TypeError(name, (*i)->type());
    else funcs.insert(t);
  }
  return funcs;
}

}
}

#endif
