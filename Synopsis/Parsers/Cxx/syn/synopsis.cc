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
    PyObject *scope = PyObject_CallMethod(ast, "Scope", "siissO", file.c_str(), -1, 1, "C++", "file", PyList_New(0));
    pushScope(scope);
    m_accessability = Default;
}

Synopsis::~Synopsis()
{
    Trace trace("Synopsis::~Synopsis");
    PyObject *file = PyObject_CallMethod(scopes.back(), "declarations", 0);
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
    PyObject *pyname = Py_BuildValue("(s)", name.c_str());
    PyObject *base = PyObject_CallMethod(type, "Base", "sO", "C++", pyname);
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

PyObject *Synopsis::addFunctionType(PyObject *ret, const vector<string>& pre, const vector<PyObject *> & params)
{
    Trace trace("Synopsis::addFunctionType");
    PyObject *func = PyObject_CallMethod(type, "Function", "sOOO", "C++", ret, V2L(pre), V2L(params));
    return func;
}

PyObject *Synopsis::addForward(size_t line, bool main, const string &type, const string &name)
{
    Trace trace("Synopsis::addForward");
    PyObject *pyname = V2L(scopedName(name));
    PyObject *forward = PyObject_CallMethod(ast, "Forward", "siissO", file.c_str(), line, main, "C++", type.c_str(), pyname);
    //PyObject_CallMethod(scopes.back(), "append", "O", forward);
    addDeclaration(forward);
    return forward;
}

PyObject *Synopsis::addComment(PyObject* decl, const char* text)
{
    Trace trace("Synopsis::addComment");
    PyObject *pytext = PyString_FromString(text);
    PyObject *comment = PyObject_CallMethod(ast, "Comment", "Osi", pytext, file.c_str(), -1);
    PyObject *comments = PyObject_CallMethod(decl, "comments", 0);
    PyObject_CallMethod(comments, "append", "O", comment);
    return comment;
}

PyObject *Synopsis::addDeclarator(size_t line, bool main, const string &name, const vector<size_t> &sizes)
{
    Trace trace("Synopsis::addDeclarator");
    PyObject *pyname = V2L(scopedName(name));
    PyObject *declarator = PyObject_CallMethod(ast, "Declarator", "siisOO", file.c_str(), line, main, "C++", pyname, V2L(sizes));
    //PyObject_CallMethod(scopes.back(), "append", "O", declarator);
    return declarator;
}

PyObject *Synopsis::addScope(size_t line, bool main, const string &type, const string &name)
{
    Trace trace("Synopsis::addScope");
    PyObject *pyname = V2L(scopedName(name));
    PyObject *scope = PyObject_CallMethod(ast, "Scope", "siissO", file.c_str(), line, main, "C++", type.c_str(), pyname);
    //PyObject_CallMethod(scopes.back(), "append", "O", scope);
    addDeclaration(scope);
    return scope;
}

PyObject *Synopsis::addModule(size_t line, bool main, const string &name)
{
    Trace trace("Synopsis::addModule");
    PyObject *pyname = V2L(scopedName(name));
    PyObject *module = PyObject_CallMethod(ast, "Module", "siissO", file.c_str(), line, main, "C++", "namespace", pyname);
    //PyObject_CallMethod(scopes.back(), "append", "O", module);
    addDeclaration(module);
    return module;
}

void Synopsis::popScope()
{
    scopes.pop_back();
    lookup_scopes = lookup_scope_stack.top();
    lookup_scope_stack.pop();
}

void Synopsis::pushScope(PyObject* scope)
{
    scopes.push_back(scope);
    PyObject* name = PyObject_CallMethod(scope, "name", 0);
    if (!name) { cout << "Warning: pushScope: scope.name failed.\n         ";
	PyErr_Print(); return; }
    lookup_scope_stack.push(lookup_scopes);
    lookup_scopes.push_front(name);
}

void Synopsis::pushClass(PyObject* clas)
{
    scopes.push_back(clas);
    lookup_scope_stack.push(lookup_scopes);
    pushClassBases(clas);
}

/* This is a post-order recursive algorithm, since the base classes must be inserted
 * in reverse order: 
 *  class A; class B; class C: A; class D: B, C;
 * Final order for D must be: D, B, C, A
 */
void Synopsis::pushClassBases(PyObject* clas)
{
    PyObject* parents = PyObject_CallMethod(clas, "parents", 0);
    if (!parents) { PyErr_Print(); return; }

    size_t size = PyList_Size(parents);
    for (size_t i = size; i > 0;) {
	i--;
	PyObject* declared = PyObject_CallMethod(PyList_GetItem(parents, i), "parent", 0);
	PyObject* baseclass = PyObject_CallMethod(declared, "declaration", 0);
	pushClassBases(baseclass);
    }
	    
    PyObject* name = PyObject_CallMethod(clas, "name", 0);
    if (!name) { cout << "Warning: pushClassBases, clas.name failed:\n         ";
	PyObject_Print(clas, stdout,0);cout<<endl;PyErr_Print(); return; }
    lookup_scopes.push_front(name);
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
    // TODO: This should only need doing once per pushScope
    PyObject* slist = PyList_New(lookup_scopes.size());
    int index = 0;
    list<PyObject*>::iterator iter = lookup_scopes.begin();
    while (iter != lookup_scopes.end()) {
	PyObject* item = *iter++;
	Py_INCREF(item);
	PyList_SetItem(slist, index, item);
	index++;
    }
    PyObject *pyname = Py_BuildValue("[s]", name.c_str());
    PyObject *type = PyObject_CallMethod(dictionary, "lookup", "OO", pyname, slist);
    return type;
}

/* Resolve typedefs */
PyObject* Synopsis::resolveDeclared(PyObject* ptype)
{
    //cout << "*** ResolveDeclared: ";PyObject_Print(ptype,stdout,0);cout<<endl;
    if (ptype == Py_None) return NULL;
    PyObject* ptype_class = PyObject_GetAttrString(ptype, "__class__");
    if (ptype_class != PyObject_GetAttrString(type, "Declared"))
	return NULL;
    // Declared type:
    PyObject* name = PyObject_CallMethod(ptype, "name", 0);
    return name;
}

/* Lookup a qualified name.
 * To do this, the first name in the vector is looked up to find a base scope,
 * and then the rest of the names are appended for the final name.
 * This is wrong: each name should be looked up in terms of the
 * so-far-determined scope, but we don't store that scope information (yet).
 */
PyObject *Synopsis::lookupType(const vector<string>& qualified)
{
    //cout << "Qualified lookup.." << endl;
    if (!qualified.front().empty()) {
	PyObject* type = lookupType(qualified.front());
	PyObject* name = resolveDeclared(type);
	if (!name) return NULL;
	PyObject* pyname = PyList_New(PyList_Size(name)+qualified.size()-1);
	int ix = 0;
	for (; ix < PyList_Size(name); ix++)
	    PyList_SetItem(pyname, ix, PyList_GetItem(name, ix));
	vector<string>::const_iterator iter = qualified.begin()+1;
	while (iter != qualified.end()) {
	    PyList_SetItem(pyname, ix, PyString_FromString(iter->c_str()));
	    iter++; ix++;
	}
	PyObject* obj = PyObject_CallMethod(dictionary, "__getitem__", "O", pyname);
	if (!obj) {
	    // cout<<"Warning: Constructed Qualified name not found!\n         ";
	    // cout<<"Name: ";PyObject_Print(pyname,stdout,0);cout<<"\n         ";
	    // cout<<"First Type: ";PyObject_Print(type,stdout,0);cout<<endl;
	    return NULL;}
	return obj;
    } else {
	PyObject* pyname = PyList_New(qualified.size()-1);
	int ix = 0;
	vector<string>::const_iterator iter = qualified.begin()+1;
	while (iter != qualified.end()) {
	    PyList_SetItem(pyname, ix, PyString_FromString(iter->c_str()));
	    iter++; ix++;
	}
	PyObject* obj = PyObject_CallMethod(dictionary, "__getitem__", "O", pyname);
	if (!obj) {
	    // cout<<"Warning: Constructed Qualified name not found!\n         ";
	    // cout<<"Name: ";PyObject_Print(pyname,stdout,0);cout<<endl;
	    return NULL;}
	return obj;
    }
    return NULL;
}

PyObject *Synopsis::Inheritance(PyObject *parent, const vector<string> &attributes)
{
    Trace trace("Synopsis::Inheritance");
    PyObject *inheritance = PyObject_CallMethod(ast, "Inheritance", "sOO", "inherits", parent, V2L(attributes));
    PyObject_CallMethod(scopes.back(), "append", "O", inheritance);
    return inheritance;
}

PyObject *Synopsis::addClass(size_t line, bool main, const string &type, const string &name)
{
    Trace trace("Synopsis::addClass");
    PyObject *pyname = V2L(scopedName(name));
    PyObject *clas = PyObject_CallMethod(ast, "Class", "siissO", file.c_str(), line, main, "C++", type.c_str(), pyname);
    //PyObject *clas = PyObject_CallMethod(ast, "Class", "siisss", file.c_str(), line, main, "C++", type.c_str(), name.c_str());
    //if (!PyObject_CallMethod(scopes.back(), "append", "O", clas)) {
    //  cout << "addClass:: scopes.back().append(clas) "; PyErr_Print();
    //}
    addDeclaration(clas);
    return clas;
}

PyObject *Synopsis::addTypedef(size_t line, bool main, const string &type, const string& name, PyObject *alias, bool constr, PyObject* declarator)
{
    Trace trace("Synopsis::addTypedef");
    PyObject *typed = PyObject_CallMethod( ast, "Typedef", "siissOOiO", 
	file.c_str(), line, main, "C++", type.c_str(), V2L(scopedName(name)),
	alias, constr, declarator
    );
    PyObject_CallMethod(scopes.back(), "append", "O", typed);
    addDeclaration(typed);
    return typed;
}

PyObject *Synopsis::Enumerator(size_t line, bool main, const string &name, const string &value)
{
    Trace trace("Synopsis::addEnumerator");
    PyObject *pyname = V2L(scopedName(name));
    PyObject *enumerator = PyObject_CallMethod(ast, "Enumerator", "siisOs", file.c_str(), line, main, "C++", pyname, value.c_str());
    return enumerator;
}

PyObject *Synopsis::addEnum(size_t line, bool main, const string &name, const vector<PyObject *> &enumerators)
{
    Trace trace("Synopsis::addEnum");
    PyObject *pyname = V2L(scopedName(name));
    PyObject *enu = PyObject_CallMethod(ast, "Enum", "siisOO", file.c_str(), line, main, "C++", pyname, V2L(enumerators));
    addDeclaration(enu);
    return enu;
}

PyObject *Synopsis::addVariable(size_t line, bool main, const string& name, PyObject *type, bool constr, PyObject* declarator)
{
    Trace trace("Synopsis::addVariable");
    PyObject *var = PyObject_CallMethod(ast, "Variable", "siissOOiO",
	file.c_str(), line, main, "C++", "variable", 
	V2L(scopedName(name)), type, constr, declarator
    );
    addDeclaration(var);
    return var;
}

PyObject *Synopsis::addConst(size_t line, bool main, PyObject *type, const string &name, const string &value)
{
    Trace trace("Synopsis::addConst");
    PyObject *cons = PyObject_CallMethod(ast, "Const", "siissOss", file.c_str(),
                                         line, main, "C++", "const", type, name.c_str(), name.c_str());
    PyObject_CallMethod(scopes.back(), "append", "O", cons);
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
    PyObject *function = PyObject_CallMethod(ast, "Function", "siissOOs", file.c_str(), line, main, "C++", "function", V2L(pre),
                         type, name.c_str());
    PyObject_CallMethod(scopes.back(), "append", "O", function);
    return function;
}

PyObject *Synopsis::addOperation(size_t line, bool main, const vector<string> &pre, PyObject *type, const string &name, const string& realname, const vector<PyObject*>& params)
{
    Trace trace("Synopsis::addOperation");
    PyObject *pyname = V2L(scopedName(name));
    PyObject *pyrname = V2L(scopedName(realname));
    PyObject *pyparams = V2L(params);
    PyObject *operation = PyObject_CallMethod(ast, "Operation", "siissOOOO",
                          file.c_str(), line, main, "C++", "function", V2L(pre), type, pyname, pyrname);
    if (!operation) {
        PyErr_Print(); return operation;
    }
    PyObject_CallMethod(scopes.back(), "append", "O", operation);
    PyObject* parameters = PyObject_CallMethod(operation, "parameters", "");
    PyObject_CallMethod(parameters, "extend", "O", pyparams);
    addDeclaration(operation);
    return operation;
}

vector<string> Synopsis::scopedName(const string &name)
{
    Trace trace("Synopsis::scopedName("+name+")");
    PyObject *current = scopes.back();
    PyObject *sname = PyObject_CallMethod(current, "name", 0);
    assertObject(sname);
    //PyList_Check(sname); // warning: Statement with no effect (???)
    size_t size = PyTuple_Size(sname);
    vector<string> scope(size);
    for (size_t i = 0; i != size; ++i)
        scope[i] = PyString_AsString(PyTuple_GetItem(sname, i));
    if (!name.empty()) scope.push_back(name);
    Py_DECREF(sname);
    return scope;
}

// void Synopsis::pushNamespace(size_t line, bool main, const string &name)
// {
//     PyObject *module = addModule(-1, true, name);
//     /* PyObject *type = */ addDeclared(name, module);
//     pushScope(module);
// }
// 
// void Synopsis::pushClass(size_t line, bool main, const string &meta, const string &name)
// {
//     PyObject *clas = addClass(-1, true, meta, name);
//     /* PyObject *type = */ addDeclared(name, clas);
//     pushScope(clas);
// }

void Synopsis::setAccessability(Accessability axs)
{
    m_accessability = axs;
}

void Synopsis::pushAccess(Accessability axs)
{
    m_access_stack.push(m_accessability);
    m_accessability = axs;
}

void Synopsis::popAccess()
{
    m_accessability = m_access_stack.top();
    m_access_stack.pop();
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
    PyObject *scope = scopes.back();
    PyObject *declarations = PyObject_CallMethod(scope, "declarations", 0);
    PyObject_CallMethod(declarations, "append", "O", declaration);
    PyObject_CallMethod(declaration, "set_accessability", "i", m_accessability);
}
