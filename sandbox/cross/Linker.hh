//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/Trace.hh>
#include <Synopsis/Buffer.hh>
#include <Synopsis/Lexer.hh>
#include <Synopsis/Parser.hh>
#include <Synopsis/PTree.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/PTree/Writer.hh>
#include <Synopsis/SymbolLookup.hh>
#include <Synopsis/TypeAnalysis.hh>
#include <iostream>
#include <iomanip>
#include <fstream>

#ifndef Linker_hh_
#define Linker_hh_

class Linker : private Synopsis::SymbolTable::Walker
{
public:
  Linker(Synopsis::Buffer &buffer, Synopsis::SymbolTable::Scope *, std::ostream &);

  void process(Synopsis::PTree::Node *node);

private:
  void link(Synopsis::PTree::Node *, Synopsis::PTree::Encoding const &);

  void visit(Synopsis::PTree::Keyword *node);
  void visit(Synopsis::PTree::Literal *node);
  void visit(Synopsis::PTree::Identifier *node);
  void visit(Synopsis::PTree::Name *node);
  void visit(Synopsis::PTree::ClassSpec *node);

  Synopsis::Buffer &my_buffer;
  std::ostream     &my_os;
};

#endif
