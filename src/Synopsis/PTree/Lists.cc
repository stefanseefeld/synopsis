//
// Copyright (C) 1997-2000 Shigeru Chiba
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/PTree/Lists.hh>
#include <Synopsis/PTree/operations.hh>
#include <Synopsis/PTree/Encoding.hh>
#include <iostream>

namespace Synopsis
{
namespace PTree
{

DeclSpec::DeclSpec(List *l, Encoding const &type,
		   StorageClass storage, unsigned int flags, bool decl, bool def)
  : List(l->car(), l->cdr()),
    my_type(type),
    my_storage_class(storage),
    my_is_friend(flags & FRIEND),
    my_is_typedef(flags & TYPEDEF),
    my_declares_class_or_enum(decl),
    my_defines_class_or_enum(def)
{
}

Declarator::Declarator(List *list, Encoding const &t, Encoding const &n, List *comment)
  : List(list->car(), list->cdr()),
    my_type(t),
    my_name(n),
    my_comments(comment)
{
}

Node *Declarator::initializer()
{
  size_t l = PTree::length(this);
  if (l < 2) return 0;
  if (Node *assign = nth(this, l - 2))
    if (*assign == '=')
      return tail(this, l - 1); // initializer-clause
  if (Node *expr = nth(this, l - 1))
    if (!expr->is_atom() && expr->car() && *expr->car() == '(')
      return nth<1>(static_cast<List *>(expr)); // expression-list
  return 0;
}

Name::Name(Node *p, const Encoding &name)
  : List(p->car(), p->cdr()),
    my_name(name)
{
}

FstyleCastExpr::FstyleCastExpr(const Encoding &type, Node *p, List *q)
  : List(p, q),
    my_type(type)
{
}

ClassSpec::ClassSpec(const Encoding &name, Node *car, List *cdr, List *c)
  : List(car, cdr),
    my_name(name),
    my_comments(c)
{
}

namespace
{
class NameFinder : private Visitor
{
public:
  Encoding name(Node const *name)
  {
    const_cast<Node *>(name)->accept(this);
    return my_encoding;
  }
private:
  virtual void visit(Identifier *id) { my_encoding = Encoding(id);}
  virtual void visit(Name *name) { my_encoding = name->encoded_name();}
  Encoding my_encoding;
};
}

Encoding name(Node const *node)
{
  NameFinder finder;
  return finder.name(node);
}

}
}
