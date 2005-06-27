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
#include <Synopsis/SymbolTable.hh>
#include <Synopsis/TypeAnalysis.hh>
#include <Synopsis/TypeAnalysis/Display.hh>
#include <Synopsis/Trace.hh>
#include <fstream>

using namespace Synopsis;
namespace PT = Synopsis::PTree;
namespace ST = Synopsis::SymbolTable;
namespace TA = Synopsis::TypeAnalysis;

class TypeFinder : private ST::SymbolVisitor
{
public:
  TypeFinder() : my_repo(TA::TypeRepository::instance()), my_type(0) {}
  TA::Type const *find(ST::Symbol const *s)
  {
    s->accept(this);
    return my_type;
  }
private:
  virtual void visit(ST::TypeName const *t)
  {
    std::cout << "typename " << t << std::endl;
  };
  virtual void visit(ST::TypedefName const *t)
  {
    my_type = my_repo->lookup(t->type(), t->scope());
  }
  virtual void visit(ST::ClassName const *c)
  {
    std::cout << "class " << c << std::endl;
  }
  virtual void visit(ST::EnumName const *e)
  {
    std::cout << "enum " << e << std::endl;
  }

  TA::TypeRepository *my_repo;
  TA::Type const *    my_type;
};

void display_types(ST::Scope const *scope, std::ostream &os)
{
  for (ST::Scope::symbol_iterator i = scope->symbols_begin();
       i != scope->symbols_end();
       ++i)
  {
    TypeFinder finder;
    TA::Type const *type = finder.find(i->second);
    if (!type) continue;
    os << "Type : " << i->first.unmangled() << " = ";
    display(type, os);
    os << std::endl;
  }
  for (ST::Scope::scope_iterator i = scope->scopes_begin();
       i != scope->scopes_end();
       ++i)
    display_types(i->second, os);
}


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
    if (debug) Trace::enable(Trace::TYPEANALYSIS);

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
    display_types(symbols.current_scope(), std::cout);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
