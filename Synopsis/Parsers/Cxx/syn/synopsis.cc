#include "synopsis.hh"

#ifdef DO_TRACE
int Trace::level = 0;
#endif

#define assertObject(pyo) if (!pyo) PyErr_Print(); assert(pyo)

Synopsis::Synopsis(const char *f, PyObject *decl, PyObject *dict)
  : file(f), declarations(decl), dictionary(dict)
{
  Trace trace("Synopsis::Synopsis");
  ast  = PyImport_ImportModule("Synopsis.AST");
  assertObject(ast);
  type = PyImport_ImportModule("Synopsis.Type");
  assertObject(type);
  addBase("char");
  addBase("wchar_t");
  addBase("bool");
  addBase("short");
  addBase("int");
  addBase("long");
  addBase("signed");
  addBase("unsigned");
  addBase("float");
  addBase("double");
  addBase("void");
  addBase("...");
  //. well...
  addBase("exception");
  PyObject *scope = PyObject_CallMethod(ast, "Scope", "siissO", file, -1, 1, "C++", "file", PyList_New(0));
  pushScope(scope);
}

Synopsis::~Synopsis()
{
  Trace trace("Synopsis::~Synopsis");
  PyObject *file = PyObject_CallMethod(scopes.top(), "declarations", 0);
  size_t size = PyList_Size(file);
  for (size_t i = 0; i != size; i++)
    PyObject_CallMethod(declarations, "append", "O", PyList_GetItem(file, i));
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(ast);
}

PyObject *Synopsis::addBase(const string &name)
{
  Trace trace("Synopsis::addBase");
  PyObject *pyname = Py_BuildValue("[O]", PyString_FromString(name.c_str()));
  PyObject *base = PyObject_CallMethod(type, "Base", "ss", "C++", name.c_str());
  PyObject_SetItem(dictionary, pyname, base);
  return base;
}

PyObject *Synopsis::addForward(const string &name)
{
  Trace trace("Synopsis::addForward");
  PyObject *pyname = V2L(scopedName(name));
  PyObject *forward = PyObject_CallMethod(type, "Forward", "sO", "C++", pyname);
  PyObject_SetItem(dictionary, pyname, forward);
  return forward;
}

PyObject *Synopsis::addDeclared(const string &name, PyObject *declaration)
{
  Trace trace("Synopsis::addDeclared");
  PyObject *pyname = V2L(scopedName(name));
  PyObject *declared = PyObject_CallMethod(type, "Declared", "sOO", "C++", pyname, declaration);
  PyObject_SetItem(dictionary, pyname, declared);
  return declared;
}

PyObject *Synopsis::addTemplate(const string &name, PyObject *declaration, PyObject *parameters)
{
  Trace trace("Synopsis::addTemplate");
  PyObject *pyname = V2L(scopedName(name));
  PyObject *templ = PyObject_CallMethod(type, "Template", "sOOO", "C++", pyname, declaration, parameters);
  PyObject_SetItem(dictionary, pyname, templ);
  return templ;
}

PyObject *Synopsis::addModifier(PyObject *alias, const vector<string> &pre, const vector<string> &post)
{
  Trace trace("Synopsis::addModifier");
  PyObject *modifier = PyObject_CallMethod(type, "Modifier", "sOOO", "C++", alias, V2L(pre), V2L(post));
  return modifier;
}

PyObject *Synopsis::addParametrized(PyObject *templ, const vector<PyObject *> &parameters)
{
  Trace trace("Synopsis::addParametrized");
  PyObject *parametrized = PyObject_CallMethod(type, "Parametrized", "sOO", "C++", templ, V2L(parameters));
  return parametrized;
}

PyObject *Synopsis::addForward(size_t line, bool main, const string &type, const string &name)
{
  Trace trace("Synopsis::addForward");
  PyObject *pyname = V2L(scopedName(name));
  PyObject *forward = PyObject_CallMethod(ast, "Forward", "siissO", file, line, main, "C++", type.c_str(), pyname);
  //PyObject_CallMethod(scopes.top(), "append", "O", forward);
  addDeclaration(forward);
  return forward;
}

PyObject *Synopsis::addDeclarator(size_t line, bool main, const string &name, const vector<size_t> &sizes)
{
  Trace trace("Synopsis::addDeclarator");
  PyObject *pyname = V2L(scopedName(name));
  PyObject *declarator = PyObject_CallMethod(ast, "Declarator", "siisOO", file, line, main, "C++", pyname, V2L(sizes));
  //PyObject_CallMethod(scopes.top(), "append", "O", declarator);
  return declarator;
}

PyObject *Synopsis::addScope(size_t line, bool main, const string &type, const string &name)
{
  Trace trace("Synopsis::addScope");
  PyObject *pyname = V2L(scopedName(name));
  PyObject *scope = PyObject_CallMethod(ast, "Scope", "siissO", file, line, main, "C++", type.c_str(), pyname);
  //PyObject_CallMethod(scopes.top(), "append", "O", scope);
  addDeclaration(scope);
  return scope;
}

