//
// Copyright (C) 2003 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/AST/ASTKit.hh>
#include <ctool/ctool.h>
#include "Translator.hh"
#include "Trace.hh"
#include <signal.h>
#include <sys/wait.h>
#include <fstream>

using namespace Synopsis::AST; // import all AST objects...
namespace Python = Synopsis; // ...and the others into 'Python'

namespace
{

int verbose = 0;
int debug = 0;
const char *prefix = 0;
ASTKit *kit;
AST *ast;

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
    PyObject *py_ast;
    if (!PyArg_ParseTuple(args, "Oszii",
                          &py_ast,
                          &input,
                          &prefix,
                          &verbose,
                          &debug))
      return 0;

    Py_INCREF(py_ast);

    // since everything in this file is accessed only during the execution
    // of ucpp_parse, we can safely manage these objects in this scope yet
    // reference them globally (for convenience)
    std::auto_ptr<AST> ast_ptr(new AST(py_ast));
    ast = ast_ptr.get();

    std::auto_ptr<ASTKit> kit_ptr(new ASTKit());
    kit = kit_ptr.get();

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

    return ast->ref();
  }
  catch (const std::exception &e)
  {
    std::cerr << "Internal error : " << e.what() << std::endl;
    return 0;
  }
}

PyMethodDef ctool_methods[] = {{(char*)"parse", ctool_parse, METH_VARARGS},
			       {0, 0}};
};

extern "C" void initctool()
{
  PyObject *m = Py_InitModule((char*)"ctool", ctool_methods);
  PyObject_SetAttrString(m, (char*)"version", PyString_FromString("0.1"));
}
