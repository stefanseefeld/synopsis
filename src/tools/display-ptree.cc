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
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/Trace.hh>
#include <fstream>

using namespace Synopsis;

int usage(const char *command)
{
  std::cerr << "Usage: " << command << " [-g <filename>] [-t <category>] [-r] [-e] <input>" << std::endl;
  return -1;
}

int main(int argc, char **argv)
{
  bool encoding = false;
  bool typeinfo = false;
  std::string dotfile;
  const char *input = argv[1];
  if (argc == 1) return usage(argv[0]);
  for (int i = 1; i < argc - 1; ++i)
  {
    if (argv[i] == std::string("-g"))
    {
      // make sure an argument is provided
      if (++i == argc - 1 || argv[i][0] == '-')
      {
	usage(argv[0]);
	return -1;
      }
      dotfile = argv[i];
    }
    else if (argv[i] == std::string("-e")) encoding = true;
    else if (argv[i] == std::string("-r")) typeinfo = true;
    else if (argv[i] == std::string("-t") && ++i != argc - 1)
    {
      if (argv[i] == std::string("ptree")) { Trace::enable(Trace::PTREE);}
      else if (argv[i] == std::string("symbols")) { Trace::enable(Trace::SYMBOLLOOKUP);}
      else if (argv[i] == std::string("types")) { Trace::enable(Trace::TYPEANALYSIS);}
      else if (argv[i] == std::string("parsing")) { Trace::enable(Trace::PARSING);}
      else { std::cerr << "unknown trace category '" << argv[i] << "\'\n"; return -1;}
    }
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
    PTree::Node *node = parser.parse();
    const Parser::ErrorList &errors = parser.errors();
    if (errors.size())
    {
      for (Parser::ErrorList::const_iterator i = errors.begin();
	   i != errors.end();
	   ++i)
	(*i)->write(std::cerr);
      return -1;
    }
    if (dotfile.empty())
    {
      PTree::display(node, std::cout, encoding, typeinfo);
    }
    else
    {
      std::ofstream os(dotfile.c_str());
      PTree::generate_dot_file(node, os);
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
