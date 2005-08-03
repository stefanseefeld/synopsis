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

  void link(Synopsis::PTree::Node *node);

private:
  void visit(Synopsis::PTree::Identifier *node);
  void visit(Synopsis::PTree::Name *node);

  Synopsis::Buffer &my_buffer;
  std::ostream     &my_os;
};

#endif
