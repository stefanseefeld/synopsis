//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/SymbolFactory.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/PTree/Writer.hh>
#include <Synopsis/SymbolLookup/Scopes.hh>
#include <Synopsis/TypeAnalysis/ConstEvaluator.hh>
#include <Synopsis/Trace.hh>
#include <cassert>

using namespace Synopsis;
using namespace PTree;
using namespace SymbolLookup;
using namespace TypeAnalysis;

namespace
{
PTree::Node const *strip_cv_from_integral_type(PTree::Node const *integral)
{
  if(integral == 0) return 0;

  if(!integral->is_atom())
    if(PTree::is_a(integral->car(), Token::CONST, Token::VOLATILE))
      return PTree::second(integral);
    else if(PTree::is_a(PTree::second(integral), Token::CONST, Token::VOLATILE))
      return integral->car();

  return integral;
}

PTree::ClassSpec const *get_class_template_spec(PTree::Node const *body)
{
  if(*PTree::third(body) == ';') // template declaration
  {
    PTree::Node const *spec = strip_cv_from_integral_type(PTree::second(body));
    return dynamic_cast<PTree::ClassSpec const *>(spec);
  }
  return 0;
}

}


SymbolFactory::SymbolFactory(Language l)
  : my_language(l),
    my_prototype(0)
{
  // define the global scope
  my_scopes.push(new Namespace(0, 0));
}

