#include <Synopsis/Trace.hh>
#include <Buffer.hh>
#include <Lexer.hh>
#include <Parser.hh>
#include <Encoding.hh>
#include <PTree.hh>
#include <PTree/Writer.hh>
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace Synopsis;

class DeclaratorFinder : public PTree::Visitor
{
public:
  DeclaratorFinder(std::ostream &os) : my_os(os) {}
  void find(PTree::Node *node) { node->accept(this);}
private:
  virtual void visit(PTree::Declaration *node)
  {
    my_os << "Declaration : " << std::endl;
    my_os << PTree::reify(node) << std::endl;
    for (PTree::Node *rest = PTree::third(node); rest; rest = rest->cdr())
    {
      PTree::Node *p = rest->car();
      p->accept(this);
    }
  }

  virtual void visit(PTree::Declarator *decl) 
  {
    my_os << "Declarator : " << std::endl;
    my_os << "name : " << std::endl;
    const unsigned char *name = (const unsigned char *)decl->encoded_name();
    Encoding::print(my_os, (const char *)name);
    my_os << std::endl;
    PTree::Node *name_node = Encoding::MakePtree(name, 0);
    my_os << PTree::reify(name_node) << std::endl;

    my_os << "type : " << std::endl;
    const unsigned char *type = (const unsigned char *)decl->encoded_type();
    Encoding::print(my_os, (const char *)type);
    my_os << std::endl;
    PTree::Node *type_node = Encoding::MakePtree(type, 0);
    my_os << PTree::reify(type_node) << std::endl;
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
  std::ofstream ofs(argv[1]);
  std::ifstream ifs(argv[2]);
  Buffer buffer(ifs.rdbuf());
  Lexer lexer(&buffer);
  Parser parser(&lexer);
  DeclaratorFinder finder(ofs);
  PTree::Node *node;
  while (parser.rProgram(node)) finder.find(node);
}
