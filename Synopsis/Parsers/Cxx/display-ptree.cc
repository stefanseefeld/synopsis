#include <Buffer.hh>
#include <Lexer.hh>
#include <Parser.hh>
#include <Class.hh>
#include <MetaClass.hh>
#include <ClassWalker.hh>
#include <PTree.hh>
#include <PTree/Display.hh>
#include <SymbolLookup.hh>
#include <Synopsis/Trace.hh>
#include <fstream>

using Synopsis::Trace;

int usage(const char *command)
{
  std::cerr << "Usage: " << command << " [-d] [-t] [-r] <input>" << std::endl;
  return -1;
}

int main(int argc, char **argv)
{
  bool translate = false;
  bool typeinfo = false;
  bool debug = false;
  const char *input = argv[1];
  if (argc == 1) return usage(argv[0]);
  for (int i = 1; i < argc - 1; ++i)
  {
    if (argv[i] == std::string("-t")) translate = true;
    else if (argv[i] == std::string("-r")) typeinfo = true;
    else if (argv[i] == std::string("-d")) debug = true;
    else return usage(argv[0]);
  }
  input = argv[argc - 1];
  try
  {
    if (debug) Trace::enable_debug();

    Class::do_init_static();
    Metaclass::do_init_static();
    Environment::do_init_static();
    PTree::Encoding::do_init_static();

    std::ifstream ifs(input);
    Buffer buffer(ifs.rdbuf());
    Lexer lexer(&buffer);
    SymbolLookup::Table symbols;
    Parser parser(lexer, symbols);
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
