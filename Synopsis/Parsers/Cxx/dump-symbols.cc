#include <Buffer.hh>
#include <Lexer.hh>
#include <Parser.hh>
#include <PTree.hh>
#include <PTree/Display.hh>
#include <SymbolLookup.hh>
#include <Synopsis/Trace.hh>
#include <fstream>

using Synopsis::Trace;

int usage(const char *command)
{
  std::cerr << "Usage: " << command << " <input>" << std::endl;
  return -1;
}

int main(int argc, char **argv)
{
  bool debug = false;
  const char *input = argv[1];
  if (argc == 1) return usage(argv[0]);
  for (int i = 1; i < argc - 1; ++i)
  {
    if (argv[i] == std::string("-d")) debug = true;
    else return usage(argv[0]);
  }
  input = argv[argc - 1];
  try
  {
    if (debug) Trace::enable_debug();

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

    symbols.current_scope().dump(std::cout, 0);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
