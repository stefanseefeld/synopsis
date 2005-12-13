//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_SymbolFactory_hh_
#define Synopsis_SymbolFactory_hh_

#include <Synopsis/SymbolTable/Scope.hh>
#include <Synopsis/TypeAnalysis/TemplateRepository.hh>
#include <stack>

namespace Synopsis
{
namespace SymbolTable
{
class PrototypeScope;
class TemplateParameterScope;
}

//. SymbolFactory populates a symbol table.
class SymbolFactory : private PTree::Visitor
{
public:
  //.
  enum Language { NONE = 0x00, C99 = 0x01, CXX = 0x02};

  //. Create a symbol lookup table for the given language.
  //. Right now only CXX is supported.
  SymbolFactory(Language = CXX);

  SymbolTable::Scope *current_scope() { return my_scopes.top();}

  void enter_scope(SymbolTable::Scope *, PTree::NamespaceSpec const *);
  void enter_scope(SymbolTable::Scope *, PTree::ClassSpec const *);
  void enter_scope(SymbolTable::Scope *, PTree::Node const *);
  void enter_scope(SymbolTable::Scope *, PTree::FunctionDefinition const *);
  void enter_scope(SymbolTable::Scope *, PTree::List const *);
  void enter_scope(SymbolTable::Scope *, PTree::TemplateParameterList const *);
  void enter_scope(SymbolTable::Scope *, PTree::Block const *);
  void leave_scope();

  void declare(SymbolTable::Scope *, PTree::Declaration const *);
  //. declare the enumeration as a new TYPE as well as all the enumerators as CONST
  void declare(SymbolTable::Scope *, PTree::EnumSpec const *);
  //. declare the namespace as a new NAMESPACE
  void declare(SymbolTable::Scope *, PTree::NamespaceSpec const *);
  //. Declare a class.
  //. If this is a template specialization declare it with
  //. the template repository, else declare it here.
  void declare(SymbolTable::Scope *, PTree::ClassSpec const *);
  //. Declare a class-name from an elaborated-type-specifier.
  void declare(SymbolTable::Scope *, PTree::ElaboratedTypeSpec const *);
  void declare(SymbolTable::Scope *, PTree::TypeParameter const *);
  void declare(SymbolTable::Scope *, PTree::UsingDirective const *);
  void declare(SymbolTable::Scope *, PTree::ParameterDeclaration const *);
  void declare(SymbolTable::Scope *, PTree::UsingDeclaration const *);

  //. During the parsing of a template declaration
  //. (specifically, a partial template specialization), we need to lookup
  //. symbols in the template parameter scope, before it got integrated
  //. into a class scope. Consider:
  //. template <typename T> struct Container<T *> ...
  //. where we need to lookup 'T' while parsing the template-id 'Container<T *>'.
  //. For now let the parser explicitely look into the template parameter scope,
  //. but eventually redesign this.
  SymbolTable::SymbolSet lookup_template_parameter(PTree::Encoding const &);

private:
  typedef std::stack<SymbolTable::Scope *> Scopes;

  virtual void visit(PTree::SimpleDeclaration *);
  virtual void visit(PTree::FunctionDefinition *);

  //. Lookup the scope of a qualified name.
  //. The encoded name is modified in place to
  //. refer to the unqualified name.
  SymbolTable::Scope *lookup_scope_of_qname(PTree::Encoding &, PTree::Node const *);

  void declare_template_specialization(PTree::Encoding const &,
				       PTree::TemplateDeclaration const *,
				       SymbolTable::Scope *);

  //. The language / dialect we are working in. The behavior may depend on it.
  //. In particular, 'NONE' means do nothing.
  Language                      my_language;
  //. The current scope stack.
  Scopes                        my_scopes;
  //. When parsing a function definition the declarator is seen first,
  //. and thus a prototype is created to hold the parameters.
  //. Later, when the function definition proper is seen, the symbols
  //. are transfered and the prototype is deleted.
  SymbolTable::PrototypeScope *my_prototype;
  //. When parsing a class or function template the template-parameter-list
  //. is seen first. Since ClassSpec and Declarator don't know they are part
  //. of a template declaration, we cache it here so it gets consumed when
  //. the Class or PrototypeScope are created.
  //. FIXME: Should ClassSpec get a flag so it knows it's a template, similar
  //.        to Encodings helt in Declarators ?
  SymbolTable::TemplateParameterScope *my_template_parameters;

  TypeAnalysis::TemplateRepository    *my_template_repository;
};

}

#endif