PyObject *Synopsis::addModule(size_t line, bool main, const string &name)
{
  Trace trace("Synopsis::addModule");
  PyObject *pyname = V2L(scopedName(name));
  PyObject *module = PyObject_CallMethod(ast, "Module", "siissO", file, line, main, "C++", "namespace", pyname);
  //PyObject_CallMethod(scopes.top(), "append", "O", module);
  addDeclaration(module);
  return module;
}

PyObject *Synopsis::lookupType(const string &name, PyObject *scopes)
{
  Trace trace("Synopsis::lookupType");
  size_t size = PyList_Size(scopes);
  PyObject *slist = PyList_New(size + 1);
  for (size_t i = 0; i != size; ++i)
    {
      PyObject *item = PyList_GetItem(scopes, i);
      Py_INCREF(item);
      PyList_SetItem(slist, i, item);
    }
  PyList_SetItem(slist, size, V2L(scopedName(string())));
  PyObject *pyname = Py_BuildValue("[s]", name.c_str());
  if (!pyname) {PyErr_Print(); return 0; }
  PyObject *type = PyObject_CallMethod(dictionary, "lookup", "OO", pyname, slist);
  if (!type) { PyErr_Print(); return 0; }
  return type;
}

PyObject *Synopsis::lookupType(const string &name)
{
  Trace trace("Synopsis::lookupType(name)");
  PyObject *pyname = Py_BuildValue("[s]", name.c_str());
  if (!pyname) {PyErr_Print(); return 0; }
  PyObject *slist = PyList_New(1);
  PyObject *sname = PyObject_CallMethod(scopes.top(), "name", 0);
  PyList_SetItem(slist, 0, sname);
  PyObject *type = PyObject_CallMethod(dictionary, "lookup", "OO", pyname, slist);
  if (!type) { PyErr_Print(); return 0; }
  return type;
}

PyObject *Synopsis::Inheritance(PyObject *parent, const vector<string> &attributes)
{
  Trace trace("Synopsis::Inheritance");
  PyObject *inheritance = PyObject_CallMethod(ast, "Inheritance", "sOO", "inherits", parent, V2L(attributes));
  PyObject_CallMethod(scopes.top(), "append", "O", inheritance);
  return inheritance;
}

PyObject *Synopsis::addClass(size_t line, bool main, const string &type, const string &name)
{
  Trace trace("Synopsis::addClass");
  PyObject *pyname = V2L(scopedName(name));
  PyObject *clas = PyObject_CallMethod(ast, "Class", "siissO", file, line, main, "C++", type.c_str(), pyname);
  //PyObject *clas = PyObject_CallMethod(ast, "Class", "siisss", file, line, main, "C++", type.c_str(), name.c_str());
  //if (!PyObject_CallMethod(scopes.top(), "append", "O", clas)) {
  //  cout << "addClass:: scopes.top().append(clas) "; PyErr_Print();
  //}
  addDeclaration(clas);
  return clas;
}

PyObject *Synopsis::addTypedef(size_t line, bool main, const string &type, PyObject *alias, bool constr, vector<PyObject *> &declarators)
{
  Trace trace("Synopsis::addTypedef");
  PyObject *typed = PyObject_CallMethod(ast, "Typedef", "siissOiO", file, line, main, "C++", type.c_str(), alias, constr,
					V2L(declarators));
  PyObject_CallMethod(scopes.top(), "append", "O", typed);
  addDeclaration(typed);
  return typed;
}

PyObject *Synopsis::Enumerator(size_t line, bool main, const string &name, const string &value)
{
  Trace trace("Synopsis::addEnumerator");
  PyObject *enumerator = PyObject_CallMethod(ast, "Enumerator", "siisss", file, line, main, "C++", name.c_str(), value.c_str());
  return enumerator;
}

PyObject *Synopsis::addEnum(size_t line, bool main, const string &name, const vector<PyObject *> &enumerators)
{
  Trace trace("Synopsis::addEnum");
  PyObject *pyname = V2L(scopedName(name));
  PyObject *enu = PyObject_CallMethod(ast, "Enum", "siisOO", file, line, main, "C++", pyname, V2L(enumerators));
  addDeclaration(enu);
  return enu;
}

PyObject *Synopsis::addVariable(size_t line, bool main, PyObject *type, bool constr, const vector<PyObject *> &declarators)
{
  Trace trace("Synopsis::addVariable");
  PyObject *var = PyObject_CallMethod(ast, "Variable", "siissOiO", file, line, main, "C++", "variable", type, constr, V2L(declarators));
  PyObject_CallMethod(scopes.top(), "append", "O", var);
  return var;
}

PyObject *Synopsis::addConst(size_t line, bool main, PyObject *type, const string &name, const string &value)
{
  Trace trace("Synopsis::addConst");
  PyObject *cons = PyObject_CallMethod(ast, "Const", "siissOss", file,
    line, main, "C++", "const", type, name.c_str(), name.c_str());
  PyObject_CallMethod(scopes.top(), "append", "O", cons);
  return cons;
}

