#include <Synopsis/Trace.hh>
#include <Buffer.hh>
#include <Lexer.hh>
#include <Parser.hh>
#include <PTree.hh>
#include <PTree/Display.hh>
#include <PTree/Writer.hh>
#include <SymbolLookup.hh>
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace Synopsis;

class SymbolFinder : public SymbolLookup::Visitor
{
public:
  SymbolFinder(SymbolLookup::Table &table, std::ostream &os)
    : SymbolLookup::Visitor(table), my_os(os) {}
  void find(PTree::Node *node) { node->accept(this);}
private:
  virtual void visit(PTree::Declarator *decl)
  {
  }

  std::ostream &my_os;
};

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    std::cerr << "Usage: " << argv[0] << " <output> <input>" << std::endl;
    exit(-1);
  }
  try
  {
    std::ofstream ofs(argv[1]);
    std::ifstream ifs(argv[2]);
    Buffer buffer(ifs.rdbuf());
    Lexer lexer(&buffer);
    SymbolLookup::Table symbols;
    Parser parser(lexer, symbols);
    PTree::Node *node = parser.parse();
    SymbolFinder finder(symbols, ofs);
    finder.find(node);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
