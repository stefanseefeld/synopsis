//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/Buffer.hh>
#include <Synopsis/Lexer.hh>
#include <Synopsis/SymbolFactory.hh>
#include <Synopsis/Parser.hh>
#include <Synopsis/PTree.hh>
#include <Synopsis/PTree/Writer.hh>
#include <Synopsis/SymbolTable.hh>
#include <Synopsis/SymbolTable/Display.hh>
#include <Synopsis/Trace.hh>
#include <fstream>

using namespace Synopsis;
namespace PT = PTree;
namespace ST = SymbolTable;

int usage(const char *command)
{
  std::cerr << "Usage: " << command << " [-t <category>] [-r] <input>" << std::endl;
  return -1;
}

int main(int argc, char **argv)
{
  bool display_references = true;
  char const *input = argv[1];
  if (argc == 1) return usage(argv[0]);
  for (int i = 1; i < argc - 1; ++i)
  {
    if (argv[i] == std::string("-t") && ++i != argc - 1)
    {
      if (argv[i] == std::string("ptree")) { Trace::enable(Trace::PTREE);}
      else if (argv[i] == std::string("symbols")) { Trace::enable(Trace::SYMBOLLOOKUP);}
      else if (argv[i] == std::string("types")) { Trace::enable(Trace::TYPEANALYSIS);}
      else if (argv[i] == std::string("parsing")) { Trace::enable(Trace::PARSING);}
      else { std::cerr << "unknown trace category '" << argv[i] << "\'\n"; return -1;}
    }
    else if (argv[i] == std::string("-r"))
      display_references = true;
    else return usage(argv[0]);
  }
  input = argv[argc - 1];
  try
  {
    std::ifstream ifs(input);
    Buffer buffer(ifs.rdbuf(), input);
    Lexer lexer(&buffer);
    SymbolFactory symbols;
    Parser parser(lexer, symbols);
    PT::Node *node = parser.parse();
    const Parser::ErrorList &errors = parser.errors();
    if (errors.size())
    {
      for (Parser::ErrorList::const_iterator i = errors.begin();
	   i != errors.end();
	   ++i)
	(*i)->write(std::cerr);
      return -1;
    }
    if (display_references)
    {
      ST::References const &references = symbols.references();
      for (ST::References::const_iterator i = references.begin(); i != references.end(); ++i)
      {
        std::string filename;
        unsigned long line;
        unsigned long column;
        buffer.origin(i->first->begin(), filename, line, column);
        std::cout << PT::reify(i->first) << " (referenced at " << filename << ':' << line << ':' << column << ") : declared as "
                  << type_name(i->second);
        buffer.origin(i->second->ptree()->begin(), filename, line, column);
        std::cout << " (at " << filename << ':' << line << ':' << column << ')' << std::endl;
      }
    }
    else
      ST::display(symbols.current_scope(), std::cout);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
