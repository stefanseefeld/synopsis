#include <Synopsis/Trace.hh>
#include <Buffer.hh>
#include <Lexer.hh>
#include <Parser.hh>
#include <PTree.hh>
#include <PTree/Display.hh>
#include <fstream>

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
  Parser parser(&lexer);
  PTree::Node *node = parser.parse();
  PTree::display(node, ofs, true);
}
