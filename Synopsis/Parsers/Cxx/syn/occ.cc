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
#include "synopsis.hh"
#include "walker.h"
#include "token.h"
#include "buffer.h"
#include "parse.h"
#include "ptree-core.h"
#include "encoding.h"

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
public:
  PyWalker(Parser *, Synopsis *);
  ~PyWalker();
//   virtual Ptree *TranslateDeclaration(Ptree *);
//   virtual Ptree *TranslateTemplateDecl(Ptree *);
  virtual Ptree *TranslateDeclarator(Ptree *);
  virtual Ptree *TranslateTypedef(Ptree *);
  virtual Ptree *TranslateNamespaceSpec(Ptree *);
  virtual Ptree *TranslateClassSpec(Ptree *);
  virtual Ptree *TranslateTemplateClass(Ptree *, Ptree *);
  virtual vector<PyObject *> TranslateInheritanceSpec(Ptree *);
  virtual Ptree *TranslateClassBody(Ptree *);
private:
  //. extract the name of the node
  static string getName(Ptree *);
  //. return a Type object for the given name and the given list of scopes
  PyObject *lookupType(Ptree *, PyObject *);
  //. return a Type object for the given parse tree and the given list of scopes
  Synopsis *synopsis;
  PyObject *_result; // Current working value
  vector<PyObject *> _declarators;
};

PyWalker::PyWalker(Parser *p, Synopsis *s)
  : Walker(p),
    synopsis(s)
{
  Trace trace("PyWalker::PyWalker");
}

PyWalker::~PyWalker()
{
  Trace trace("PyWalker::~PyWalker");
}
  
string PyWalker::getName(Ptree *node)
{
  Trace trace("PyWalker::getName");
  if (node && node->IsLeaf())
    return node ? string(node->GetPosition(), node->GetLength()) : string();
  else
    {
      cerr << "occ internal error in 'PyWalker::getName' : node is ";
      node->Display();
      exit(-1);
    }
}

// void PyWalker::addDeclaration(PyObject *declaration)
// {
//   Trace trace("PyWalker::addDeclaration");
//   PyObject *decl = PyObject_CallMethod(scope(), "declarations", "");
//   assertObject(decl);
//   PyObject_CallMethod(decl, "append", "O", declaration);
// }

// void PyWalker::addType(PyObject *name, PyObject *type)
// {
//   Trace trace("PyWalker::addType");
// #ifndef DEBUG
//   PyObject *old_type = PyObject_GetItem(_types, name);
//   if (!old_type) // we should check whether this is a forward declaration...
//     PyObject_SetItem(_types, name, type);
//   cout << "addType " << PyString_AsString(PyObject_Str(name)) << endl;
// #endif
// }

PyObject *PyWalker::lookupType(Ptree *node, PyObject *scopes)
{
  if (node->IsLeaf()) return synopsis->lookupType(getName(node), scopes);
//   else if (node->Length() == 2) //template
//     {
      
//     }
  else
    {
      cerr << "occ internal error in 'PyWalker::lookupType' : node is ";
      node->Display();
      exit(-1);
      return 0;
    }
}

Ptree *PyWalker::TranslateDeclarator(Ptree *declarator)
{
  Trace trace("PyWalker::TranslateDeclarator");
  if (declarator->What() != ntDeclarator) return declarator;
  char* encname = declarator->GetEncodedName();
  char* enctype = declarator->GetEncodedType();
  if (!encname || !enctype) return declarator;
//   cout << "encoding '";
//   Encoding::Print(cout, enctype); cout << "' '";
//   Encoding::Print(cout, encname); cout << '\'' << endl;
  ///...
  return declarator;
}

/*
 * a typedef declaration contains three items:
 *         * 'typedef'
 *         * the alias type
 *         * the type declarators
 * unfortunately they may be folded together, such as if the alias type
 * is a derived type (pointer, reference), or a fuction pointer.
 * for now let's ignore these cases and simply treat typedefs as a 
 * list of three nodes...
 */
Ptree *PyWalker::TranslateTypedef(Ptree *node)
{
  Trace trace("PyWalker::TranslateTypedef");
  _declarators.clear();
  Ptree *tspec = TranslateTypespecifier(node->Second());
  for (Ptree *declarator = node->Third(); declarator; declarator = declarator->ListTail(2))
    TranslateDeclarator(declarator->Car());
  //. now traverse the declarators list and insert them into the AST...
}

/*
 * a namespace declaration contains three items:
 *         * 'namespace'
 *         * the name
 *         * the body (a list)
 */
Ptree *PyWalker::TranslateNamespaceSpec(Ptree *node)
{
  Trace trace("PyWalker::TranslateNamespaceSpec");
  synopsis->pushNamespace(-1, 1, getName(node->Cadr()));
  Translate(Ptree::Third(node));
  synopsis->popScope();
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
  Ptree* userkey;
  Ptree* class_def;
  if(node->Car()->IsLeaf())
    {
      userkey = 0;
      class_def = node;
    }
  else
    {
      userkey = node->First();
      class_def = node->Rest();
    }
  if(Ptree::Length(class_def) == 4 && class_def->Second()->IsLeaf())
    {
      string type = getName(class_def->First());
      string name = getName(class_def->Second());
      PyObject *clas = synopsis->addClass(-1, true, type, name);
      synopsis->addDeclared(name, clas);
      //. parents...
      vector<PyObject *> parents = TranslateInheritanceSpec(class_def->Nth(2));
      Synopsis::addInheritance(clas, parents);
      synopsis->pushScope(clas);
      //. the body...
      TranslateClassBody(class_def->Nth(3));
      synopsis->popScope();
    }
  return node;
}

Ptree *PyWalker::TranslateTemplateClass(Ptree *temp_def, Ptree *class_spec)
{
  Trace trace("PyWalker::TranslateTemplateClass");
  temp_def->Display();
  class_spec->Display();
  Ptree *userkey;
  Ptree *class_def;
  if(class_spec->Car()->IsLeaf())
    {
      userkey = nil;
      class_def = class_spec;
    }
  else
    {
      userkey = class_spec->Car();
      class_def = class_spec->Cdr();
    }
  //...  
  return 0;
}

vector<PyObject *> PyWalker::TranslateInheritanceSpec(Ptree *node)
{
  Trace trace("PyWalker::TranslateInheritanceSpec");
  vector<PyObject *> ispec;
  while(node)
    {
      node = node->Cdr();		// skip : or ,
      //. the attributes
      vector<string> attributes(node->Car()->Length() - 1);
      for (size_t i = 0; i != node->Car()->Length() - 1; ++i)
	attributes[i] = getName(node->Car()->Nth(i));
      //. look up the parent class
      PyObject *pdecl = lookupType(node->Car()->Last()->Car(), Py_BuildValue("[]"));
//       assertObject(pdecl);
      //. add it to the list
      ispec.push_back(synopsis->Inheritance(pdecl, attributes));
      node = node->Cdr();
    }
  return ispec;
}

Ptree *PyWalker::TranslateClassBody(Ptree *node)
{
  Trace trace("PyWalker::TranslateClassBody");
  return node;
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
  Trace trace("RunOpencxx");
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
  Synopsis synopsis(file, declarations, types);
  PyWalker walker(&parse, &synopsis);
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
  Trace trace("occParse");
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
  PyObject *pylist = PyList_New(argc - 1);
  for (size_t i = 1; i != argc; ++i)
    PyList_SetItem(pylist, i - 1, PyString_FromString(argv[i]));
  getopts(pylist, cppargs, occargs);
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
