//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/SymbolFactory.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/PTree/Writer.hh>
#include <Synopsis/SymbolTable.hh>
#include <Synopsis/SymbolLookup.hh>
#include <Synopsis/TypeAnalysis/ConstEvaluator.hh>
#include <Synopsis/Trace.hh>
#include <cassert>
#include <vector>
#include <stdexcept>

using namespace Synopsis;
namespace PT = Synopsis::PTree;
namespace ST = Synopsis::SymbolTable;
namespace TA = Synopsis::TypeAnalysis;

namespace
{
PT::Node const *strip_cv_from_integral_type(PT::Node const *integral)
{
  if(integral == 0) return 0;

  if(!integral->is_atom())
    if(PT::is_a(integral->car(), Token::CONST, Token::VOLATILE))
      return PT::second(integral);
    else if(PT::is_a(PT::second(integral), Token::CONST, Token::VOLATILE))
      return integral->car();

  return integral;
}

PT::ClassSpec const *get_class_template_spec(PT::Node const *body)
{
  if(*PT::third(body) == ';') // template declaration
  {
    PT::Node const *spec = strip_cv_from_integral_type(PT::second(body));
    return dynamic_cast<PT::ClassSpec const *>(spec);
  }
  return 0;
}

//. Look up the scope corresponding to a base-specifier.
//. FIXME: The lookup may require a template instantiation, or
//.        fail because it involves a dependent name. This requires
//.        more design.
class BaseClassScopeFinder : private PT::Visitor
{
public:
  BaseClassScopeFinder(ST::Scope *scope) : my_scope(scope), my_result(0) {}
  ST::Class *lookup(PT::Node const *node)
  {
    const_cast<PT::Node *>(node)->accept(this);
    return my_result;
  }
private:
  virtual void visit(PT::Identifier *node)
  {
    PT::Encoding name = PT::Encoding::simple_name(node);
    ST::SymbolSet symbols = my_scope->unqualified_lookup(name, ST::Scope::ELABORATE);
    if (symbols.empty()) throw ST::Undefined(name, my_scope, node);
    else
    {
      ST::ClassName const *class_ = dynamic_cast<ST::ClassName const *>(*symbols.begin());
      if (!class_) throw InternalError("Base specifier not a class.");
      my_result = class_->as_scope();
    }
  }
  virtual void visit(PT::Name *node)
  {
    PT::Encoding name = node->encoded_name();
    // FIXME: This will fail if the name is a template or a dependent name.
    ST::SymbolSet symbols = ::lookup(name, my_scope, ST::Scope::ELABORATE);
    if (symbols.empty()) throw ST::Undefined(name, my_scope, node);
    else
    {
      ST::ClassName const *class_ = dynamic_cast<ST::ClassName const *>(*symbols.begin());
      if (!class_) throw InternalError("Base specifier not a class.");
      my_result = class_->as_scope();
    }
  }

  ST::Scope * my_scope;
  ST::Class * my_result;
};

//. Counts the number of parameters, as well as the number of default arguments.
//. If the function was declared before, check that the default argument
//. constraints hold (8.3.6/6)
class DefaultArgumentFinder : private PT::Visitor
{
public:
  DefaultArgumentFinder(size_t &params, size_t &default_args)
    : my_params(params), my_default_args(default_args) {}
  void find(PT::Declarator const *func)
  {
    const_cast<PT::Declarator *>(func)->accept(this);
  }
private:
  virtual void visit(PT::List *node)
  {
    if (node->car()) node->car()->accept(this);
    if (node->cdr()) node->cdr()->accept(this);
  }
  virtual void visit(PT::ParameterDeclaration *node)
  {
    ++my_params;
    if (PT::length(PT::third(node->car())) == 3 || my_default_args)
      // If we already encountered an initializer for a previous parameter,
      // this one has to have one too, though not necessarily expressed in
      // this declarator (see 8.3.6/6).
      ++my_default_args;
  }

  size_t &my_params;
  size_t &my_default_args;
};

}


