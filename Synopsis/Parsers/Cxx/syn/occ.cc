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

void getopts(PyObject *args, vector<const char *> &cppflags, vector<const char *> &occflags)
{
    showProgram = doCompile = verboseMode = makeExecutable = false;
    doTranslate = regularCpp = makeSharedLibrary = preprocessTwice = false;
    doPreprocess = true;
    sharedLibraryName = 0;
    syn_main_only = false;
    syn_extract_tails = false;
    syn_use_gcc = false;

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
	vector<const char *> args = flags;
	char *cc = getenv("CC");
	args.insert(args.begin(), cc ? cc : "c++");
	args.push_back("-C"); // keep comments
	args.push_back("-lg"); // gcc-like line numbers
	args.push_back("-Y"); // define system macros in tune.h
	//args.push_back("-E"); // stop after preprocessing
	// Add includes
	args.push_back("-I");
	args.push_back("/usr/include/g++-3/");
	//args.push_back("-I");
	//args.push_back("/usr/include/linux/");
	args.push_back("-I");
	args.push_back("/usr/lib/gcc-lib/i386-linux/2.95.3/include/");
	args.push_back("-o"); // output to...
	args.push_back(dest);
	//args.push_back("-x"); // language c++
	//args.push_back("c++");
	args.push_back(file);

	// Call ucpp
	int status = ucpp_main(args.size(), (char **)args.begin());
	if (status != 0) {
	    cout << "ucpp returned error flag." << endl;
	    exit(1);
	}
    }
    return dest;
}

char *RunOpencxx(const char *src, const char *file, const vector<const char *> &args, PyObject *types, PyObject *declarations)
{
  Trace trace("RunOpencxx");
  std::set_unexpected(unexpected);
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
  
    // Calculate source filename
    string source(src);
    if (source.compare(string(syn_basename), 0, strlen(syn_basename)) == 0)
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
    ofstream* of = NULL;
    if (syn_storage) {
	of = new ofstream(syn_storage);
	swalker.setStoreLinks(true, of);
    }
    try {
	while(parse.rProgram(def))
	    swalker.Translate(def);
    } catch (...) {
	cout << "Warning: an uncaught exception occurred when translating the parse tree" << endl;
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
  PyObject *parserargs, *types, *declarations, *config;
  if (!PyArg_ParseTuple(args, "sO!OO!O", &src, &PyList_Type, &parserargs, &types, &PyList_Type, &declarations, &config)) return 0;
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
  -m                                   Unly keep declarations from the main file
  -b basepath                          Strip basepath from start of filenames" << endl;                           
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
    char *src = NULL;
    vector<const char *> cppargs;
    vector<const char *> occargs;
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
	cerr << "Usage: " << argv[0] << " <filename>" << endl;
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
