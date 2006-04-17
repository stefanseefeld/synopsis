//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/SymbolFactory.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/SymbolTable/Display.hh>
#include <Synopsis/PTree/Writer.hh>
#include <Synopsis/SymbolTable.hh>
#include <Synopsis/SymbolLookup.hh>
#include <Synopsis/TypeAnalysis/ConstEvaluator.hh>
#include <Synopsis/TypeAnalysis/TypeRepository.hh>
#include <Synopsis/Trace.hh>
#include <cassert>
#include <vector>
#include <stdexcept>

using namespace Synopsis;
namespace PT = Synopsis::PTree;
namespace ST = Synopsis::SymbolTable;
namespace TA = Synopsis::TypeAnalysis;

namespace Synopsis
{
namespace SymbolTable
{
Class * DEPENDENT_SCOPE = 0;
}
}

namespace
{
//. Look up the scope corresponding to a symbol.
class SymbolScopeMapper : private ST::SymbolVisitor
{
public:
  ST::Class *to_scope(ST::Symbol const *s)
  {
    scope_ = 0;
    s->accept(this);
    return scope_;
  }
private:
  virtual void visit(ST::TypedefName const *s)
  {
    PT::Encoding encoding;
    ST::Symbol const *a = resolve_typedef(s, encoding);
    if (a && a != s) a->accept(this);
  }
  virtual void visit(ST::ClassName const *s) { scope_ = s->as_scope();}
  virtual void visit(ST::ClassTemplateName const *s) { scope_ = s->as_scope();}

  ST::Class *scope_;
};

//. Counts the number of parameters, as well as the number of default arguments.
//. If the function was declared before, check that the default argument
//. constraints hold (8.3.6/6)
class DefaultArgumentFinder : private PT::Visitor
{
public:
  DefaultArgumentFinder(size_t &params, size_t &default_args)
    : params_(params), default_args_(default_args) {}
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
  virtual void visit(PT::ParameterDeclaration *decl)
  {
    ++params_;
    // If we already encountered an initializer for a previous parameter,
    // this one has to have one too, though not necessarily expressed in
    // this declarator (see 8.3.6/6).
    if (default_args_ || decl->initializer()) ++default_args_;
  }

  size_t &params_;
  size_t &default_args_;
};

//. ETScopeFinder finds the smallest enclosing scope which a class-name
//. from an elaborated-type-specifier should be declared in.
//. See [basic.scope.pdecl]/5
class ETScopeFinder : private ST::ScopeVisitor
{
public:
  ETScopeFinder() : scope_(0) {}
  ST::Scope *find(ST::Scope *s)
  {
    s->accept(this);
    return scope_;
  }
private:
  virtual void visit(ST::LocalScope *s) { scope_ = s;}
  virtual void visit(ST::PrototypeScope *s) { s->outer_scope()->accept(this);}
  virtual void visit(ST::FunctionScope *s) { scope_ = s;}
  virtual void visit(ST::Class *s) { s->outer_scope()->accept(this);}
  virtual void visit(ST::Namespace *s) { scope_ = s;}

  ST::Scope *scope_;
};

}


SymbolFactory::SymbolFactory(Language l)
  : language_(l),
    prototype_(0),
    tparameters_(0),
    templates_(new TA::TemplateRepository())
{
  // define the global scope
  ST::Namespace *global = new ST::Namespace(0, 0);
  scopes_.push(global);
  // FIXME: Do we need a special GCC-extension flag, too ?
  PT::Encoding name;
  name.append_with_length("__builtin_va_list", 17);
  global->declare(name, new ST::TypeName(PT::Encoding(), 0, true, global));

  ST::DEPENDENT_SCOPE = new ST::DependentScope();
}

void SymbolFactory::enter_scope(ST::Scope *scope)
{
  Trace trace("SymbolFactory::enter_scope(Scope)", Trace::SYMBOLLOOKUP);
  scopes_.push(scope);
}

