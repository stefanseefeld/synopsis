// vim: set ts=8 sts=2 sw=2 et:
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <occ/walker.h>
#include <occ/token.h>
#include <occ/buffer.h>
#include <occ/parse.h>
#include <occ/ptree-core.h>
#include <occ/ptree.h>
#include <occ/encoding.h>

// Stupid OCC and stupid macros!
#undef Scope

#include "synopsis.hh"
#include "swalker.hh"
#include "builder.hh"
#include "dumper.hh"


// ucpp_main is the renamed main() func of ucpp, since it is included in this
// module
extern "C" int ucpp_main(int argc, char** argv);

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


bool verbose;

// If true then everything but what's in the main file will be stripped
bool syn_main_only, syn_extract_tails, syn_use_gcc, syn_fake_std;

// If set then this is stripped from the start of all filenames
const char* syn_basename = "";

// If set then this is the filename to store links to
const char* syn_file_syntax = 0;

// If set then this is the filename to store xref to
const char* syn_file_xref = 0;


#ifdef DEBUG
// For use in gdb, since decl->Display() doesn't work too well..
void show(Ptree* p)
{
    p->Display();
}
#endif

//. Override unexpected() to print a message before we abort
void unexpected()
{
    std::cout << "Warning: Aborting due to unexpected exception." << std::endl;
    throw std::bad_exception();
}

