// File: dumper.hh
// Dumper is a visitor similar to Formatter/DUMP.py

#ifndef H_SYNOPSIS_CPP_DUMPER
#define H_SYNOPSIS_CPP_DUMPER

#include "ast.hh"
#include "type.hh"

//. Dumper displays the AST to the screen
class Dumper : public AST::Visitor, public Type::Visitor {
public:
    Dumper();

    void onlyShow(string fname);

    //
    // Type Visitor
    //
    string format(Type::Type*);
    virtual void visitType(Type::Type*);
    virtual void visitForward(Type::Forward*);
    virtual void visitModifier(Type::Modifier*);
    virtual void visitNamed(Type::Named*);
    virtual void visitBase(Type::Base*);
    virtual void visitDeclared(Type::Declared*);
    virtual void visitTemplateType(Type::Template*);
    virtual void visitParameterized(Type::Parameterized*);
    virtual void visitFuncPtr(Type::FuncPtr*);

    //
    // AST Visitor
    //
    void visit(const vector<AST::Declaration*>&);
    string format(AST::Parameter*);
    void visit(const vector<AST::Comment*>&);
    virtual void visitDeclaration(AST::Declaration*);
    virtual void visitScope(AST::Scope*);
    virtual void visitNamespace(AST::Namespace*);
    virtual void visitClass(AST::Class*);
    virtual void visitOperation(AST::Operation*);
    virtual void visitVariable(AST::Variable*);
    virtual void visitTypedef(AST::Typedef*);
    virtual void visitEnum(AST::Enum*);
    virtual void visitEnumerator(AST::Enumerator*);

private:
    //. The indent depth
    int m_indent;
    //. The indent string
    string m_indent_string;
    //. Increases indent
    void indent();
    //. Decreases indent
    void undent();
    //. The Type String
    string m_type;
    //. The current scope name
    AST::Name m_scope;
    //. Returns the given Name relative to the current scope
    string colonate(const AST::Name& name);
    //. Only show this filename, if set
    string m_filename;

};

#endif // header guard
