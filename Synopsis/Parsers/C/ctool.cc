// $Id: ctool.cc,v 1.2 2003/08/20 03:25:16 stefan Exp $
//
// This file is a part of Synopsis.
// Copyright (C) 2003 Stefan Seefeld
//
// Synopsis is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
// $Log: ctool.cc,v $
// Revision 1.2  2003/08/20 03:25:16  stefan
// little fixes
//
// Revision 1.1  2003/08/20 02:16:37  stefan
// first steps towards a C parser backend (based on the ctool)
//
//

#include <ctool/ctool.h>
#include "Translator.hh"
#include "Trace.hh"
#include <signal.h>
#include <sys/wait.h>
#ifdef PYTHON_INCLUDE
#  include PYTHON_INCLUDE
#else
#  include <Python.h>
#endif
#include <fstream>

#if 1
int Trace::level = 0;
#endif

bool verbose;

// If true then everything but what's in the main file will be stripped
bool syn_main_only;
bool syn_extract_tails;
std::string syn_cpp;
bool syn_multi_files;

// If set then this is stripped from the start of all filenames
const char* syn_basename = "";

// If set then this is the prefix for the filename to store links to
const char* syn_syntax_prefix = 0;

// A place to temporarily store Python's thread state
PyThreadState *pythread_save;

