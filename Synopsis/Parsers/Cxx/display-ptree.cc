#include <Buffer.hh>
#include <Lexer.hh>
#include <Parser.hh>
#include <Class.hh>
#include <MetaClass.hh>
#include <ClassWalker.hh>
#include <PTree.hh>
#include <PTree/Display.hh>
#include <fstream>

int usage(const char *command)
{
  std::cerr << "Usage: " << command << " [-t] [-r] <input>" << std::endl;
  return -1;
}

int main(int argc, char **argv)
{
  bool translate = false;
  bool typeinfo = false;
  const char *input = argv[1];
  switch (argc)
  {
    case 2: break;
    case 3:
      if (argv[1] == std::string("-t")) translate = true;
      else if (argv[1] == std::string("-r")) typeinfo = true;
      else return usage(argv[0]);
      input = argv[2];
      break;
    case 4:
      if (!(argv[1] == std::string("-t") && argv[2] == std::string("-r")) &&
	  !(argv[1] == std::string("-r") && argv[2] == std::string("-t")))
	return usage(argv[0]);
      translate = true;
      typeinfo = true;
      input = argv[3];
      break;
    default: return usage(argv[0]);
  }
  try
  {
    Class::do_init_static();
    Metaclass::do_init_static();
    Environment::do_init_static();
    PTree::Encoding::do_init_static();

    std::ifstream ifs(input);
    Buffer buffer(ifs.rdbuf());
    Lexer lexer(&buffer);
    Parser parser(&lexer);

    if (translate)
    {
      ClassWalker translator(&parser);
      PTree::Display display(std::cout, true, typeinfo);
      PTree::Node *node;
      while (parser.parse(node))
      {
	PTree::Node *result = translator.translate(node);
	display.display(result);
      }
    }
    else
    {
      PTree::Display display(std::cout, true, typeinfo);
      PTree::Node *node;
      while (parser.parse(node)) display.display(node);
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
