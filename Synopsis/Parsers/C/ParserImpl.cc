//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <boost/python.hpp>
#include <Synopsis/Trace.hh>
#include <Synopsis/Timer.hh>
#include <Synopsis/PTree.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/SymbolLookup.hh>
#include <Synopsis/Buffer.hh>
#include <Synopsis/Lexer.hh>
#include <Synopsis/SymbolFactory.hh>
#include <Synopsis/Parser.hh>
#include "ASGTranslator.hh"
#include <Support/ErrorHandler.hh>
#include <fstream>

using namespace Synopsis;
namespace bpl = boost::python;

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
                  bool primary_file_only, char const *sxr_prefix,
                  bool verbose, bool debug, bool profile)
{
  std::set_unexpected(unexpected);
  ErrorHandler error_handler();

//   if (verbose) ::verbose = true;
  if (debug) Synopsis::Trace::enable(Trace::TRANSLATION);

  if (!input_file || *input_file == '\0') throw std::runtime_error("no input file");
  std::ifstream ifs(cpp_file);
  Buffer buffer(ifs.rdbuf(), input_file);
  Lexer lexer(&buffer, Lexer::GCC);
  SymbolFactory symbols(SymbolFactory::C99);
  Parser parser(lexer, symbols, Parser::GCC);
  Timer timer;
  PTree::Node *ptree = parser.parse();
  if (profile)
    std::cout << "Parser::parse took " << timer.elapsed() << " seconds" << std::endl;
  const Parser::ErrorList &errors = parser.errors();
  if (!errors.empty())
  {
    for (Parser::ErrorList::const_iterator i = errors.begin(); i != errors.end(); ++i)
      (*i)->write(std::cerr);
    throw std::runtime_error("The input contains errors.");
  }
  else if (ptree)
  {
    ASGTranslator translator(input_file, base_path, primary_file_only, ir, verbose, debug);
    timer.reset();
    translator.translate(ptree, buffer);
    if (profile)
      std::cout << "ASG translation took " << timer.elapsed() << " seconds" << std::endl;
  }
  return ir;
}

}

BOOST_PYTHON_MODULE(ParserImpl)
{
  bpl::scope scope;
  scope.attr("version") = "0.2";
  bpl::def("parse", parse);
  bpl::object processor = bpl::import("Synopsis.Processor");
  bpl::object error_base = processor.attr("Error");
  error_type = bpl::object(bpl::handle<>(PyErr_NewException("ParserImpl.ParseError",
                                                            error_base.ptr(), 0)));
  scope.attr("ParseError") = error_type;
}
