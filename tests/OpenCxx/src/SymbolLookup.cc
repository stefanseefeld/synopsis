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
  SymbolFinder(Buffer const &buffer, SymbolLookup::Table &table, std::ostream &os)
    : SymbolLookup::Visitor(table), my_buffer(buffer), my_os(os) {}
  void find(PTree::Node *node) { node->accept(this);}
private:
  virtual void visit(PTree::Declarator *decl)
  {
    PTree::Encoding name = decl->encoded_name();
    std::cout << "Declarator : " << name << std::endl;
    // now find it.
    SymbolLookup::SymbolSet symbols = table().lookup(name);
    if (!symbols.empty())
    {
      std::string filename;
      SymbolLookup::Symbol const *first = *symbols.begin();
      unsigned long line_number = my_buffer.origin(first->ptree()->begin(), filename);
      std::cout << "declared at line " << line_number << " in " << filename << std::endl;
    }
    else
      std::cout << "undeclared ! " << std::endl;
  }

  virtual void visit(PTree::Name *n)
  {
    PTree::Encoding name = n->encoded_name();
    std::cout << "Name : " << name << std::endl;
    // now find it.
    SymbolLookup::SymbolSet symbols = table().lookup(name);
    if (!symbols.empty())
    {
      std::string filename;
      SymbolLookup::Symbol const *first = *symbols.begin();
      unsigned long line_number = my_buffer.origin(first->ptree()->begin(), filename);
      std::cout << "declared at line " << line_number << " in " << filename << std::endl;
    }
    else
      std::cout << "undeclared ! " << std::endl;
  }
  
  Buffer const &my_buffer;
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
    Buffer buffer(ifs.rdbuf(), argv[2]);
    Lexer lexer(&buffer);
    SymbolLookup::Table symbols;
    Parser parser(lexer, symbols);
    PTree::Node *node = parser.parse();
    SymbolFinder finder(buffer, symbols, ofs);
    finder.find(node);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
