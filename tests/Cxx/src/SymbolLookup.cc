#include <Synopsis/Trace.hh>
#include <Synopsis/Buffer.hh>
#include <Synopsis/Lexer.hh>
#include <Synopsis/Parser.hh>
#include <Synopsis/PTree.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/PTree/Writer.hh>
#include <Synopsis/SymbolLookup.hh>
#include <Synopsis/TypeAnalysis.hh>
#include <iostream>
#include <iomanip>
#include <fstream>

using Synopsis::Trace;
using Synopsis::Buffer;
using Synopsis::Lexer;
using Synopsis::SymbolFactory;
using Synopsis::Parser;
namespace PT = Synopsis::PTree;
namespace ST = Synopsis::SymbolTable;
namespace TA = Synopsis::TypeAnalysis;

class SymbolFinder : private ST::Walker
{
public:
  SymbolFinder(Buffer const &buffer, ST::Scope *scope, std::ostream &os)
    : ST::Walker(scope), my_buffer(buffer), my_os(os), my_in_template_decl(false) {}
  void find(PT::Node *node) { node->accept(this);}
private:
  using ST::Walker::visit;
  virtual void visit(PT::Identifier *id)
  {
    Trace trace("SymbolFinder::visit(Identifier)", Trace::SYMBOLLOOKUP);
    PT::Encoding name(id);
    my_os << "Identifier : " << name.unmangled() << std::endl;
    lookup(name);
  }

  virtual void visit(PT::Block *node)
  {
    Trace trace("SymbolFinder::visit(Block)", Trace::SYMBOLLOOKUP);
    Walker::visit(node);
  }

  virtual void visit(PT::TemplateDeclaration *tdecl)
  {
    Trace trace("SymbolFinder::visit(TemplateDeclaration)", Trace::SYMBOLLOOKUP);
    traverse_parameters(tdecl);
    bool saved_in_template_decl = my_in_template_decl;
    my_in_template_decl = true;
    PT::Node *fourth = PT::nth(tdecl, 4);
    if (!fourth->is_atom())
      fourth->accept(this); // This is a declaration.
    else
    {
      // We are in a template template parameter.
      // Visit the identifier and the default type, if present.
      PT::Node *fifth = PT::nth(tdecl, 5);
      if (fifth) fifth->accept(this); // This is an identifier
    }
    my_in_template_decl = saved_in_template_decl;
  }

  virtual void visit(PT::NamespaceSpec *node)
  {
    Trace trace("SymbolFinder::visit(NamespaceSpec)", Trace::SYMBOLLOOKUP);
    PT::Node const *name = PT::nth<1>(node);
    PT::Encoding ename;
    if (name) ename.simple_name(static_cast<PT::Atom const *>(name));
    else ename.append_with_length("<anonymous>");
    my_os << "Namespace : " << ename.unmangled() << std::endl;
    lookup(ename);
    traverse_body(node);
  }

  virtual void visit(PT::SimpleDeclaration *node)
  {
    Trace trace("SymbolFinder::visit(SimpleDeclaration)", Trace::SYMBOLLOOKUP);

    PT::DeclSpec *spec = node->decl_specifier_seq();
    if (spec) visit(spec);
    PT::List *declarators = node->declarators();
    if (declarators) visit(declarators);
  }

  virtual void visit(PT::UsingDirective *udir)
  {
    PT::Node *node = PT::nth<2>(udir);
    PT::Encoding name = node->encoded_name();
    my_os << "Namespace : " << name.unmangled() << std::endl;
    lookup(name);
  }
  virtual void visit(PT::UsingDeclaration *udecl)
  { visit(static_cast<PT::List *>(udecl));}
  virtual void visit(PT::NamespaceAlias *alias)
  {
    {
      PT::Node *node = PT::nth<1>(alias);
      PT::Encoding name(static_cast<PT::Atom *>(node));
      my_os << "Namespace : " << name.unmangled() << std::endl;
      lookup(name);
    }
    {
      PT::Node *node = PT::nth(alias, 3);
      PT::Encoding name = node->encoded_name();
      my_os << "Namespace : " << name.unmangled() << std::endl;
      lookup(name);
    }
  }

  virtual void visit(PT::FunctionDefinition *node)
  {
    Trace trace("SymbolFinder::visit(FunctionDefinition)", Trace::SYMBOLLOOKUP);
    PT::Node *decl = PT::nth<1>(node);
    visit(static_cast<PT::Declarator *>(decl)); // visit the declarator
    try { traverse_body(node);}
    catch (ST::Undefined const &e) {} // just ignore
  }

