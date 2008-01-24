//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "ASGTranslator.hh"
#include "SXRGenerator.hh"
#include <Synopsis/Trace.hh>
#include <Synopsis/Timer.hh>
#include <Synopsis/PTree.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/SymbolLookup.hh>
#include <Synopsis/Buffer.hh>
#include <Synopsis/Lexer.hh>
#include <Synopsis/SymbolFactory.hh>
#include <Synopsis/Parser.hh>
#include <Support/ErrorHandler.hh>
#include <Support/path.hh>
#include <boost/filesystem/convenience.hpp>
#include <fstream>

using namespace Synopsis;
namespace fs = boost::filesystem;

namespace
{
bpl::object error_type;

//. Override unexpected() to print a message before we abort
void unexpected()
{
  std::cout << "Warning: Aborting due to unexpected exception." << std::endl;
  throw std::bad_exception();
}

bpl::object parse(bpl::object ir,
                  char const *cpp_file, char const *input_file, char const *base_path,
                  bool primary_file_only, char const *syntax_prefix, char const *xref_prefix,
                  bool verbose, bool debug, bool profile)
{
  std::set_unexpected(unexpected);
  ErrorHandler error_handler();

  if (debug) Synopsis::Trace::enable(Trace::TRANSLATION);

  if (!input_file || *input_file == '\0') throw std::runtime_error("no input file");

  std::ifstream ifs(cpp_file);
  Buffer buffer(ifs.rdbuf(), input_file);
  Lexer lexer(&buffer, Lexer::CXX|Lexer::GCC);
  SymbolFactory symbols(SymbolFactory::CXX);
  Parser parser(lexer, symbols, Parser::CXX|Parser::GCC);
  Timer timer;
  PTree::Node *ptree = parser.parse();
  if (profile)
    std::cout << "C++ parser took " << timer.elapsed() 
              << " seconds" << std::endl;
  const Parser::ErrorList &errors = parser.errors();
  if (!errors.empty())
  {
    for (Parser::ErrorList::const_iterator i = errors.begin(); i != errors.end(); ++i)
      (*i)->write(std::cerr);
    throw std::runtime_error("The input contains errors.");
  }
  else if (ptree)
  {
    timer.reset();
    ASGTranslator translator(symbols.current_scope(),
                             input_file, base_path, primary_file_only, ir, verbose, debug);
    translator.translate(ptree, buffer);
    if (profile)
      std::cout << "ASG translation took " << timer.elapsed() 
                << " seconds" << std::endl;
    if (syntax_prefix)
    {
      timer.reset();
      std::string long_filename = make_full_path(input_file);
      std::string short_filename = make_short_path(input_file, base_path);
      bpl::object file = ir.attr("files")[short_filename];
      // Undo the preprocesser's macro expansions.
      bpl::dict macro_calls = bpl::dict(file.attr("macro_calls"));
      std::string sxr = std::string(syntax_prefix) + "/" + short_filename + ".sxr";
      create_directories(fs::path(sxr).branch_path());
      std::ofstream ofs(sxr.c_str());
      SXRGenerator generator(buffer, symbols.current_scope(), ofs);
      generator.process(ptree);
      if (profile)
        std::cout << "SXR generation took " << timer.elapsed() 
                  << " seconds" << std::endl;
    }
  }
  return ir;
}

}

BOOST_PYTHON_MODULE(ParserImpl)
{
  bpl::scope scope;
  scope.attr("version") = "0.2";
  bpl::def("parse", parse);
  bpl::object module = bpl::import("Synopsis.Processor");
  bpl::object error_base = module.attr("Error");
  error_type = bpl::object(bpl::handle<>(PyErr_NewException("ParserImpl.ParseError",
                                                            error_base.ptr(), 0)));
  scope.attr("ParseError") = error_type;
}
