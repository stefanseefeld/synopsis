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
#include <Synopsis/TypeAnalysis/TypeRepository.hh>
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
//. Find the name being declared. The node wrapped inside the template-declaration
//. is one of these:
//.   elaborated-type-specifier (i.e. class template forward declaration)
//.   class-specifier (i.e. a class template definition)
//.   function-definition (i.e. a function template definition)
//.   declaration (i.e. a function template declaration)
//.   template-definition (a nested item from the list above)
class TemplateNameFinder : private PT::Visitor
{
public:
  TemplateNameFinder() : my_is_function(false) {}
  PT::Encoding find(PT::TemplateDecl const *decl, bool &is_function)
  {
    PT::nth(const_cast<PT::TemplateDecl *>(decl), 4)->accept(this);
    is_function = my_is_function;
    return my_name;
  }

private:
  void visit(PT::Identifier *id) { my_name.simple_name(id);}
  void visit(PT::List *list)
  {
    // This is assumed to be a declarator list. Since we are looking at a
    // template-declaration, we know there is only one.
    list->car()->accept(this);
  }
  void visit(PT::Declarator *decl) 
  {
    my_name = decl->encoded_name();
    my_is_function = decl->encoded_type().is_function();
  }
  void visit(PT::TemplateDecl *decl) { PT::nth(decl, 4)->accept(this);}
  void visit(PT::DeclSpec *declspec) { my_name = declspec->type();}
  void visit(PT::ClassSpec *spec) { my_name = spec->encoded_name();}
  void visit(PT::Declaration *decl)
  {
    // If there is no declarator the decl-specifier-seq declares a type,
    // and thus, a class template.
    if (!PT::nth<1>(decl)) decl->car()->accept(this);
    // Else it is either a simple-declaration or function-definition. In the first
    // case, the second element is a declarator-list (with a single declarator - this
    // is a template-declaration), in the second a single declarator.
    // In both cases the declarator's encoded name will tell us what we are looking for.
    else PT::nth<1>(decl)->accept(this);
  }

