//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <PTree/Display.hh>
#include <PTree/Writer.hh>
#include <SymbolLookup/Table.hh>
#include <SymbolLookup/Scopes.hh>
#include <SymbolLookup/ConstEvaluator.hh>
#include <Synopsis/Trace.hh>
#include <cassert>

using Synopsis::Trace;

using namespace PTree;
using namespace SymbolLookup;

Table::Table(Language /* ignore for now */)
{
  // define the global scope
  my_scopes.push(new Scope());
}

Table &Table::enter_scope()
{
  Trace trace("Table::enter_scope");
  my_scopes.push(new Scope());
  return *this;
}

Table &Table::enter_namespace(const PTree::NamespaceSpec *spec)
{
  Trace trace("Table::enter_namespace");
  Namespace *ns = new Namespace(spec, my_scopes.top());
  my_scopes.top()->declare_scope(spec, ns);
  my_scopes.push(ns);
  return *this;
}

Table &Table::enter_class(const PTree::ClassSpec *spec)
{
  Trace trace("Table::enter_class");
  Class *cl = new Class(spec, my_scopes.top());
  my_scopes.top()->declare_scope(spec, cl);
  my_scopes.push(cl);
  return *this;
}

Table &Table::enter_function(const PTree::Declaration *decl)
{
  Trace trace("Table::enter_function");
  FunctionScope *func = new FunctionScope(decl, my_scopes.top());
  my_scopes.top()->declare_scope(decl, func);
  my_scopes.push(func);
  return *this;
}

Table &Table::enter_block(const PTree::List *block)
{
  Trace trace("Table::enter_block");
  LocalScope *scope = new LocalScope(block, my_scopes.top());
  my_scopes.top()->declare_scope(block, scope);
  my_scopes.push(scope);
  return *this;
}

void Table::leave_scope()
{
  Trace trace("Table::leave_scope");
  Scope *top = my_scopes.top();
  my_scopes.pop();
  top->unref();
}

Scope &Table::current_scope()
{
  return *my_scopes.top();
}

void Table::declare(Declaration *d)
{
  Trace trace("Table::declare(Declaration *)");
  Node *decls = third(d);
  if(is_a(decls, Token::ntDeclarator))
  {
    // function definition
    PTree::Encoding name = decls->encoded_name();
    PTree::Encoding type = decls->encoded_type();

    // if the name is qualified, it has to be
    // declared already. If it hasn't, raise an error
    if (name.is_qualified())
    {
      std::set<Symbol const *> symbols = lookup(name);
      if (symbols.empty()) throw Undefined(name);
      return;
    }
    
    // FIXME: raise an error if this function was already defined
    my_scopes.top()->declare(name, new FunctionName(type, decls));
  }
  else
  {
    // function or variable declaration
    PTree::Node *storage_spec = PTree::first(d);
    PTree::Node *type_spec = PTree::second(d);
    if (decls->is_atom()) ; // it is a ';'
    else
    {
      for (; decls; decls = decls->cdr())
      {
	PTree::Node *decl = decls->car();
	if (PTree::is_a(decl, Token::ntDeclarator))
	{
	  PTree::Encoding name = decl->encoded_name();
	  PTree::Encoding type = decl->encoded_type();
	  if (type.is_function())
	    my_scopes.top()->declare(name, new FunctionName(type, decl));
	  else
	    my_scopes.top()->declare(name, new VariableName(type, decl));
	}
      }
    }
  }
}

void Table::declare(Typedef *td)
{
  Trace trace("Table::declare(Typedef *)");
  Node *declarations = third(td);
  while(declarations)
  {
    Node *d = declarations->car();
    if(type_of(d) == Token::ntDeclarator)
    {
      Encoding name = d->encoded_name();
      Encoding type = d->encoded_type();
      my_scopes.top()->declare(name, new TypeName(type, d));
    }
    declarations = tail(declarations, 2);
  }
}

