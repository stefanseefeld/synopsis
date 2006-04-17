#include <Synopsis/Trace.hh>
#include <Synopsis/Buffer.hh>
#include <Synopsis/Lexer.hh>
#include <Synopsis/SymbolFactory.hh>
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
  }

  virtual void visit(PTree::ClassSpec *node)
  {
    PTree::Encoding name = node->encoded_name();
    my_os << "ClassSpec : " << name << ' ' << name.unmangled() << std::endl;
    // Visit the body, if there is one.
    if(PTree::length(node) == 4)
      PTree::nth(node, 3)->accept(this);
  }

  virtual void visit(PTree::EnumSpec *node)
  {
    PTree::Encoding name = node->encoded_name();
    my_os << "EnumSpec : " << name << ' ' << name.unmangled() << std::endl;
  }

  virtual void visit(PTree::FunctionDefinition *node)
  {
    my_os << "FunctionDefinition : " << std::endl;
    my_os << PTree::reify(node) << std::endl;
//     visit(node->decl_specifier_seq());
//     visit(node->declarators());
  }

  virtual void visit(PTree::SimpleDeclaration *node)
  {
    my_os << "Declaration : " << std::endl;
    my_os << PTree::reify(node) << std::endl;
    PT::DeclSpec *spec = node->decl_specifier_seq();
    if (spec) visit(spec);
    PT::List *declarators = node->declarators();
    if (declarators) visit(declarators);
  }

  virtual void visit(PTree::TemplateDeclaration *node)
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

    PTree::Encoding type = node->encoded_type();
    my_os << "type : " << type << ' ' << type.unmangled() << std::endl;
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
    EncodingFinder finder(ofs);
    PTree::Node *node = parser.parse();
    finder.find(node);
  }
  catch (std::exception const &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
