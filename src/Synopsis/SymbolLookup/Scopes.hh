//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_SymbolLookup_Scopes_hh_
#define Synopsis_SymbolLookup_Scopes_hh_

#include <Synopsis/SymbolLookup/Scope.hh>
#include <string>
#include <list>

namespace Synopsis
{
namespace SymbolLookup
{

class Namespace;

class TemplateParameterScope : public Scope
{
public:
  virtual SymbolSet 
  unqualified_lookup(PTree::Encoding const &, LookupContext) const;

  virtual void dump(std::ostream &, size_t indent) const;
};

class LocalScope : public Scope
{
public:
  LocalScope(PTree::List const *node, Scope const *outer)
    : my_node(node), my_outer(outer->ref()) {}

  virtual Scope const *global() const { return my_outer->global();}
  virtual SymbolSet 
  unqualified_lookup(PTree::Encoding const &, LookupContext) const;

  virtual void dump(std::ostream &, size_t indent) const;
protected:
  ~LocalScope() { my_outer->unref();}

private:
  PTree::List const *my_node;
  Scope       const *my_outer;
};

class PrototypeScope;

class FunctionScope : public Scope
{
public:
  FunctionScope(PTree::Declaration const *, PrototypeScope *, Scope const *);

  virtual void use(PTree::UsingDirective const *);
  virtual Scope const *global() const { return my_outer->global();}
  virtual SymbolSet 
  unqualified_lookup(PTree::Encoding const &, LookupContext) const;
  virtual SymbolSet 
  qualified_lookup(PTree::Encoding const &, LookupContext) const;

  // FIXME: what is 'name' ? (template parameters...)
  std::string name() const;

  virtual void dump(std::ostream &, size_t indent) const;
protected:
  ~FunctionScope() { my_outer->unref();}

private:
  typedef std::set<Namespace const *> Using;

  PTree::Declaration     const *my_decl;
  Scope                  const *my_outer;
  TemplateParameterScope const *my_parameters;
  Using                         my_using;
};

class PrototypeScope : public Scope
{
  friend class FunctionScope;
public:
  PrototypeScope(PTree::Node const *decl, Scope const *outer)
    : my_decl(decl), my_outer(outer->ref()) {}

  virtual Scope const *global() const { return my_outer->global();}
  virtual SymbolSet 
  unqualified_lookup(PTree::Encoding const &, LookupContext) const;

  PTree::Node const *declaration() const { return my_decl;}
  virtual void dump(std::ostream &, size_t indent) const;
protected:
  ~PrototypeScope() { my_outer->unref();}

private:
  PTree::Node const *my_decl;
  Scope const *      my_outer;
};

class Class : public Scope
{
public:
  Class(PTree::ClassSpec const *spec, Scope const *outer)
    : my_spec(spec), my_outer(outer->ref())
  {
  }

  virtual Scope const *global() const { return my_outer->global();}
  virtual SymbolSet 
  unqualified_lookup(PTree::Encoding const &, LookupContext) const;

  // FIXME: what is 'name' ? (template parameters...)
  std::string name() const;

  virtual void dump(std::ostream &, size_t indent) const;
protected:
  ~Class() { my_outer->unref();}
private:
  PTree::ClassSpec       const *my_spec;
  Scope                  const *my_outer;
  TemplateParameterScope const *my_parameters;
};

class Namespace : public Scope
{
public:
  Namespace(PTree::NamespaceSpec const *spec, Namespace const *outer)
    : my_spec(spec),
      my_outer(outer ? static_cast<Namespace const *>(outer->ref()) : 0)
  {
  }
  //. Find a nested namespace.
  Namespace *find_namespace(PTree::NamespaceSpec const *name) const;

  virtual void use(PTree::UsingDirective const *);
  virtual Scope const *global() const 
  { return my_outer ? my_outer->global() : this;}
  virtual SymbolSet 
  unqualified_lookup(PTree::Encoding const &, LookupContext) const;
  virtual SymbolSet 
  qualified_lookup(PTree::Encoding const &, LookupContext) const;

  // FIXME: should that really be a string ? It may be better to be conform with
  // Class::name, which, if the class is a template, can't be a string (or can it ?)
  std::string name() const;

  virtual void dump(std::ostream &, size_t indent) const;
protected:
  ~Namespace() { if (my_outer) my_outer->unref();}
private:
  typedef std::set<Namespace const *> Using;

  SymbolSet 
  unqualified_lookup(PTree::Encoding const &, LookupContext, Using &) const;
  SymbolSet 
  qualified_lookup(PTree::Encoding const &, LookupContext, Using &) const;

  PTree::NamespaceSpec const *my_spec;
  Namespace const *           my_outer;
  Using                       my_using;
};

}
}

#endif
