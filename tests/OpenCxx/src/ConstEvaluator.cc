#include <Synopsis/Trace.hh>
#include <Buffer.hh>
#include <Lexer.hh>
#include <Parser.hh>
#include <PTree.hh>
#include <PTree/ConstEvaluator.hh>
#include <PTree/Display.hh>
#include <PTree/Writer.hh>
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace Synopsis;

class InitializerFinder : public PTree::Visitor
{
public:
  InitializerFinder(std::ostream &os, const PTree::Scope &s)
    : my_os(os), my_symbols(s) {}
  void find(PTree::Node *node) { node->accept(this);}
private:
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
	if (!PTree::evaluate_const(initializer, my_symbols, value))
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
      if (PTree::evaluate_const(initializer, my_symbols, value))
	my_os << "value : " << value << std::endl;
      else
	my_os << "value : none" << std::endl;
    }
  }

  std::ostream       &my_os;
  const PTree::Scope &my_symbols;
};

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    std::cerr << "Usage: " << argv[0] << " <output> <input>" << std::endl;
    exit(-1);
  }
  try
  {
    std::ofstream ofs(argv[1]);
    std::ifstream ifs(argv[2]);
    Buffer buffer(ifs.rdbuf());
    Lexer lexer(&buffer);
    Parser parser(&lexer);
    InitializerFinder finder(ofs, *parser.scope());
    PTree::Node *node;
    while (parser.parse(node)) finder.find(node);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
