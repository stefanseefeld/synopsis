//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _SymbolLookup_Scopes_hh
#define _SymbolLookup_Scopes_hh

#include <SymbolLookup/Scope.hh>
#include <string>

namespace SymbolLookup
{

class TemplateParameterScope : public Scope
{
public:
  virtual std::set<Symbol const *> lookup(const PTree::Encoding &) const throw();

  virtual void dump(std::ostream &, size_t indent) const;
};

class LocalScope : public Scope
{
public:
  LocalScope(Scope *outer) : my_outer(outer->ref()) {}
  virtual const Scope *global() const { return my_outer->global();}
  virtual std::set<Symbol const *> lookup(const PTree::Encoding &) const throw();

  virtual void dump(std::ostream &, size_t indent) const;
protected:
  ~LocalScope() { my_outer->unref();}

private:
  Scope *my_outer;
};

class FunctionScope : public Scope
{
public:
  FunctionScope(const PTree::Declaration *decl, Scope *outer)
    : my_decl(decl), my_outer(outer->ref()) {}
  virtual const Scope *global() const { return my_outer->global();}
  virtual std::set<Symbol const *> lookup(const PTree::Encoding &) const throw();

  // FIXME: what is 'name' ? (template parameters...)
  std::string name() const;

  virtual void dump(std::ostream &, size_t indent) const;
protected:
  ~FunctionScope() { my_outer->unref();}

private:
  const PTree::Declaration *my_decl;
  Scope                    *my_outer;
  TemplateParameterScope   *my_parameters;
};

class PrototypeScope : public Scope
{
public:
  PrototypeScope(Scope *outer) : my_outer(outer->ref()) {}
  virtual const Scope *global() const { return my_outer->global();}
  virtual std::set<Symbol const *> lookup(const PTree::Encoding &) const throw();

  virtual void dump(std::ostream &, size_t indent) const;
protected:
  ~PrototypeScope() { my_outer->unref();}

private:
  Scope *my_outer;
};

class Class : public Scope
{
public:
  Class(const PTree::ClassSpec *spec, Scope *outer)
    : my_spec(spec), my_outer(outer->ref())
  {
  }
  virtual const Scope *global() const { return my_outer->global();}
  virtual std::set<Symbol const *> lookup(const PTree::Encoding &) const throw();

  // FIXME: what is 'name' ? (template parameters...)
  std::string name() const;

  virtual void dump(std::ostream &, size_t indent) const;
protected:
  ~Class() { my_outer->unref();}
private:
  const PTree::ClassSpec *my_spec;
  Scope                  *my_outer;
  TemplateParameterScope *my_parameters;
};

class Namespace : public Scope
{
public:
  Namespace(const PTree::NamespaceSpec *spec, Scope *outer)
    : my_spec(spec), my_outer(outer->ref())
  {
  }
  virtual const Scope *global() const { return my_outer->global();}
  virtual std::set<Symbol const *> lookup(const PTree::Encoding &) const throw();

  // FIXME: should that really be a string ? It may be better to be conform with
  // Class::name, which, if the class is a template, can't be a string (or can i ?)
  std::string name() const;

  virtual void dump(std::ostream &, size_t indent) const;
protected:
  ~Namespace() { my_outer->unref();}
private:
  const PTree::NamespaceSpec *my_spec;
  Scope                      *my_outer;
};

}

#endif
