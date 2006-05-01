//
// Copyright (C) 2006 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/Buffer.hh>
#include <Synopsis/PTree.hh>
#include <Synopsis/SymbolTable.hh>
#include <Synopsis/TypeAnalysis.hh>
#include <iostream>

#ifndef SXRGenerator_hh_
#define SXRGenerator_hh_

class SXRGenerator : private Synopsis::SymbolTable::Walker
{
public:
  SXRGenerator(Synopsis::Buffer &buffer, Synopsis::SymbolTable::Scope *, std::ostream &);

  void process(Synopsis::PTree::Node *node);

private:
  void link(Synopsis::PTree::Node *, Synopsis::PTree::Encoding const &);

  void visit(Synopsis::PTree::Keyword *node);
  void visit(Synopsis::PTree::Literal *node);
  void visit(Synopsis::PTree::Identifier *node);
  void visit(Synopsis::PTree::Name *node);
  void visit(Synopsis::PTree::ClassSpec *node);

  Synopsis::Buffer &buffer_;
  std::ostream     &os_;
};

#endif
