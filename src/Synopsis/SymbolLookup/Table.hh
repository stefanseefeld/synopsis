//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_SymbolLookup_Table_hh_
#define Synopsis_SymbolLookup_Table_hh_

#include <Synopsis/SymbolLookup/Scope.hh>
#include <Synopsis/SymbolLookup/ConstEvaluator.hh>
#include <stack>

namespace Synopsis
{
namespace SymbolLookup
{

class PrototypeScope;

//. Table provides a facade to the SymbolLookup module...
class Table
{
public:
  //.
  enum Language { NONE = 0x00, C99 = 0x01, CXX = 0x02};

  //. A Guard provides RAII - like protection for the scope stack
  struct Guard
  {
    //. Construct a guard for the given table.
    //. If the pointer is 0 there is no cleanup to do
    Guard(Table *t) : table(t) {}
    ~Guard();
    Table *table;
  };

  //. Create a symbol lookup table for the given language.
  //. Right now only CXX is supported.
  Table(Language = CXX);

  Table &enter_namespace(PTree::NamespaceSpec const *);
  Table &enter_class(PTree::ClassSpec const *);
  Table &enter_function_declaration(PTree::Node const *);
  Table &enter_function_definition(PTree::Declaration const *);
  Table &enter_block(PTree::List const *);
  void leave_scope();

  Scope &current_scope();

  bool evaluate_const(PTree::Node const *node, long &value);

  void declare(PTree::Declaration const *);
  void declare(PTree::Typedef const *);
  //. declare the enumeration as a new TYPE as well as all the enumerators as CONST
  void declare(PTree::EnumSpec const *);
  //. declare the namespace as a new NAMESPACE
  void declare(PTree::NamespaceSpec const *);
  //. declare the class as a new TYPE
  void declare(PTree::ClassSpec const *);
  void declare(PTree::TemplateDecl const *);
  void declare(PTree::UsingDirective const *);
  void declare(PTree::ParameterDeclaration const *);
  void declare(PTree::UsingDeclaration const *);

  //. look up the encoded name and return a set of matching symbols.
  SymbolSet lookup(PTree::Encoding const &,
		   Scope::LookupContext = Scope::DEFAULT) const;

private:
  typedef std::stack<Scope *> Scopes;

  Language                      my_language;
  Scopes                        my_scopes;
  //. When parsing a function definition the declarator is seen first,
  //. and thus a prototype is created to hold the parameters.
  //. Later, when the function definition proper is seen, the symbols
  //. are transfered and the prototype is deleted.
  SymbolLookup::PrototypeScope *my_prototype;
};

inline Table::Guard::~Guard() { if (table) table->leave_scope();}

inline bool Table::evaluate_const(PTree::Node const *node, long &value)
{
  if (my_language == NONE) return false;
  if (!node) return false;
  ConstEvaluator e(*my_scopes.top());
  return e.evaluate(const_cast<PTree::Node *>(node), value);
}

}
}

#endif
