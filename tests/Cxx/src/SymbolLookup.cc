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

class SymbolFinder : private Walker
{
public:
  SymbolFinder(Buffer const &buffer, Scope *scope, std::ostream &os)
    : Walker(scope), my_buffer(buffer), my_os(os), my_in_template_decl(false) {}
  void find(PTree::Node *node) { node->accept(this);}
private:
  using Walker::visit;
  virtual void visit(PTree::Identifier *iden)
  {
    Trace trace("SymbolFinder::visit(Identifier)", Trace::SYMBOLLOOKUP);
    PTree::Encoding name = PTree::Encoding::simple_name(iden);
    my_os << "Identifier : " << name.unmangled() << std::endl;
    lookup(name);
  }

  virtual void visit(PTree::Block *node)
  {
    Trace trace("SymbolFinder::visit(Block)", Trace::SYMBOLLOOKUP);
    Walker::visit(node);
  }

  virtual void visit(PTree::Typedef *typed)
  {
    Trace trace("SymbolFinder::visit(Typedef)", Trace::SYMBOLLOOKUP);
    // We need to figure out how to reproduce the (encoded) name of
    // the type being aliased.
    my_os << "Type : " << "<not implemented yet>" << std::endl;
//     lookup(name);
    PTree::third(typed)->accept(this);
  }

  virtual void visit(PTree::TemplateDecl *tdecl)
  {
    Trace trace("SymbolFinder::visit(TemplateDecl)", Trace::SYMBOLLOOKUP);
    traverse_parameters(tdecl);
    bool saved_in_template_decl = my_in_template_decl;
    my_in_template_decl = true;
    PTree::Node *fourth = PTree::nth(tdecl, 4);
    if (!fourth->is_atom())
      fourth->accept(this); // This is a declaration.
    else
    {
      // We are in a template template parameter.
      // Visit the identifier and the default type, if present.
      PTree::Node *fifth = PTree::nth(tdecl, 5);
      if (fifth) fifth->accept(this); // This is an identifier
    }
    my_in_template_decl = saved_in_template_decl;
  }

  virtual void visit(PTree::NamespaceSpec *node)
  {
    Trace trace("SymbolFinder::visit(NamespaceSpec)", Trace::SYMBOLLOOKUP);
    PTree::Node const *name = PTree::second(node);
    PTree::Encoding ename;
    if (name) ename.simple_name(name);
    else ename.append_with_length("<anonymous>");
    my_os << "Namespace : " << ename.unmangled() << std::endl;
    lookup(ename);
    traverse_body(node);
  }

  virtual void visit(PTree::Declaration *node)
  {
    Trace trace("SymbolFinder::visit(Declaration)", Trace::SYMBOLLOOKUP);
    PTree::Node *type_spec = PTree::second(node);
    // FIXME: what about compound types ?
    if (type_spec && type_spec->is_atom())
    {
      PTree::Atom const *name = static_cast<PTree::Atom *>(type_spec);
      PTree::Encoding type = PTree::Encoding::simple_name(name);
      my_os << "Type : " << type.unmangled() << std::endl;
      lookup(type, Scope::DEFAULT, true);
      PTree::third(node)->accept(this);
    }
    else
      Walker::visit(node);
  }

  virtual void visit(PTree::UsingDirective *udir)
  {
    PTree::Node *node = PTree::third(udir);
    PTree::Encoding name = node->encoded_name();
    my_os << "Namespace : " << name.unmangled() << std::endl;
    lookup(name);
  }
  virtual void visit(PTree::UsingDeclaration *udecl)
  { visit(static_cast<PTree::List *>(udecl));}
  virtual void visit(PTree::NamespaceAlias *alias)
  {
    {
      PTree::Node *node = PTree::second(alias);
      PTree::Encoding name = PTree::Encoding::simple_name(static_cast<PTree::Atom *>(node));
      my_os << "Namespace : " << name.unmangled() << std::endl;
      lookup(name);
    }
    {
      PTree::Node *node = PTree::nth(alias, 3);
      PTree::Encoding name = node->encoded_name();
      my_os << "Namespace : " << name.unmangled() << std::endl;
      lookup(name);
    }
  }

