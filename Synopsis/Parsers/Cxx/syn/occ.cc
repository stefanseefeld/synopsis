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
#include "ptree.h"
#include "encoding.h"

/* TODO: figure out how to store overloaded method names :/ */

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
  virtual ~PyWalker();
  virtual Ptree *TranslateDeclaration(Ptree *);
//   virtual Ptree *TranslateTemplateDecl(Ptree *);
  virtual Ptree *TranslateDeclarator(bool, PtreeDeclarator*);
  virtual Ptree *TranslateDeclarator(Ptree *);
  virtual Ptree *TranslateFunctionImplementation(Ptree *);
  virtual Ptree *TranslateTypedef(Ptree *);
  virtual Ptree *TranslateNamespaceSpec(Ptree *);
  virtual Ptree *TranslateClassSpec(Ptree *);
  virtual Ptree *TranslateTemplateClass(Ptree *, Ptree *);
  virtual vector<PyObject *> TranslateInheritanceSpec(Ptree *);
  virtual Ptree *TranslateClassBody(Ptree *, Ptree*, Class*);
private:
  //. extract the name of the node
  static string getName(Ptree *);
  //. return a Type object for the given name and the given list of scopes
  PyObject *lookupType(Ptree *, PyObject *);
  //. return a Type object for the given parse tree and the given list of scopes
  Synopsis *synopsis;
  PyObject *_result; // Current working value
  vector<PyObject *> _declarators;

  //. Return a Type object from the encoded type
  PyObject* decodeType();
  void initTypeDecoder(string& enctype, string::iterator&);

  string* m_enctype;
  string::iterator* m_enc_iter;

  //. Nasty hack to pass stuff between methods :/
  Ptree* m_declaration; 
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
    if (node->IsLeaf())
	return synopsis->lookupType(getName(node), scopes);
    else if (node->Length() == 2) {
	//template
	// Ignore template for now and just return the name
	// FIXME: obviously this is not the right thing to do ;)
	return synopsis->lookupType(getName(node->First()), scopes);
    } else if (node->Length() > 2 && node->Second()->Eq("::")) {
	// Scoped name.. Join for now.
	Ptree* name = Ptree::Make(node->ToString());
	return synopsis->lookupType(getName(name), scopes);
    } else {
	cerr << "occ internal error in 'PyWalker::lookupType' : node is ";
	node->Display();
	exit(-1);
	return 0;
    }
}

//. Translates a declaration, either variable, typedef or function
//. Function prototype:
//.  [ [modifiers] name [declarators] ; ]
//. Function impl:
//.  [ [modifiers] name declarator [ { ... } ] ]
//. Typedef:
//.  ?
//. Variables:
//.  ?
Ptree *PyWalker::TranslateDeclaration(Ptree *declaration)
{
    Trace trace("PyWalker::TranslateDeclaration");
#ifdef DO_TRACE
    declaration->Display();
#endif
    m_declaration = declaration;
    Walker::TranslateDeclaration(declaration);
    return declaration;
}

Ptree *PyWalker::TranslateFunctionImplementation(Ptree *p)
{
    TranslateDeclarator(false, (PtreeDeclarator*)p->Third());
    return p;
}

