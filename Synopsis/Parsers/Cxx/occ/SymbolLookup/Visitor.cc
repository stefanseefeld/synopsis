//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <SymbolLookup/Visitor.hh>
#include <PTree/Lists.hh>

using namespace SymbolLookup;

void Visitor::visit(PTree::List *node)
{
  for (PTree::Node *i = node; i; i = i->cdr())
    if (i->car()) i->car()->accept(this);
}

void Visitor::visit(PTree::NamespaceSpec *spec)
{
  my_table.enter_namespace(spec);
  visit(static_cast<PTree::List *>(spec));
}

void Visitor::visit(PTree::ClassSpec *spec)
{
  my_table.enter_class(spec);
  visit(static_cast<PTree::List *>(spec));
}