// Note: Here the scope argument doesn't make sense, since a namespace-name
//       can never be qualified.
void SymbolFactory::enter_scope(ST::Scope *, PT::NamespaceSpec const *spec)
{
  Trace trace("SymbolFactory::enter_scope(NamespaceSpec)", Trace::SYMBOLLOOKUP);

  // Namespaces are only valid within namespaces.
  ST::Namespace *scope = dynamic_cast<ST::Namespace *>(scopes_.top());
  assert(scope);

  ST::Namespace *namespace_ = 0;
  // If the namespace was already opened before, we add a reference
  // to it under the current NamespaceSpec, too.
  if ((namespace_ = scope->find_namespace(spec)))
  {
    scope->declare_scope(spec, namespace_);
  }
  else
  {
    // This is a new namespace. Declare it.
    namespace_ = new ST::Namespace(spec, scope);
    scope->declare_scope(spec, namespace_);
  }
  scopes_.push(namespace_);
}

void SymbolFactory::enter_class(ST::Scope *scope,
				PT::ClassSpec const *spec,
				std::vector<ST::Symbol const *> const &b)
{
  Trace trace("SymbolFactory::enter_class", Trace::SYMBOLLOOKUP);

  ST::Class::Bases bases;
  SymbolScopeMapper mapper;
  for (std::vector<ST::Symbol const *>::const_iterator i = b.begin();
       i != b.end();
       ++i)
  {
    ST::Class const *class_ = mapper.to_scope(*i);
    trace << "looking for base scope " << class_;
    if (class_) bases.push_back(class_);
  }
  if (!scope) scope = scopes_.top();
  PT::Encoding encoded_name = spec->encoded_name();
  PT::Encoding name = encoded_name;
  if (name.is_template_id()) name = name.get_template_name();

  if (encoded_name.is_qualified()) scope = lookup_scope_of_qname(encoded_name, spec);

  ST::Class *class_ = new ST::Class(name.unmangled(),
				    spec, scope, bases, tparameters_);
  // [class] (9/2)
  // The class-name is also inserted into the scope of the class itself.
  PT::Encoding type = spec->encoded_type();
  class_->declare(name, new ST::ClassName(type, spec, false, scope));
  // [temp.local] (14.6.1)
  // Within the scope of a class template, when the name of the template is neither
  // qualified nor followed by <, it is equivalent to the name of the template
  // followed by the template-parameters enclosed in <>.
  //
  // Thus, if we are in a class template, we inject the class name also as
  // a class template name so looking for a template-name will be successful.
  trace << "is template " << tparameters_;
  if (tparameters_)
    class_->declare(name, new ST::ClassTemplateName(type, spec, false, scope));

  if (encoded_name.is_template_id())
  {
    // This is a template specialization. Declare the scope with the
    // template repository.
    ST::SymbolSet symbols = scope->find(name, ST::Scope::DECLARATION);
    if (symbols.empty())
    {
      trace << "undefined symbol " << name << " in " << static_cast<ST::Namespace *>(scope)->name();
      throw ST::Undefined(name, scope);
    }
    ST::ClassTemplateName const *primary = 
      dynamic_cast<ST::ClassTemplateName const *>(*symbols.begin());
    templates_->declare_scope(primary, spec, class_);
  }
  scope->declare_scope(spec, class_);
  scopes_.push(class_);
  tparameters_ = 0;
}

void SymbolFactory::enter_scope(ST::Scope *scope, PT::List const *decl)
{
  Trace trace("SymbolFactory::enter_scope(List)", Trace::SYMBOLLOOKUP);

  if (!scope) scope = scopes_.top();
  trace << "current scope is " << scope;
  // Create a PrototypeScope. If this is part of a function definition, we will
  // later convert it into a FunctionScope.
  prototype_ = new ST::PrototypeScope(decl, scope, tparameters_);
  scope->declare_scope(decl, prototype_);
  scopes_.push(prototype_);
//   tparameters_ = 0;
}