namespace
{

void getopts(PyObject *args, std::vector<const char *> &cppflags, std::vector<const char *> &occflags, PyObject* config)
{
    showProgram = doCompile = verboseMode = makeExecutable = false;
    doTranslate = regularCpp = makeSharedLibrary = preprocessTwice = false;
    doPreprocess = true;
    sharedLibraryName = 0;
    verbose = false;
    syn_main_only = false;
    syn_extract_tails = false;
    syn_use_gcc = false;
    syn_fake_std = false;

#define IsType(obj, type) (!PyObject_Compare(PyObject_Type(obj), (PyObject*)&Py##type##_Type))
    
    // Check config object first
    if (config) {
	PyObject* value;
	if ((value = PyObject_GetAttrString(config, "main_file")) != 0)
	  syn_main_only = PyObject_IsTrue(value);
	if ((value = PyObject_GetAttrString(config, "verbose")) != 0) verbose = true;
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
	if ((value = PyObject_GetAttrString(config, "basename")) != 0)
	  {
	    if (!IsType(value, String))
	      {
		std::cerr << "Error: basename must be a string." << std::endl;
		exit(1);
	      }
	    syn_basename = PyString_AsString(value);
	  }
	if ((value = PyObject_GetAttrString(config, "extract_tails")) != 0)
	  syn_extract_tails = PyObject_IsTrue(value);
	// 'storage' defines the filename to write syntax hilite info to (OBSOLETE)
	if ((value = PyObject_GetAttrString(config, "storage")) != 0)
	  {
	    if (!IsType(value, String))
	      {
		std::cerr << "Error: storage must be a string." << std::endl;
		exit(1);
	      }
	    syn_file_syntax = PyString_AsString(value);
	  }
	// 'syntax_file' defines the filename to write syntax hilite info to
	if ((value = PyObject_GetAttrString(config, "syntax_file")) != 0)
	  {
	    if (!IsType(value, String))
	      {
		std::cerr << "Error: syntax_file must be a string." << std::endl;
		exit(1);
	      }
	    syn_file_syntax = PyString_AsString(value);
	  }
	// 'xref_file' defines the filename to write syntax hilite info to
	if ((value = PyObject_GetAttrString(config, "xref_file")) != 0)
	  {
	    if (!IsType(value, String))
	      {
		std::cerr << "Error: xref_file must be a string." << std::endl;
		exit(1);
	      }
	    syn_file_xref = PyString_AsString(value);
	  }
	if ((value = PyObject_GetAttrString(config, "preprocessor")) != 0)
	  {
	    if (PyObject_Compare(PyObject_Type(value), (PyObject*)&PyString_Type))
	      {
		std::cerr << "Error: preprocessor must be a string." << std::endl;
		exit(1);
	      }
	    // This will be a generic preprocessor at some point
	    syn_use_gcc = !strcmp("gcc", PyString_AsString(value));
	  }
	if ((value = PyObject_GetAttrString(config, "fake_std")) != 0)
	  syn_fake_std = PyObject_IsTrue(value);
    } // if config
#undef IsType
    
    // Now check command line args
    size_t argsize = PyList_Size(args);
    for (size_t i = 0; i != argsize; ++i)
      {
	const char *argument = PyString_AsString(PyList_GetItem(args, i));
	if (strncmp(argument, "-I", 2) == 0) cppflags.push_back(argument);
	else if (strncmp(argument, "-D", 2) == 0) cppflags.push_back(argument);
	else if (strcmp(argument, "-v") == 0) verbose = true;
	else if (strcmp(argument, "-m") == 0) syn_main_only = true;
	else if (strcmp(argument, "-b") == 0)
	  syn_basename = PyString_AsString(PyList_GetItem(args, ++i));
	else if (strcmp(argument, "-t") == 0) syn_extract_tails = true;
	else if (strcmp(argument, "-s") == 0)
	  syn_file_syntax = PyString_AsString(PyList_GetItem(args, ++i));
	else if (strcmp(argument, "-x") == 0)
	  syn_file_xref = PyString_AsString(PyList_GetItem(args, ++i));
	else if (strcmp(argument, "-g") == 0) syn_use_gcc = true;
	else if (strcmp(argument, "-f") == 0) syn_fake_std = true;
      }
}
  
char *RunPreprocessor(const char *file, const std::vector<const char *> &flags)
{
    static char dest[1024];
    strcpy(dest, "/tmp/synopsis-XXXXXX");
    if (mkstemp(dest) == -1)
      {
	perror("RunPreprocessor");
	exit(1);
      }
    if (syn_use_gcc)
      {
	switch(fork())
	  {
	  case 0:
	    {
	      std::vector<const char *> args = flags;
	      char *cc = getenv("CC");
	      args.insert(args.begin(), cc ? cc : "c++");
	      args.push_back("-C"); // keep comments
	      args.push_back("-E"); // stop after preprocessing
	      args.push_back("-o"); // output to...
	      args.push_back(dest);
	      args.push_back("-x"); // language c++
	      args.push_back("c++");
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
      }
    else
      { // else use ucpp
	// Create argv vector
	std::vector<const char *> args = flags;
	char *cc = getenv("CC");
	args.insert(args.begin(), cc ? cc : "c++");
	args.push_back("-C"); // keep comments
	args.push_back("-lg"); // gcc-like line numbers
	args.push_back("-Y"); // define system macros in tune.h
	//args.push_back("-E"); // stop after preprocessing
	// Add includes
	args.push_back("-I");
	args.push_back("/usr/local/include/g++-3/");
	//args.push_back("-I");
	//args.push_back("/usr/include/linux/");
	args.push_back("-I");
	args.push_back("/usr/local/lib/gcc-lib/i686-pc-linux-gnu/2.95.2/include/");
	args.push_back("-o"); // output to...
	args.push_back(dest);
	//args.push_back("-x"); // language c++
	//args.push_back("c++");
	args.push_back(file);
	if (verbose)
	  {
	    std::cout << "calling ucpp\n";
	    for (std::vector<const char *>::iterator i = args.begin(); i != args.end(); ++i)
	      std::cout << ' ' << *i;
	    std::cout << std::endl;
	  }
	
	// Call ucpp
	int status = ucpp_main(args.size(), (char **)&*args.begin());
	if (status != 0)
	  {
	    std::cerr << "ucpp returned error flag." << std::endl;
	    exit(1);
	  }
      }
    return dest;
}

void sighandler(int signo)
{
  std::string signame;
  switch (signo)
    {
    case SIGABRT: signame = "Abort"; break;
    case SIGBUS:  signame = "Bus error"; break;
    case SIGSEGV: signame = "Segmentation Violation"; break;
    default: signame = "unknown"; break;
    };
  SWalker *instance = SWalker::instance();
  std::cerr << signame << "caught while processing " << instance->current_file()
	    << " at line " << instance->current_lineno()
	    << " (main file is '" << instance->main_file() << "')" << std::endl;
  exit(-1);
}

char *RunOpencxx(const char *src, const char *file, const std::vector<const char *> &args, PyObject *types, PyObject *declarations, PyObject* filenames)
{
  Trace trace("RunOpencxx");
  std::set_unexpected(unexpected);
  struct sigaction olda;
  struct sigaction newa;
  newa.sa_handler = &sighandler;
  sigaction(SIGSEGV, &newa, &olda);
  sigaction(SIGBUS, &newa, &olda);
  sigaction(SIGABRT, &newa, &olda);
  static char dest[1024];
  strcpy(dest, "/tmp/synopsis-XXXXXX");
  //tmpnam(dest);
  if (mkstemp(dest) == -1) {
    perror("RunOpencxx");
    exit(1);
  }
  
  std::ifstream ifs(file);
  if(!ifs)
    {
      perror(file);
      exit(1);
    }
  ProgramFile prog(ifs);
  Lex lex(&prog);
  Parser parse(&lex);
    // Calculate source filename
  std::string source(src);
  if (source.substr(0, strlen(syn_basename)) == syn_basename)
    source.erase(0, strlen(syn_basename));
  if (filenames) 
    {
      PyObject_CallMethod(filenames, "append", "s", source.c_str());
    }

  Builder builder(syn_basename);
  SWalker swalker(src, &parse, &builder, &prog);
  swalker.set_extract_tails(syn_extract_tails);
  Ptree *def;
  if (syn_fake_std)
    {
      // Fake a using from "std" to global
      builder.start_namespace("std", NamespaceNamed);
      builder.add_using_namespace(builder.global()->declared());
      builder.end_namespace();
    }
#ifdef DEBUG
  swalker.set_extract_tails(true);
  swalker.set_store_links(true, &std::cout, NULL);
  while(parse.rProgram(def))
    swalker.Translate(def);
  // // Test Synopsis
  // Synopsis synopsis(src, declarations, types);
  // if (syn_main_only) synopsis.onlyTranslateMain();
  // synopsis.translate(builder.scope());
  
  // Test Dumper
  Dumper dumper;
  if (syn_main_only) dumper.onlyShow(src);
  dumper.visit_scope(builder.scope());
#else
    std::ofstream* of_syntax = 0;
    std::ofstream* of_xref = 0;
    if (syn_file_syntax)
      of_syntax = new std::ofstream(syn_file_syntax);
    if (syn_file_xref)
      of_xref = new std::ofstream(syn_file_xref);
    if (of_syntax || of_xref)
      swalker.set_store_links(true, of_syntax, of_xref);
    try
      {
	while(parse.rProgram(def))
	  swalker.Translate(def);
      }
    catch (...)
      {
	std::cerr << "Warning: an uncaught exception occurred when translating the parse tree" << std::endl;
      }
    // Setup synopsis c++ to py convertor
    Synopsis synopsis(source, declarations, types);
    if (syn_main_only) synopsis.onlyTranslateMain();
    synopsis.set_builtin_decls(builder.builtin_decls());
    // Convert!
    synopsis.translate(builder.scope());
    if (of_syntax) delete of_syntax;
    if (of_xref) delete of_xref;
#endif
  
  if(parse.NumOfErrors() != 0)
    {
      std::cerr << "errors while parsing file " << file << std::endl;
      exit(1);
    }
  sigaction(SIGABRT, &olda, 0);
  sigaction(SIGBUS, &olda, 0);
  sigaction(SIGSEGV, &olda, 0);
  return dest;
}

PyObject *occParse(PyObject *self, PyObject *args)
{
  Trace trace("occParse");
#if 0
  Ptree::show_encoded = true;
#endif
  char *src;
  PyObject *parserargs, *types, *declarations, *config, *ast;
  if (!PyArg_ParseTuple(args, "sO!O", &src, &PyList_Type, &parserargs, &config)) return 0;
  std::vector<const char *> cppargs;
  std::vector<const char *> occargs;
  getopts(parserargs, cppargs, occargs, config);
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
  PyObject* filenames = PyObject_CallMethod(ast, "filenames", "");
  assertObject(filenames);
  declarations = PyObject_CallMethod(ast, "declarations", "");
  assertObject(declarations);
  types = PyObject_CallMethod(ast, "types", "");
  assertObject(types);
#undef assertObject

  char *cppfile = RunPreprocessor(src, cppargs);
  char *occfile = RunOpencxx(src, cppfile, occargs, types, declarations, filenames);
  unlink(cppfile);
  unlink(occfile);
  return ast;
}

PyObject *occUsage(PyObject *self, PyObject *)
{
  Trace trace("occParse");
  std::cout
    << "  -I<path>                             Specify include path to be used by the preprocessor\n"
    << "  -D<macro>                            Specify macro to be used by the preprocessor\n"
    << "  -m                                   Unly keep declarations from the main file\n"
    << "  -b basepath                          Strip basepath from start of filenames" << std::endl;
  Py_INCREF(Py_None);
  return Py_None;
}

PyMethodDef occ_methods[] =
{
  {(char*)"parse",            occParse,               METH_VARARGS},
  {(char*)"usage",            occUsage,               METH_VARARGS},
  {0, 0}
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
    char *src = 0;
    std::vector<const char *> cppargs;
    std::vector<const char *> occargs;
    //   getopts(argc, argv, cppargs, occargs);
    Py_Initialize();
    int i, py_i;
    bool all_args = false;
    for (i = 1, py_i = 0; i != argc; ++i)
	if (!all_args && argv[i][0] != '-') src = argv[i];
	else if (!strcmp(argv[i], "--")) all_args = true;
        else ++py_i;
    PyObject *pylist = PyList_New(py_i);
    all_args = false;
    for (i = 1, py_i = 0; i != argc; ++i)
	if (!all_args && argv[i][0] != '-') continue;
	else if (!strcmp(argv[i], "--")) all_args = true;
        else PyList_SetItem(pylist, py_i++, PyString_FromString(argv[i]));
    getopts(pylist, cppargs, occargs, NULL);
    if (!src || *src == '\0') {
	std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
	exit(-1);
    }
    char *cppfile = RunPreprocessor(src, cppargs);
    PyObject* type = PyImport_ImportModule("Synopsis.Core.Type");
    PyObject* types = PyObject_CallMethod(type, "Dictionary", 0);
    char *occfile = RunOpencxx(src, cppfile, occargs, types, PyList_New(0), NULL);
    unlink(cppfile);
    unlink(occfile);
}

#endif