namespace
{

//. Override unexpected() to print a message before we abort
void unexpected()
{
  std::cout << "Warning: Aborting due to unexpected exception." << std::endl;
  throw std::bad_exception();
}

void getopts(PyObject *args, std::vector<const char *> &cppflags, 
	     std::vector<const char *> &ctflags,
	     PyObject *config,
	     PyObject *extra_files)
{
  verbose = false;
  syn_main_only = false;
  syn_extract_tails = false;
  syn_multi_files = false;

#define IsType(obj, type) (Py##type##_Check(obj))

#define OPT_FLAG(syn_flag, config_name) \
if ((value = PyObject_GetAttrString(config, config_name)) != 0)\
    syn_flag = PyObject_IsTrue(value);\
Py_XDECREF(value);

#define OPT_STRING(syn_name, config_name) \
if ((value = PyObject_GetAttrString(config, config_name)) != 0)\
{ if (!IsType(value, String)) throw "Error: " config_name " must be a string.";\
  syn_name = PyString_AsString(value);\
}\
Py_XDECREF(value);

  // Check config object first
  if (config)
  {
    PyObject* value;
    OPT_FLAG(verbose, "verbose");
    OPT_FLAG(syn_main_only, "main_file");
    // Grab the include paths
    if ((value = PyObject_GetAttrString(config, "include_path")) != 0)
    {
      if (!IsType(value, List))
      {
	std::cerr << "Error: include_path must be a list of strings." << std::endl;
	exit(1);
      }
      // Loop through the include paths
      for (int i=0, end=PyList_Size(value); i < end; i++)
      {
	PyObject* item = PyList_GetItem(value, i);
	if (!item || !IsType(item, String))
	{
	  std::cerr << "Error: include_path must be a list of strings." << std::endl;
	  exit(1);
	}
	// mem leak.. how to fix?
	char* buf = new char[PyString_Size(item)+3];
	strcpy(buf, "-I");
	strcat(buf, PyString_AsString(item));
	cppflags.push_back(buf);
      } // for
    }
    Py_XDECREF(value);
    // Grab the list of defines
    if ((value = PyObject_GetAttrString(config, "defines")) != 0)
    {
      if (!IsType(value, List))
      {
	std::cerr << "Error: defines must be a list of strings." << std::endl;
	exit(1);
      }
      // Loop through the include paths
      for (int i=0, end=PyList_Size(value); i < end; i++)
      {
	PyObject* item = PyList_GetItem(value, i);
	if (!item || !IsType(item, String))
	{
	  std::cerr << "Error: defines must be a list of strings." << std::endl;
	  exit(1);
	}
	// mem leak.. how to fix?
	char* buf = new char[PyString_Size(item)+3];
	strcpy(buf, "-D");
	strcat(buf, PyString_AsString(item));
	cppflags.push_back(buf);
      } // for
    }
    Py_XDECREF(value);

    // Basename is the prefix to strip from filenames
    OPT_STRING(syn_basename, "basename");
    // Extract tails tells the parser to find tail comments
    OPT_FLAG(syn_extract_tails, "extract_tails");
    // 'syntax_prefix' defines the prefix for the filename to write syntax hilite info to
    OPT_STRING(syn_syntax_prefix, "syntax_prefix");
    // 'preprocessor' defines the preprocessor string
    char *temp_string = 0;
    OPT_STRING(temp_string, "preprocessor");
    syn_cpp = temp_string ? temp_string : "cpp";
    // If multiple_files is set then the parser handles multiple files
    // included from the main one at the same time (they get into the AST
    OPT_FLAG(syn_multi_files, "multiple_files");
  } // if config
#undef OPT_STRING
#undef OPT_FLAG
#undef IsType

  // Now check command line args
  size_t argsize = PyList_Size(args);
  for (size_t i = 0; i != argsize; ++i)
  {
    const char *argument = PyString_AsString(PyList_GetItem(args, i));
    if (strncmp(argument, "-I", 2) == 0)
    {
      cppflags.push_back(argument);
      if (strlen(argument) == 2)
	cppflags.push_back(PyString_AsString(PyList_GetItem(args, ++i)));
    }
    else if (strncmp(argument, "-D", 2) == 0)
    {
      cppflags.push_back(argument);
      if (strlen(argument) == 2)
	cppflags.push_back(PyString_AsString(PyList_GetItem(args, ++i)));
    }
    else if (strcmp(argument, "-v") == 0)
      verbose = true;
    else if (strcmp(argument, "-m") == 0)
      syn_main_only = true;
    else if (strcmp(argument, "-b") == 0)
      syn_basename = PyString_AsString(PyList_GetItem(args, ++i));
    else if (strcmp(argument, "-t") == 0)
      syn_extract_tails = true;
  }

  // If multi_files is set, we check the extra_files argument to see if it
  // has a list of filenames like it should do
  if (extra_files && PyList_Check(extra_files))
  {
#if 0
    size_t extra_size = PyList_Size(extra_files);
    if (extra_size > 0)
    {
      PyObject* item;
      const char* string;
      syn_extra_filenames = new std::vector<const char*>;
      for (size_t i = 0; i < extra_size; i++)
      {
	item = PyList_GetItem(extra_files, i);
	string = PyString_AsString(item);
 	syn_extra_filenames->push_back(string);
      }
    }
#endif
  }
}

char *RunPreprocessor(const char *file, const std::vector<const char *> &flags)
{
  static char dest[1024];
  strcpy(dest, "/tmp/synopsis-XXXXXX");
  int temp_fd = mkstemp(dest);
  if (temp_fd == -1)
  {
    perror("RunPreprocessor");
    exit(1);
  }
  // Not interested in the open file, just the unique filename
  close(temp_fd);

  // Release Python's global interpreter lock
  pythread_save = PyEval_SaveThread();
       
  switch(fork())
  {
    case 0:
    {
      std::vector<const char *> args;
      char *cc = getenv("CC");
      if (cc)
      {
	// separate command and arguments
	do
	{
	  args.push_back(cc);
	  cc = strchr(cc, ' ');                  // find next whitespace...
	  while (cc && *cc == ' ') *cc++ = '\0'; // ...and skip to next non-ws
	}
	while (cc && *cc != '\0');
      }
      else
      {
	args.push_back("cpp");
      }
      args.insert(args.end(), flags.begin(), flags.end());
      args.push_back("-C"); // keep comments
      args.push_back("-E"); // stop after preprocessing
      args.push_back("-o"); // output to...
      args.push_back(dest);
      //args.push_back("-x"); // language c++
      //args.push_back("c++");
      args.push_back(file);
      if (verbose)
      {
	std::cout << "calling external preprocessor\n" << args[0];
	for (std::vector<const char *>::iterator i = args.begin(); i != args.end(); ++i)
	  std::cout << ' ' << *i;
	std::cout << std::endl;
      }
      args.push_back(0);
      execvp(args[0], (char **)&*args.begin());
      perror("cannot invoke compiler");
      exit(-1);
      break;
    }
    case -1:
      perror("RunPreprocessor");
      exit(-1);
      break;
    default:
    {
      int status;
      wait(&status);
      if (status != 0)
      {
	if (WIFEXITED(status))
	  std::cout << "exited with status " << WEXITSTATUS(status) << std::endl;
	else if (WIFSIGNALED(status))
	  std::cout << "stopped with status " << WTERMSIG(status) << std::endl;
	exit(1);
      }
    }
  } // switch
  return dest;
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
//   SWalker *instance = SWalker::instance();
#if 0
  std::cerr << signame << " caught while processing " << instance->current_file()->filename()
	    << " at line " << instance->current_lineno()
	    << std::endl;
#endif
  std::cerr << signame << " caught" << std::endl;
  exit(-1);
}

void RunCTool(const char *src, const char *file,
	      const std::vector<const char *> &args,
	      PyObject *ast, PyObject *types, PyObject *declarations,
	      PyObject* files)
{
  Trace trace("RunCTool");
  std::set_unexpected(unexpected);
  struct sigaction olda;
  struct sigaction newa;
  newa.sa_handler = &sighandler;
  sigaction(SIGSEGV, &newa, &olda);
  sigaction(SIGBUS, &newa, &olda);
  sigaction(SIGABRT, &newa, &olda);
  
  Project  *prj = new Project();
//   TransUnit *unit = prj->parse(file_list[i], use_cpp, cpp_dir,
// 			       keep_cpp_file, cpp_file, cpp_cmmd, cd_cmmd);

  sigaction(SIGABRT, &olda, 0);
  sigaction(SIGBUS, &olda, 0);
  sigaction(SIGSEGV, &olda, 0);
}

void do_parse(const char *src, 
	      const std::vector<const char *>& cppargs, 
	      const std::vector<const char *>& ctargs, 
	      PyObject *ast, PyObject *types, PyObject *declarations,
	      PyObject* files)
{
  Trace trace("do_parse");
  // Run the preprocessor
  char *cppfile = RunPreprocessor(src, cppargs);

  // Run CTool to generate the AST
  RunCTool(src, cppfile, ctargs, ast, types, declarations, files);
  unlink(cppfile);
}

PyObject *ctoolParse(PyObject *self, PyObject *args)
{
  Trace trace("ctoolParse");

  char *src;
  PyObject *extra_files, *parserargs, *types, *declarations, *config, *ast;
  if (!PyArg_ParseTuple(args, "sOO!O", &src, &extra_files, &PyList_Type, &parserargs, &config))
    return 0;
  std::vector<const char *> cppargs;
  std::vector<const char *> ctargs;
  getopts(parserargs, cppargs, ctargs, config, extra_files);
  if (!src || *src == '\0')
  {
    std::cerr << "No source file" << std::endl;
    exit(-1);
  }
  // Make AST object
#define assertObject(pyo) if (!pyo) PyErr_Print(); assert(pyo)
  PyObject* ast_module = PyImport_ImportModule("Synopsis.Core.AST");
  assertObject(ast_module);
  ast = PyObject_CallMethod(ast_module, "AST", "");
  assertObject(ast);
  PyObject* files = PyObject_CallMethod(ast, "files", "");
  assertObject(files);
  declarations = PyObject_CallMethod(ast, "declarations", "");
  assertObject(declarations);
  types = PyObject_CallMethod(ast, "types", "");
  assertObject(types);
#undef assertObject

  do_parse(src, cppargs, ctargs, ast, types, declarations, files);

  Py_DECREF(ast_module);
  Py_DECREF(declarations);
  Py_DECREF(files);
  Py_DECREF(types);

  return ast;
}

PyObject *ctoolUsage(PyObject *self, PyObject *)
{
  Trace trace("ctoolUsage");
  std::cout
    << "  -I<path>                             Specify include path to be used by the preprocessor\n"
    << "  -D<macro>                            Specify macro to be used by the preprocessor\n"
    << "  -m                                   Unly keep declarations from the main file\n"
    << "  -b basepath                          Strip basepath from start of filenames" << std::endl;
  Py_INCREF(Py_None);
  return Py_None;
}

PyMethodDef ctool_methods[] =
  {
    {(char*)"parse", ctoolParse, METH_VARARGS},
    {(char*)"usage", ctoolUsage, METH_VARARGS},
    {0, 0}
  };
};

extern "C" void initctool()
{
  PyObject *m = Py_InitModule((char*)"ctool", ctool_methods);
  PyObject_SetAttrString(m, (char*)"version", PyString_FromString("0.1"));
}