void SymbolFactory::enter_scope(PTree::NamespaceSpec const *spec)
{
  Trace trace("SymbolFactory::enter_scope(NamespaceSpec)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;

  // Namespaces are only valid within namespaces.
  Namespace *scope = dynamic_cast<Namespace *>(my_scopes.top());
  assert(scope);

  Namespace *namespace_ = 0;
  // If the namespace was already opened before, we add a reference
  // to it under the current NamespaceSpec, too.
  if (namespace_ = scope->find_namespace(spec))
  {
    scope->declare_scope(spec, namespace_);
  }
  else
  {
    // This is a new namespace. Declare it.
    namespace_ = new Namespace(spec, scope);
    scope->declare_scope(spec, namespace_);
  }
  my_scopes.push(namespace_);
}

void SymbolFactory::enter_scope(PTree::ClassSpec const *spec)
{
  Trace trace("SymbolFactory::enter_scope(ClassSpec)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;

  Scope *scope = my_scopes.top();
  Class *class_ = new Class(spec, scope);
  scope->declare_scope(spec, class_);
  my_scopes.push(class_);
}

void SymbolFactory::enter_scope(PTree::Node const *decl)
{
  Trace trace("SymbolFactory::enter_scope(Node)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;
  PTree::display(decl, std::cout, false, true);
  Scope *scope = my_scopes.top();
  // Create a PrototypeScope. If this is part of a function definition, we will
  // later convert it into a FunctionScope.
  my_prototype = new PrototypeScope(decl, scope);
  scope->declare_scope(decl, my_prototype);
  my_scopes.push(my_prototype);
}

void SymbolFactory::enter_scope(PTree::FunctionDefinition const *decl)
{
  Trace trace("SymbolFactory::enter_scope(FunctionDefinition)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;

  assert(my_prototype);
  Scope *scope = my_scopes.top();
  // Transfer all symbols from the previously seen function declaration
  // into the newly created FunctionScope, and remove the PrototypeScope.
  FunctionScope *func = new FunctionScope(decl, my_prototype, scope);
  scope->remove_scope(my_prototype->declaration());
  scope->declare_scope(decl, func);
  my_prototype = 0;
  my_scopes.push(func);
}

void SymbolFactory::enter_scope(PTree::TemplateDecl const *params)
{
  Trace trace("SymbolFactory::enter_scope(TemplateDecl)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;

  Scope *scope = my_scopes.top();
  TemplateParameterScope *templ = new TemplateParameterScope(params, scope);
  scope->declare_scope(params, templ);
  my_scopes.push(templ);
}

void SymbolFactory::enter_scope(PTree::Block const *block)
{
  Trace trace("SymbolFactory::enter_scope(Block)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;

  Scope *scope = my_scopes.top();
  LocalScope *local = new LocalScope(block, scope);
  scope->declare_scope(block, local);
  my_scopes.push(local);
}

void SymbolFactory::leave_scope()
{
  Trace trace("SymbolFactory::leave_scope", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;

  Scope *scope = my_scopes.top();
  my_scopes.pop();
  scope->unref();
}

void SymbolFactory::declare(PTree::Declaration const *d)
{
  Trace trace("SymbolFactory::declare(Declaration *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;

  PTree::Node const *decls = PTree::third(d);
  if(PTree::is_a(decls, Token::ntDeclarator))
  {
    // function definition,
    // declare it only once (but allow overloading)

    PTree::Encoding name = decls->encoded_name();
    PTree::Encoding type = decls->encoded_type();

    // If the name is qualified, it has to be
    // declared already. If it hasn't, raise an error.
    Scope *scope = my_scopes.top();
    if (name.is_qualified())
    {
      SymbolSet symbols = scope->lookup(name, Scope::DECLARATION);
      if (symbols.empty()) throw Undefined(name);
      // FIXME: We need type analysis / overload resolution
      //        here to take the right symbol.
      Symbol const *symbol = *symbols.begin();
      while (name.is_qualified()) name = name.get_symbol();
      scope = symbol->scope();
      // TODO: check whether this is the definition of a previously
      //       declared function, according to 3.1/2 [basic.def]
      scope->remove(symbol);
    }
    scope->declare(name, new FunctionName(type, decls, true, scope));
  }
  else
  {
    // Function or variable declaration.
    PTree::Node const *storage_spec = PTree::first(d);
    PTree::Node const *type_spec = PTree::second(d);
    if (decls->is_atom()) ; // it is a ';'
    else
    {
      for (; decls; decls = decls->cdr())
      {
	PTree::Node const *decl = decls->car();
	if (PTree::is_a(decl, Token::ntDeclarator))
	{
	  PTree::Encoding name = decl->encoded_name();
	  PTree::Encoding const &type = decl->encoded_type();

	  Scope *scope = my_scopes.top();
	  if (name.is_qualified())
	  {
	    SymbolSet symbols = scope->lookup(name, Scope::DECLARATION);
	    if (symbols.empty()) throw Undefined(name);
	    // FIXME: We need type analysis / overload resolution
	    //        here to take the right symbol.
	    Symbol const *symbol = *symbols.begin();
	    while (name.is_qualified()) name = name.get_symbol();
	    scope = symbol->scope();
	    // TODO: check whether this is the definition of a previously
	    //       declared variable, according to 3.1/2 [basic.def]
	    scope->remove(symbol);
	  }

	  if (type.is_function()) // It's a function declaration.
	    scope->declare(name, new FunctionName(type, decl, false, scope));
	  else                    // It's a variable definition.
	    scope->declare(name, new VariableName(type, decl, true, scope));
	}
      }
    }
  }
}

void SymbolFactory::declare(Typedef const *td)
{
  Trace trace("SymbolFactory::declare(Typedef *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;
  PTree::Node const *declarations = third(td);
  while(declarations)
  {
    PTree::Node const *d = declarations->car();
    if(type_of(d) == Token::ntDeclarator)
    {
      Encoding const &name = d->encoded_name();
      Encoding const &type = d->encoded_type();
      Scope *scope = my_scopes.top();
      scope->declare(name, new TypedefName(type, d, scope));
    }
    declarations = tail(declarations, 2);
  }
}

void SymbolFactory::declare(EnumSpec const *spec)
{
  Trace trace("SymbolFactory::declare(EnumSpec *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;
  PTree::Node const *tag = second(spec);
  Encoding const &name = spec->encoded_name();
  Encoding const &type = spec->encoded_type();
  Scope *scope = my_scopes.top();
  if(tag && tag->is_atom()) 
    scope->declare(name, new EnumName(type, spec, my_scopes.top()));
  // else it's an anonymous enum

  PTree::Node const *body = third(spec);
  // The numeric value of an enumerator is either specified
  // by an explicit initializer or it is determined by incrementing
  // by one the value of the previous enumerator.
  // The default value for the first enumerator is 0
  long value = -1;
  for (PTree::Node const *e = second(body); e; e = rest(rest(e)))
  {
    PTree::Node const *enumerator = e->car();
    bool defined = true;
    if (enumerator->is_atom()) ++value;
    else  // [identifier = initializer]
    {
      PTree::Node const *initializer = third(enumerator);
      
      defined = evaluate_const(current_scope(), initializer, value);
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
    PTree::Encoding name(enumerator->position(), enumerator->length());
    if (defined)
      scope->declare(name, new ConstName(type, value, enumerator, true, scope));
    else
      scope->declare(name, new ConstName(type, enumerator, true, scope));
  }
}

void SymbolFactory::declare(NamespaceSpec const *spec)
{
  Trace trace("SymbolFactory::declare(NamespaceSpec *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;
  // Beware anonymous namespaces !
  Encoding name;
  if (second(spec)) name.simple_name(second(spec));
  else name.append_with_length("<anonymous>");
  Scope *scope = my_scopes.top();
  // Namespaces can be reopened, so only declare it if it isn't already known.
  SymbolSet symbols = scope->find(name, Scope::SCOPE);
  if (symbols.empty())
  {
    scope->declare(name, new NamespaceName(spec->encoded_type(), spec, true, scope));
  }
  // FIXME: assert that the found symbol really refers to a namespace !
}

void SymbolFactory::declare(ClassSpec const *spec)
{
  Trace trace("SymbolFactory::declare(ClassSpec *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;
  Encoding const &name = spec->encoded_name();
  // If class spec contains a class body, it's a definition.
  PTree::ClassBody const *body = spec->body();

  Scope *scope = my_scopes.top();

  SymbolSet symbols = scope->find(name, Scope::DEFAULT);
  for (SymbolSet::iterator i = symbols.begin(); i != symbols.end(); ++i)
  {
    // If the symbol was defined as a different type, the program is ill-formed.
    // Else if the symbol corresponds to a forward-declared class, replace it.
    if (ClassName const *class_ = dynamic_cast<ClassName const *>(*i))
    {
      if (class_->is_definition())
      {
	if (body)
	  throw MultiplyDefined(name, spec, class_->ptree()); // ODR
	else return; // Ignore forward declaration if symbol is already defined.
      }
      else if (body) scope->remove(*i); // Remove forward declaration.
      else return;                      // Don't issue another forward declaration.
    }
    else if (TypeName const *type = dynamic_cast<TypeName const *>(*i))
      // Symbol already defined as different type.
      throw MultiplyDefined(name, spec, type->ptree());
  }
  if (body)
    scope->declare(name, new ClassName(spec->encoded_type(), spec, true, scope));
  else
    scope->declare(name, new ClassName(spec->encoded_type(), spec, false, scope));
}

void SymbolFactory::declare(TemplateDecl const *tdecl)
{
  Trace trace("SymbolFactory::declare(TemplateDecl *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;
  PTree::Node const *body = PTree::nth(tdecl, 4);
  PTree::ClassSpec const *class_spec = get_class_template_spec(body);
  Scope *scope = my_scopes.top();
  if (class_spec)
  {
    Encoding const &name = class_spec->encoded_name();
    scope->declare(name, new ClassTemplateName(Encoding(), tdecl, true, scope));
  }
  else
  {
    PTree::Node const *decl = PTree::third(body);
    PTree::Encoding const &name = decl->encoded_name();
    scope->declare(name, new FunctionTemplateName(Encoding(), decl, scope));
  }
}

void SymbolFactory::declare(PTree::TypeParameter const *tparam)
{
  Trace trace("SymbolFactory::declare(TypeParameter *)", Trace::SYMBOLLOOKUP);
  Scope *scope = my_scopes.top();

  PTree::Node const *first = PTree::first(tparam);
  if (dynamic_cast<PTree::Kwd::Typename const *>(first) ||
      dynamic_cast<PTree::Kwd::Class const *>(first))
  {
    PTree::Node const *second = PTree::second(tparam);
    PTree::Encoding name;
    name.simple_name(second);
    scope->declare(name, new TypeName(Encoding(), tparam, true, scope));
  }
  else if (dynamic_cast<PTree::TemplateDecl const *>(first))
  {
    PTree::Node const *third = PTree::third(tparam);
    PTree::Encoding name;
    name.simple_name(third);
    scope->declare(name, new ClassTemplateName(Encoding(), tparam, true, scope));
  }
}

void SymbolFactory::declare(PTree::UsingDirective const *usingdir)
{
  Trace trace("SymbolFactory::declare(UsingDirective *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;
  my_scopes.top()->use(usingdir);
}

void SymbolFactory::declare(PTree::ParameterDeclaration const *pdecl)
{
  Trace trace("SymbolFactory::declare(ParameterDeclaration *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;
  PTree::Node const *decl = PTree::third(pdecl);
  PTree::Encoding const &name = decl->encoded_name();
  PTree::Encoding const &type = decl->encoded_type();
  if (my_prototype) // We are in a function prototype.
    my_prototype->declare(name, new VariableName(type, decl, true, my_prototype));
  else // We are in a template parameter list.
  {
    Scope *scope = my_scopes.top();
    scope->declare(name, new VariableName(type, decl, true, scope));
  }
}

void SymbolFactory::declare(PTree::UsingDeclaration const *usingdecl)
{
  Trace trace("SymbolFactory::declare(UsingDeclaration *)", Trace::SYMBOLLOOKUP);
  trace << "TBD !";
  if (my_language == NONE) return;
}