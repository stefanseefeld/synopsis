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
    PTree::Node *node = parser.parse();
    const Parser::ErrorList &errors = parser.errors();
    for (Parser::ErrorList::const_iterator i = errors.begin(); i != errors.end(); ++i)
    {
      std::cerr << i->filename << ':' << i->line << ": Error before '" 
		<< i->context << '\'' << std::endl;
    }

    if (!node) return -1;
    if (translate)
    {
      ClassWalker translator(&buffer);
      PTree::Node *result = translator.translate(node);
      PTree::display(result, std::cout, true, typeinfo);
    }
    else
    {
      PTree::display(node, std::cout, true, typeinfo);
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
