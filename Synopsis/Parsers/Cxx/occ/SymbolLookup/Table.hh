//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _SymbolLookup_Table_hh
#define _SymbolLookup_Table_hh

#include <SymbolLookup/Scope.hh>
#include <SymbolLookup/ConstEvaluator.hh>
#include <stack>

namespace SymbolLookup
{

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
  Table &enter_function(PTree::Declaration const *);
  Table &enter_block(PTree::List const *);
  void leave_scope();

  Scope &current_scope();

  bool evaluate_const(PTree::Node const *node, long &value);

  void declare(PTree::Declaration *);
  void declare(PTree::Typedef *);
  //. declare the enumeration as a new TYPE as well as all the enumerators as CONST
  void declare(PTree::EnumSpec *);
  //. declare the namespace as a new NAMESPACE
  void declare(PTree::NamespaceSpec *);
  //. declare the class as a new TYPE
  void declare(PTree::ClassSpec *);
  void declare(PTree::TemplateDecl *);

  //. look up the encoded name and return a set of matching symbols.
  virtual std::set<Symbol const *> lookup(PTree::Encoding const &) const;

private:
  typedef std::stack<Scope *> Scopes;

  static PTree::Encoding get_base_name(PTree::Encoding const &enc,
				       Scope const *&scope);
  static int get_base_name_if_template(PTree::Encoding::iterator i,
				       Scope const *&);
  static Scope const *lookup_typedef_name(PTree::Encoding::iterator,
					  size_t, Scope const *);
  static PTree::ClassSpec *get_class_template_spec(PTree::Node *);
  static PTree::Node *strip_cv_from_integral_type(PTree::Node *);

  Language my_language;
  Scopes   my_scopes;
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

#endif