void Table::declare(EnumSpec *spec)
{
  Trace trace("Table::declare(EnumSpec *)");
  Node *tag = second(spec);
  const Encoding &name = spec->encoded_name();
  const Encoding &type = spec->encoded_type();
  if(tag && tag->is_atom()) 
    my_scopes.top()->declare(name, new TypeName(type, spec));
  // else it's an anonymous enum

  Node *body = third(spec);
  // The numeric value of an enumerator is either specified
  // by an explicit initializer or it is determined by incrementing
  // by one the value of the previous enumerator.
  // The default value for the first enumerator is 0
  long value = -1;
  for (Node *e = second(body); e; e = rest(rest(e)))
  {
    Node *enumerator = e->car();
    bool defined = true;
    if (enumerator->is_atom()) ++value;
    else  // [identifier = initializer]
    {
      Node *initializer = third(enumerator);
      defined = evaluate_const(initializer, value);
      enumerator = enumerator->car();
#ifndef NDEBUG
      if (!defined)
      {
	std::cerr << "Error in evaluating enum initializer:\n"
		  << "Expression doesn't evaluate to a constant integral value:\n"
		  << reify(initializer) << std::endl;
      }
#endif
    }
    assert(enumerator->is_atom());
    Encoding name(enumerator->position(), enumerator->length());
    if (defined)
      my_scopes.top()->declare(name, new ConstName(type, value, enumerator));
    else
      my_scopes.top()->declare(name, new ConstName(type, enumerator));
  }
}

void Table::declare(NamespaceSpec *spec)
{
  Trace trace("Table::declare(NamespaceSpec *)");
  const Node *name = second(spec);
  Encoding enc = Encoding::simple_name(static_cast<Atom const *>(name));
  // FIXME: do we need a 'type' here ?
  my_scopes.top()->declare(enc, new NamespaceName(spec->encoded_type(), spec));
}

void Table::declare(ClassSpec *spec)
{
  Trace trace("Table::declare(ClassSpec *)");
  const Encoding &name = spec->encoded_name();
  my_scopes.top()->declare(name, new TypeName(spec->encoded_type(), spec));
}

void Table::declare(TemplateDecl *tdecl)
{
  Trace trace("Table::declare(TemplateDecl *)");
  PTree::Node *body = PTree::nth(tdecl, 4);
  PTree::ClassSpec *class_spec = Table::get_class_template_spec(body);
  if (class_spec)
  {
    Encoding name = class_spec->encoded_name();
    my_scopes.top()->declare(name, new ClassTemplateName(Encoding(), tdecl));
  }
  else
  {
    PTree::Node *decl = PTree::third(body);
    PTree::Encoding name = decl->encoded_name();
    my_scopes.top()->declare(name, new FunctionTemplateName(Encoding(), decl));
  }
}

std::set<Symbol const *> Table::lookup(PTree::Encoding const &name) const
{
  PTree::Encoding symbol_name = name.get_scope();
  PTree::Encoding remainder = name.get_symbol();

  const Scope *scope = my_scopes.top(); // start to look here
  if (symbol_name.is_global_scope())
  {
    scope = my_scopes.top()->global();
    symbol_name = remainder.get_scope();
    remainder = remainder.get_symbol();
  }
  while (!symbol_name.empty()) // look up nested scopes
  {
    std::set<Symbol const *> symbols = scope->lookup(symbol_name);
    if (symbols.empty()) throw Undefined(symbol_name);
    else if (symbols.size() > 1)
    {
      // can we assume here that all symbols refer to functions ?
      scope->dump(std::cerr, 0);
      throw TypeError(symbol_name, (*symbols.begin())->ptree()->encoded_type());
    }
    // now make sure the symbol indeed refers to a scope
    PTree::Node const *decl = 0;
    if (NamespaceName const *ns = dynamic_cast<NamespaceName const *>(*symbols.begin()))
      decl = ns->ptree();
    else if (TypeName const *tn = dynamic_cast<TypeName const *>(*symbols.begin()))
    {
      decl = tn->ptree();
      // test that 'decl' is a ClassSpec
    }
    // TODO: test for ClassTemplateName ...
    if (!decl)
    {
      // the symbol was found but doesn't refer to a scope
      scope->dump(std::cerr, 0);
      throw TypeError(symbol_name, (*symbols.begin())->ptree()->encoded_type());
    }
    // move into inner scope and start over the lookup
    scope = scope->lookup_scope(decl);
    if (!scope) throw InternalError("undeclared scope !");
    symbol_name = remainder.get_scope();
    remainder = remainder.get_symbol();
  }
  // now we have to look up the remainder in the innermost scope
  std::set<Symbol const *> symbols = scope->lookup(remainder);
  return symbols;
}