void SymbolFactory::enter_scope(ST::Scope *scope, PT::FunctionDefinition const *decl)
{
  Trace trace("SymbolFactory::enter_scope(FunctionDefinition)", Trace::SYMBOLLOOKUP);

  if (!scope) scope = scopes_.top();

  assert(prototype_);
  prototype_->ref();
  trace << "current scope is " << scope << ' ' << typeid(*scope).name()
	<< ' ' << decl->declarator()->encoded_name();
  scope->remove_scope(prototype_->declaration());

  // Transfer all symbols from the previously seen function declaration
  // into the newly created FunctionScope, and remove the PrototypeScope.
  ST::FunctionScope *func = new ST::FunctionScope(decl, prototype_, scope);
  scope->declare_scope(decl, func);
  prototype_ = 0;
  scopes_.push(func);
  tparameters_ = 0;
}

// Note: The scope parameter doesn't make sense here, as a template-parameter-list
//       can't be qualified.
void SymbolFactory::enter_scope(ST::Scope *, PT::TemplateParameterList const *params)
{
  Trace trace("SymbolFactory::enter_scope(TemplateParameterList)", Trace::SYMBOLLOOKUP);

  ST::Scope *scope = scopes_.top();
  tparameters_ = 
    new ST::TemplateParameterScope(params, scope, tparameters_);
  trace << "enter template parameter scope " << tparameters_;
  // We don't want to declare template parameter scopes 
  // in their parent scopes, do we ?
//   scope->declare_scope(params, templ);
  scopes_.push(tparameters_);
}

void SymbolFactory::enter_scope(ST::Scope *, PT::Block const *block)
{
  Trace trace("SymbolFactory::enter_scope(Block)", Trace::SYMBOLLOOKUP);

  ST::Scope *scope = scopes_.top();
  ST::LocalScope *local = new ST::LocalScope(block, scope);
  scope->declare_scope(block, local);
  scopes_.push(local);
}

void SymbolFactory::leave_scope()
{
  Trace trace("SymbolFactory::leave_scope", Trace::SYMBOLLOOKUP);

  ST::Scope *scope = scopes_.top();
  scopes_.pop();
  // If this was a function prototype, keep it in case we see
  // the function body and want to transform it into a function
  // scope.
  if (ST::PrototypeScope *ps = dynamic_cast<ST::PrototypeScope *>(scope))
    prototype_ = ps;
}

void SymbolFactory::declare_typedef(PT::SimpleDeclaration *d, ST::Symbol const *s)
{
  Trace trace("SymbolFactory::declare_typedef", Trace::SYMBOLLOOKUP);
  PT::DeclSpec *spec = d->decl_specifier_seq();
  PT::List *declarators = d->declarators();
  while(declarators)
  {
    PT::Node const *d = declarators->car();
    PT::Encoding const &name = d->encoded_name();
    PT::Encoding const &type = d->encoded_type();

    // Hack: in case of 'typedef struct Foo {...} Foo;'
    //       we don't generate a TypedefName, as that would clash with
    //       the ClassName of the same name.
    if (name != spec->type())
    {
      ST::Scope *scope = scopes_.top();
      ST::TypedefName *symbol = new ST::TypedefName(type, d, scope, s);
      scope->declare(name, symbol);
      // TODO: Even though a typedef doesn't introduce a new type, we may
      //       declare it as a type alias to avoid an additional lookup.
      // TA::TypeRepository::instance()->declare(symbol);
    }
    declarators = PT::tail(declarators, 2);
  }
}

