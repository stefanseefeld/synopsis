#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <stack>
#include PYTHON_INCLUDE
#include "ast.hh"
#include "type.hh"

#if DEBUG
#define DO_TRACE
class Trace
{
public:
    Trace(const std::string &s) : scope(s) { std::cout << indent() << "entering " << scope << std::endl; ++level; }
    ~Trace() { --level; std::cout << indent() << "leaving " << scope << std::endl; }
private:
    std::string indent() { return std::string(level, ' '); }
    static int level;
    std::string scope;
};
#else
class Trace
{
public:
    Trace(const std::string &) {}
    ~Trace() {}
};
#endif

//. The Synopsis class maps from C++ objects to Python objects
class Synopsis : public AST::Visitor, public Type::Visitor {
public:

    Synopsis(const std::string &mainfile, PyObject *decls, PyObject *types);
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
    PyObject *Array(Type::Array*);
    PyObject *Parameterized(Type::Parameterized*);
    PyObject *FuncPtr(Type::FuncPtr*);

    //
    // types from the Synopsis.AST module
    //
    PyObject *Declaration(AST::Declaration*);
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
    void visitDeclaration(AST::Declaration*);
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
    void visitArray(Type::Array*);
    //void visitNamed(Type::Named*);
    void visitBase(Type::Base*);
    void visitDeclared(Type::Declared*);
    void visitTemplateType(Type::Template*);
    void visitParameterized(Type::Parameterized*);
    void visitFuncPtr(Type::FuncPtr*);

private:
    //. Compiler Firewalled private data
    struct Private;
    friend struct Private;
    Private* m;

    //.
    //. helper methods
    //.
    void addComments(PyObject* pydecl, AST::Declaration* cdecl);

    ///////// EVERYTHING BELOW HERE SUBJECT TO REVIEW AND DELETION


    /*
    PyObject *lookupType(const std::string &, PyObject *);
    PyObject *lookupType(const std::string &);
    PyObject *lookupType(const std::vector<std::string>& qualified);

    static void addInheritance(PyObject *, const std::vector<PyObject *> &);
    static PyObject *N2L(const std::string &);
    static PyObject *V2L(const std::vector<std::string> &);
    static PyObject *V2L(const std::vector<PyObject *> &);
    static PyObject *V2L(const std::vector<size_t> &);
    void pushClassBases(PyObject* clas);
    PyObject* resolveDeclared(PyObject*);
    void addDeclaration(PyObject *);
    */
private:
    PyObject *m_ast;
    PyObject *m_type;
    PyObject *m_declarations;
    PyObject *m_dictionary;
    std::string m_mainfile;

    bool m_onlymain;
};
