#include <Synopsis/Trace.hh>
#include <Synopsis/Buffer.hh>
#include <Synopsis/Lexer.hh>
#include <Synopsis/Parser.hh>
#include <Synopsis/PTree.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/PTree/Writer.hh>
#include <Synopsis/SymbolLookup.hh>
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace Synopsis;

class InitializerFinder : public SymbolLookup::Visitor
{
public:
  InitializerFinder(SymbolLookup::Table &table, std::ostream &os)
    : SymbolLookup::Visitor(table), my_os(os) {}
  void find(PTree::Node *node) { node->accept(this);}
private:
  virtual void visit(PTree::List *node)
  {
    for (PTree::Node *n = node; n; n = n->cdr())
      if (n->car()) n->car()->accept(this);
  }
  virtual void visit(PTree::EnumSpec *node)
  {
    PTree::Node *body = third(node);

    long value = -1;
    for (PTree::Node *e = PTree::second(body);
	 e;
	 e = PTree::rest(rest(e)))
    {
      PTree::Node *enumerator = e->car();
      if (enumerator->is_atom()) ++value;
      else  // [identifier = initializer]
      {
	PTree::Node *initializer = PTree::third(enumerator);
	enumerator = enumerator->car();
	if (!table().evaluate_const(initializer, value))
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
  virtual void visit(PTree::Declaration *node)
  {
    PTree::Node *type = PTree::second(node);
    type->accept(this);
    PTree::Node *rest = PTree::third(node);
    if (rest->is_atom()) return; // ';'
    for (; rest; rest = rest->cdr())
    {
      PTree::Node *p = rest->car();
      p->accept(this);
    }
  }
  virtual void visit(PTree::Declarator *node) 
  {
    if (PTree::length(node) == 3)
    {
      PTree::Node *initializer = PTree::third(node);
      my_os << "initializer : " << PTree::reify(initializer) << std::endl;
      long value;
      if (table().evaluate_const(initializer, value))
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
      Trace::enable_debug();
      output = argv[2];
      input = argv[3];
    }
    else
    {
      output = argv[1];
      input = argv[2];
    }
    std::ofstream ofs(output.c_str());
    std::ifstream ifs(input.c_str());
    Buffer buffer(ifs.rdbuf(), input);
    Lexer lexer(&buffer);
    SymbolLookup::Table symbols;
    Parser parser(lexer, symbols);
    PTree::Node *node = parser.parse();
    InitializerFinder finder(symbols, ofs);
    finder.find(node);
  }
  catch (std::exception const &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