void SymbolFactory::declare(ST::Scope *s, PT::SimpleDeclaration *d)
{
  Trace trace("SymbolFactory::declare(SimpleDeclaration)", Trace::SYMBOLLOOKUP);
  trace << "template parameters " << tparameters_;
  if (s) scopes_.push(s);
  ST::Scope *scope = scopes_.top();

  PT::DeclSpec *spec = d->decl_specifier_seq();
  PT::List *decls = d->declarators();
  // The implicit type declaration by means of elaborated type specs is handled
  // elsewhere.
  if (!decls) // for example 'class Foo;'
  {
    if (spec && length(spec) == 1)
    {
      PT::ElaboratedTypeSpec *etspec = 
	dynamic_cast<PT::ElaboratedTypeSpec *>(PT::nth<0>(spec));
      if (etspec && PT::string(etspec->type()) != "enum")
      {
	PT::Encoding name = PT::name(etspec->name());
	if (!name.is_qualified())
	{
	  ST::SymbolSet symbols = scope->find(name, ST::Scope::ELABORATED);
	  if (!symbols.size())
	  {
	    if (tparameters_)
	    {
	      ST::ClassTemplateName *symbol = 
		new ST::ClassTemplateName(spec->encoded_type(), spec, false, scope);
	      scope->declare(name, symbol);
	    }
	    else
	    {
	      ST::ClassName *symbol = new ST::ClassName(name, spec, false, scope);
	      scope->declare(name, symbol);
	      TA::TypeRepository::instance()->declare(name, symbol);
	    }
	  }
	}
      }
    }
  }
  // For special functions (constructor, destructor, type conversions) there
  // may be no decl-specifier-seq.
  bool defined = spec ? spec->storage_class() != PT::DeclSpec::EXTERN : true;
  for (; decls; decls = decls->cdr())
  {
    PT::Declarator const *decl = dynamic_cast<PT::Declarator const *>(decls->car());
    if (!decl) continue; // a comma ?
      
    PT::Encoding name = decl->encoded_name();
    PT::Encoding const &type = decl->encoded_type();
    
    if (name.is_qualified())
    {
      ST::SymbolSet symbols = lookup(name, scope, ST::Scope::DECLARATION);
      if (symbols.empty())
      {
	std::cout << "!1" << std::endl;
	throw ST::Undefined(name, scope, decl);
      }
      // FIXME: We need type analysis / overload resolution
      //        here to take the right symbol.
      ST::Symbol const *forward = *symbols.begin();
      while (name.is_qualified()) name = name.get_symbol();
      scope = forward->scope();
      // TODO: check whether this is the definition of a previously
      //       declared variable, according to 3.1/2 [basic.def]
      scope->remove(forward);
    }
      
    ST::Symbol const *symbol = 0;
    if (type.is_function()) // It's a function declaration.
    {
      size_t params = 0;
      size_t default_args = 0;
      DefaultArgumentFinder finder(params, default_args);
      finder.find(decl);
      if (tparameters_)
	symbol = new ST::FunctionTemplateName(type, decl, params, default_args, false, scope);
      else
	symbol = new ST::FunctionName(type, decl, params, default_args, false, scope);
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
// 	if (TA::evaluate_const(current_scope(), initializer->car(), value))
// 	  symbol = new ST::ConstName(type, value, decl, defined, scope);
// 	else
	  symbol = new ST::ConstName(type, decl, defined, scope);
      }
      else
	symbol = new ST::VariableName(type, decl, defined, scope);
    }
    scope->declare(name, symbol);
  }
  tparameters_ = 0;
  if (s) scopes_.pop();
}

void SymbolFactory::declare_specialization(ST::Scope *s,
					   PT::SimpleDeclaration *decl)
{
  Trace trace("SymbolFactory::declare(SimpleDeclaration)", Trace::SYMBOLLOOKUP);
  tparameters_ = 0;
}

