//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _SymbolLookup_Visitor_hh
#define _SymbolLookup_Visitor_hh

#include <PTree/Visitor.hh>
#include <SymbolLookup/Table.hh>

namespace SymbolLookup
{

//. This Visitor adjusts the symbol lookup table while the parse tree
//. is being traversed such that symbols in the parse tree can be
//. looked up correctly in the right context.
class Visitor : public PTree::Visitor
{
public:
  Visitor(Table &table) : my_table(table) {}
  virtual ~Visitor() {}

  using PTree::Visitor::visit;
  virtual void visit(PTree::NamespaceSpec *);
  virtual void visit(PTree::ClassSpec *);

protected:
  Table &table() { return my_table;}

private:
  //. The symbol lookup table.
  Table &my_table;
};

}

#endif
