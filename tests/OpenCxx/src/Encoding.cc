#include <Synopsis/Trace.hh>
#include <Synopsis/Buffer.hh>
#include <Synopsis/Lexer.hh>
#include <Synopsis/Parser.hh>
#include <Synopsis/PTree.hh>
#include <Synopsis/PTree/Writer.hh>
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace Synopsis;

class EncodingFinder : public PTree::Visitor
{
public:
  EncodingFinder(std::ostream &os) : my_os(os) {}
  void find(PTree::Node *node) { node->accept(this);}
private:
  virtual void visit(PTree::List *node)
  {
    for (PTree::Node *i = node; i; i = i->cdr())
      if (i->car()) i->car()->accept(this);
  }

//   virtual void visit(PTree::Typedef *node)
//   {
//     my_os << "Typedef : " << std::endl;
//     my_os << "type specifier : " << PTree::reify(PTree::second(node)) << std::endl;
//     my_os << "typedef name : " << PTree::reify(PTree::third(node)) << std::endl;
//   }

  virtual void visit(PTree::Name *node)
  {
    my_os << "Name : " << std::endl;
    PTree::Encoding name = node->encoded_name();
    my_os << name << std::endl;
    PTree::Node *name_node = name.make_ptree(0);
    my_os << PTree::reify(name_node) << std::endl;
  }

  virtual void visit(PTree::ClassSpec *node)
  {
    my_os << "ClassSpec : " << std::endl;
    my_os << "name : " << std::endl;
    PTree::Encoding name = node->encoded_name();
    my_os << name << std::endl;
    PTree::Node *name_node = name.make_ptree(0);
    my_os << PTree::reify(name_node) << std::endl;
  }

  virtual void visit(PTree::EnumSpec *node)
  {
    my_os << "EnumSpec : " << std::endl;
    my_os << "name : " << std::endl;
    PTree::Encoding name = node->encoded_name();
    my_os << name << std::endl;
    PTree::Node *name_node = name.make_ptree(0);
    my_os << PTree::reify(name_node) << std::endl;
  }

  virtual void visit(PTree::Declaration *node)
  {
    my_os << "Declaration : " << std::endl;
    my_os << PTree::reify(node) << std::endl;
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
    my_os << "Declarator : " << std::endl;
    my_os << "name : " << std::endl;
    PTree::Encoding name = node->encoded_name();
    my_os << name << std::endl;
    {
      PTree::Encoding tmp = name;
      do
      {
	PTree::Encoding scope = tmp.get_scope();
	PTree::Encoding symbol = tmp.get_symbol();
	std::cout << "scope " << scope << " , symbol " << symbol << std::endl;
	if (scope.empty()) break;
      } while (true);
    }
    PTree::Node *name_node = name.make_ptree(0);
    my_os << PTree::reify(name_node) << std::endl;

    my_os << "type : " << std::endl;
    PTree::Encoding type = node->encoded_type();
    my_os << type << std::endl;
    PTree::Node *type_node = type.make_ptree(0);
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
  try
  {
    std::ofstream ofs(argv[1]);
    std::ifstream ifs(argv[2]);
    Buffer buffer(ifs.rdbuf(), argv[2]);
    Lexer lexer(&buffer);
    SymbolLookup::Table symbols;
    Parser parser(lexer, symbols);
    EncodingFinder finder(ofs);
    PTree::Node *node = parser.parse();
    finder.find(node);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