PyObject *Synopsis::Parameter(const string &pre, PyObject *type, const string &post, const string &name, const string &value)
{
  Trace trace("Synopsis::Parameter");
  PyObject *param = 0;
  if (value.empty() && name.empty())
    param = PyObject_CallMethod(ast, "Parameter", "sOs", pre.c_str(), type, post.c_str());
  else if (value.empty())
    param = PyObject_CallMethod(ast, "Parameter", "sOss", pre.c_str(), type, post.c_str(), name.c_str());
  else param = PyObject_CallMethod(ast, "Parameter", "sOsss", pre.c_str(), type, post.c_str(), name.c_str(), value.c_str());
  return param;
}

PyObject *Synopsis::addFunction(size_t line, bool main, const vector<string> &pre, PyObject *type, const string &name)
{
  Trace trace("Synopsis::addFunction");
  PyObject *function = PyObject_CallMethod(ast, "Function", "siissOOs", file, line, main, "C++", "function", V2L(pre),
					   type, name.c_str());
  PyObject_CallMethod(scopes.top(), "append", "O", function);
  return function;
}

PyObject *Synopsis::addOperation(size_t line, bool main, const vector<string> &pre, PyObject *type, const string &name, const vector<PyObject*>& params)
{
  Trace trace("Synopsis::addOperation");
  PyObject *pyname = V2L(scopedName(name));
  PyObject *pyparams = V2L(params);
  PyObject *operation = PyObject_CallMethod(ast, "Operation", "siissOOO", 
    file, line, main, "C++", "function", V2L(pre), type, pyname);
  if (!operation) {
    PyErr_Print(); return operation;
  }
  PyObject_CallMethod(scopes.top(), "append", "O", operation);
  PyObject* parameters = PyObject_CallMethod(operation, "parameters", "");
  PyObject_CallMethod(parameters, "extend", "O", pyparams);
  addDeclaration(operation);
  return operation;
}

vector<string> Synopsis::scopedName(const string &name)
{
  Trace trace("Synopsis::scopedName("+name+")");
  PyObject *current = scopes.top();
  PyObject *sname = PyObject_CallMethod(current, "name", 0);
  assertObject(sname);
  //PyList_Check(sname); // warning: Statement with no effect (???)
  size_t size = PyList_Size(sname);
  vector<string> scope(size);
  for (size_t i = 0; i != size; ++i)
    scope[i] = PyString_AsString(PyList_GetItem(sname, i));
  if (!name.empty()) scope.push_back(name);
  Py_DECREF(sname);
  return scope;
}

void Synopsis::pushNamespace(size_t line, bool main, const string &name)
{
  PyObject *module = addModule(-1, true, name);
  /* PyObject *type = */ addDeclared(name, module);
  pushScope(module);
}

void Synopsis::pushClass(size_t line, bool main, const string &meta, const string &name)
{
  PyObject *clas = addClass(-1, true, meta, name);
  /* PyObject *type = */ addDeclared(name, clas);
  pushScope(clas);
}

void Synopsis::addInheritance(PyObject *clas, const vector<PyObject *> &parents)
{
  PyObject *to = PyObject_CallMethod(clas, "parents", 0);
  PyObject *from = V2L(parents);
  size_t size = PyList_Size(from);
  for (size_t i = 0; i != size; ++i)
    PyList_Append(to, PyList_GetItem(from, i));
  Py_DECREF(to);
  Py_DECREF(from);
}

PyObject *Synopsis::N2L(const string &name)
{
  Trace trace("Synopsis::N2L");
  vector<string> scope;
  string::size_type b = 0;
  while (b < name.size())
    {
      string::size_type e = name.find("::", b);
      scope.push_back(name.substr(b, e == string::npos ? e : e - b));
      b = e == string::npos ? string::npos : e + 2;
    }
  PyObject *pylist = PyList_New(scope.size());
  for (size_t i = 0; i != scope.size(); ++i)
    PyList_SetItem(pylist, i, PyString_FromString(scope[i].c_str()));
  return pylist;
}

PyObject *Synopsis::V2L(const vector<string> &strings)
{
  Trace trace("Synopsis::V2L(vector<string>)");
  PyObject *pylist = PyList_New(strings.size());
  for (size_t i = 0; i != strings.size(); ++i)
    PyList_SetItem(pylist, i, PyString_FromString(strings[i].c_str()));
  return pylist;
}

PyObject *Synopsis::V2L(const vector<PyObject *> &objs)
{
  Trace trace("Synopsis::V2L(vector<PyObject*>)");
  PyObject *pylist = PyList_New(objs.size());
  for (size_t i = 0; i != objs.size(); ++i)
    PyList_SetItem(pylist, i, objs[i]);
  return pylist;
}

PyObject *Synopsis::V2L(const vector<size_t> &sizes)
{
  //Trace trace("Synopsis::V2L");
  PyObject *pylist = PyList_New(sizes.size());
  for (size_t i = 0; i != sizes.size(); ++i)
    PyList_SetItem(pylist, i, PyInt_FromLong(sizes[i]));
  return pylist;
}

void Synopsis::addDeclaration(PyObject *declaration)
{
  PyObject *scope = scopes.top();
  PyObject *declarations = PyObject_CallMethod(scope, "declarations", 0);
  PyObject_CallMethod(declarations, "append", "O", declaration);
}
