//
// Copyright (C) 1997-2000 Shigeru Chiba
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <PTree/Lists.hh>
#include <PTree/operations.hh>
#include <PTree/Encoding.hh>
#include <iostream>

using namespace PTree;

Declarator::Declarator(Node *list, Encoding &t, Encoding &n, Node *dname)
  : List(list->car(), list->cdr()),
    my_type(t),
    my_name(n),
    my_declared_name(dname),
    my_comments(0)
{
}

Declarator::Declarator(Encoding &t, Encoding &n, Node *dname)
  : List(0, 0),
    my_type(t),
    my_name(n),
    my_declared_name(dname),
    my_comments(0)
{
}

Declarator::Declarator(Node *p, Node *q, Encoding &t, Encoding &n, Node *dname)
  : List(p, q),
    my_type(t),
    my_name(n),
    my_declared_name(dname),
    my_comments(0)
{
}

Declarator::Declarator(Node *list, Encoding &t)
  : List(list->car(), list->cdr()),
    my_type(t),
    my_declared_name(0),
    my_comments(0)
{
}

Declarator::Declarator(Encoding &t)
  : List(0, 0),
    my_type(t),
    my_declared_name(0),
    my_comments(0)
{
}

Declarator::Declarator(Declarator *decl, Node *p, Node *q)
  : List(p, q),
    my_type(decl->my_type),
    my_name(decl->my_name),
    my_declared_name(decl->my_declared_name),
    my_comments(0)
{
}

Name::Name(Node *p, const Encoding &name)
  : List(p->car(), p->cdr()),
    my_name(name)
{
}

FstyleCastExpr::FstyleCastExpr(const Encoding &type, Node *p, Node *q)
  : List(p, q),
    my_type(type)
{
}

ClassSpec::ClassSpec(Node *p, Node *q, Node *c)
  : List(p, q),
    my_comments(c)
{
}

ClassSpec::ClassSpec(const Encoding &name, Node *car, Node *cdr, Node *c)
  : List(car, cdr),
    my_name(name),
    my_comments(c)
{
}