void SymbolFactory::declare(ST::Scope *s, PT::FunctionDefinition *def)
{
  Trace trace("SymbolFactory::declare(FunctionDefinition)", Trace::SYMBOLLOOKUP);
  trace << "template parameters " << tparameters_;
  if (s) scopes_.push(s);

  PT::Declarator *declarator = def->declarator();

  PT::Encoding name = declarator->encoded_name();
  PT::Encoding type = declarator->encoded_type();

  size_t params = 0;
  size_t default_args = 0;
  DefaultArgumentFinder finder(params, default_args);
  finder.find(declarator);

  // If the name is qualified, it has to be
  // declared already. If it hasn't, raise an error.
  ST::Scope *scope = scopes_.top();

  // TODO: If this is an explicit specialization, look up the primary template
  //       and register the specialization with the template repository.
//   if (tparameters_ && 

  ST::Symbol const *symbol = 0;
  if (tparameters_)
    symbol = new ST::FunctionTemplateName(type, def, params, default_args, true, scope);
  else
    symbol = new ST::FunctionName(type, def, params, default_args, true, scope);
  scope->declare(name, symbol);

  tparameters_ = 0;
  if (s) scopes_.pop();
}

void SymbolFactory::declare_specialization(ST::Scope *s,
					   PT::FunctionDefinition *def)
{
  Trace trace("SymbolFactory::declare(FunctionDefinition)", Trace::SYMBOLLOOKUP);
  tparameters_ = 0;
}

void SymbolFactory::declare(ST::Scope *scope, PT::EnumSpec const *spec)
{
  Trace trace("SymbolFactory::declare(EnumSpec)", Trace::SYMBOLLOOKUP);

  if (!scope) scope = scopes_.top();
  if (PT::Atom const *name = spec->name())
  {
    ST::EnumName const *symbol = new ST::EnumName(spec, scopes_.top());
    scope->declare(PT::Encoding(name), symbol);
    TA::TypeRepository::instance()->declare(PT::Encoding(name), symbol);
  }
  // else it's an anonymous enum

  // The numeric value of an enumerator is either specified
  // by an explicit initializer or it is determined by incrementing
  // by one the value of the previous enumerator.
  // The default value for the first enumerator is 0
  long value = -1;
  for (PT::List const *e = spec->enumerators(); e; e = PT::tail(e, 2))
  {
    PT::Node const *enumerator = e->car();
    bool defined = true;
    if (enumerator->is_atom()) ++value;
    else  // [identifier = initializer]
    {
      PT::Node const *initializer = PT::nth<2>(static_cast<PT::List const *>(enumerator));
      
//       defined = TA::evaluate_const(current_scope(), initializer, value);
      enumerator = enumerator->car();
    }
    assert(enumerator->is_atom());
    PT::Encoding name(static_cast<PT::Atom const *>(enumerator));
    PT::Encoding type; // FIXME: ??
    if (defined)
      scope->declare(name, new ST::ConstName(type, value, enumerator, true, scope));
    else
      scope->declare(name, new ST::ConstName(type, enumerator, true, scope));
  }
}

void SymbolFactory::declare(ST::Scope *scope, PT::NamespaceSpec const *spec)
{
  Trace trace("SymbolFactory::declare(NamespaceSpec)", Trace::SYMBOLLOOKUP);

  // Beware anonymous namespaces !
  PT::Encoding name;
  if (PT::nth<1>(spec)) name.simple_name(static_cast<PT::Atom const *>(PT::nth<1>(spec)));
  else name.append_with_length("<anonymous>", 11);
  if (!scope) scope = scopes_.top();
  // Namespaces can be reopened, so only declare it if it isn't already known.
  ST::SymbolSet symbols = scope->find(name, ST::Scope::SCOPE);
  if (symbols.empty())
  {
    scope->declare(name, new ST::NamespaceName(spec->encoded_type(), spec, true, scope));
  }
  // FIXME: assert that the found symbol really refers to a namespace !
}

