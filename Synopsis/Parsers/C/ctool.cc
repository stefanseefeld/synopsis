//
// Copyright (C) 2003 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/AST/ASTKit.hh>
#include "Translator.hh"
#include "Debugger.hh"
#include "Printer.hh"
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

const char *base_path = 0;

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

//. return portably the current working directory
const std::string &get_cwd()
{
  static std::string path;
  if (path.empty())
#ifdef __WIN32__
  {
    DWORD size;
    if ((size = ::GetCurrentDirectoryA(0, 0)) == 0)
    {
      delete [] buf;
      throw std::runtime_error("error accessing current working directory");
    }
    char *buf = new char[size];
    if (::GetCurrentDirectoryA(size, buf) == 0)
    {
      delete [] buf;
      throw std::runtime_error("error accessing current working directory");
    }
    path = buf;
    delete [] buf;
  }
#else
    for (long path_max = 32;; path_max *= 2)
    {
      char *buf = new char[path_max];
      if (::getcwd(buf, path_max) == 0)
      {
	if (errno != ERANGE)
	{
	  delete [] buf;
	  throw std::runtime_error(strerror(errno));
	}
      }
      else
      {
	path = buf;
	delete [] buf;
	return path;
      }
      delete [] buf;
    }
#endif
  return path;
}

// normalize and absolutize the given path path
std::string normalize_path(std::string filename)
{
#ifdef __WIN32__
  char separator = '\\';
  const char *pat1 = "\\.\\";
  const char *pat2 = "\\..\\";
#else
  char separator = '/';
  const char *pat1 = "/./";
  const char *pat2 = "/../";
#endif
  if (filename[0] != separator)
    filename.insert(0, get_cwd() + separator);

  // nothing to do...
  if (filename.find(pat1) == std::string::npos &&
      filename.find(pat2) == std::string::npos) return filename;
  
  // for the rest we'll operate on a decomposition of the filename...
  typedef std::vector<std::string> Path;
  Path path;

  std::string::size_type b = 0;
  while (b < filename.size())
  {
    std::string::size_type e = filename.find(separator, b);
    path.push_back(std::string(filename, b, e-b));
    b = e == std::string::npos ? std::string::npos : e + 1;
  }

  // remove all '.' and '' components
  path.erase(std::remove(path.begin(), path.end(), "."), path.end());
  path.erase(std::remove(path.begin(), path.end(), ""), path.end());
  // now collapse '..' components with the preceding one
  while (true)
  {
    Path::iterator i = std::find(path.begin(), path.end(), "..");
    if (i == path.end()) break;
    if (i == path.begin()) throw std::invalid_argument("invalid path");
    path.erase(i - 1, i + 1); // remove two components
  }

  // now rebuild the path as a string
  std::string retn = '/' + path.front();
  for (Path::iterator i = path.begin() + 1; i != path.end(); ++i)
    retn += '/' + *i;
  return retn;
}

const char *strip_base_path(const char *filename)
{
  if (!base_path) return filename;
  size_t length = strlen(base_path);
  if (strncmp(filename, base_path, length) == 0)
    return filename + length;
  return filename;
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

    Synopsis::AST::AST ast(py_ast);
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
			 {(char*)"dump", ctool_print, METH_VARARGS},
			 {0, 0}};
};

extern "C" void initctool()
{
  Python::Module module = Python::Module::define("ctool", methods);
  module.set_attr("version", "0.1");
}
