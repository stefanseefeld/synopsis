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

    void visit(const vector<AST::Declaration*>&);
    virtual void visitDeclaration(AST::Declaration*);
    virtual void visitScope(AST::Scope*);
    virtual void visitNamespace(AST::Namespace*);
    virtual void visitClass(AST::Class*);
    virtual void visitOperation(AST::Operation*);

private:
    //. The indent depth
    int m_indent;
    //. The indent string
    string m_indent_string;
    //. Increases indent
    void indent();
    //. Decreases indent
    void undent();

};

#endif // header guard
