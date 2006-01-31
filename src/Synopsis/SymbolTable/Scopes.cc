//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/SymbolTable/Scopes.hh>
#include <Synopsis/SymbolTable/Display.hh>
#include <Synopsis/PTree/Writer.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/Trace.hh>
#include <sstream>
#include <cassert>

using namespace Synopsis;
namespace PT = Synopsis::PTree;
using namespace Synopsis::SymbolTable;

SymbolSet 
TemplateParameterScope::find(PTree::Encoding const &name,
			     LookupContext context) const
{
  Trace trace("TemplateParameterScope::find", Trace::SYMBOLLOOKUP, name);
  SymbolSet symbols = Scope::find(name, context);
  if (symbols.empty() && my_outer_template_parameters)
    symbols = my_outer_template_parameters->find(name, context);
  return symbols;
}

SymbolSet 
TemplateParameterScope::unqualified_lookup(PT::Encoding const &name,
					   LookupContext context,
					   Scopes &searched) const
{
  Trace trace("TemplateParameterScope::unqualified_lookup", Trace::SYMBOLLOOKUP, name);
  searched.insert(this);
  SymbolSet symbols = find(name, context);
  if (symbols.empty() && my_outer_template_parameters)
    symbols = my_outer_template_parameters->unqualified_lookup(name, context, searched);
  if (symbols.empty())
  {
    if (searched.find(my_outer) == searched.end())
      return my_outer->unqualified_lookup(name, context, searched);
  }
  return symbols;
}

SymbolSet 
LocalScope::unqualified_lookup(PT::Encoding const &name,
			       LookupContext context,
			       Scopes &searched) const
{
  Trace trace("LocalScope::unqualified_lookup", Trace::SYMBOLLOOKUP, name);
  searched.insert(this);
  SymbolSet symbols = find(name, context);
  if (symbols.empty())
  {
    if (searched.find(my_outer) == searched.end())
      return my_outer->unqualified_lookup(name, context, searched);
  }
  return symbols;
}

FunctionScope::FunctionScope(PT::Declaration const *decl, 
			     PrototypeScope *proto,
			     Scope *outer)
  : my_decl(decl), my_outer(outer->ref()),
    my_parameters(proto)
{
}

void FunctionScope::use(Namespace const *ns)
{
  my_using.insert(ns);
}

SymbolSet 
FunctionScope::unqualified_lookup(PT::Encoding const &name,
				  LookupContext context,
				  Scopes &searched) const
{
  Trace trace("FunctionScope::unqualified_lookup", Trace::SYMBOLLOOKUP, name);
  searched.insert(this);
  SymbolSet symbols = find(name, context);
  {
    SymbolSet more = my_parameters->find(name, context);
    symbols.insert(more.begin(), more.end());
  }
  if (my_parameters->template_parameters())
  {
    SymbolSet more = my_parameters->template_parameters()->find(name, context);
    symbols.insert(more.begin(), more.end());
  }
  if (symbols.size()) return symbols;

  // see 7.3.4 [namespace.udir]
  for (Using::const_iterator i = my_using.begin(); i != my_using.end(); ++i)
    if (searched.find(*i) != searched.end())
    {
      SymbolSet more = (*i)->unqualified_lookup(name, context | USING, searched);
      symbols.insert(more.begin(), more.end());
    }
  if (symbols.empty() && my_outer && searched.find(my_outer) == searched.end())
    return my_outer->unqualified_lookup(name, context, searched);
  else
    return symbols;
}

std::string FunctionScope::name() const
{
  std::ostringstream oss;
  oss << PT::reify(PT::nth<1>(my_decl));
  return oss.str();
}

SymbolSet 
FunctionScope::qualified_lookup(PT::Encoding const &name,
				LookupContext context,
				Scopes &searched) const
{
  Trace trace("FunctionScope::qualified_lookup", Trace::SYMBOLLOOKUP, name);
  searched.insert(this);
  // FIXME: what is a qualified name in function scope ?


  // find symbol locally
  SymbolSet symbols = find(name, context);
  {
    SymbolSet more = my_parameters->find(name, context);
    symbols.insert(more.begin(), more.end());
  }
//   if (my_parameters->template_parameters())
//   {
//     SymbolSet more = my_parameters->template_parameters()->unqualified_lookup(name, context, searched);
//     symbols.insert(more.begin(), more.end());
//   }
  // see 7.3.4 [namespace.udir]
  if (symbols.empty())
    for (Using::const_iterator i = my_using.begin(); i != my_using.end(); ++i)
      if (searched.find(*i) == searched.end())
      {
	SymbolSet more = (*i)->qualified_lookup(name, context, searched);
	symbols.insert(more.begin(), more.end());
      }
  return symbols;
}

