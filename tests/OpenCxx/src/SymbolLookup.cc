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
using namespace SymbolLookup;

class SymbolFinder : public Visitor
{
public:
  SymbolFinder(Buffer const &buffer, Table &table, std::ostream &os)
    : Visitor(table), my_buffer(buffer), my_os(os) {}
  void find(PTree::Node *node) { node->accept(this);}
private:
  virtual void visit(PTree::Identifier *iden)
  {
    PTree::Encoding name = PTree::Encoding::simple_name(iden);
    std::cout << "Identifier : " << name << std::endl;
    lookup(name);
  }

  virtual void visit(PTree::Typedef *typed)
  {
    // We need to figure out how to reproduce the (encoded) name of
    // the type being aliased.
    std::cout << "Type : " << "<not implemented yet>" << std::endl;
//     lookup(name);
    PTree::third(typed)->accept(this);
  }

  virtual void visit(PTree::Declarator *decl)
  {
    PTree::Encoding name = decl->encoded_name();
    PTree::Encoding type = decl->encoded_type();
    std::cout << "Declarator : " << name << std::endl;
    lookup(name);
    // Visit the initializer, if there is any.
    if (type.is_function()) return;
    size_t length = PTree::length(decl);
    if (length >= 2 && *PTree::nth(decl, length - 2) == '=')
    {
      PTree::Node *init = PTree::tail(decl, length - 1);
      init->accept(this); // assign initializer
    }
    else
    {
      PTree::Node *last = PTree::last(decl)->car();
      if(last && !last->is_atom() && *last->car() == '(')
	PTree::second(last)->accept(this);
    }
  }

  virtual void visit(PTree::Name *n)
  {
    PTree::Encoding name = n->encoded_name();
    std::cout << "Name : " << name << std::endl;
    lookup(name);
  }
  
  virtual void visit(PTree::FuncallExpr *node)
  {
    PTree::Node *function = node->car();
    PTree::Encoding name;
    if (function->is_atom()) name.simple_name(function);
    else name = function->encoded_name(); // function is a 'PTree::Name'
    std::cout << "Function : " << name << ' ' << std::endl;
    Symbol const *symbol = resolve_funcall(node, current_scope());
    if (symbol)
    {
      std::string filename;
      unsigned long line_number = my_buffer.origin(symbol->ptree()->begin(), filename);
      std::cout << "declared at line " << line_number << " in " << filename << std::endl;
    }
    else
      std::cout << "undeclared ! " << std::endl;
  }

  void lookup(PTree::Encoding const &name)
  {
    SymbolSet symbols = current_scope()->lookup(name);
    if (!symbols.empty())
    {
      Symbol const *first = *symbols.begin();
      std::string filename;
      unsigned long line_number = my_buffer.origin(first->ptree()->begin(), filename);
      std::cout << "declared at line " << line_number << " in " << filename << std::endl;
    }
    else
      std::cout << "undeclared ! " << std::endl;
  }

  Buffer const &my_buffer;
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
    Table symbols;
    Parser parser(lexer, symbols);
    PTree::Node *node = parser.parse();
    SymbolFinder finder(buffer, symbols, ofs);
    finder.find(node);
  }
  catch (std::exception const &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