  virtual void visit(PTree::FunctionDefinition *node)
  {
    Trace trace("SymbolFinder::visit(FunctionDefinition)", Trace::SYMBOLLOOKUP);
    PTree::Node *decl = PTree::third(node);
    visit(static_cast<PTree::Declarator *>(decl)); // visit the declarator
    try { traverse_body(node);}
    catch (Undefined const &e) {} // just ignore
  }

  virtual void visit(PTree::Declarator *decl)
  {
    Trace trace("SymbolFinder::visit(Declarator)", Trace::SYMBOLLOOKUP);
    PTree::Encoding name = decl->encoded_name();
    // FIXME: The parser currently parses type specifiers that contain a template_id
    //        such that in contains declarators with empty names.
    //        Ignore these until the parser is fixed.
    if (name.empty()) return;
    PTree::Encoding type = decl->encoded_type();
    my_os << "Declarator : " << name.unmangled() << std::endl;
    lookup(name, Scope::DECLARATION);
    if (type.is_function()) return;
    if (PTree::Node *initializer = decl->initializer())
      initializer->accept(this);
  }

  virtual void visit(PTree::Name *n)
  {
    Trace trace("SymbolFinder::visit(Name)", Trace::SYMBOLLOOKUP);
    PTree::Encoding name = n->encoded_name();
    my_os << "Name : " << name.unmangled() << std::endl;
    lookup(name);
  }
  
  virtual void visit(PTree::ClassSpec *node)
  {
    Trace trace("SymbolFinder::visit(ClassSpec)", Trace::SYMBOLLOOKUP);
    PTree::Encoding name = node->encoded_name();
    my_os << "ClassSpec : " << name.unmangled() << std::endl;
    // FIXME: A ClassSpec may correspond to a class template, in which case
    //        we shouldn't do a elaborate lookup. The visit(TemplateDecl)
    //        method above will thus deal with the lookup itself.
    if (my_in_template_decl)
      lookup(name);
    else
      lookup(name, Scope::ELABORATE);
    
    for (PTree::Node const *base_clause = node->base_clause();
	 base_clause;
	 base_clause = PTree::rest(PTree::rest(base_clause)))
    {
      PTree::Node const *parent = PTree::last(PTree::second(base_clause))->car();
      const_cast<PTree::Node *>(parent)->accept(this);
    }
    traverse_body(node);
  }

  virtual void visit(PTree::FuncallExpr *node)
  {
    Trace trace("SymbolFinder::visit(FuncallExpr)", Trace::SYMBOLLOOKUP);
    PTree::Node *function = node->car();
    PTree::Encoding name;
    if (function->is_atom()) name.simple_name(function);
    else name = function->encoded_name(); // function is a 'PTree::Name'
    my_os << "Function : " << name.unmangled() << std::endl;
    lookup(name);
  }

  void lookup(PTree::Encoding const &name,
	      Scope::LookupContext c = Scope::DEFAULT,
	      bool type = false)
  {
    Trace trace("SymbolFinder::lookup", Trace::SYMBOLLOOKUP);
    SymbolSet symbols = current_scope()->lookup(name, c);
    if (!symbols.empty())
    {
      if (type) // Expect a single match that is a type-name.
      {
	TypeName const *type = dynamic_cast<TypeName const *>(*symbols.begin());
	if (type)
	{
	  std::string filename;
	  unsigned long line_number = my_buffer.origin(type->ptree()->begin(), filename);
	  my_os << "declared at line " << line_number << " in " << filename << std::endl;
	  return;
	}
	else my_os << "only non-types found" << std::endl;
      }

      for (SymbolSet::iterator s = symbols.begin(); s != symbols.end(); ++s)
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
      Trace::enable(Trace::SYMBOLLOOKUP);
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
    Buffer buffer(ifs.rdbuf(), "<input>");
    Lexer lexer(&buffer);
    SymbolFactory symbols;
    Parser parser(lexer, symbols);
    PTree::Node *node = parser.parse();
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
