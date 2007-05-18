//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/AST/ASTKit.hh>
#include <Synopsis/Python/Module.hh>
#include <Synopsis/Trace.hh>
#include <Synopsis/Timer.hh>
#include <Synopsis/PTree.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/SymbolLookup.hh>
#include <Synopsis/Buffer.hh>
#include <Synopsis/Lexer.hh>
#include <Synopsis/SymbolFactory.hh>
#include <Synopsis/Parser.hh>
#include "ASTTranslator.hh"
#include <Support/ErrorHandler.hh>
#include <fstream>

using namespace Synopsis;

namespace
{
PyObject *error;

//. Override unexpected() to print a message before we abort
void unexpected()
{
  std::cout << "Warning: Aborting due to unexpected exception." << std::endl;
  throw std::bad_exception();
}

PyObject *parse(PyObject * /* self */, PyObject *args)
{
  PyObject *py_ast;
  const char *src, *cppfile;
  char const * base_path = "";
  char const * syntax_prefix = 0;
  char const * xref_prefix = 0;
  int primary_file_only, verbose, debug, profile;
  if (!PyArg_ParseTuple(args, "Ossizzziii",
                        &py_ast, &cppfile, &src,
                        &primary_file_only,
                        &base_path,
                        &syntax_prefix,
                        &xref_prefix,
                        &verbose,
                        &debug,
			&profile))
    return 0;

  Py_INCREF(py_ast);
  AST::AST ast(py_ast);
  Py_INCREF(py_ast);

  std::set_unexpected(unexpected);
  ErrorHandler error_handler();

//   if (verbose) ::verbose = true;
  if (debug) Synopsis::Trace::enable(Trace::TRANSLATION);

  if (!src || *src == '\0')
  {
    PyErr_SetString(PyExc_RuntimeError, "no input file");
    return 0;
  }
  try
  {
    std::ifstream ifs(cppfile);
    Buffer buffer(ifs.rdbuf(), src);
    Lexer lexer(&buffer, Lexer::GCC);
    SymbolFactory symbols(SymbolFactory::C99);
    Parser parser(lexer, symbols, Parser::GCC);
    Timer timer;
    PTree::Node *ptree = parser.parse();
    if (profile)
      std::cout << "Parser::parse took " << timer.elapsed() 
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
      ASTTranslator translator(src, base_path, primary_file_only, ast, verbose, debug);
      timer.reset();
      translator.translate(ptree, buffer);
      if (profile)
        std::cout << "ASTTranslator::translate took " << timer.elapsed() 
                  << " seconds" << std::endl;
    }
  }
  catch (std::exception const &e)
  {
    PyErr_SetString(error, e.what());
    return 0;
  }
  return py_ast;
}

PyMethodDef methods[] = {{(char*)"parse", parse, METH_VARARGS},
			 {0}};
}

extern "C" void initParserImpl()
{
  Python::Module module = Python::Module::define("ParserImpl", methods);
  module.set_attr("version", "0.1");
  Python::Object processor = Python::Object::import("Synopsis.Processor");
  Python::Object error_base = processor.attr("Error");
  error = PyErr_NewException("ParserImpl.ParseError", error_base.ref(), 0);
  module.set_attr("ParseError", error);
}
