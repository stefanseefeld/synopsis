#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <python1.5/Python.h>
#include "walker.h"
#include "token.h"
#include "buffer.h"
#include "parse.h"
#include "ptree-core.h"

/*
 * some debugging help first
 */
#define assertObject(pyo) if (!pyo) PyErr_Print(); assert(pyo)
//inline void assertObject(PyObject *obj) {if (!obj) PyErr_Print(); assert(obj);}

#if 0
class Trace
{
public:
  Trace(const string &s) : scope(s) { cout << "entering " << scope << endl;}
  ~Trace() { cout << "leaving " << scope << endl;}
private:
  string scope;
};
#else
class Trace
{
public:
  Trace(const string &) {}
  ~Trace() {}
};
#endif

/*
 * the following isn't used anywhere. Though it has to be defined and initialized to some dummy default
 * values since it is required by the opencxx.a module, which I don't want to modify...
 */
bool showProgram;
bool doCompile;
bool makeExecutable;
bool doPreprocess;
bool doTranslate;
bool verboseMode;
bool regularCpp;
bool makeSharedLibrary;
char* sharedLibraryName;
bool preprocessTwice;

void RunSoCompiler(const char *) {}
void *LoadSoLib(char *) { return 0;}
void *LookupSymbol(void *, char *) { return 0;}

class PyWalker : public Walker
{
  typedef vector<string> scopedName_t;
  typedef vector<PyObject *> scope_t;
public:
  PyWalker(const char *, Parser *, PyObject *, PyObject *);
  ~PyWalker();
//   virtual Ptree *TranslateTypedef(Ptree *);
  virtual Ptree *TranslateNamespaceSpec(Ptree *);
  virtual Ptree *TranslateClassSpec(Ptree *);
private:
  static string getName(Ptree *node)
  {
    Trace trace("PyWalker::getName");
    return node ? string(node->GetPosition(), node->GetLength()) : string();
  }
  PyObject *scopedName(const string &);
  PyObject *scope() { return _scope.back();}
  void addDeclaration(PyObject *);
  void addType(PyObject *, PyObject *);

  const char *file;
  PyObject *_astm;
  PyObject *_typem;
  PyObject *_decl;
  PyObject *_types;
  PyObject *_result; // Current working value
  scopedName_t _scopedName;
  scope_t      _scope;
};

PyWalker::PyWalker(const char *f, Parser *p, PyObject *d, PyObject *t)
  : Walker(p),
    file(f),
    _decl(d),
    _types(t)
{
  Trace trace("PyWalker::PyWalker");
  _astm  = PyImport_ImportModule("Synopsis.AST");
  _typem = PyImport_ImportModule("Synopsis.Type");
  assertObject(_astm);
  assertObject(_typem);
  PyObject *fileScope = PyObject_CallMethod(_astm, "Scope", "siissN", file, 0, 1, "C++", "file", scopedName(string()));
  _scope.push_back(fileScope);
}

PyWalker::~PyWalker()
{
  Trace trace("PyWalker::~PyWalker");
  PyObject *decl = PyObject_CallMethod(scope(), "declarations", "");
  size_t size = PyList_Size(decl);
#ifndef DEBUG
  for (size_t i = 0; i != size; i++)
    PyObject_CallMethod(_decl, "append", "O", PyList_GetItem(decl, i));
#endif
  Py_DECREF(_astm);
  Py_DECREF(_typem);
}

PyObject *PyWalker::scopedName(const string &name)
{
  Trace trace("PyWalker::scopedName");
  size_t lsize = _scopedName.size();
  if (name.size()) lsize += 1;
  PyObject *pylist = PyList_New(lsize);
  for (size_t i = 0; i != _scopedName.size(); i++)
    PyList_SetItem(pylist, i, PyString_FromString(_scopedName[i].c_str()));
  if (name.size())
    PyList_SetItem(pylist, _scopedName.size(), PyString_FromString(name.c_str()));
  return pylist;
}

void PyWalker::addDeclaration(PyObject *declaration)
{
  Trace trace("PyWalker::addDeclaration");
  PyObject *decl = PyObject_CallMethod(scope(), "declarations", "");
  assertObject(decl);
  PyObject_CallMethod(decl, "append", "O", declaration);
}

void PyWalker::addType(PyObject *name, PyObject *type)
{
  Trace trace("PyWalker::addType");
#ifndef DEBUG
  PyObject_CallMethod(_types, "add", "OO", name, type);
#endif
}

/*
 * a typedef declaration contains three items:
 *         * 'typedef'
 *         * the alias type
 *         * the type declarator
 */
// Ptree *PyWalker::TranslateTypedefSpec(Ptree *node)
// {
//   string name = getName(node->Cadr());
//   PyObject *sname = scopedName(name);
//   _result = PyObject_CallMethod(_astm, "Typedef", "siissOiO", file, -1, true, "C++", "typedef", alias, 0, declarators);
//   addDeclaration(_result);
// //   for all declarators:
// //   addType(sname, PyObject_CallMethod(_typem, "Declared", "sOO", "C++", sname, _result));
//   _scopedName.push_back(name);
//   _scope.push_back(_result);
//   Translate(Ptree::Third(node));
//   _scope.pop_back();
//   _scopedName.pop_back();
// }

