//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/AST/ASTKit.hh>
#include <Synopsis/ErrorHandler.hh>

#include <Synopsis/Python/Module.hh>
#include <Synopsis/Trace.hh>
#include <Synopsis/ErrorHandler.hh>
#include <Synopsis/PTree.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/SymbolLookup.hh>
#include <Synopsis/Buffer.hh>
#include <Synopsis/Lexer.hh>
#include <Synopsis/Parser.hh>
#include "ASTTranslator.hh"
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

PyObject *parse(PyObject *self, PyObject *args)
{
  PyObject *py_ast;
  const char *src, *cppfile;
  char const * base_path = "";
  char const * syntax_prefix = 0;
  char const * xref_prefix = 0;
  int main_file_only, verbose, debug;
  if (!PyArg_ParseTuple(args, "Ossizzzii",
                        &py_ast, &cppfile, &src,
                        &main_file_only,
                        &base_path,
                        &syntax_prefix,
                        &xref_prefix,
                        &verbose,
                        &debug))
    return 0;

  Py_INCREF(py_ast);
  AST::AST ast(py_ast);
  Py_INCREF(py_ast);

  std::set_unexpected(unexpected);
  ErrorHandler error_handler();

//   if (verbose) ::verbose = true;
  if (debug) Synopsis::Trace::enable_debug();

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
    SymbolLookup::Table symbols(SymbolLookup::Table::C99);
    Parser parser(lexer, symbols);
    PTree::Node *ptree = parser.parse();
    const Parser::ErrorList &errors = parser.errors();
    for (Parser::ErrorList::const_iterator i = errors.begin(); i != errors.end(); ++i)
      (*i)->write(std::cerr);
    if (ptree) 
    {
      ASTTranslator translator(src, base_path, main_file_only, ast, verbose, debug);
      translator.translate(ptree, buffer);
    }
  }
  catch (std::exception const &e)
  {
    std::cerr << "Caught exception : " << typeid(e).name() << ' ' << e.what() << std::endl;
    PyErr_SetString(error, e.what());
    return 0;
  }
  return py_ast;
}

PyMethodDef methods[] = {{(char*)"parse", parse, METH_VARARGS},
			 {0, 0}};
}

extern "C" void initParserImpl()
{
  Python::Module module = Python::Module::define("ParserImpl", methods);
  module.set_attr("version", "0.1");
  error = PyErr_NewException("ParserImpl.Error", 0, 0);
  module.set_attr("Error", error);
}