SymbolFactory::SymbolFactory(Language l)
  : my_language(l),
    my_prototype(0),
    my_template_parameters(0),
    my_template_repository(new TA::TemplateRepository())
{
  // define the global scope
  my_scopes.push(new ST::Namespace(0, 0));
}

void SymbolFactory::enter_scope(PT::NamespaceSpec const *spec)
{
  Trace trace("SymbolFactory::enter_scope(NamespaceSpec)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;

  // Namespaces are only valid within namespaces.
  ST::Namespace *scope = dynamic_cast<ST::Namespace *>(my_scopes.top());
  assert(scope);

  ST::Namespace *namespace_ = 0;
  // If the namespace was already opened before, we add a reference
  // to it under the current NamespaceSpec, too.
  if (namespace_ = scope->find_namespace(spec))
  {
    scope->declare_scope(spec, namespace_);
  }
  else
  {
    // This is a new namespace. Declare it.
    namespace_ = new ST::Namespace(spec, scope);
    scope->declare_scope(spec, namespace_);
  }
  my_scopes.push(namespace_);
}

void SymbolFactory::enter_scope(PT::ClassSpec const *spec)
{
  Trace trace("SymbolFactory::enter_scope(ClassSpec)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;

  BaseClassScopeFinder base_finder(my_scopes.top());
  ST::Class::Bases bases;
  for (PT::Node const *base_clause = spec->base_clause();
       base_clause;
       base_clause = PT::rest(PT::rest(base_clause)))
  {
    // The last node is the name, the others access specs or 'virtual'
    PT::Node const *parent = PT::last(PT::second(base_clause))->car();
    ST::Class *class_ = base_finder.lookup(parent);
    if (class_) bases.push_back(class_);
    else ; // FIXME
  }
  ST::Scope *scope = my_scopes.top();
  PT::Encoding name = spec->encoded_name();

  if (name.is_qualified()) scope = lookup_scope_of_qname(name, spec);

  ST::Class *class_ = new ST::Class(name.unmangled(),
				    spec, scope, bases, my_template_parameters);
  if (name.is_template_id())
  {
    // This is a template specialization. Declare the scope with the
    // template repository.
    PT::Encoding::iterator begin = name.begin() + 1;
    PT::Encoding tname(begin, begin + *begin - 0x80 + 1);
    ST::SymbolSet symbols = scope->find(tname, ST::Scope::DECLARATION);
    if (symbols.empty())
      throw ST::Undefined(tname, scope);
    ST::ClassTemplateName const *primary = dynamic_cast<ST::ClassTemplateName const *>(*symbols.begin());
    my_template_repository->declare_scope(primary, spec, class_);
  }
  else
    scope->declare_scope(spec, class_);
  my_scopes.push(class_);
  my_template_parameters = 0;
}

void SymbolFactory::enter_scope(PT::Node const *decl)
{
  Trace trace("SymbolFactory::enter_scope(Node)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;
  ST::Scope *scope = my_scopes.top();
  // Create a PrototypeScope. If this is part of a function definition, we will
  // later convert it into a FunctionScope.
  my_prototype = new ST::PrototypeScope(decl, scope, my_template_parameters);
  scope->declare_scope(decl, my_prototype);
  my_scopes.push(my_prototype);
  my_template_parameters = 0;
}

void SymbolFactory::enter_scope(PT::FunctionDefinition const *decl)
{
  Trace trace("SymbolFactory::enter_scope(FunctionDefinition)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;

  ST::Scope *scope = my_scopes.top();

  assert(my_prototype);
  my_prototype->ref();
  scope->remove_scope(my_prototype->declaration());

  // look at the declarator's encoded name
  PT::Encoding name = PT::third(decl)->encoded_name();
  if (name.is_qualified())
    scope = lookup_scope_of_qname(name, PT::third(decl));

  // Transfer all symbols from the previously seen function declaration
  // into the newly created FunctionScope, and remove the PrototypeScope.
  ST::FunctionScope *func = new ST::FunctionScope(decl, my_prototype, scope);
  scope->declare_scope(decl, func);
  my_prototype = 0;
  my_scopes.push(func);
}

void SymbolFactory::enter_scope(PT::TemplateDecl const *params)
{
  Trace trace("SymbolFactory::enter_scope(TemplateDecl)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;

  ST::Scope *scope = my_scopes.top();
  ST::TemplateParameterScope *templ = new ST::TemplateParameterScope(params, scope);
  scope->declare_scope(params, templ);
  my_scopes.push(templ);
}

void SymbolFactory::enter_scope(PT::Block const *block)
{
  Trace trace("SymbolFactory::enter_scope(Block)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;

  ST::Scope *scope = my_scopes.top();
  ST::LocalScope *local = new ST::LocalScope(block, scope);
  scope->declare_scope(block, local);
  my_scopes.push(local);
}

void SymbolFactory::leave_scope()
{
  Trace trace("SymbolFactory::leave_scope", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;

  ST::Scope *scope = my_scopes.top();
  my_scopes.pop();
  // If this was a function prototype, keep it in case we see
  // the function body and want to transform it into a function
  // scope.
  if (ST::PrototypeScope *ps = dynamic_cast<ST::PrototypeScope *>(scope))
    my_prototype = ps;
  else if (ST::TemplateParameterScope *ts = dynamic_cast<ST::TemplateParameterScope *>(scope))
    my_template_parameters = ts;
  else
    scope->unref();
}

void SymbolFactory::declare(PT::Declaration const *d)
{
  Trace trace("SymbolFactory::declare(Declaration *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;

  PT::Node const *decls = PT::third(d);
  if(PT::is_a(decls, Token::ntDeclarator))
  {
    // function definition,
    // declare it only once (but allow overloading)

    PT::Encoding name = decls->encoded_name();
    PT::Encoding type = decls->encoded_type();

    size_t params = 0;
    size_t default_args = 0;
    DefaultArgumentFinder finder(params, default_args);
    finder.find(static_cast<PT::Declarator const *>(decls));

    // If the name is qualified, it has to be
    // declared already. If it hasn't, raise an error.
    ST::Scope *scope = my_scopes.top();
    if (name.is_qualified())
    {
      scope = lookup_scope_of_qname(name, decls);
      ST::SymbolSet symbols = scope->find(name, ST::Scope::DECLARATION);
      // FIXME: We need type analysis / overload resolution
      //        here to take the right symbol.
      ST::Symbol const *symbol = *symbols.begin();
      // TODO: check whether this is the definition of a previously
      //       declared function, according to 3.1/2 [basic.def]
      scope->remove(symbol);
    }
    scope->declare(name, new ST::FunctionName(type, d,
					      params, default_args,
					      true, scope));
  }
  else
  {
    // Function or variable declaration.
//     PT::Node const *storage_spec = PT::first(d);
//     PT::Node const *type_spec = PT::second(d);
    if (decls->is_atom()) ; // it is a ';'
    else
    {
      for (; decls; decls = decls->cdr())
      {
	PT::Declarator const *decl = dynamic_cast<PT::Declarator const *>(decls->car());
	if (!decl) continue; // a comma ?

	PT::Encoding name = decl->encoded_name();
	PT::Encoding const &type = decl->encoded_type();

	ST::Scope *scope = my_scopes.top();
	if (name.is_qualified())
	{
	  ST::SymbolSet symbols = lookup(name, scope, ST::Scope::DECLARATION);
	  if (symbols.empty()) throw ST::Undefined(name, scope, decl);
	  // FIXME: We need type analysis / overload resolution
	  //        here to take the right symbol.
	  ST::Symbol const *symbol = *symbols.begin();
	  while (name.is_qualified()) name = name.get_symbol();
	  scope = symbol->scope();
	  // TODO: check whether this is the definition of a previously
	  //       declared variable, according to 3.1/2 [basic.def]
	  scope->remove(symbol);
	}

	if (type.is_function()) // It's a function declaration.
	{
	  size_t params = 0;
	  size_t default_args = 0;
	  DefaultArgumentFinder finder(params, default_args);
	  finder.find(decl);
	  scope->declare(name, new ST::FunctionName(type, decl,
						    params, default_args,
						    false, scope));
	}
	else
	{
	  PT::Node *initializer = const_cast<PT::Declarator *>(decl)->initializer();
	  // FIXME: Checking the encoding for constness is not sufficient, as the
	  //        type id may be an alias (typedef) to a const type.
	  if (initializer && 
	      (type.front() == 'C' || 
	       (type.front() == 'V' && *(type.begin() + 1) == 'C')))
	  {
	    long value;
	    if (TA::evaluate_const(current_scope(), initializer->car(), value))
	      scope->declare(name, new ST::ConstName(type, value, decl, true, scope));
	    else
	      scope->declare(name, new ST::ConstName(type, decl, true, scope));
	  }
	  else
	    scope->declare(name, new ST::VariableName(type, decl, true, scope));
	}
      }
    }
  }
}

void SymbolFactory::declare(PT::Typedef const *td)
{
  Trace trace("SymbolFactory::declare(Typedef *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;
  PT::Node const *declarations = PT::third(td);
  while(declarations)
  {
    PT::Node const *d = declarations->car();
    if(PT::type_of(d) == Token::ntDeclarator)
    {
      PT::Encoding const &name = d->encoded_name();
      PT::Encoding const &type = d->encoded_type();
      ST::Scope *scope = my_scopes.top();
      scope->declare(name, new ST::TypedefName(type, d, scope));
    }
    declarations = PT::tail(declarations, 2);
  }
}

void SymbolFactory::declare(PT::EnumSpec const *spec)
{
  Trace trace("SymbolFactory::declare(EnumSpec *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;
  PT::Node const *tag = PT::second(spec);
  PT::Encoding const &name = spec->encoded_name();
  PT::Encoding const &type = spec->encoded_type();
  ST::Scope *scope = my_scopes.top();
  if(tag && tag->is_atom()) 
    scope->declare(name, new ST::EnumName(type, spec, my_scopes.top()));
  // else it's an anonymous enum

  PT::Node const *body = PT::third(spec);
  // The numeric value of an enumerator is either specified
  // by an explicit initializer or it is determined by incrementing
  // by one the value of the previous enumerator.
  // The default value for the first enumerator is 0
  long value = -1;
  for (PT::Node const *e = PT::second(body); e; e = PT::rest(PT::rest(e)))
  {
    PT::Node const *enumerator = e->car();
    bool defined = true;
    if (enumerator->is_atom()) ++value;
    else  // [identifier = initializer]
    {
      PT::Node const *initializer = PT::third(enumerator);
      
      defined = TA::evaluate_const(current_scope(), initializer, value);
      enumerator = enumerator->car();
#ifndef NDEBUG
      if (!defined)
      {
	std::cerr << "Error in evaluating enum initializer:\n"
		  << "Expression doesn't evaluate to a constant integral value:\n"
		  << PT::reify(initializer) << std::endl;
      }
#endif
    }
    assert(enumerator->is_atom());
    PT::Encoding name = PT::Encoding::simple_name(static_cast<PT::Atom const *>(enumerator));
    if (defined)
      scope->declare(name, new ST::ConstName(type, value, enumerator, true, scope));
    else
      scope->declare(name, new ST::ConstName(type, enumerator, true, scope));
  }
}

void SymbolFactory::declare(PT::NamespaceSpec const *spec)
{
  Trace trace("SymbolFactory::declare(NamespaceSpec *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;
  // Beware anonymous namespaces !
  PT::Encoding name;
  if (PT::second(spec)) name.simple_name(PT::second(spec));
  else name.append_with_length("<anonymous>");
  ST::Scope *scope = my_scopes.top();
  // Namespaces can be reopened, so only declare it if it isn't already known.
  ST::SymbolSet symbols = scope->find(name, ST::Scope::SCOPE);
  if (symbols.empty())
  {
    scope->declare(name, new ST::NamespaceName(spec->encoded_type(), spec, true, scope));
  }
  // FIXME: assert that the found symbol really refers to a namespace !
}

void SymbolFactory::declare(PT::ClassSpec const *spec)
{
  Trace trace("SymbolFactory::declare(ClassSpec *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;

  ST::Scope *scope = my_scopes.top();
  PT::Encoding name = spec->encoded_name();

  if (name.is_qualified()) scope = lookup_scope_of_qname(name, spec);

  // If the name is a template-id, we are looking at a template specialization.
  // Declare it in the template repository instead.
  if (name.is_template_id())
  {
    declare_template_specialization(name.get_template_name(), spec, scope);
    return;
  }
  // If class spec contains a class body, it's a definition.
  PT::ClassBody const *body = const_cast<PT::ClassSpec *>(spec)->body();

  ST::SymbolSet symbols = scope->find(name, ST::Scope::DEFAULT);
  for (ST::SymbolSet::iterator i = symbols.begin(); i != symbols.end(); ++i)
  {
    // If the symbol was defined as a different type, the program is ill-formed.
    // Else if the symbol corresponds to a forward-declared class, replace it.
    if (ST::ClassName const *class_ = dynamic_cast<ST::ClassName const *>(*i))
    {
      if (class_->is_definition())
      {
	if (body)
	  throw ST::MultiplyDefined(name, spec, class_->ptree()); // ODR
	else return; // Ignore forward declaration if symbol is already defined.
      }
      else if (body) scope->remove(*i); // Remove forward declaration.
      else return;                      // Don't issue another forward declaration.
    }
    else if (ST::TypeName const *type = dynamic_cast<ST::TypeName const *>(*i))
      // Symbol already defined as different type.
      throw ST::MultiplyDefined(name, spec, type->ptree());
  }
  if (body)
    scope->declare(name, new ST::ClassName(spec->encoded_type(), spec, true, scope));
  else
    scope->declare(name, new ST::ClassName(spec->encoded_type(), spec, false, scope));
}

void SymbolFactory::declare(PT::TemplateDecl const *tdecl)
{
  Trace trace("SymbolFactory::declare(TemplateDecl *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;
  PT::Node const *body = PT::nth(tdecl, 4);
  PT::ClassSpec const *class_spec = get_class_template_spec(body);
  ST::Scope *scope = my_scopes.top();
  if (class_spec)
  {
    PT::Encoding name = class_spec->encoded_name();

    if (name.is_qualified()) scope = lookup_scope_of_qname(name, tdecl);

    // If the name is a template-id, we are looking at a template specialization.
    // Declare it in the template repository instead.
    if (name.is_template_id())
      declare_template_specialization(name.get_template_name(), class_spec, scope);
    else
    {
      // Find any forward declarations for this symbol.
      scope->declare(name, new ST::ClassTemplateName(PT::Encoding(), tdecl, true, scope));
    }
  }
  else
  {
    PT::Node const *decl = PT::third(body);
    PT::Encoding name = decl->encoded_name();
    if (name.is_qualified())
    {
      scope = lookup_scope_of_qname(name, decl);
      ST::SymbolSet symbols = scope->find(name, ST::Scope::DECLARATION);
      // FIXME: We need type analysis / overload resolution
      //        here to take the right symbol.
      ST::Symbol const *symbol = *symbols.begin();
      // TODO: check whether this is the definition of a previously
      //       declared function, according to 3.1/2 [basic.def]
      scope->remove(symbol);
    }
    size_t params = 0;
    size_t default_args = 0;
    DefaultArgumentFinder finder(params, default_args);
    finder.find(static_cast<PT::Declarator const *>(decl));
    scope->declare(name, new ST::FunctionTemplateName(PT::Encoding(), decl,
						      params, default_args, scope));
  }
}

void SymbolFactory::declare(PT::TypeParameter const *tparam)
{
  Trace trace("SymbolFactory::declare(TypeParameter *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;
  ST::Scope *scope = my_scopes.top();

  PT::Node const *first = PT::first(tparam);
  if (dynamic_cast<PT::Kwd::Typename const *>(first) ||
      dynamic_cast<PT::Kwd::Class const *>(first))
  {
    PT::Node const *second = PT::second(tparam);
    PT::Encoding name;
    name.simple_name(second);
    scope->declare(name, new ST::TypeName(PT::Encoding(), tparam, true, scope));
  }
  else if (PT::TemplateDecl const *tdecl = 
	   dynamic_cast<PT::TemplateDecl const *>(first))
  {
    // tdecl has 4 or 5 members:
    // [template < parameter-list > class id]
    // [template < parameter-list > class]
    PT::Encoding name;
    PT::Node const *pname = PT::nth(tdecl, 5);
    if (pname) name.simple_name(pname);
    scope->declare(name, new ST::ClassTemplateName(PT::Encoding(), tdecl, true, scope));
  }
}

void SymbolFactory::declare(PT::UsingDirective const *udir)
{
  Trace trace("SymbolFactory::declare(UsingDirective *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;
  ST::Scope *scope = my_scopes.top();

  PTree::Encoding name = PTree::third(udir)->encoded_name();
  ST::SymbolSet symbols = lookup(name, scope, ST::Scope::DEFAULT);
  if (symbols.empty())
    throw ST::Undefined(name, scope, udir);
  // now get the namespace associated with that symbol:
  ST::Symbol const *symbol = *symbols.begin();
  // FIXME: may this symbol-to-scope conversion be implemented
  //        by the appropriate Symbol subclass(es) ?
  ST::Scope const *s = symbol->scope()->find_scope(name, symbol);
  ST::Namespace const *namespace_ = dynamic_cast<ST::Namespace const *>(s);

  if (ST::Namespace *ns = dynamic_cast<ST::Namespace *>(scope))
    ns->use(namespace_);
  else if (ST::FunctionScope *fs = dynamic_cast<ST::FunctionScope *>(scope))
    fs->use(namespace_);
  else
    throw std::runtime_error("using directive in wrong context.");
}

void SymbolFactory::declare(PT::ParameterDeclaration const *pdecl)
{
  Trace trace("SymbolFactory::declare(ParameterDeclaration *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;
  PT::Node const *decl = PT::third(pdecl);
  PT::Encoding const &name = decl->encoded_name();
  PT::Encoding const &type = decl->encoded_type();
  if (!name.empty())
  {
    ST::Scope *scope = my_scopes.top();
    scope->declare(name, new ST::VariableName(type, decl, true, scope));
  }
}

void SymbolFactory::declare(PT::UsingDeclaration const *udecl)
{
  Trace trace("SymbolFactory::declare(UsingDeclaration *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;
  PT::Encoding const &name = PT::third(udecl)->encoded_name();
  PT::Encoding alias;
  for (PT::Encoding::name_iterator i = name.begin_name();
       i != name.end_name();
       ++i)
    alias = *i; // find last component

  ST::Scope *scope = my_scopes.top();
  ST::SymbolSet symbols = lookup(name, scope, ST::Scope::DECLARATION);

  for (ST::SymbolSet::iterator i = symbols.begin(); i != symbols.end(); ++i)
  {
    // TODO: check validity according to [namespace.udecl] (7.3.3)
    scope->declare(alias, *i);
  }
}

ST::Scope *SymbolFactory::lookup_scope_of_qname(PT::Encoding &name,
						PT::Node const *decl)
{
  Trace trace("SymbolFactory::lookup_scope_of_qname", Trace::SYMBOLLOOKUP);

  ST::Scope *scope = my_scopes.top();
  ST::SymbolSet symbols = lookup(name, scope, ST::Scope::DECLARATION);
  if (symbols.empty()) throw ST::Undefined(name, scope, decl);
  ST::Symbol const *symbol = *symbols.begin();
  while (name.is_qualified()) name = name.get_symbol();
  scope = symbol->scope();
  return scope;
}

void SymbolFactory::declare_template_specialization(PT::Encoding const &name,
						    PT::ClassSpec const *spec,
						    ST::Scope *scope)
{
  Trace trace("SymbolFactory::declare_template_specialization", Trace::SYMBOLLOOKUP);
  // Find primary template and declare specialization.
  ST::SymbolSet symbols = scope->find(name, ST::Scope::DECLARATION);
  if (symbols.empty())
    throw ST::Undefined(name, scope);
  ST::ClassTemplateName const * templ = dynamic_cast<ST::ClassTemplateName const *>(*symbols.begin());
  if (!templ)
    throw ST::TypeError(name, (*symbols.begin())->type());
  my_template_repository->declare(templ, spec);
}