SymbolSet 
PrototypeScope::unqualified_lookup(PT::Encoding const &name,
				   LookupContext context,
				   Scopes &searched) const
{
  Trace trace("PrototypeScope::unqualified_lookup", Trace::SYMBOLLOOKUP);
  searched.insert(this);
  SymbolSet symbols = find(name, context);
  if (symbols.empty() && my_parameters)
  {
    SymbolSet more = my_parameters->find(name, context);
    symbols.insert(more.begin(), more.end());
  }
  if (symbols.empty() && my_outer && searched.find(my_outer) == searched.end())
    return my_outer->unqualified_lookup(name, context, searched);
  return symbols;
}

std::string PrototypeScope::name() const
{
  std::ostringstream oss;
  oss << PT::reify(my_decl);
  return oss.str();
}

Class::Class(std::string const &name, PT::ClassSpec const *spec, Scope *outer,
	     Bases const &bases, TemplateParameterScope const *params)
  : my_spec(spec), my_name(name),
    my_outer(outer ? outer->ref() : 0),
    my_bases(bases),
    my_parameters(params)
{
}

Class::~Class()
{
  if (my_outer) my_outer->unref();
}

SymbolSet 
Class::unqualified_lookup(PT::Encoding const &name,
			  LookupContext context,
			  Scopes &searched) const
{
  Trace trace("Class::unqualified_lookup", Trace::SYMBOLLOOKUP);
  trace << name << ' ' << context;
  searched.insert(this);
  SymbolSet symbols = find(name, context);
  if (!symbols.empty()) return symbols;
  trace << "looking in parameters";
  if (my_parameters)
  {
    SymbolSet more = my_parameters->find(name, context & ~ELABORATED);
    symbols.insert(more.begin(), more.end());
  }
  if (!symbols.empty()) return symbols;
  trace << "looking in base classes";
  for (Bases::const_iterator i = my_bases.begin(); i != my_bases.end(); ++i)
    if (searched.find(*i) == searched.end())
    {
      SymbolSet more = (*i)->unqualified_lookup(name, context, searched);
      symbols.insert(more.begin(), more.end());
    }
  if (symbols.empty() && my_outer && searched.find(my_outer) == searched.end())
    return my_outer->unqualified_lookup(name, context, searched);
  else
    return symbols;
}

Namespace *Namespace::find_namespace(PT::NamespaceSpec const *spec) const
{
  std::string name = "<anonymous>";
  PT::Node const *ns_name = PT::nth<1>(spec);
  if (ns_name)
    name.assign(ns_name->position(), ns_name->length());
  for (ScopeTable::const_iterator i = my_scopes.begin();
       i != my_scopes.end();
       ++i)
  {
    Namespace *ns = dynamic_cast<Namespace *>(i->second);
    if (ns && name == ns->name()) return ns;
  }
  return 0;
}

void Namespace::use(Namespace const *ns)
{
  my_using.insert(ns);
}

std::string Namespace::name() const
{
  if (!my_spec) return "<global>";
  PT::Node const *name_spec = PT::nth<1>(my_spec);
  if (name_spec) return std::string(name_spec->position(), name_spec->length());
  else return "<anonymous>";
}

SymbolSet Namespace::unqualified_lookup(PT::Encoding const &name,
					LookupContext context,
					Scopes &searched) const
{
  Trace trace("Namespace::unqualified_lookup", Trace::SYMBOLLOOKUP, this->name());
  trace << name;
  searched.insert(this);
  SymbolSet symbols = find(name, context);
  if (!symbols.empty()) return symbols;

  // see 7.3.4 [namespace.udir]
  for (Using::const_iterator i = my_using.begin(); i != my_using.end(); ++i)
    if (searched.find(*i) == searched.end())
    {
      SymbolSet more = (*i)->unqualified_lookup(name, context | USING, searched);
      symbols.insert(more.begin(), more.end());
    }
  if (!symbols.empty() ||
      context & USING ||
      !my_outer ||
      searched.find(my_outer) != searched.end())
    return symbols;
  else
    return my_outer->unqualified_lookup(name, context, searched);
}

SymbolSet Namespace::qualified_lookup(PT::Encoding const &name,
				      LookupContext context,
				      Scopes &searched) const
{
  Trace trace("Namespace::qualified_lookup", Trace::SYMBOLLOOKUP, this->name());
  trace << name;
  searched.insert(this);

  // find symbol locally
  SymbolSet symbols = find(name, context);

  // see 3.4.3.2/2 and 3.4.3.2/6
  if (!symbols.size() && (context ^ DECLARATION || name.is_qualified()))
    for (Using::const_iterator i = my_using.begin(); i != my_using.end(); ++i)
      if (searched.find(*i) == searched.end())
      {
	SymbolSet more = (*i)->qualified_lookup(name, context, searched);
	symbols.insert(more.begin(), more.end());
      }
  return symbols;
}

