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
  virtual SymbolSet unqualified_lookup(PTree::Encoding const &, bool) const;

  virtual void dump(std::ostream &, size_t indent) const;
};

class LocalScope : public Scope
{
public:
  LocalScope(PTree::List const *node, Scope const *outer)
    : my_node(node), my_outer(outer->ref()) {}

  virtual Scope const *global() const { return my_outer->global();}
  virtual SymbolSet unqualified_lookup(PTree::Encoding const &, bool) const;

  virtual void dump(std::ostream &, size_t indent) const;
protected:
  ~LocalScope() { my_outer->unref();}

private:
  PTree::List const *my_node;
  Scope       const *my_outer;
};

class FunctionScope : public Scope
{
public:
  FunctionScope(PTree::Declaration const *decl, Scope const *outer)
    : my_decl(decl), my_outer(outer->ref()) {}

  virtual void use(PTree::Using const *);
  virtual Scope const *global() const { return my_outer->global();}
  virtual SymbolSet unqualified_lookup(PTree::Encoding const &, bool) const;

  // FIXME: what is 'name' ? (template parameters...)
  std::string name() const;

  virtual void dump(std::ostream &, size_t indent) const;
protected:
  ~FunctionScope() { my_outer->unref();}

private:
  typedef std::list<Namespace const *> Using;

  PTree::Declaration     const *my_decl;
  Scope                  const *my_outer;
  TemplateParameterScope const *my_parameters;
  Using                         my_using;
};

class PrototypeScope : public Scope
{
public:
  PrototypeScope(Scope const *outer) : my_outer(outer->ref()) {}

  virtual Scope const *global() const { return my_outer->global();}
  virtual SymbolSet unqualified_lookup(PTree::Encoding const &, bool) const;

  virtual void dump(std::ostream &, size_t indent) const;
protected:
  ~PrototypeScope() { my_outer->unref();}

private:
  Scope const *my_outer;
};

class Class : public Scope
{
public:
  Class(PTree::ClassSpec const *spec, Scope const *outer)
    : my_spec(spec), my_outer(outer->ref())
  {
  }

  virtual Scope const *global() const { return my_outer->global();}
  virtual SymbolSet unqualified_lookup(PTree::Encoding const &, bool) const;

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
  Namespace(PTree::NamespaceSpec const *spec, Scope const *outer)
    : my_spec(spec), my_outer(outer ? outer->ref() : 0)
  {
  }
  Namespace *find_namespace(std::string const &name) const;

  virtual void use(PTree::Using const *);
  virtual Scope const *global() const { return my_outer ? my_outer->global() : this;}
  virtual SymbolSet unqualified_lookup(PTree::Encoding const &, bool) const;

  // FIXME: should that really be a string ? It may be better to be conform with
  // Class::name, which, if the class is a template, can't be a string (or can it ?)
  std::string name() const;

  virtual void dump(std::ostream &, size_t indent) const;
protected:
  ~Namespace() { if (my_outer) my_outer->unref();}
private:
  typedef std::list<Namespace const *> Using;

  PTree::NamespaceSpec const *my_spec;
  Scope                const *my_outer;
  Using                       my_using;
};

}
}

#endif
