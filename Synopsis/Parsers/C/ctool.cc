//
// Copyright (C) 2003 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/AST/ASTKit.hh>
#include <Synopsis/Trace.hh>
#include <Synopsis/Path.hh>
#include <Synopsis/ErrorHandler.hh>
#include "Translator.hh"
#include "Debugger.hh"
#include "Printer.hh"
#include "Statement.hh"
#include "File.hh"
#include <fstream>

using namespace Synopsis;

namespace
{

const char *base_path = 0;

//. Override unexpected() to print a message before we abort
void unexpected()
{
  std::cout << "Warning: Aborting due to unexpected exception." << std::endl;
  throw std::bad_exception();
}

class IncludeFixer : public StatementVisitor
{
public:
  IncludeFixer(Python::List i) : my_includes(i) {}
  virtual void traverse_include(InclStemnt *);
  void fix_file(File *);
private:
  Python::TypedList<AST::Include> my_includes;
};

void IncludeFixer::traverse_include(InclStemnt *s)
{
  for (size_t i = 0; i != my_includes.size(); ++i)
  {
    AST::Include inc = my_includes.get(i);
    std::string name = inc.target().name();
    if (name == s->filename)
    {
      s->text = inc.name();
      break;
    }
  }
}

void IncludeFixer::fix_file(File *f)
{
  for (Statement *s = f->my_head; s; s = s->next)
    s->accept(this);
}

PyObject *ctool_parse(PyObject *self, PyObject *args)
{
  try
  {
    char *input;
    char *filename;
    char *syntax_prefix;
    char *xref_prefix;
    int verbose = 0;
    int debug = 0;

    PyObject *py_ast;
    if (!PyArg_ParseTuple(args, "Osszzzii",
                          &py_ast,
                          &input,
			  &filename,
                          &base_path,
			  &syntax_prefix,
			  &xref_prefix,
                          &verbose,
                          &debug))
      return 0;

    AST::AST ast(py_ast);
    Py_INCREF(py_ast);

    std::set_unexpected(unexpected);

    ErrorHandler error_handler();
    
    if (debug) Trace::enable_debug();
    try
    {
      File *file = File::parse(input, filename);
      if (debug)
      {
	StatementDebugger debugger(std::cout);
	debugger.debug_file(file);
      }
      StatementTranslator translator(ast, ast.declarations(), 
				     verbose == 1, debug == 1);
      translator.translate_file(file);
    }
    catch (const Python::Object::TypeError &e)
    {
      std::cerr << "TypeError : " << e.what() << std::endl;
    }
    catch (const Python::Object::AttributeError &e)
    {
      std::cerr << "AttributeError : " << e.what() << std::endl;
    }
    catch (const Python::Object::KeyError &e)
    {
      std::cerr << "KeyError : " << e.what() << std::endl;
    }
    catch (const Python::Object::ImportError &e)
    {
      std::cerr << "ImportError : " << e.what() << std::endl;
    }
    catch (const std::exception &e)
    {
      std::cerr << "internal error : " << e.what() << std::endl;
    }

    return py_ast;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Internal error : " << e.what() << std::endl;
    return 0;
  }
}

PyObject *ctool_print(PyObject *self, PyObject *args)
{
  try
  {
    char *input;
    char *filename;
    char *output;
    int symbols = 0;
    int debug = 0;

    PyObject *py_ast;
    if (!PyArg_ParseTuple(args, "Osssii",
                          &py_ast,
                          &input,
			  &filename,
			  &output,
                          &symbols,
                          &debug))
      return 0;
    Synopsis::AST::AST ast(py_ast);
    Py_INCREF(py_ast);

    std::set_unexpected(unexpected);

    ErrorHandler error_handler();
    
    try
    {
      Path path(filename);
      std::string src = path.abs().str();

      Python::List includes;
      for (Python::Dict::iterator i = ast.files().begin();
	   i != ast.files().end();
	   ++i)
      {
  	AST::SourceFile file = i->get(1);
 	if (src == file.long_name())
	{
	  includes = file.includes();
 	  break;
 	}
      }

      File *file = File::parse(input, filename);

      IncludeFixer fixer(includes);
      fixer.fix_file(file);

      std::ofstream ofs(output);
      StatementPrinter printer(ofs);
      printer.print_file(file);
      ofs << std::endl;

      if (symbols || debug)
      {
	ofs << "Symbols:";
	file->my_contxt.syms->Show(ofs);
	ofs << "Tags:";
	file->my_contxt.tags->Show(ofs);
	ofs << "Labels:";
	file->my_contxt.labels->Show(ofs);
      }
    }
    catch (const std::exception &e)
    {
      std::cerr << "internal error : " << e.what() << std::endl;
    }

    Py_INCREF(Py_None);
    return Py_None;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Internal error : " << e.what() << std::endl;
    return 0;
  }
}

PyMethodDef methods[] = {{(char*)"parse", ctool_parse, METH_VARARGS},
			 {(char*)"dump", ctool_print, METH_VARARGS},
			 {0, 0}};
};

extern "C" void initctool()
{
  Python::Module module = Python::Module::define("ctool", methods);
  module.set_attr("version", "0.1");
}