  PT::Encoding my_name;
  bool         my_is_function;
};

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
    PT::Encoding name(node);
    ST::SymbolSet symbols = my_scope->unqualified_lookup(name, ST::Scope::ELABORATED);
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
    ST::SymbolSet symbols = ::lookup(name, my_scope, ST::Scope::ELABORATED);
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
  virtual void visit(PT::ParameterDeclaration *decl)
  {
    ++my_params;
    // If we already encountered an initializer for a previous parameter,
    // this one has to have one too, though not necessarily expressed in
    // this declarator (see 8.3.6/6).
    if (my_default_args || decl->initializer()) ++my_default_args;
  }

  size_t &my_params;
  size_t &my_default_args;
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
  : my_language(l),
    my_prototype(0),
    my_template_parameters(0),
    my_template_repository(new TA::TemplateRepository())
{
  // define the global scope
  ST::Namespace *global = new ST::Namespace(0, 0);
  my_scopes.push(global);
  // FIXME: Do we need a special GCC-extension flag, too ?
  PT::Encoding name;
  name.append_with_length("__builtin_va_list");
  global->declare(name, new ST::TypeName(PT::Encoding(), 0, true, global));
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
  for (PT::List const *base_clause = spec->base_clause();
       base_clause;
       base_clause = PT::tail(base_clause, 2))
  {
    // The last node is the name, the others access specs or 'virtual'
    PT::Node const *parent = PT::last(static_cast<PT::List *>(PT::nth<1>(base_clause)))->car();
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
  PT::Encoding name = PT::nth<1>(decl)->encoded_name();
  if (name.is_qualified())
    scope = lookup_scope_of_qname(name, PT::nth<1>(decl));

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
  // If this is a typedef, we process it here:
  PT::DeclSpec *spec = static_cast<PT::DeclSpec *>(PT::nth<0>(d));
  if (spec && spec->is_typedef())
  {
    PT::List const *declarators = static_cast<PT::List *>(PT::nth<1>(d));
    while(declarators)
    {
      PT::Node const *d = declarators->car();
      if(PT::type_of(d) == Token::ntDeclarator)
      {
	PT::Encoding const &name = d->encoded_name();
	PT::Encoding const &type = d->encoded_type();
	ST::Scope *scope = my_scopes.top();
	ST::TypedefName *symbol = new ST::TypedefName(type, d, scope);
	scope->declare(name, symbol);
	// TODO: Even though a typedef doesn't introduce a new type, we may
	//       declare it as a type alias to avoid an additional lookup.
	// TA::TypeRepository::instance()->declare(symbol);
      }
      declarators = PT::tail(declarators, 2);
    }
  }
  else
  {
    // Disambiguate: either Declaration or FunctionDefinition.
    const_cast<PT::Declaration *>(d)->accept(this);
  }
}

void SymbolFactory::declare(PT::EnumSpec const *spec)
{
  Trace trace("SymbolFactory::declare(EnumSpec *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;
  ST::Scope *scope = my_scopes.top();
  if (PT::Atom const *name = spec->name())
  {
    ST::EnumName const *symbol = new ST::EnumName(spec, my_scopes.top());
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
    PT::Encoding name(static_cast<PT::Atom const *>(enumerator));
    PT::Encoding type; // FIXME: ??
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
  if (PT::nth<1>(spec)) name.simple_name(static_cast<PT::Atom const *>(PT::nth<1>(spec)));
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

//   // If the name is a template-id, we are looking at a template specialization.
//   // Declare it in the template repository instead.
//   if (name.is_template_id())
//   {
//     declare_template_specialization(name.get_template_name(), spec, scope);
//     return;
//   }
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
      // TODO: Remove the type associated with the forward declaration, too.
      else if (body) scope->remove(*i); // Remove forward declaration.
      else return;                      // Don't issue another forward declaration.
    }
    else if (ST::TypeName const *type = dynamic_cast<ST::TypeName const *>(*i))
      // Symbol already defined as different type.
      throw ST::MultiplyDefined(name, spec, type->ptree());
  }
  ST::ClassName const *symbol; 
  if (body)
    symbol = new ST::ClassName(spec->encoded_type(), spec, true, scope);
  else
    symbol = new ST::ClassName(spec->encoded_type(), spec, false, scope);
  scope->declare(name, symbol);
  TA::TypeRepository::instance()->declare(name, symbol);
}

void SymbolFactory::declare(PTree::ElaboratedTypeSpec const *spec)
{
  Trace trace("SymbolFactory::declare(ElaboratedTypeSpec *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;

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
  ST::Scope *scope = finder.find(my_scopes.top());
  PT::Encoding name = spec->name()->encoded_name();
  if (!name.is_qualified())
  {
    ST::SymbolSet symbols = scope->find(name, ST::Scope::ELABORATED);
    if (!symbols.size())
    {
      ST::ClassName *symbol = new ST::ClassName(name, spec, false, scope);
      scope->declare(name, symbol);
      TA::TypeRepository::instance()->declare(name, symbol);
    }
  }
}

void SymbolFactory::declare(PT::TemplateDecl const *tdecl)
{
  Trace trace("SymbolFactory::declare(TemplateDecl *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;
  ST::Scope *scope = my_scopes.top();
  std::cout << "template decl" << std::endl;
  PT::display(tdecl, std::cout);
  // A template declaration either declares a class template, or a function template.
  // However, as it may contain multiple nesting levels care has to be taken to
  // create the appropriate symbol in the correct scope.
  
  TemplateNameFinder finder;
  bool is_function;
  PT::Encoding name = finder.find(tdecl, is_function);
  PT::List const *declaration = static_cast<PT::List *>(PT::nth(tdecl, 4));
  if (!is_function)
  {
    if (name.is_qualified()) scope = lookup_scope_of_qname(name, tdecl);

    // If the name is a template-id, we are looking at a template specialization.
    // Declare it in the template repository instead.
    if (name.is_template_id())
      declare_template_specialization(name.get_template_name(), tdecl, scope);
    else
    {
      // Find any forward declarations for this symbol.
      ST::ClassTemplateName const *symbol = new ST::ClassTemplateName(PT::Encoding(), tdecl, true, scope);
      scope->declare(name, symbol);
    }
  }
  else
  {
    PT::Node const *decl = PT::nth<1>(declaration);
    // This is either a function (template) declaration or 
    // a function (template) definition. Thus, declaration is
    // either a PT::Declaration or a PT::FunctionDefinition.
    // Depending on which it is, second child node is either 
    // a list of declarators (must have length 1 in case of a 
    // template-declaration), or a declarator.
    if (!dynamic_cast<PT::Declarator const *>(decl))
      decl = PT::nth<0>(static_cast<PT::List const *>(decl));
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
    ST::FunctionTemplateName const *symbol = new ST::FunctionTemplateName(PT::Encoding(), decl,
									  params, default_args, scope);
    scope->declare(name, symbol);
  }
}

void SymbolFactory::declare(PT::TypeParameter const *tparam)
{
  Trace trace("SymbolFactory::declare(TypeParameter *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;
  ST::Scope *scope = my_scopes.top();
  PT::Node const *first = PT::nth<0>(tparam);
  if (dynamic_cast<PT::Kwd::Typename const *>(first) ||
      dynamic_cast<PT::Kwd::Class const *>(first))
  {
    PT::Node const *second = PT::nth<1>(tparam);
    PT::Encoding name(static_cast<PT::Atom const *>(second));
    ST::TypeName const *symbol = new ST::TypeName(PT::Encoding(), tparam, true, scope);
    scope->declare(name, symbol);
    TA::TypeRepository::instance()->declare(name, symbol);
  }
  else if (PT::TemplateDecl const *tdecl = 
	   dynamic_cast<PT::TemplateDecl const *>(first))
  {
    // tdecl has 4 or 5 members:
    // [template < parameter-list > class id]
    // [template < parameter-list > class]
    PT::Encoding name;
    PT::Node const *pname = PT::nth(tdecl, 5);
    if (pname) name.simple_name(static_cast<PT::Atom const *>(pname));
    ST::ClassTemplateName const *symbol = new ST::ClassTemplateName(PT::Encoding(), tdecl, true, scope);
    scope->declare(name, symbol);
  }
}

void SymbolFactory::declare(PT::UsingDirective const *udir)
{
  Trace trace("SymbolFactory::declare(UsingDirective *)", Trace::SYMBOLLOOKUP);
  if (my_language == NONE) return;
  ST::Scope *scope = my_scopes.top();

  PTree::Encoding name = PTree::nth<2>(udir)->encoded_name();
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
  PT::Node const *decl = PT::nth<2>(pdecl);
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
  PT::Encoding const &name = udecl->name()->encoded_name();
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

ST::SymbolSet SymbolFactory::lookup_template_parameter(PT::Encoding const &name)
{
  Trace trace("SymbolFactory::lookup_template_parameter", Trace::SYMBOLLOOKUP);
  if (!my_template_parameters) return ST::SymbolSet();
  else return my_template_parameters->find(name, ST::Scope::DEFAULT);
}

void SymbolFactory::visit(PT::Declaration *decl)
{
  Trace trace("SymbolFactory::visit(Declaration)", Trace::SYMBOLLOOKUP);
  ST::Scope *scope = my_scopes.top();

  PT::DeclSpec *spec = static_cast<PT::DeclSpec *>(PT::nth<0>(decl));
  PT::Node const *decls = PT::nth<1>(decl);
  // The implicit type declaration by means of elaborated type specs is dealt
  // elsewhere.
  if (!decls || decls->is_atom()) // it is a ';'
  {
    if (length(spec) == 1)
    {
      PT::ElaboratedTypeSpec *etspec = 
	dynamic_cast<PT::ElaboratedTypeSpec *>(PT::nth<0>(spec));
      if (etspec && PT::string(etspec->type()) != "enum")
      {
	PT::Encoding name = etspec->name()->encoded_name();
	if (!name.is_qualified())
	{
	  ST::SymbolSet symbols = scope->find(name, ST::Scope::ELABORATED);
	  if (!symbols.size())
	  {
	    ST::ClassName *symbol = new ST::ClassName(name, spec, false, scope);
	    scope->declare(name, symbol);
	    TA::TypeRepository::instance()->declare(name, symbol);
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
      if (symbols.empty()) throw ST::Undefined(name, scope, decl);
      // FIXME: We need type analysis / overload resolution
      //        here to take the right symbol.
      ST::Symbol const *forward = *symbols.begin();
      while (name.is_qualified()) name = name.get_symbol();
      scope = forward->scope();
      // TODO: check whether this is the definition of a previously
      //       declared variable, according to 3.1/2 [basic.def]
      scope->remove(forward);
    }
      
    ST::Symbol const *symbol;
    if (type.is_function()) // It's a function declaration.
    {
      size_t params = 0;
      size_t default_args = 0;
      DefaultArgumentFinder finder(params, default_args);
      finder.find(decl);
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
	if (TA::evaluate_const(current_scope(), initializer->car(), value))
	  symbol = new ST::ConstName(type, value, decl, defined, scope);
	else
	  symbol = new ST::ConstName(type, decl, defined, scope);
      }
      else
	symbol = new ST::VariableName(type, decl, defined, scope);
    }
    scope->declare(name, symbol);
  }
}

void SymbolFactory::visit(PT::FunctionDefinition *def)
{
  Trace trace("SymbolFactory::visit(FunctionDefinition)", Trace::SYMBOLLOOKUP);
  PT::Node const *declarator = PT::nth<1>(def);
  // declare it only once (but allow overloading)

  PT::Encoding name = declarator->encoded_name();
  PT::Encoding type = declarator->encoded_type();

  size_t params = 0;
  size_t default_args = 0;
  DefaultArgumentFinder finder(params, default_args);
  finder.find(static_cast<PT::Declarator const *>(declarator));

  // If the name is qualified, it has to be
  // declared already. If it hasn't, raise an error.
  ST::Scope *scope = my_scopes.top();
  if (name.is_qualified())
  {
    scope = lookup_scope_of_qname(name, declarator);
    ST::SymbolSet symbols = scope->find(name, ST::Scope::DECLARATION);
    // FIXME: We need type analysis / overload resolution
    //        here to take the right symbol.
    ST::Symbol const *forward = *symbols.begin();
    // TODO: check whether this is the definition of a previously
    //       declared function, according to 3.1/2 [basic.def]
    scope->remove(forward);
  }
  ST::Symbol const *symbol = new ST::FunctionName(type, def, params, default_args,
						  true, scope);
  scope->declare(name, symbol);
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
						    PT::TemplateDecl const *decl,
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
  my_template_repository->declare(templ, decl);
}
