#include <Buffer.hh>
#include <Lexer.hh>
#include <Parser.hh>
#include <PTree.hh>
#include <PTree/Display.hh>
#include <fstream>

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " <input>" << std::endl;
    exit(-1);
  }
  try
  {
    std::ifstream ifs(argv[1]);
    Buffer buffer(ifs.rdbuf());
    Lexer lexer(&buffer);
    Parser parser(&lexer);
    PTree::Display display(std::cout, true);
    PTree::Node *node;
    while (parser.rProgram(node)) display.display(node);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