//. Translates a declarator
//. Function proto:
//.   [ { * | & }* name ( [params] ) ]
//. param:
//.   [ [types] { [ { * | & }* name ] { = value } } ]
Ptree *PyWalker::TranslateDeclarator(bool, PtreeDeclarator* decl)
{
    Trace trace("PyWalker::TranslateDeclarator(bool,PtreeDeclarator*)");
    if (decl->What() != ntDeclarator) return decl;
    char* encname = decl->GetEncodedName();
    char* enctype = decl->GetEncodedType();
    if (!encname || !enctype) return decl;
    if (enctype[0] == 'F') {
	//cout << "\n*** Function: "; decl->Display();
#if 0
	cout << "encoding '";
	Encoding::Print(cout, enctype); cout << "' '";
	Encoding::Print(cout, encname); cout << '\'' << endl;
#endif
	// Figure out parameter types:
	string enctype_s(enctype);
	string::iterator enc_iter = enctype_s.begin()+1;
	initTypeDecoder(enctype_s, enc_iter);
	
	// Create parameter objects
	vector<PyObject*> params;
	Ptree *p = decl->Rest();
	while (p && !p->Car()->Eq('(')) p = Ptree::Rest(p);
	if (!p) { cout << "Warning: error finding params!" << endl; return decl; }
	Ptree* pparams = p->Second();
	while (pparams) {
	    string name, value;
	    if (pparams->Car()->Eq(',')) pparams = pparams->Cdr();
	    Ptree* param = pparams->First();
	    //Ptree* ptype = param->First();
	    PyObject* type = decodeType();
	    if (!type) break; // end of encoding
	    if (param->Length() > 1) {
		Ptree* pname = param->Second();
		//cout << "pname == "; pname->Display();
		if (pname && pname->Car()) {
		    while (pname && (pname->Car()->Eq('*') || pname->Car()->Eq('&'))) pname = pname->Cdr();
		    if (pname) {
			name = pname->Car()->ToString();
			//cout << "name == " << name << endl;
		    }
		}
		if (param->Length() > 2) {
		    value = param->Nth(3)->ToString();
		}
	    }
	    params.push_back(synopsis->Parameter("", type, "", name, value));
	    pparams = Ptree::Rest(pparams);
	}

	// Figure out the return type:
	//enc_iter = enctype_s.begin() + enctype_s.find('_') + 1;
	while (*enc_iter++ != '_');
	PyObject* returnType = decodeType();

	// Find name:
	for (p = decl; p->Car()->Eq('*') || p->Car()->Eq('&'); p = p->Cdr());
	string name = p ? p->Car()->ToString() : "";
    
	// cout << "\n\e[1m"<<name<<"\e[m - ";decl->Display();

	// Figure out premodifiers
	vector<string> premod;
	p = Ptree::First(m_declaration);
	while (p) {
	    premod.push_back(p->ToString());
	    p = Ptree::Rest(p);
	}
	PyObject* oper = synopsis->addOperation(-1, true, premod, returnType, name, params);
	synopsis->addDeclared(name, oper);
    }
    return decl;
}

void PyWalker::initTypeDecoder(string& enctype, string::iterator& iter)
{
    m_enctype = &enctype;
    m_enc_iter = &iter;
    //cout << "*** Starting decoding of \"" << enctype << "\"" << endl;
}

PyObject* PyWalker::decodeType()
{
    /* NOTE: This function is far from done. See encoding.cc, and also need to
     * find a better way of getting integral types than addBase() */

    // OpenC++ uses an offset for some things..
    const int DigitOffset = 0x80;

    string::iterator end = m_enctype->end(), &iter = *m_enc_iter;
    // PyObject* obj = 0;
    vector<string> premod, postmod;
    string name;
    while (iter != end && !name.length()) {
	int c = int((unsigned char)*iter++);
	//cout << "*** '" << (char)c << "' (" << *iter << ")" << endl;
	if (c == 'P') { postmod.insert(postmod.begin(), "*"); }
	else if (c == 'R') { postmod.insert(postmod.begin(), "&"); }
	else if (c == 'S') { premod.push_back("signed"); }
	else if (c == 'U') { premod.push_back("unsigned"); }
	else if (c == 'C') { premod.push_back("const"); }
	else if (c == 'A') { premod.push_back("[]"); }
	else if (c == 'i') { name = "int"; }
	else if (c == 'v') { name = "void"; }
	else if (c == 'b') { name = "bool"; }
	else if (c == 's') { name = "short"; }
	else if (c == 'c') { name = "char"; }
	else if (c == 'l') { name = "long"; }
	else if (c == 'j') { name = "long long"; }
	else if (c == 'f') { name = "float"; }
	else if (c == 'd') { name = "double"; }
	else if (c == 'r') { name = "long double"; }
	else if (c == 'e') { name = "..."; }
	else if (c == '?') { return Py_None; }
	else if (c > DigitOffset) {
	    int count = c - DigitOffset;
	    // FIXME: resize name instead
	    string myname(count, '\0');
	    copy_n(iter, count, myname.begin());
	    iter += count;
	    name = myname;
	}
	else if (c == 'Q') {
	    // Qualified type: first is num of scopes, each a name.
	    int scopes = int((unsigned char)*iter++) - DigitOffset;
	    vector<string> names;
	    while (scopes--) {
		int length = int((unsigned char)*iter++) - DigitOffset;
		string myname(length, '\0');
		copy_n(iter, length, myname.begin());
		names.push_back(myname);
		iter += length;
	    }
	    // For now just colonate them
	    string name = names[0];
	    for (size_t i = 1; i < names.size(); i++)
		name += "::" + names[i];
	}
	else if (c == '_') { return NULL; }
	else if (c == 'F') {
	    // Function ptr.. argh!
	    // FIXME: currently its just skipped
	    while (decodeType());
	    decodeType();
	    return Py_None;
	}
	else { cout << "Unknown char decoding '"<<*m_enctype<<"': "<<char(c)<<" "<<c<<" at "<<(iter-m_enctype->begin())<<endl; }
    }
    if (!name.length()) { return Py_None; }

    PyObject *baseType = synopsis->lookupType(name);
    if (premod.empty() && postmod.empty())
	return baseType;
    return synopsis->addModifier(baseType, premod, postmod);
}

