// File: dumper.hh
// Dumper is a visitor similar to Formatter/DUMP.py

#ifndef H_SYNOPSIS_CPP_DUMPER
#define H_SYNOPSIS_CPP_DUMPER

#include "ast.hh"
#include "type.hh"

//. Formats Types in a way suitable for output
class TypeFormatter : public Type::Visitor {
public:
    TypeFormatter();

    //. Sets the current scope
    void setScope(const AST::Name& scope);

    //
    // Type Visitor
    //
    //. Returns a formatter string for given type
    std::string format(const Type::Type*);
    virtual void visitType(Type::Type*);
    virtual void visitUnknown(Type::Unknown*);
    virtual void visitModifier(Type::Modifier*);
    virtual void visitNamed(Type::Named*);
    virtual void visitBase(Type::Base*);
    virtual void visitDeclared(Type::Declared*);
    virtual void visitTemplateType(Type::Template*);
    virtual void visitParameterized(Type::Parameterized*);
    virtual void visitFuncPtr(Type::FuncPtr*);

protected:
    //. The Type String
    std::string m_type;
    //. The current scope name
    AST::Name m_scope;
    //. Returns the given Name relative to the current scope
    std::string colonate(const AST::Name& name);
};

//. Dumper displays the AST to the screen
class Dumper : public AST::Visitor, public TypeFormatter {
public:
    Dumper();

    //. Sets to only show decls with given filename
    void onlyShow(const std::string &fname);
    
    std::string formatParam(AST::Parameter*);
    //
    // AST Visitor
    //
    void visit(const std::vector<AST::Declaration*>&);
    void visit(const std::vector<AST::Comment*>&);
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
    std::string m_indent_string;
    //. Increases indent
    void indent();
    //. Decreases indent
    void undent();
    //. Only show this filename, if set
    std::string m_filename;

};

#endif // header guard