/*
 * a namespace declaration contains three items:
 *         * 'namespace'
 *         * the name
 *         * the body (a list)
 */
Ptree *PyWalker::TranslateNamespaceSpec(Ptree *node)
{
  Trace trace("PyWalker::TranslateNamespaceSpec");
  string name = getName(node->Cadr());
  PyObject *sname = scopedName(name);
  _result = PyObject_CallMethod(_astm, "Module", "siissN", file, -1, true, "C++", "namespace", sname);
  addDeclaration(_result);
  addType(sname, PyObject_CallMethod(_typem, "Declared", "sOO", "C++", sname, _result));
  _scopedName.push_back(name);
  _scope.push_back(_result);
  Translate(Ptree::Third(node));
  _scope.pop_back();
  _scopedName.pop_back();
}

/*
 * a class declaration contains four items:
 *         * 'class'
 *         * the name
 *         * inheritance spec
 *         * the body (a list)
 */
Ptree *PyWalker::TranslateClassSpec(Ptree *node)
{
  Trace trace("PyWalker::TranslateClassSpec");
//   node->Display();
  Ptree* userkey;
  Ptree* class_def;
  if(node->Car()->IsLeaf())
    {
//       cout << "is leaf" << endl;
//       node->Car()->Display();
      userkey = 0;
      class_def = node;
    }
  else
    {
//       cout << "is not leaf" << endl;
      userkey = node->First();
      class_def = node->Rest();
    }
  if(Ptree::Length(class_def) == 4 && class_def->Second()->IsLeaf())
    {
//       cout << "here1" << endl;
//       class_def->Second()->Display();
      string type = getName(class_def->First());
      string name = getName(class_def->Second());
//       cout << "here2" << endl;
      PyObject *sname = scopedName(name);
      _result = PyObject_CallMethod(_astm, "Class", "siissN", file, -1, true, "C++", type.c_str(), sname);
      addDeclaration(_result);
      addType(sname, PyObject_CallMethod(_typem, "Declared", "sOO", "C++", sname, _result));
      _scopedName.push_back(name);
      _scope.push_back(_result);
      Translate(class_def->Nth(4));
      _scope.pop_back();
      _scopedName.pop_back();
    }
}

static void getopts(PyObject *args, vector<const char *> &cppflags, vector<const char *> &occflags)
{
  showProgram = false;
  doCompile = false;
  verboseMode = false;
  makeExecutable = false;
  doPreprocess = true;
  doTranslate = false;
  regularCpp = false;
  makeSharedLibrary = false;
  sharedLibraryName = 0;
  preprocessTwice = false;

  size_t argsize = PyList_Size(args);
  for (size_t i = 0; i != argsize; ++i)
    {
      const char *argument = PyString_AsString(PyList_GetItem(args, i));
      if (strncmp(argument, "-I", 2) == 0) cppflags.push_back(argument);
      else if (strncmp(argument, "-D", 2) == 0) cppflags.push_back(argument);
    }
}

static char *RunPreprocessor(const char *file, const vector<const char *> &flags)
{
  static char dest[1024];
  tmpnam(dest);
  switch(fork())
    {
    case 0:
      {
	vector<const char *> args = flags;
	char *cc = getenv("CC");
	args.insert(args.begin(), cc ? cc : "c++");
	args.push_back("-E");
	args.push_back("-o");
	args.push_back(dest);
	args.push_back("-x");
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

static char *RunOpencxx(const char *file, const vector<const char *> &args, PyObject *types, PyObject *declarations)
{
  static char dest[1024];
  tmpnam(dest);
  ifstream ifs(file);
  if(!ifs)
    {
      perror(file);
      exit(1);
    }
  ProgramFile prog(ifs);
  Lex lex(&prog);
  Parser parse(&lex);
  PyWalker walker(file, &parse, declarations, types);
//   Walker walker(&parse);
  Ptree *def;
  while(parse.rProgram(def))
    walker.Translate(def);
  if(parse.NumOfErrors() != 0)
    {
      cerr << "errors while parsing file " << file << endl;
      exit(1);
    }
  return dest;
}

extern "C" {

static PyObject *occParse(PyObject *self, PyObject *args)
{
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
  char *occfile = RunOpencxx(cppfile, occargs, types, declarations);
  unlink(cppfile);
  unlink(occfile);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyMethodDef occ_methods[] =
{
  {(char*)"parse",            occParse,               METH_VARARGS},
  {NULL, NULL}
};

void initocc()
{
  PyObject* m = Py_InitModule((char*)"occ", occ_methods);
  PyObject_SetAttrString(m, (char*)"version", PyString_FromString("0.1"));
}
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
  getopts(0, 0, cppargs, occargs);
  if (!src || *src == '\0')
    {
      cerr << "No source file" << endl;
      exit(-1);
    }
  Py_Initialize();
  char *cppfile = RunPreprocessor(src, cppargs);
  char *occfile = RunOpencxx(cppfile, occargs, 0, 0);
  unlink(cppfile);
  unlink(occfile);
}

#endif