//. get_base_name() returns "Foo" if ENCODE is "Q[2][1]X[3]Foo", for example.
//. If an error occurs, the function returns 0.
PTree::Encoding Table::get_base_name(const PTree::Encoding &enc, const Scope *&scope)
{
  if(enc.empty()) return enc;
  const Scope *s = scope;
  PTree::Encoding::iterator i = enc.begin();
  if(*i == 'Q')
  {
    int n = *(i + 1) - 0x80;
    i += 2;
    while(n-- > 1)
    {
      int m = *i++;
      if(m == 'T') m = Table::get_base_name_if_template(i, s);
      else if(m < 0x80) return PTree::Encoding(); // error?
      else
      {	 // class name
	m -= 0x80;
	if(m <= 0)
	{		// if global scope (e.g. ::Foo)
	  if(s) s = s->global();
	}
	else s = lookup_typedef_name(i, m, s);
      }
      i += m;
    }
    scope = s;
  }
  if(*i == 'T')
  {		// template class
    int m = *(i + 1) - 0x80;
    int n = *(i + m + 2) - 0x80;
    return PTree::Encoding(i, i + m + n + 3);
  }
  else if(*i < 0x80) return PTree::Encoding();
  else return PTree::Encoding(i + 1, i + 1 + size_t(*i - 0x80));
}

int Table::get_base_name_if_template(Encoding::iterator i, const Scope *&scope)
{
  int m = *i - 0x80;
  if(m <= 0) return *(i+1) - 0x80 + 2;

  if(scope)
  {
    std::set<Symbol const *> symbols = scope->lookup(Encoding((const char*)&*(i + 1), m));
    // FIXME !! (see Environment)
    if (symbols.size()) return m + (*(i + m + 1) - 0x80) + 2;
  }
  // the template name was not found.
  scope = 0;
  return m + (*(i + m + 1) - 0x80) + 2;
}

const Scope *Table::lookup_typedef_name(Encoding::iterator i, size_t s,
					const Scope *scope)
{
//   TypeInfo tinfo;
//   Bind *bind;
//   Class *c = 0;

  if(scope)
    ;
//     if (scope->LookupType(Encoding((const char *)&*i, s), bind) && bind)
//       switch(bind->What())
//       {
//         case Bind::isClassName :
// 	  c = bind->ClassMetaobject();
// 	  break;
//         case Bind::isTypedefName :
// 	  bind->GetType(tinfo, env);
// 	  c = tinfo.class_metaobject();
// 	  /* if (c == 0) */
// 	  env = 0;
// 	  break;
//         default :
// 	  break;
//       }
//     else if (env->LookupNamespace(Encoding((const char *)&*i, s)))
//     {
//       /* For the time being, we simply ignore name spaces.
//        * For example, std::T is identical to ::T.
//        */
//       env = env->GetBottom();
//     }
//     else env = 0; // unknown typedef name

//   return c ? c->GetEnvironment() : env;
  return 0;
}

PTree::ClassSpec *Table::get_class_template_spec(PTree::Node *body)
{
  if(*PTree::third(body) == ';')
  {
    PTree::Node *spec = strip_cv_from_integral_type(PTree::second(body));
    return dynamic_cast<PTree::ClassSpec *>(spec);
  }
  return 0;
}

PTree::Node *Table::strip_cv_from_integral_type(PTree::Node *integral)
{
  if(integral == 0) return 0;

  if(!integral->is_atom())
    if(PTree::is_a(integral->car(), Token::CONST, Token::VOLATILE))
      return PTree::second(integral);
    else if(PTree::is_a(PTree::second(integral), Token::CONST, Token::VOLATILE))
      return integral->car();

  return integral;
}
