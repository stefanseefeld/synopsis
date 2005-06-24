#include <Synopsis/Trace.hh>
#include <Synopsis/Buffer.hh>
#include <Synopsis/Lexer.hh>
#include <Synopsis/SymbolFactory.hh>
#include <Synopsis/Parser.hh>
#include <Synopsis/PTree.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/PTree/Writer.hh>
#include <Synopsis/SymbolTable.hh>
#include <Synopsis/TypeAnalysis.hh>
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace Synopsis;
namespace PT = Synopsis::PTree;
namespace ST = Synopsis::SymbolTable;

class InitializerFinder : private ST::Walker
{
public:
  InitializerFinder(ST::Scope *global, std::ostream &os)
    : ST::Walker(global), my_os(os) {}
  void find(PT::Node *node) { node->accept(this);}
private:
  virtual void visit(PT::List *node)
  {
    for (PT::Node *n = node; n; n = n->cdr())
      if (n->car()) n->car()->accept(this);
  }
  virtual void visit(PT::EnumSpec *node)
  {
    PT::Node *body = third(node);

    long value = -1;
    for (PT::Node *e = PT::second(body); e; e = PT::rest(rest(e)))
    {
      PT::Node *enumerator = e->car();
      if (enumerator->is_atom()) ++value;
      else  // [identifier = initializer]
      {
	PT::Node *initializer = PT::third(enumerator);
	enumerator = enumerator->car();
	if (!TypeAnalysis::evaluate_const(current_scope(), initializer, value))
	{
	  std::cerr << "Error in evaluating enum initializer :\n"
		    << "Expression doesn't evaluate to a constant integral value" << std::endl;
	}
      }
      assert(enumerator->is_atom());
      std::string name(enumerator->position(), enumerator->length());
      my_os << "enumerator : " << name << ", "
	    << "value : " << value << std::endl;
    }
  }
  virtual void visit(PT::Declaration *node)
  {
    PT::Node *type = PT::second(node);
    type->accept(this);
    PT::Node *rest = PT::third(node);
    if (rest->is_atom()) return; // ';'
    for (; rest; rest = rest->cdr())
    {
      PT::Node *p = rest->car();
      p->accept(this);
    }
  }
  virtual void visit(PT::Declarator *node) 
  {
    if (PT::length(node) == 3)
    {
      PT::Node *initializer = PT::third(node);
      my_os << "initializer : " << PT::reify(initializer) << std::endl;
      long value;
      if (TypeAnalysis::evaluate_const(current_scope(), initializer, value))
	my_os << "value : " << value << std::endl;
      else
	my_os << "value : none" << std::endl;
    }
  }

  std::ostream &my_os;
};

int main(int argc, char **argv)
{
  if (argc < 3)
  {
    std::cerr << "Usage: " << argv[0] << " [-d] <output> <input>" << std::endl;
    exit(-1);
  }
  try
  {
    std::string output;
    std::string input;
    if (argv[1] == std::string("-d"))
    {
      Trace::enable(Trace::ALL);
      output = argv[2];
      input = argv[3];
    }
    else
    {
      output = argv[1];
      input = argv[2];
    }
    std::ofstream ofs;
    {
      if (output != "-")
	ofs.open(output.c_str());
      else
      {
	ofs.copyfmt(std::cout);
	static_cast<std::basic_ios<char> &>(ofs).rdbuf(std::cout.rdbuf());
      }
    }
    std::ifstream ifs(input.c_str());
    Buffer buffer(ifs.rdbuf(), input);
    Lexer lexer(&buffer);
    SymbolFactory symbols;
    Parser parser(lexer, symbols);
    PT::Node *node = parser.parse();
    InitializerFinder finder(symbols.current_scope(), ofs);
    finder.find(node);
  }
  catch (std::exception const &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
