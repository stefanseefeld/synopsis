#include <Buffer.hh>
#include <Lexer.hh>
#include <Parser.hh>
#include <Class.hh>
#include <MetaClass.hh>
#include <ClassWalker.hh>
#include <PTree.hh>
#include <PTree/Display.hh>
#include <fstream>

int main(int argc, char **argv)
{
  if (argc != 2 && (argc != 3 || argv[1] != std::string("-t")))
  {
    std::cerr << "Usage: " << argv[0] << " [-t] <input>" << std::endl;
    exit(-1);
  }
  try
  {
    if (argc == 3) // with translation
    {
      Class::do_init_static();
      Metaclass::do_init_static();
      Environment::do_init_static();
      PTree::Encoding::do_init_static();

      std::ifstream ifs(argv[2]);
      Buffer buffer(ifs.rdbuf());
      Lexer lexer(&buffer);
      Parser parser(&lexer);
      ClassWalker translator(&parser);
      PTree::Display display(std::cout, true);
      PTree::Node *node;
      while (parser.parse(node))
      {
	PTree::Node *result = translator.translate(node);
	display.display(result);
      }
    }
    else // no translation
    {
      std::ifstream ifs(argv[1]);
      Buffer buffer(ifs.rdbuf());
      Lexer lexer(&buffer);
      Parser parser(&lexer);
      PTree::Display display(std::cout, true);
      PTree::Node *node;
      while (parser.parse(node)) display.display(node);
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