void SymbolFactory::declare(ST::Scope *scope, PT::ClassSpec const *spec)
{
  Trace trace("SymbolFactory::declare(ClassSpec)", Trace::SYMBOLLOOKUP);
  trace << scope;
  if (!scope) scope = scopes_.top();
  PT::Encoding name = spec->encoded_name();

  // Different scenarios are possible:
  //
  // 1) We are declaring a simple class.
  // 2) We are declaring a class template.
  // 3) We are declaring a specialization for a class template.
  // 4) We are declaring a partial specialization for a class template.
  //
  // 

  // If the name is a template-id, we are looking at a template specialization.
  // Declare it in the template repository instead.
  if (name.is_template_id())
  {
    trace << "has template parameters " << tparameters_;
    ST::SymbolSet symbols = scope->find(name, ST::Scope::DECLARATION);
    if (symbols.empty())
    {
      std::cout << "!4" << std::endl;
      throw ST::Undefined(name, scope);
    }
    ST::ClassTemplateName const * templ = dynamic_cast<ST::ClassTemplateName const *>(*symbols.begin());
    if (!templ)
      throw ST::TypeError(name, (*symbols.begin())->type());
    std::cout << "declare template specialization" << std::endl;
//   templates_->declare(templ, decl);
    return;
  }
  ST::SymbolSet symbols = scope->find(name, ST::Scope::DEFAULT);
  for (ST::SymbolSet::iterator i = symbols.begin(); i != symbols.end(); ++i)
  {
    // If the symbol was defined as a different type, the program is ill-formed.
    // Else if the symbol corresponds to a forward-declared class, replace it.
    if (ST::ClassName const *class_ = dynamic_cast<ST::ClassName const *>(*i))
    {
      trace << "found class " << class_->is_definition() << ' ' << tparameters_;
      if (class_->is_definition())
	throw ST::MultiplyDefined(name, spec, class_->ptree()); // ODR
      // FIME: if we have 'template <typename T> struct Foo<T>::Bar'
      //       Bar is *not* a class template !
      else if (tparameters_) // type mismatch
	throw ST::MultiplyDefined(name, spec, class_->ptree()); // type mismatch
      // TODO: Remove the type associated with the forward declaration, too.
      else scope->remove(*i); // Remove forward declaration.
    }
    else if (ST::ClassTemplateName const *class_ = dynamic_cast<ST::ClassTemplateName const *>(*i))
    {
      if (class_->is_definition())
	throw ST::MultiplyDefined(name, spec, class_->ptree()); // ODR
      else if (!tparameters_) // type mismatch
	throw ST::MultiplyDefined(name, spec, class_->ptree()); // type mismatch
      // TODO: Remove the type associated with the forward declaration, too.
      else scope->remove(*i); // Remove forward declaration.
    }
    else if (ST::TypeName const *type = dynamic_cast<ST::TypeName const *>(*i))
      // Symbol already defined as different type.
      throw ST::MultiplyDefined(name, spec, type->ptree());
  }
  trace << "has template parameters " << tparameters_;
  if (tparameters_)
  {
    ST::ClassTemplateName *symbol = 
      new ST::ClassTemplateName(spec->encoded_type(), spec, true, scope);
    scope->declare(name, symbol);
    std::cout << "declare class template " << std::endl;
//     templates_->declare(symbol, tparameters_);
  }
  else
  {
    ST::ClassName *symbol = 
      new ST::ClassName(spec->encoded_type(), spec, true, scope);
    scope->declare(name, symbol);
    TA::TypeRepository::instance()->declare(name, symbol);
  }
}

