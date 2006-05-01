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
#include <Support/ErrorHandler.hh>
#include <Support/Path.hh>
#include "ASTTranslator.hh"
#include "SXRGenerator.hh"
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
    Lexer lexer(&buffer, Lexer::CXX|Lexer::GCC);
    SymbolFactory symbols(SymbolFactory::CXX);
    Parser parser(lexer, symbols, Parser::CXX|Parser::GCC);
    Timer timer;
    PTree::Node *ptree = parser.parse();
    if (profile)
      std::cout << "C++ parser took " << timer.elapsed() 
		<< " seconds" << std::endl;
    const Parser::ErrorList &errors = parser.errors();
    if (!errors.size() && ptree)
    {
      timer.reset();
      ASTTranslator translator(symbols.current_scope(),
			       src, base_path, primary_file_only, ast, verbose, debug);
      translator.translate(ptree, buffer);
      if (profile)
	std::cout << "AST translation took " << timer.elapsed() 
		  << " seconds" << std::endl;
      if (syntax_prefix)
      {
	timer.reset();
	Path path(src);
	path.abs();
 	std::string long_filename = path.str();
	Path base(base_path);
	base.abs();
 	path.strip(base.str());
 	std::string short_filename = path.str();
 	AST::SourceFile file = ast.files().get(short_filename);
	std::string sxr = std::string(syntax_prefix) + "/" + short_filename + ".sxr";
	makedirs(Path(sxr).dirname());
 	std::ofstream ofs(sxr.c_str());
 	SXRGenerator generator(buffer, symbols.current_scope(), ofs);
 	generator.process(ptree);
	if (profile)
	  std::cout << "SXR generation took " << timer.elapsed() 
		    << " seconds" << std::endl;
      }
    }
    else
    {
      for (Parser::ErrorList::const_iterator i = errors.begin(); i != errors.end(); ++i)
	(*i)->write(std::cerr);
      throw std::runtime_error("The input contains errors.");
    }
  }
  catch (std::invalid_argument const &e)
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
