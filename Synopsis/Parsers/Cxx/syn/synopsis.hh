#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <stack>
#if PYTHON_MAJOR == 2
#  if PYTHON_MINOR == 0
#    include <python2.0/Python.h>
#  else
#    error "this python version is not supported yet"
#  endif
#elif PYTHON_MAJOR == 1
#  if PYTHON_MINOR == 6
#    include <python1.6/Python.h>
#  elif PYTHON_MINOR == 5
#    include <python1.5/Python.h>
#  else
#    error "this python version is not supported yet"
#  endif
#endif

#include "ast.hh"
#include "type.hh"

#if DEBUG
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

//. The Synopsis class maps from C++ objects to Python objects
class Synopsis : public AST::Visitor, public Type::Visitor {
public:

    Synopsis(string mainfile, PyObject *decls, PyObject *types);
    ~Synopsis();

    void onlyTranslateMain();
    void translate(AST::Scope* global);

    //
    // types from the Synopsis.Type module
    //
    PyObject *Base(Type::Base*);
    PyObject *Unknown(Type::Named*);
    PyObject *Declared(Type::Declared*);
    PyObject *Template(Type::Template*);
    PyObject *Modifier(Type::Modifier*);
    PyObject *Parameterized(Type::Parameterized*);
    PyObject *FuncPtr(Type::FuncPtr*);

    //
    // types from the Synopsis.AST module
    //
    PyObject *Forward(AST::Forward*);
    PyObject *Scope(AST::Scope*);
    PyObject *Namespace(AST::Namespace*);
    PyObject *Inheritance(AST::Inheritance*);
    PyObject *Class(AST::Class*);
    PyObject *Typedef(AST::Typedef*);
    PyObject *Enumerator(AST::Enumerator*);
    PyObject *Enum(AST::Enum*);
    PyObject *Variable(AST::Variable*);
    PyObject *Const(AST::Const*);
    PyObject *Parameter(AST::Parameter*);
    PyObject *Function(AST::Function*);
    PyObject *Operation(AST::Operation*);
    PyObject *Comment(AST::Comment*);

    //
    // AST::Visitor methods
    //
    //void visitDeclaration(AST::Declaration*);
    void visitScope(AST::Scope*);
    void visitNamespace(AST::Namespace*);
    void visitClass(AST::Class*);
    void visitInheritance(AST::Inheritance*);
    void visitForward(AST::Forward*);
    void visitTypedef(AST::Typedef*);
    void visitVariable(AST::Variable*);
    void visitConst(AST::Const*);
    void visitEnum(AST::Enum*);
    void visitEnumerator(AST::Enumerator*);
    void visitFunction(AST::Function*);
    void visitOperation(AST::Operation*);
    void visitParameter(AST::Parameter*);
    void visitComment(AST::Comment*);

    //
    // Type::Visitor methods
    //
    //void visitType(Type::Type*);
    void visitUnknown(Type::Unknown*);
    void visitModifier(Type::Modifier*);
    //void visitNamed(Type::Named*);
    void visitBase(Type::Base*);
    void visitDeclared(Type::Declared*);
    void visitTemplateType(Type::Template*);
    void visitParameterized(Type::Parameterized*);
    void visitFuncPtr(Type::FuncPtr*);

private:
    //. Compiler Firewalled private data
    struct Private;
    friend Private;
    Private* m;

    //.
    //. helper methods
    //.
    void addComments(PyObject* pydecl, AST::Declaration* cdecl);

    ///////// EVERYTHING BELOW HERE SUBJECT TO REVIEW AND DELETION


    /*
    PyObject *lookupType(const string &, PyObject *);
    PyObject *lookupType(const string &);
    PyObject *lookupType(const vector<string>& qualified);

    static void addInheritance(PyObject *, const vector<PyObject *> &);
    static PyObject *N2L(const string &);
    static PyObject *V2L(const vector<string> &);
    static PyObject *V2L(const vector<PyObject *> &);
    static PyObject *V2L(const vector<size_t> &);
    void pushClassBases(PyObject* clas);
    PyObject* resolveDeclared(PyObject*);
    void addDeclaration(PyObject *);
    */
private:
    PyObject *m_ast;
    PyObject *m_type;
    PyObject *m_declarations;
    PyObject *m_dictionary;
    string m_mainfile;

    bool m_onlymain;
};