  virtual void visit(PT::Declarator *decl)
  {
    Trace trace("SymbolFinder::visit(Declarator)", Trace::SYMBOLLOOKUP);
    PT::Encoding name = decl->encoded_name();
    // FIXME: The parser currently parses type specifiers that contain a template_id
    //        such that in contains declarators with empty names.
    //        Ignore these until the parser is fixed.
    if (name.empty()) return;
    PT::Encoding type = decl->encoded_type();
    my_os << "Declarator : " << name.unmangled() << std::endl;
    lookup(name, ST::Scope::DECLARATION);
    if (type.is_function()) return;
    if (PT::Node *initializer = decl->initializer())
      initializer->accept(this);
  }

  virtual void visit(PT::Name *n)
  {
    Trace trace("SymbolFinder::visit(Name)", Trace::SYMBOLLOOKUP);
    PT::Encoding name = n->encoded_name();
    my_os << "Name : " << name.unmangled() << std::endl;
    lookup(name);
  }
  
  virtual void visit(PT::ClassSpec *node)
  {
    Trace trace("SymbolFinder::visit(ClassSpec)", Trace::SYMBOLLOOKUP);
    PT::Encoding name = node->encoded_name();
    my_os << "ClassSpec : " << name.unmangled() << std::endl;
    // FIXME: A ClassSpec may correspond to a class template, in which case
    //        we shouldn't do a elaborate lookup. The visit(TemplateDecl)
    //        method above will thus deal with the lookup itself.
    if (my_in_template_decl)
      lookup(name);
    else
      lookup(name, ST::Scope::ELABORATED);
    
    for (PT::List const *base_clause = node->base_clause();
	 base_clause;
	 base_clause = PT::tail(base_clause, 2))
    {
      PT::Node const *parent = PT::last(static_cast<PT::List *>(PT::nth<1>(base_clause)))->car();
      const_cast<PT::Node *>(parent)->accept(this);
    }
    traverse_body(node);
  }

  virtual void visit(PT::ElaboratedTypeSpec *node)
  {
    Trace trace("SymbolFinder::visit(ElaboratedTypeSpec)", Trace::SYMBOLLOOKUP);
    PT::Encoding name = node->name()->encoded_name();
    my_os << "ElaboratedTypeSpec : " << name.unmangled() << std::endl;
    lookup(name, ST::Scope::ELABORATED);
  }

  virtual void visit(PT::FuncallExpr *node)
  {
    Trace trace("SymbolFinder::visit(FuncallExpr)", Trace::SYMBOLLOOKUP);
    PT::Node *function = node->car();
    PT::Encoding name;
    if (function->is_atom()) name.simple_name(static_cast<PT::Atom *>(function));
    else name = function->encoded_name(); // function is a 'PT::Name'
    my_os << "Function : " << name.unmangled() << std::endl;
    lookup(name);
    ST::Symbol const *symbol = TA::resolve_funcall(node, current_scope());
  }

  void lookup(PT::Encoding const &name,
	      ST::Scope::LookupContext c = ST::Scope::DEFAULT,
	      bool type = false)
  {
    Trace trace("SymbolFinder::lookup", Trace::SYMBOLLOOKUP);
    ST::SymbolSet symbols = ST::Walker::lookup(name, c);
    if (!symbols.empty())
    {
      if (type) // Expect a single match that is a type-name.
      {
	ST::TypeName const *type = dynamic_cast<ST::TypeName const *>(*symbols.begin());
	if (type)
	{
	  std::string filename;
	  unsigned long line_number = my_buffer.origin(type->ptree()->begin(), filename);
	  my_os << "declared at line " << line_number << " in " << filename << std::endl;
	  return;
	}
	else my_os << "only non-types found" << std::endl;
      }

      for (ST::SymbolSet::iterator s = symbols.begin(); s != symbols.end(); ++s)
      {
	std::string filename;
	unsigned long line_number = my_buffer.origin((*s)->ptree()->begin(), filename);
	my_os << "declared at line " << line_number << " in " << filename << std::endl;
      }
    }
    else
      my_os << "undeclared ! " << std::endl;
  }

  Buffer const &my_buffer;
  std::ostream &my_os;
  bool          my_in_template_decl;
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
      Trace::enable(Trace::SYMBOLLOOKUP|Trace::TYPEANALYSIS);
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
    Buffer buffer(ifs.rdbuf(), "<input>");
    Lexer lexer(&buffer);
    SymbolFactory symbols;
    Parser parser(lexer, symbols);
    PT::Node *node = parser.parse();
    const Parser::ErrorList &errors = parser.errors();
    for (Parser::ErrorList::const_iterator i = errors.begin(); i != errors.end(); ++i)
      (*i)->write(ofs);
    if (errors.empty())
    {
      SymbolFinder finder(buffer, symbols.current_scope(), ofs);
      finder.find(node);
    }
  }
  catch (std::exception const &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
