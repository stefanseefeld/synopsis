#include <Synopsis/Trace.hh>
#include <Buffer.hh>
#include <Lexer.hh>
#include <Parser.hh>
#include <PTree.hh>
#include <PTree/Display.hh>
#include <PTree/Writer.hh>
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace Synopsis;

class ScopeFinder : public PTree::Visitor
{
public:
  ScopeFinder(std::ostream &os)
    : my_os(os) {}
  void find(PTree::Node *node) { node->accept(this);}
private:
  void visit(PTree::List *l) 
  {
    if (l->car()) l->car()->accept(this);
    if (l->cdr()) l->cdr()->accept(this);
  }

  virtual void visit(PTree::ClassSpec *spec)
  {
    PTree::Node *name = PTree::second(spec);
    my_os << "Class Spec for '" << PTree::reify(name) << "':\n";
    PTree::Node *body = PTree::tail(spec, 3);
    body->accept(this);
  }
  virtual void visit(PTree::ClassBody *body)
  {
    PTree::Scope *scope = body->scope();
    scope->dump(my_os);
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
    ScopeFinder finder(ofs);
    PTree::Node *node;
    while (parser.parse(node)) finder.find(node);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
