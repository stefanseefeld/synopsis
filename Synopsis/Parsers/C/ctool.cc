//
// Copyright (C) 2003 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "Translator.hh"
#include "PrintTraversal.hh"
#include "File.hh"
#include "Trace.hh"
#include <signal.h>
#include <sys/wait.h>
#include <fstream>

using namespace Synopsis::AST; // import all AST objects...
namespace Python = Synopsis; // ...and the others into 'Python'

bool Trace::debug = false;
int Trace::level = 0;

namespace
{

//. Override unexpected() to print a message before we abort
void unexpected()
{
  std::cout << "Warning: Aborting due to unexpected exception." << std::endl;
  throw std::bad_exception();
}

void sighandler(int signo)
{
  std::string signame;
  switch (signo)
  {
    case SIGABRT:
        signame = "Abort";
        break;
    case SIGBUS:
        signame = "Bus error";
        break;
    case SIGSEGV:
        signame = "Segmentation Violation";
        break;
    default:
        signame = "unknown";
        break;
  };
  // can we generate a report here ?
  std::cerr << signame << " caught" << std::endl;
  exit(-1);
}

PyObject *ctool_parse(PyObject *self, PyObject *args)
{
  try
  {
    char *input;
    char *filename;
    char *base_path;
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

    Py_INCREF(py_ast);
    

    std::set_unexpected(unexpected);
    struct sigaction olda;
    struct sigaction newa;
    newa.sa_handler = &sighandler;
    sigaction(SIGSEGV, &newa, &olda);
    sigaction(SIGBUS, &newa, &olda);
    sigaction(SIGABRT, &newa, &olda);
    
    if (debug) Trace::enable_debug();
    try
    {
      File *file = File::parse(input, filename);
      Translator translator(py_ast, verbose == 1, debug == 1);
      translator.traverse_file(file);
    }
    catch (const std::exception &e)
    {
      std::cerr << "internal error : " << e.what() << std::endl;
    }
    sigaction(SIGABRT, &olda, 0);
    sigaction(SIGBUS, &olda, 0);
    sigaction(SIGSEGV, &olda, 0);

    return py_ast;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Internal error : " << e.what() << std::endl;
    return 0;
  }
}

PyObject *ctool_dump(PyObject *self, PyObject *args)
{
  try
  {
    char *input;
    char *filename;
    char *output;
    int symbols = 0;
    int debug = 0;

    PyObject *py_ast;
    if (!PyArg_ParseTuple(args, "sssii",
                          &input,
			  &filename,
			  &output,
                          &symbols,
                          &debug))
      return 0;

    std::set_unexpected(unexpected);
    struct sigaction olda;
    struct sigaction newa;
    newa.sa_handler = &sighandler;
    sigaction(SIGSEGV, &newa, &olda);
    sigaction(SIGBUS, &newa, &olda);
    sigaction(SIGABRT, &newa, &olda);
    
    try
    {
      std::ofstream ofs(output);
      File *file = File::parse(input, filename);
      PrintTraversal printer(ofs, debug == 1);
      printer.traverse_file(file);
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
    sigaction(SIGABRT, &olda, 0);
    sigaction(SIGBUS, &olda, 0);
    sigaction(SIGSEGV, &olda, 0);

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
			 {(char*)"dump", ctool_dump, METH_VARARGS},
			 {0, 0}};
};

extern "C" void initctool()
{
  Python::Module module = Python::Module::define("ctool", methods);
  module.set_attr("version", "0.1");
}
