#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <python1.5/Python.h>

#if 0
#define DO_TRACE
class Trace
{
public:
    Trace(const string &s) : scope(s) { cout << indent() << "entering " << scope << endl; ++level; }
    ~Trace() { --level; cout << indent() << "leaving " << scope << endl; }
private:
    string indent() { return string(level, ' '); }
    static int level;
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

class Synopsis
{
public:
    //. Access specifiers. @see Python/Synopsis.AST for official definition
    enum Accessability {
	Default=0, Public, Protected, Private
    };

    Synopsis(const char *, PyObject *, PyObject *);
    ~Synopsis();
    //.
    //. types from the Synopsis.Type module
    //.
    PyObject *addBase(const string &);
    PyObject *addForward(const string &);
    PyObject *addDeclared(const string &, PyObject *);
    PyObject *addTemplate(const string &, PyObject *, PyObject *);
    PyObject *addModifier(PyObject *, const vector<string> &, const vector<string> &);
    PyObject *addParametrized(PyObject *, const vector<PyObject *> &);

    //.
    //. types from the Synopsis.AST module
    //.
    PyObject *addForward(size_t, bool, const string &, const string &);
    PyObject *addDeclarator(size_t, bool, const string &, const vector<size_t> &);
    PyObject *addScope(size_t, bool, const string &, const string &);
    PyObject *addModule(size_t, bool, const string &);
    PyObject *Inheritance(PyObject *, const vector<string> &);
    PyObject *addClass(size_t, bool, const string &, const string &);
    PyObject *addTypedef(size_t, bool, const string &, const string&, PyObject *, bool, PyObject*);
    PyObject *Enumerator(size_t, bool, const string &, const string &);
    PyObject *addEnum(size_t, bool, const string &, const vector<PyObject *> &);
    PyObject *addVariable(size_t, bool, const string&, PyObject *, bool, PyObject*);
    PyObject *addConst(size_t, bool, PyObject *, const string &, const string &);
    PyObject *Parameter(const string &, PyObject *, const string &, const string & = string(), const string & = string());
    PyObject *addFunction(size_t, bool, const vector<string> &, PyObject *, const string &);
    PyObject *addOperation(size_t, bool, const vector<string> &, PyObject *, const string &, const string&, const vector<PyObject*>&);

    //.
    //. helper methods
    //.
    vector<string> scopedName(const string &);
    void pushScope(PyObject *scope) { scopes.push_back(scope);}
    void popScope() { scopes.pop_back();}
    void pushNamespace(size_t, bool, const string &);
    void pushClass(size_t, bool, const string &, const string &);
    void pushClass(size_t l, bool m, const string &n) { pushClass(l, m, "class", n);}
    void pushStruct(size_t l, bool m, const string &n) { pushClass(l, m, "struct", n);}

    void setAccessability(Accessability);
    void pushAccess(Accessability);
    void popAccess();

    PyObject *addComment(PyObject* decl, const char* text);
    void setAccessability(PyObject* decl, Accessability xs);

    PyObject *lookupType(const string &, PyObject *);
    PyObject *lookupType(const string &);

    static void addInheritance(PyObject *, const vector<PyObject *> &);
    static PyObject *N2L(const string &);
    static PyObject *V2L(const vector<string> &);
    static PyObject *V2L(const vector<PyObject *> &);
    static PyObject *V2L(const vector<size_t> &);
private:
    void addDeclaration(PyObject *);
    const char *file;
    PyObject *ast;
    PyObject *type;
    PyObject *declarations;
    PyObject *dictionary;
    vector<PyObject *> scopes;

    //. Current accessability of declarations
    Synopsis::Accessability m_accessability;

    //. Stack of current accessabilities for nested classes
    stack<Synopsis::Accessability> m_access_stack;
};
