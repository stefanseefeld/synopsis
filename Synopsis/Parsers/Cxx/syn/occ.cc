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

#define ASSERT_RESULT     if (!_result) PyErr_Print(); assert(_result)
#define ASSERT_PYOBJ(pyo) if (!pyo)     PyErr_Print(); assert(pyo)

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
  static string getName(Ptree *node) { return node ? string(node->GetPosition(), node->GetLength()) : string();}
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
  _astm  = PyImport_ImportModule("Synopsis.AST");
  _typem = PyImport_ImportModule("Synopsis.Type");
  ASSERT_PYOBJ(_astm);
  ASSERT_PYOBJ(_typem);
  PyObject *fileScope = PyObject_CallMethod(_astm, "Scope", "siissN", file, 0, 1, "C++", "file", scopedName(string()));
  _scope.push_back(fileScope);
}

PyWalker::~PyWalker()
{
  PyObject *decl = PyObject_CallMethod(scope(), "declarations", "");
  size_t size = PyList_Size(decl);
  for (size_t i = 0; i != size; i++)
    {
      PyObject *item = PyList_GetItem(decl, i);
      ASSERT_PYOBJ(item);
      PyObject *ok = PyObject_CallMethod(_decl, "append", "O", item);
      ASSERT_PYOBJ(ok);
    }
//   Py_DECREF(_astm);
//   Py_DECREF(_typem);
}

PyObject *PyWalker::scopedName(const string &name)
{
  size_t lsize = _scopedName.size();
  if (name.size()) lsize += 1;
  PyObject *pylist = PyList_New(lsize);
  for (size_t i = 0; i != _scopedName.size(); i++)
    {
      PyObject *pystr = PyString_FromString(_scopedName[i].c_str());
      PyList_SetItem(pylist, i, pystr);
    }
  if (name.size())
    {
      PyObject *pystr = PyString_FromString(name.c_str());
      PyList_SetItem(pylist, _scopedName.size(), pystr);
    }
  return pylist;
}

void PyWalker::addDeclaration(PyObject *declaration)
{
  PyObject *decl = PyObject_CallMethod(scope(), "declarations", "");
  ASSERT_PYOBJ(decl);
  PyObject_CallMethod(decl, "append", "O", declaration);
}

void PyWalker::addType(PyObject *name, PyObject *type)
{
  PyObject_CallMethod(_types, "add", "OO", name, type);
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
  string name = getName(node->Cadr());
  PyObject *sname = scopedName(name);
  _result = PyObject_CallMethod(_astm, "Class", "siissN", file, -1, true, "C++", "class", sname);
  addDeclaration(_result);
  addType(sname, PyObject_CallMethod(_typem, "Declared", "sOO", "C++", sname, _result));
  _scopedName.push_back(name);
  _scope.push_back(_result);
  Translate(Ptree::Nth(node, 4));
  _scope.pop_back();
  _scopedName.pop_back();
}

static void getopts(int argc, char **argv, vector<const char *> &cppflags, vector<const char *> &occflags)
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
  if (!PyArg_ParseTuple(args, "sOOO", &src, &parserargs, &types, &declarations)) return 0;
  vector<const char *> cppargs;
  vector<const char *> occargs;
//   getopts(argc, argv, cppargs, occargs);
  getopts(0, 0, cppargs, occargs);
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
