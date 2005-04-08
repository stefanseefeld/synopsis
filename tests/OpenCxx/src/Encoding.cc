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
    if (node->car()) node->car()->accept(this);
    if (node->cdr()) node->cdr()->accept(this);
  }

//   virtual void visit(PTree::Typedef *node)
//   {
//     my_os << "Typedef : " << std::endl;
//     my_os << "type specifier : " << PTree::reify(PTree::second(node)) << std::endl;
//     my_os << "typedef name : " << PTree::reify(PTree::third(node)) << std::endl;
//   }

  virtual void visit(PTree::Name *node)
  {
    PTree::Encoding name = node->encoded_name();
    my_os << "Name : " << name << ' ' << name.unmangled() << std::endl;
    PTree::Node *name_node = name.make_ptree(0);
    my_os << PTree::reify(name_node) << std::endl;
  }

  virtual void visit(PTree::ClassSpec *node)
  {
    PTree::Encoding name = node->encoded_name();
    my_os << "ClassSpec : " << name << ' ' << name.unmangled() << std::endl;
    PTree::Node *name_node = name.make_ptree(0);
    my_os << PTree::reify(name_node) << std::endl;
    // Visit the body, if there is one.
    if(PTree::length(node) == 4)
      PTree::nth(node, 3)->accept(this);
  }

  virtual void visit(PTree::EnumSpec *node)
  {
    PTree::Encoding name = node->encoded_name();
    my_os << "EnumSpec : " << name << ' ' << name.unmangled() << std::endl;
    PTree::Node *name_node = name.make_ptree(0);
    my_os << PTree::reify(name_node) << std::endl;
  }

  virtual void visit(PTree::Declaration *node)
  {
    my_os << "Declaration : " << std::endl;
    my_os << PTree::reify(node) << std::endl;
    PTree::second(node)->accept(this);
    PTree::Node *rest = PTree::third(node);
    if (rest->is_atom()) return; // ';'
    else if(PTree::is_a(rest, Token::ntDeclarator))
      rest->accept(this);
    else
      for (; rest; rest = rest->cdr())
      {
	PTree::Node *p = rest->car();
	p->accept(this);
      }
  }

  virtual void visit(PTree::TemplateDecl *node)
  {
    my_os << "TemplateDecl : " << std::endl;
    my_os << PTree::reify(node) << std::endl;
    PTree::Node *decl = PTree::nth(node, 4);
    decl->accept(this);
  }

  virtual void visit(PTree::Declarator *node) 
  {
    PTree::Encoding name = node->encoded_name();
    my_os << "Declarator : " << name << ' ' << name.unmangled() << std::endl;
    PTree::Node *name_node = name.make_ptree(0);
    my_os << PTree::reify(name_node) << std::endl;

    PTree::Encoding type = node->encoded_type();
    my_os << "type : " << type << ' ' << type.unmangled() << std::endl;
    PTree::Node *type_node = type.make_ptree(0);
    my_os << PTree::reify(type_node) << std::endl;
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
    EncodingFinder finder(ofs);
    PTree::Node *node = parser.parse();
    finder.find(node);
  }
  catch (std::exception const &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