void SymbolFactory::declare(ST::Scope *scope, PT::ElaboratedTypeSpec const *spec)
{
  Trace trace("SymbolFactory::declare(ElaboratedTypeSpec)", Trace::SYMBOLLOOKUP);

  // [basic.scope.pdecl]/5
  // The point of declaration of a class first declared in an 
  // elaborated-type-specifier is as follows:
  //
  //   - for an elaborated-type-specifier of the form
  //       class-key identifier ;
  //     the elaborated-type-specifier declares the identifier to be a
  //     class-name in the scope that contains the declaration, otherwise
  //
  //   - for an elaborated-type-specifier of the form
  //       class-key identifier
  //     if the elaborated-type-specifier is used in the decl-specifier-seq or
  //     parameter-declaration-clause of a function defined in namespace scope,
  //     the identifier is declared as a class-name in the namespace that contains
  //     the declaration; otherwise, except as a friend declaration, the identifier
  //     is declared in the smallest non-class, non-function-prototype scope that
  //     contains the declaration.
  
  // The first case above is handled as a declaration. The second case is
  // handled here.
  
  // Check for class-key. Enums aren't declared this way.
  std::string key = PT::string(spec->type());
  if (key == "enum") return;
  // Now find the scope into which to inject the declaration.
  ETScopeFinder finder;
  scope = finder.find(scope ? scope : scopes_.top());
  PT::Encoding name = PT::name(spec->name());
  ST::SymbolSet symbols = scope->find(name, ST::Scope::DEFAULT);
  for (ST::SymbolSet::iterator i = symbols.begin(); i != symbols.end(); ++i)
  {
    ST::Symbol const *type = dynamic_cast<ST::TypeName const *>(*i);
    // Skip non-types.
    if (!type) continue;

    // FIXME: what is this doing here ?
//     PT::Encoding encoding;
//     type = resolve_typedef(type, encoding);

    // If the symbol was already defined, do nothing.
    // Else if the symbol was defined as a different type, the program is ill-formed.
    // Else if the symbol corresponds to a forward-declared class, replace it.
    if (ST::ClassName const *class_ = dynamic_cast<ST::ClassName const *>(type))
    {
      if (class_->is_definition()) return;
      else if (tparameters_)
	throw ST::MultiplyDefined(name, spec, class_->ptree()); // type mismatch
      // TODO: Remove the type associated with the forward declaration, too.
      else scope->remove(*i); // Remove old forward declaration.
    }
    else if (ST::ClassTemplateName const *class_ = dynamic_cast<ST::ClassTemplateName const *>(type))
    {
      if (class_->is_definition())
      {
	if (!tparameters_)
	  throw ST::MultiplyDefined(name, spec, class_->ptree()); // type mismatch
	// TODO: check to see that the number and type of template parameters match

	// consume the template parameter list(s) but don't redeclare the template.
	tparameters_ = 0;
	return;
      }
      else if (!tparameters_) // type mismatch
	throw ST::MultiplyDefined(name, spec, class_->ptree()); // type mismatch
      // TODO: Remove the type associated with the forward declaration, too.
      else scope->remove(*i); // Remove forward declaration.
    }
    else if (ST::TypeName const *type = dynamic_cast<ST::TypeName const *>(type))
      // Symbol already defined as different type.
      throw ST::MultiplyDefined(name, spec, type->ptree());
  }
  if (!name.is_qualified())
  {
    ST::SymbolSet symbols = scope->find(name, ST::Scope::ELABORATED);
    // TODO: Check that there are no mismatching entries.
    if (tparameters_)
    {
      ST::ClassTemplateName *symbol = 
	new ST::ClassTemplateName(name, spec, false, scope);
      scope->declare(name, symbol);
    }
    else
    {
      ST::ClassName *symbol = 
	new ST::ClassName(name, spec, false, scope);
      scope->declare(name, symbol);
      TA::TypeRepository::instance()->declare(name, symbol);
    }
  }
  // This declaration consumed the template parameter list.
  tparameters_ = 0;
}

void SymbolFactory::declare(ST::Scope *scope, PT::TypeParameter const *tparam)
{
  Trace trace("SymbolFactory::declare(TypeParameter)", Trace::SYMBOLLOOKUP);

  if (!scope) scope = scopes_.top();
  PT::Node const *keyword = PT::nth<0>(tparam);
  if (dynamic_cast<PT::Kwd::Typename const *>(keyword) ||
      dynamic_cast<PT::Kwd::Class const *>(keyword))
  {
    PT::Node const *second = PT::nth<1>(tparam);
    PT::Encoding name(static_cast<PT::Atom const *>(second));
    ST::DependentName const *symbol = new ST::DependentName(tparam, scope);
    scope->declare(name, symbol);
    TA::TypeRepository::instance()->declare(name, symbol);
  }
  else if (dynamic_cast<PT::Kwd::Template const *>(keyword))
  {
    // [template < parameter-list > class id]
    PT::Encoding name;
    PT::Node const *pname = PT::nth(tparam, 5);
    if (pname) name.simple_name(static_cast<PT::Atom const *>(pname));
    ST::ClassTemplateName const *symbol = new ST::ClassTemplateName(PT::Encoding(), tparam, true, scope);
    scope->declare(name, symbol);
  }
}

