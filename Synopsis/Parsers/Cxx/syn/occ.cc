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

// If true then everything but what's in the main file will be stripped
bool syn_main_only, syn_extract_tails, syn_use_gcc;

// If set then this is stripped from the start of all filenames
const char* syn_basename = "";

// If set then this is the filename to store links to
const char* syn_storage = 0;


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
    cout << "Warning: Aborting due to unexpected exception." << endl;
    throw std::bad_exception();
}

namespace
{

void getopts(PyObject *args, std::vector<const char *> &cppflags, vector<const char *> &occflags, PyObject* config)
{
    showProgram = doCompile = verboseMode = makeExecutable = false;
    doTranslate = regularCpp = makeSharedLibrary = preprocessTwice = false;
    doPreprocess = true;
    sharedLibraryName = 0;
    syn_main_only = false;
    syn_extract_tails = false;
    syn_use_gcc = false;

#define IsType(obj, type) (!PyObject_Compare(PyObject_Type(obj), (PyObject*)&Py##type##_Type))
    
    // Check config object first
    PyObject* value;
    if ((value = PyObject_GetAttrString(config, "main_file")) != NULL) {
	syn_main_only = PyObject_IsTrue(value);
    }
    if ((value = PyObject_GetAttrString(config, "verbose")) != NULL) {
	// ignore
    }
    if ((value = PyObject_GetAttrString(config, "include_path")) != NULL) {
	if (!IsType(value, List)) {
	    cerr << "Error: include_path must be a list of strings." << endl;
	    exit(1);
	}
	// Loop through the include paths
	for (int i=0, end=PyList_Size(value); i < end; i++) {
	    PyObject* item = PyList_GetItem(value, i);
	    if (!item || !IsType(item, String)) {
		cerr << "Error: include_path must be a list of strings." << endl;
		exit(1);
	    }
	    // mem leak.. how to fix?
	    char* buf = new char[PyString_Size(item)+3];
	    strcpy(buf, "-I");
	    strcat(buf, PyString_AsString(item));
	    cppflags.push_back(buf);
	} // for
    }
    if ((value = PyObject_GetAttrString(config, "defines")) != NULL) {
	if (!IsType(value, List)) {
	    cerr << "Error: defines must be a list of strings." << endl;
	    exit(1);
	}
	// Loop through the include paths
	for (int i=0, end=PyList_Size(value); i < end; i++) {
	    PyObject* item = PyList_GetItem(value, i);
	    if (!item || !IsType(item, String)) {
		cerr << "Error: defines must be a list of strings." << endl;
		exit(1);
	    }
	    // mem leak.. how to fix?
	    char* buf = new char[PyString_Size(item)+3];
	    strcpy(buf, "-D");
	    strcat(buf, PyString_AsString(item));
	    cppflags.push_back(buf);
	} // for
    }
    if ((value = PyObject_GetAttrString(config, "basename")) != NULL) {
	if (!IsType(value, String)) {
	    cerr << "Error: basename must be a string." << endl;
	    exit(1);
	}
	syn_basename = PyString_AsString(value);
    }
    if ((value = PyObject_GetAttrString(config, "extract_tails")) != NULL) {
	syn_extract_tails = PyObject_IsTrue(value);
    }
    if ((value = PyObject_GetAttrString(config, "storage")) != NULL) {
	if (!IsType(value, String)) {
	    cerr << "Error: storage must be a string." << endl;
	    exit(1);
	}
	syn_storage = PyString_AsString(value);
    }
    if ((value = PyObject_GetAttrString(config, "preprocessor")) != NULL) {
	if (PyObject_Compare(PyObject_Type(value), (PyObject*)&PyString_Type)) {
	    cerr << "Error: preprocessor must be a string." << endl;
	    exit(1);
	}
	// This will be a generic preprocessor at some point
	syn_use_gcc = !strcmp("gcc", PyString_AsString(value));
    }
#undef IsType
    
    // Now check command line args
    size_t argsize = PyList_Size(args);
    for (size_t i = 0; i != argsize; ++i) {
	const char *argument = PyString_AsString(PyList_GetItem(args, i));
	if (strncmp(argument, "-I", 2) == 0) cppflags.push_back(argument);
	else if (strncmp(argument, "-D", 2) == 0) cppflags.push_back(argument);
	else if (strcmp(argument, "-m") == 0) syn_main_only = true;
	else if (strcmp(argument, "-b") == 0)
	    syn_basename = PyString_AsString(PyList_GetItem(args, ++i));
	else if (strcmp(argument, "-t") == 0) syn_extract_tails = true;
	else if (strcmp(argument, "-s") == 0)
	    syn_storage = PyString_AsString(PyList_GetItem(args, ++i));
	else if (strcmp(argument, "-g") == 0) syn_use_gcc = true;
    }
}
  
char *RunPreprocessor(const char *file, const vector<const char *> &flags)
{
    static char dest[1024] = "/tmp/synopsis-XXXXXX";
    if (mkstemp(dest) == -1) {
	perror("RunPreprocessor");
	exit(1);
    }
    if (syn_use_gcc) {
	switch(fork()) {
	    case 0: {
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
	    default: {
		int status;
		wait(&status);
		if (status != 0) {
		    if (WIFEXITED(status))
			cout << "exited with status " << WEXITSTATUS(status) << endl;
		    else if (WIFSIGNALED(status))
			cout << "stopped with status " << WTERMSIG(status) << endl;
		    exit(1);
		}
	    }
	} // switch
    } else { // else use ucpp
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

char *RunOpencxx(const char *src, const char *file, const std::vector<const char *> &args, PyObject *types, PyObject *declarations)
{
  Trace trace("RunOpencxx");
  std::set_unexpected(unexpected);
  static char dest[1024] = "/tmp/synopsis-XXXXXX";
  //tmpnam(dest);
  if (mkstemp(dest) == -1) {
    perror("RunPreprocessor");
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

  Builder builder(syn_basename);
  SWalker swalker(src, &parse, &builder, &prog);
  swalker.setExtractTails(syn_extract_tails);
  Ptree *def;
#ifdef DEBUG
  swalker.setExtractTails(true);
  swalker.setStoreLinks(true, &cout);
  while(parse.rProgram(def))
    swalker.Translate(def);
  // // Test Synopsis
  // Synopsis synopsis(src, declarations, types);
  // if (syn_main_only) synopsis.onlyTranslateMain();
  // synopsis.translate(builder.scope());
  
  // Test Dumper
  Dumper dumper;
  if (syn_main_only) dumper.onlyShow(src);
  dumper.visitScope(builder.scope());
#else
    std::ofstream* of = 0;
    if (syn_storage) {
	of = new std::ofstream(syn_storage);
	swalker.setStoreLinks(true, of);
    }
    try {
	while(parse.rProgram(def))
	    swalker.Translate(def);
    } catch (...) {
	std::cerr << "Warning: an uncaught exception occurred when translating the parse tree" << std::endl;
    }
    // Setup synopsis c++ to py convertor
    Synopsis synopsis(source, declarations, types);
    if (syn_main_only) synopsis.onlyTranslateMain();
    // Convert!
    synopsis.translate(builder.scope());
    if (of) delete of;
#endif
  
  if(parse.NumOfErrors() != 0)
    {
      std::cerr << "errors while parsing file " << file << std::endl;
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
  PyObject *parserargs, *types, *declarations, *config;
  if (!PyArg_ParseTuple(args, "sO!OO!O", &src, &PyList_Type, &parserargs, &types, &PyList_Type, &declarations, &config)) return 0;
  std::vector<const char *> cppargs;
  std::vector<const char *> occargs;
  getopts(parserargs, cppargs, occargs, config);
  if (!src || *src == '\0')
    {
      std::cerr << "No source file" << std::endl;
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
  std::cout << "
  -I<path>                             Specify include path to be used by the preprocessor
  -D<macro>                            Specify macro to be used by the preprocessor
  -m                                   Unly keep declarations from the main file
  -b basepath                          Strip basepath from start of filenames" << endl;                           
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
    for (i = 1, py_i = 0; i != argc; ++i)
	if (argv[i][0] == '-') ++py_i;
    PyObject *pylist = PyList_New(py_i);
    for (i = 1, py_i = 0; i != argc; ++i)
	if (argv[i][0] != '-') src = argv[i];
	else PyList_SetItem(pylist, py_i++, PyString_FromString(argv[i]));
    getopts(pylist, cppargs, occargs);
    if (!src || *src == '\0') {
	std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
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
