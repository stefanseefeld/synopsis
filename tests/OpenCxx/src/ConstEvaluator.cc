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
  InitializerFinder(std::ostream &os) : my_os(os) {}
  void find(PTree::Node *node) { node->accept(this);}
private:
  virtual void visit(PTree::EnumSpec *node)
  {
    my_os << "enum : " << std::endl;
    PTree::display(node, my_os, false);
  }
  virtual void visit(PTree::Declaration *node)
  {
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
      unsigned long value;
      if (PTree::evaluate_const(initializer, value))
	my_os << "value : " << value << std::endl;
      else
	my_os << "value : none" << std::endl;
    }
  }

  std::ostream &my_os;
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
    InitializerFinder finder(ofs);
    PTree::Node *node;
    while (parser.parse(node)) finder.find(node);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
