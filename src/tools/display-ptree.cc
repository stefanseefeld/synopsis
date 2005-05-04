//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/Buffer.hh>
#include <Synopsis/Lexer.hh>
#include <Synopsis/Parser.hh>
#include <Synopsis/PTree.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/SymbolLookup.hh>
#include <Synopsis/Trace.hh>
#include <fstream>

using namespace Synopsis;

int usage(const char *command)
{
  std::cerr << "Usage: " << command << " [-g <filename>] [-d] [-r] <input>" << std::endl;
  return -1;
}

class DotFileGenerator : public PTree::Visitor
{
public:
  DotFileGenerator(std::string const &filename) : my_os(filename.c_str()) {}
  void write(PTree::Node *ptree)
  {
    my_os << "digraph PTree\n{\n"
	  << "node[fillcolor=\"#ffffcc\", pencolor=\"#424242\" style=\"filled\"];\n";
    ptree->accept(this);
    my_os << '}' << std::endl;
  }
private:
  virtual void visit(PTree::Atom *a)
  {
    my_os << (long)a 
	  << " [label=\"" << std::string(a->position(), a->length())
	  << "\" fillcolor=\"#ffcccc\"];\n";
  }
  virtual void visit(PTree::List *l)
  {
    my_os << (long)l 
	  << " [label=\"" << typeid(*l).name() << "\"];\n";
    if (l->car())
    {
      l->car()->accept(this);
      my_os << (long)l << "->" 
	    << (long)l->car() << ';' << std::endl;
    }
    if (l->cdr())
    {
      l->cdr()->accept(this);
      my_os << (long)l << "->" 
	    << (long)l->cdr() << ';' << std::endl;
    }
  }

  std::ofstream my_os;
};

int main(int argc, char **argv)
{
  bool typeinfo = false;
  bool debug = false;
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
    else if (argv[i] == std::string("-r")) typeinfo = true;
    else if (argv[i] == std::string("-d")) debug = true;
    else return usage(argv[0]);
  }
  input = argv[argc - 1];
  try
  {
    if (debug) Trace::enable(Trace::ALL);

    PTree::Encoding::do_init_static();

    std::ifstream ifs(input);
    Buffer buffer(ifs.rdbuf(), input);
    Lexer lexer(&buffer);
    SymbolLookup::Table symbols;
    Parser parser(lexer, symbols);
    PTree::Node *node = parser.parse();
    const Parser::ErrorList &errors = parser.errors();
    for (Parser::ErrorList::const_iterator i = errors.begin(); i != errors.end(); ++i)
      (*i)->write(std::cerr);

    if (!node) return -1;
    if (dotfile.empty())
    {
      PTree::display(node, std::cout, true, typeinfo);
    }
    else
    {
      DotFileGenerator dot(dotfile);
      dot.write(node);
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