Ptree *PyWalker::TranslateDeclarator(Ptree *declarator)
{
  Trace trace("PyWalker::TranslateDeclarator");
#ifdef DO_TRACE
    declarator->Display();
#endif
  if (declarator->What() != ntDeclarator) return declarator;
  char* encname = declarator->GetEncodedName();
  char* enctype = declarator->GetEncodedType();
  if (!encname || !enctype) return declarator;
#ifdef DO_TRACE
   cout << "encoding '";
   Encoding::Print(cout, enctype); cout << "' '";
   Encoding::Print(cout, encname); cout << '\'' << endl;
#endif
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
  /* Ptree *tspec = */ TranslateTypespecifier(node->Second());
  for (Ptree *declarator = node->Third(); declarator; declarator = declarator->ListTail(2))
    TranslateDeclarator(declarator->Car());
  //. now traverse the declarators list and insert them into the AST...
  return node;
}

/*
 * a namespace declaration contains three items:
 *         * 'namespace'
 *         * the name
 *         * the body (a list)
 */
Ptree *PyWalker::TranslateNamespaceSpec(Ptree *node)
{
  //Trace trace("PyWalker::TranslateNamespaceSpec");
  synopsis->pushNamespace(-1, 1, getName(node->Cadr()));
  Translate(Ptree::Third(node));
  synopsis->popScope();
  return node;
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
  //Trace trace("PyWalker::TranslateClassSpec");
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
      Class* meta = MakeClassMetaobject(node, userkey, class_def);
      //. the body...
      TranslateClassBody(class_def->Nth(3), class_def->Nth(2), meta);
      synopsis->popScope();
    }
  return node;
}

Ptree *PyWalker::TranslateTemplateClass(Ptree *temp_def, Ptree *class_spec)
{
  //Trace trace("PyWalker::TranslateTemplateClass");
  //temp_def->Display();
  //class_spec->Display();
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
  //Trace trace("PyWalker::TranslateInheritanceSpec");
  vector<PyObject *> ispec;
  while(node)
    {
      node = node->Cdr();		// skip : or ,
      //. the attributes
      vector<string> attributes(node->Car()->Length() - 1);
      for (int i = 0; i != node->Car()->Length() - 1; ++i)
	attributes[i] = getName(node->Car()->Nth(i));
      //. look up the parent class
      PyObject *pdecl = lookupType(node->Car()->Last()->Car(), Py_BuildValue("[]"));
      node = node->Cdr();
      if (pdecl == Py_None) continue;
//       assertObject(pdecl);
      //. add it to the list
      ispec.push_back(synopsis->Inheritance(pdecl, attributes));
    }
  return ispec;
}

Ptree *PyWalker::TranslateClassBody(Ptree *block, Ptree *bases, Class *meta)
{
  //Trace trace("PyWalker::TranslateClassBody");
  Walker::TranslateClassBody(block, bases, meta);
  return block;
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
