#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "synopsis.hh"
#include "walker.h"
#include "token.h"
#include "buffer.h"
#include "parse.h"
#include "ptree-core.h"
#include "ptree.h"
#include "encoding.h"

// Stupid OCC and stupid macros!
#undef Scope

#include "swalker.hh"
#include "builder.hh"
#include "dumper.hh"

/* The following aren't used anywhere. Though it has to be defined and initialized to some dummy default
 * values since it is required by the opencxx.a module, which I don't want to modify...
 */
bool showProgram, doCompile, makeExecutable, doPreprocess, doTranslate;
bool verboseMode, regularCpp, makeSharedLibrary, preprocessTwice;
char* sharedLibraryName;

/* These also are just dummies needed by the opencxx module */
void RunSoCompiler(const char *) {}
void *LoadSoLib(char *) { return 0;}
void *LookupSymbol(void *, char *) { return 0;}

// If true then everything but what's in the main file will be stripped
bool syn_main_only;


#ifdef DEBUG
// For use in gdb, since decl->Display() doesn't work too well..
void show(Ptree* p)
{
    p->Display();
}
#endif

namespace
{

void getopts(PyObject *args, vector<const char *> &cppflags, vector<const char *> &occflags)
{
  showProgram = doCompile = verboseMode = makeExecutable = false;
  doTranslate = regularCpp = makeSharedLibrary = preprocessTwice = false;
  doPreprocess = true;
  sharedLibraryName = 0;
  syn_main_only = false;
  
  size_t argsize = PyList_Size(args);
  for (size_t i = 0; i != argsize; ++i)
    {
      const char *argument = PyString_AsString(PyList_GetItem(args, i));
      if (strncmp(argument, "-I", 2) == 0) cppflags.push_back(argument);
      else if (strncmp(argument, "-D", 2) == 0) cppflags.push_back(argument);
      else if (strcmp(argument, "-m") == 0) syn_main_only = true;
    }
}
  
char *RunPreprocessor(const char *file, const vector<const char *> &flags)
{
  static char dest[1024] = "/tmp/synopsis-XXXXXX";
  //tmpnam(dest);
  if (mkstemp(dest) == -1) {
    perror("RunPreprocessor");
    exit(1);
  }
  switch(fork())
    {
    case 0:
      {
	vector<const char *> args = flags;
	char *cc = getenv("CC");
	args.insert(args.begin(), cc ? cc : "c++");
	args.push_back("-C"); // keep comments
	args.push_back("-E"); // stop after preprocessing
	args.push_back("-o"); // output to...
	args.push_back(dest);
	args.push_back("-x"); // language c++
	args.push_back("c++");
	args.push_back(file);
	args.push_back(0);
	execvp(args[0], (char **)args.begin());
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
	if(status != 0)
	  {
	    if (WIFEXITED(status))
	      cout << "exited with status " << WEXITSTATUS(status) << endl;
	    else if (WIFSIGNALED(status))
	      cout << "stopped with status " << WTERMSIG(status) << endl;
	    exit(1);
	  }
      }
    }
  return dest;
}

char *RunOpencxx(const char *src, const char *file, const vector<const char *> &args, PyObject *types, PyObject *declarations)
{
  Trace trace("RunOpencxx");
  static char dest[1024] = "/tmp/synopsis-XXXXXX";
  //tmpnam(dest);
  if (mkstemp(dest) == -1) {
    perror("RunPreprocessor");
    exit(1);
  }
  
  ifstream ifs(file);
  if(!ifs)
    {
      perror(file);
      exit(1);
    }
  ProgramFile prog(ifs);
  Lex lex(&prog);
  Parser parse(&lex);
  
  Builder builder;
  SWalker swalker(&parse, &builder);
  Ptree *def;
  while(parse.rProgram(def))
    swalker.Translate(def);
#ifdef DEBUG
  // Test Synopsis
  //Synopsis synopsis(src, declarations, types);
  //if (syn_main_only) synopsis.onlyTranslateMain();
  //synopsis.translate(builder.scope());
  
  // Test Dumper
  Dumper dumper;
  if (syn_main_only) dumper.onlyShow(src);
  dumper.visitScope(builder.scope());
#else
  Synopsis synopsis(src, declarations, types);
  if (syn_main_only) synopsis.onlyTranslateMain();
  synopsis.translate(builder.scope());
#endif
  
  if(parse.NumOfErrors() != 0)
    {
      cerr << "errors while parsing file " << file << endl;
      exit(1);
    }
  return dest;
}

PyObject *occParse(PyObject *self, PyObject *args)
{
  Trace trace("occParse");
#if 0
  Ptree::show_encoded = true;
#endif
  char *src;
  PyObject *parserargs, *types, *declarations;
  if (!PyArg_ParseTuple(args, "sO!OO!", &src, &PyList_Type, &parserargs, &types, &PyList_Type, &declarations)) return 0;
  vector<const char *> cppargs;
  vector<const char *> occargs;
  getopts(parserargs, cppargs, occargs);
  if (!src || *src == '\0')
    {
      cerr << "No source file" << endl;
      exit(-1);
    }
  char *cppfile = RunPreprocessor(src, cppargs);
  char *occfile = RunOpencxx(src, cppfile, occargs, types, declarations);
  unlink(cppfile);
  unlink(occfile);
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject *occUsage(PyObject *self, PyObject *)
{
  Trace trace("occParse");
  cout << "
  -I<path>                             Specify include path to be used by the preprocessor
  -D<macro>                            Specify macro to be used by the preprocessor
  -m                                   Unly keep declarations from the main file" << endl;                           
  Py_INCREF(Py_None);
  return Py_None;
}

PyMethodDef occ_methods[] =
{
  {(char*)"parse",            occParse,               METH_VARARGS},
  {(char*)"usage",            occUsage,               METH_VARARGS},
  {NULL, NULL}
};
};

extern "C" void initocc()
{
  PyObject* m = Py_InitModule((char*)"occ", occ_methods);
  PyObject_SetAttrString(m, (char*)"version", PyString_FromString("0.1"));
}

#ifdef DEBUG

int main(int argc, char **argv)
{
  if (argc != 2)
    {
      cout << "Usage: " << argv[0] << " <filename>" << endl;
      exit(-1);
    }
  char *src = argv[1];
  vector<const char *> cppargs;
  vector<const char *> occargs;
  //   getopts(argc, argv, cppargs, occargs);
  Py_Initialize();
  PyObject *pylist = PyList_New(argc - 1);
  for (int i = 1; i != argc; ++i)
    PyList_SetItem(pylist, i - 1, PyString_FromString(argv[i]));
  getopts(pylist, cppargs, occargs);
  if (!src || *src == '\0')
    {
      cerr << "No source file" << endl;
      exit(-1);
    }
  char *cppfile = RunPreprocessor(src, cppargs);
  PyObject* type = PyImport_ImportModule("Synopsis.Core.Type");
  PyObject* types = PyObject_CallMethod(type, "Dictionary", 0);
  char *occfile = RunOpencxx(src, cppfile, occargs, types, PyList_New(0));
  unlink(cppfile);
  unlink(occfile);
}

#endif