void SymbolFactory::declare(ST::Scope *scope, PT::UsingDirective const *udir)
{
  Trace trace("SymbolFactory::declare(UsingDirective *)", Trace::SYMBOLLOOKUP);

  if (!scope) scope = scopes_.top();

  PT::Encoding name = PT::nth<2>(udir)->encoded_name();
  ST::SymbolSet symbols = lookup(name, scope, ST::Scope::DEFAULT);
  if (symbols.empty())
  {
    std::cout << "!2" << std::endl;
    throw ST::Undefined(name, scope, udir);
  }
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

void SymbolFactory::declare(ST::Scope *scope, PT::ParameterDeclaration const *pdecl)
{
  Trace trace("SymbolFactory::declare(ParameterDeclaration)", Trace::SYMBOLLOOKUP);
  PT::Declarator const *decl = pdecl->declarator();
  PT::Encoding const &name = decl->encoded_name();
  PT::Encoding const &type = decl->encoded_type();
  trace << type << ' ' << name;
  if (!name.empty())
  {
    if (!scope) scope = scopes_.top();
    scope->declare(name, new ST::VariableName(type, decl, true, scope));
  }
}

void SymbolFactory::declare(ST::Scope *, PT::UsingDeclaration const *udecl)
{
  Trace trace("SymbolFactory::declare(UsingDeclaration)", Trace::SYMBOLLOOKUP);

  // TODO: See Parser::using_declaration: this is very inefficient and redundant !
  PT::Encoding const &name = udecl->name()->encoded_name();
  PT::Encoding alias;
  for (PT::Encoding::name_iterator i = name.begin_name();
       i != name.end_name();
       ++i)
    alias = *i; // find last component

  ST::Scope *scope = scopes_.top();
  ST::SymbolSet symbols = lookup(name, scope, ST::Scope::DECLARATION);

  for (ST::SymbolSet::iterator i = symbols.begin(); i != symbols.end(); ++i)
  {
    // TODO: check validity according to [namespace.udecl] (7.3.3)
    scope->declare(alias, *i);
  }
}

ST::SymbolSet SymbolFactory::lookup_template_parameter(PT::Encoding const &name)
{
  Trace trace("SymbolFactory::lookup_template_parameter", Trace::SYMBOLLOOKUP);
  trace << name;
  if (!tparameters_) return ST::SymbolSet();
  else return tparameters_->find(name, ST::Scope::DEFAULT);
}

ST::Scope *SymbolFactory::lookup_scope_of_qname(PT::Encoding &name,
						PT::Node const *decl)
{
  Trace trace("SymbolFactory::lookup_scope_of_qname", Trace::SYMBOLLOOKUP);

  ST::Scope *scope = scopes_.top();
  ST::SymbolSet symbols = lookup(name, scope, ST::Scope::DECLARATION);
  if (symbols.empty())
  {
    std::cout << "!3" << std::endl;
    throw ST::Undefined(name, scope, decl);
  }
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
  {
    std::cout << "!4" << std::endl;
    throw ST::Undefined(name, scope);
  }
  ST::ClassTemplateName const * templ = dynamic_cast<ST::ClassTemplateName const *>(*symbols.begin());
  if (!templ)
    throw ST::TypeError(name, (*symbols.begin())->type());
//   templates_->declare(templ, decl);
}
