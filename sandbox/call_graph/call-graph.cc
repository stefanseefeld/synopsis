//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/Buffer.hh>
#include <Synopsis/Lexer.hh>
#include <Synopsis/Parser.hh>
#include <Synopsis/PTree.hh>
#include <Synopsis/SymbolLookup.hh>
#include <Synopsis/PTree/Writer.hh>
#include <Synopsis/Trace.hh>
#include <fstream>
#include <set>

using namespace Synopsis;
namespace PT = Synopsis::PTree;
namespace SL = Synopsis::SymbolLookup;

class EnclosingFunctionFinder : private SL::ScopeVisitor
{
public:
  //. Find the nearest enclosing scope that is either
  //. a function or namespace (i.e. global scope).
  SL::Scope const *find(SL::Scope const *s)
  {
    my_scope = 0;
    const_cast<SL::Scope *>(s)->accept(this);
    return my_scope;
  }
private:

  virtual void visit(SL::LocalScope *s) 
  { const_cast<SL::Scope *>(s->outer_scope())->accept(this);}
  virtual void visit(SL::FunctionScope *s) { my_scope = s;}
  virtual void visit(SL::Namespace *s) { my_scope = s;}

  SL::Scope const *my_scope;
};

//. A simple call graph generator.
//. This class finds function call expressions and
//. reports the (function) scope of the call together
//. with the name of the callee.
//. As this is only a demo of part of the Synopsis API
//. no attempt has been made for completeness, i.e. there
//. is no pointer analysis, etc.
//. Further, only C sources are supported as for C++
//. type analysis would be needed to do proper
//. overload resolution, which is not yet implemented.
class CallGraphGenerator : private SL::Walker
{
public:
  CallGraphGenerator(std::ostream &os, SL::Scope *symbols)
    : Walker(symbols), my_os(os) {}
  void generate(PT::Node const *node) 
  {
    my_os << "digraph CallGraph\n{\n"
	  << "node[fillcolor=\"#ffffcc\", pencolor=\"#424242\" style=\"filled\"];\n";
    const_cast<PT::Node *>(node)->accept(this);
    my_os << '}' << std::endl;
  }
private:
  void visit(PT::FuncallExpr *func)
  {
    PT::Node *name = PT::first(func);

    EnclosingFunctionFinder function_finder;
    SL::Scope const *scope = function_finder.find(current_scope());
    std::string caller = "<global>";
    if (SL::FunctionScope const *func = dynamic_cast<SL::FunctionScope const *>(scope))
      caller = func->name();
    std::string callee = PT::reify(name);
    if (my_cache.find(caller) == my_cache.end())
    {
      my_cache.insert(caller);
      my_os << '"' << caller << "\" [label=\"" << caller << "\"];\n";
    }
    if (my_cache.find(callee) == my_cache.end())
    {
      my_cache.insert(callee);
      my_os << '"' << callee << "\" [label=\"" << callee << "\"];\n";
    }
    my_os << '"' << caller << "\" -> \"" << callee << '"' << std::endl;
  }

  std::set<std::string> my_cache;
  std::ostream &        my_os;
};

int usage(const char *command)
{
  std::cerr << "Usage: " << command 
	    << " [-C] [-d] [-o <output>] <input>" << std::endl;
  return -1;
}

int main(int argc, char **argv)
{
  const char *input = argv[1];
  std::ofstream output;
  if (argc == 1) return usage(argv[0]);
  input = argv[argc - 1];
  bool cxx = false;
  for (size_t i = 1; i != argc - 1; ++i)
  {
    if (argv[i] == std::string("-C")) cxx = true;
    else if (argv[i] == std::string("-d")) Trace::enable();
    else if (argv[i] == std::string("-o") && i < argc - 1)
      output.open(argv[++i]);
    else return usage(argv[0]);
  }
  if (!output.is_open())
  {
    output.copyfmt(std::cout);
    output.clear(std::cout.rdstate());
    static_cast<std::basic_ios<char> &>(output).rdbuf(std::cout.rdbuf());
  }
//   try
  {
    // FIXME: For now we have to initialize the Encoding...
    PT::Encoding::do_init_static();

    std::ifstream ifs(input);
    Buffer buffer(ifs.rdbuf(), input);
    int tokenset = Lexer::C | Lexer::GCC | Lexer::MSVC;
    int ruleset = 0;
    if (cxx)
    {
      tokenset |= Lexer::CXX;
      ruleset = Parser::CXX | Parser::MSVC;
    }
    Lexer lexer(&buffer, tokenset);
    SymbolFactory symbols;
    Parser parser(lexer, symbols, ruleset);
    PT::Node *node = parser.parse();
    const Parser::ErrorList &errors = parser.errors();
    for (Parser::ErrorList::const_iterator i = errors.begin(); i != errors.end(); ++i)
      (*i)->write(std::cerr);

    if (!node) return -1;
    CallGraphGenerator generator(output, symbols.current_scope());
    generator.generate(node);
  }
//   catch (const std::exception &e)
//   {
//     std::cerr << "Caught exception : " << e.what() << std::endl;
//     throw;
//   }
}
