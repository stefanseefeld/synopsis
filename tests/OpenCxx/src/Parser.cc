#include <Synopsis/Trace.hh>
#include <Synopsis/Buffer.hh>
#include <Synopsis/Lexer.hh>
#include <Synopsis/Parser.hh>
#include <Synopsis/PTree.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/SymbolLookup.hh>
#include <fstream>

using namespace Synopsis;

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    std::cerr << "Usage: " << argv[0] << " <output> <input>" << std::endl;
    exit(-1);
  }
  std::ofstream ofs(argv[1]);
  std::ifstream ifs(argv[2]);
  Buffer buffer(ifs.rdbuf());
  Lexer lexer(&buffer);
  SymbolLookup::Table symbols;
  Parser parser(lexer, symbols);
  PTree::Node *node = parser.parse();
  PTree::display(node, ofs, true);
}
