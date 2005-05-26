//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/Buffer.hh>
#include <Synopsis/Lexer.hh>
#include <Synopsis/SymbolFactory.hh>
#include <Synopsis/Parser.hh>
#include <Synopsis/PTree.hh>
#include <Synopsis/SymbolLookup.hh>
#include <Synopsis/Trace.hh>
#include <fstream>

using namespace Synopsis;

class SymbolDisplay : private SymbolLookup::SymbolVisitor
{
public:
  SymbolDisplay(std::ostream &os, size_t indent)
    : my_os(os), my_indent(indent, ' ') {}
  void display(PTree::Encoding const &name, SymbolLookup::Symbol const *symbol)
  {
    my_name = name.unmangled();
    symbol->accept(this);
    my_os << std::endl;
  }
private:
  std::ostream &prefix(std::string const &type)
  { return my_os << my_indent << type;}
  virtual void visit(SymbolLookup::Symbol const *) {}
  virtual void visit(SymbolLookup::VariableName const *name)
  {
    prefix("Variable:          ") << my_name << ' ' << name->type().unmangled();
  }
  virtual void visit(SymbolLookup::ConstName const *name)
  {
    prefix("Const:             ") << my_name << ' ' << name->type().unmangled();
    if (name->defined()) my_os << " (" << name->value() << ')';
  }
  virtual void visit(SymbolLookup::TypeName const *name)
  {
    prefix("Type:              ") << my_name << ' ' << name->type().unmangled();
  }
  virtual void visit(SymbolLookup::TypedefName const *name)
  {
    prefix("Typedef:           ") << my_name << ' ' << name->type().unmangled();
  }
  virtual void visit(SymbolLookup::ClassName const *name)
  {
    prefix("Class:             ") << my_name << ' ' << name->type().unmangled();
  }
  virtual void visit(SymbolLookup::EnumName const *name)
  {
    prefix("Enum:              ") << my_name << ' ' << name->type().unmangled();
  }
  virtual void visit(SymbolLookup::ClassTemplateName const *name)
  {
    prefix("Class template:    ") << my_name << ' ' << name->type().unmangled();
  }
  virtual void visit(SymbolLookup::FunctionName const *name)
  {
    prefix("Function:          ") << my_name << ' ' << name->type().unmangled();
  }
  virtual void visit(SymbolLookup::FunctionTemplateName const *name)
  {
    prefix("Function template: ") << my_name << ' ' << name->type().unmangled();
  }
  virtual void visit(SymbolLookup::NamespaceName const *name)
  {
    prefix("Namespace:         ") << my_name << ' ' << name->type().unmangled();
  }

  std::ostream &my_os;
  std::string   my_indent;
  std::string   my_name;
};

class ScopeDisplay : private SymbolLookup::ScopeVisitor
{
public:
  ScopeDisplay(std::ostream &os) : my_os(os), my_indent(0) {}
  virtual ~ScopeDisplay() {}
  void display(SymbolLookup::Scope *s) { s->accept(this);}
private:
  virtual void visit(SymbolLookup::TemplateParameterScope *s)
  {
    indent() << "TemplateParameterScope:\n";
    dump(s);
  }
  virtual void visit(SymbolLookup::LocalScope *s)
  {
    indent() << "LocalScope:\n";
    dump(s);
  }
  virtual void visit(SymbolLookup::PrototypeScope *s)
  {
    indent() << "PrototypeScope '" << s->name() << "':\n";
    dump(s);
  }
  virtual void visit(SymbolLookup::FunctionScope *s)
  {
    indent() << "FunctionScope '" << s->name() << "':\n";
    dump(s);
  }
  virtual void visit(SymbolLookup::Class *s)
  {
    indent() << "Class '" << s->name() << "':\n";
    dump(s);
  }
  virtual void visit(SymbolLookup::Namespace *s)
  {
    indent() << "Namespace '" << s->name() << "':\n";
    dump(s);
  }

  void dump(SymbolLookup::Scope const *s)
  {
    ++my_indent;
    for (SymbolLookup::Scope::symbol_iterator i = s->symbols_begin();
	 i != s->symbols_end();
	 ++i)
    {
      SymbolDisplay display(my_os, my_indent);
      display.display(i->first, i->second);
    }
    for (SymbolLookup::Scope::scope_iterator i = s->scopes_begin();
	 i != s->scopes_end();
	 ++i)
      i->second->accept(this);
    --my_indent;
  }
  std::ostream &indent()
  {
    size_t i = my_indent;
    while (i--) my_os.put(' ');
    return my_os;
  }

  std::ostream &my_os;
  size_t        my_indent;
};



int usage(const char *command)
{
  std::cerr << "Usage: " << command << " <input>" << std::endl;
  return -1;
}

int main(int argc, char **argv)
{
  bool debug = false;
  const char *input = argv[1];
  if (argc == 1) return usage(argv[0]);
  for (int i = 1; i < argc - 1; ++i)
  {
    if (argv[i] == std::string("-d")) debug = true;
    else return usage(argv[0]);
  }
  input = argv[argc - 1];
  try
  {
    if (debug) Trace::enable(Trace::SYMBOLLOOKUP);

    std::ifstream ifs(input);
    Buffer buffer(ifs.rdbuf(), input);
    Lexer lexer(&buffer);
    SymbolFactory symbols;
    Parser parser(lexer, symbols);
    PTree::Node *node = parser.parse();
    const Parser::ErrorList &errors = parser.errors();
    for (Parser::ErrorList::const_iterator i = errors.begin(); i != errors.end(); ++i)
      (*i)->write(std::cerr);

    ScopeDisplay display(std::cout);
    display.display(symbols.current_scope());
  }
  catch (const std::exception &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
  }
}
