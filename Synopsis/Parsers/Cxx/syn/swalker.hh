// File: swalker.h
// This file contains the class SWalker, which is a subclass of OCC's Walker
// class that overrides *all* the Translate* methods to remove the translating
// aspects of the code. The cpu saving in doing this is minimal, since the
// translation code is always skipped with the first if(), and the return
// Ptree* is still there. Removing this would require changing the OCC source
// which I am not prepared to do yet.
//
// @author Stephen Davies

#ifndef H_SYNOPSIS_CPP_SWALKER
#define H_SYNOPSIS_CPP_SWALKER

#include "../walker.h"

#include <vector>
using std::vector;

// Forward declarations
class ostream;
class Builder; class Program;
class Decoder; class TypeFormatter;

namespace AST { class Parameter; class Inheritance; class Declaration; }
namespace Type { class Type; }

//. A walker that creates an AST. All Translate* methods have been overridden
//. to remove the translation code.
class SWalker : public Walker {
public:
    //. Constructor
    SWalker(string source, Parser*, Builder*, Program*);
    virtual ~SWalker() {}

    //. Sets extract tails to true.
    //. This will cause the parser to create dummy declarations for comments
    //. before close braces or the end of the file
    void setExtractTails(bool value) { m_extract_tails = value; }
    //. Sets store links to true.
    //. This will cause the whole ptree to be traversed, and any linkable
    //. identifiers found will be stored
    void setStoreLinks(bool value, ostream* storage) { m_store_links = value; m_storage = storage; }

    //. Get a name from the ptree
    string getName(Ptree*);

    //. Update the line number
    void updateLineNumber(Ptree*);

    //. Store a link at the given node
    void storeLink(Ptree* node, bool def, const vector<string>& name);
    //. Store a link at the given node, if the type is amenable to it
    void storeLink(Ptree* node, bool def, Type::Type*);
    //. Store a span of the given class at the given node
    void storeSpan(Ptree* node, const char* clas);

    void addComments(AST::Declaration* decl, Ptree* comments);
    void addComments(AST::Declaration* decl, CommentedLeaf* node);
    void addComments(AST::Declaration* decl, PtreeDeclarator* node);
    void addComments(AST::Declaration* decl, PtreeDeclaration* decl);
    void addComments(AST::Declaration* decl, PtreeNamespaceSpec* decl);

    void TranslateParameters(Ptree* p_params, vector<AST::Parameter*>& params);
    void TranslateFunctionName(char* encname, string& realname, Type::Type*& returnType);
    virtual Ptree* TranslateDeclarator(Ptree*);
    void TranslateTypedefDeclarator(Ptree* node);
    vector<AST::Inheritance*> TranslateInheritanceSpec(Ptree *node);
    //. Returns a formatter string of the parameters. The idea is that this
    //. string will be appended to the function name to form the 'name' of the
    //. function.
    string formatParameters(vector<AST::Parameter*>& params);

    // default translation
    virtual Ptree* TranslatePtree(Ptree*);

    virtual Ptree* TranslateTypedef(Ptree*);
    virtual Ptree* TranslateTemplateDecl(Ptree*);
    virtual Ptree* TranslateTemplateInstantiation(Ptree*);
    //virtual Ptree* TranslateTemplateInstantiation(Ptree*, Ptree*, Ptree*, Class*);
    virtual Ptree* TranslateExternTemplate(Ptree*);
    virtual Ptree* TranslateTemplateClass(Ptree*, Ptree*);
    virtual Ptree* TranslateTemplateFunction(Ptree*, Ptree*);
    virtual Ptree* TranslateMetaclassDecl(Ptree*);
    virtual Ptree* TranslateLinkageSpec(Ptree*);
    virtual Ptree* TranslateNamespaceSpec(Ptree*);
    virtual Ptree* TranslateUsing(Ptree*);
    virtual Ptree* TranslateDeclaration(Ptree*);
    virtual Ptree* TranslateStorageSpecifiers(Ptree*);
    virtual Ptree* TranslateDeclarators(Ptree*);
    virtual Ptree* TranslateArgDeclList(bool, Ptree*, Ptree*);
    virtual Ptree* TranslateInitializeArgs(PtreeDeclarator*, Ptree*);
    virtual Ptree* TranslateAssignInitializer(PtreeDeclarator*, Ptree*);
    virtual Ptree* TranslateFunctionImplementation(Ptree*);
    virtual Ptree* TranslateFunctionBody(Ptree*);
    virtual Ptree* TranslateBrace(Ptree*);
    virtual Ptree* TranslateBlock(Ptree*);
    //virtual Ptree* TranslateClassBody(Ptree*, Ptree*, Class*);

    virtual Ptree* TranslateClassSpec(Ptree*);
    //virtual Class* MakeClassMetaobject(Ptree*, Ptree*, Ptree*);
    //virtual Ptree* TranslateClassSpec(Ptree*, Ptree*, Ptree*, Class*);
    virtual Ptree* TranslateEnumSpec(Ptree*);

    virtual Ptree* TranslateAccessSpec(Ptree*);
    virtual Ptree* TranslateAccessDecl(Ptree*);
    virtual Ptree* TranslateUserAccessSpec(Ptree*);

    virtual Ptree* TranslateIf(Ptree*);
    virtual Ptree* TranslateSwitch(Ptree*);
    virtual Ptree* TranslateWhile(Ptree*);
    virtual Ptree* TranslateDo(Ptree*);
    virtual Ptree* TranslateFor(Ptree*);
    virtual Ptree* TranslateTry(Ptree*);
    virtual Ptree* TranslateBreak(Ptree*);
    virtual Ptree* TranslateContinue(Ptree*);
    virtual Ptree* TranslateReturn(Ptree*);
    virtual Ptree* TranslateGoto(Ptree*);
    virtual Ptree* TranslateCase(Ptree*);
    virtual Ptree* TranslateDefault(Ptree*);
    virtual Ptree* TranslateLabel(Ptree*);
    virtual Ptree* TranslateExprStatement(Ptree*);

    virtual Ptree* TranslateTypespecifier(Ptree*);

    virtual Ptree* TranslateComma(Ptree*);
    virtual Ptree* TranslateAssign(Ptree*);
    virtual Ptree* TranslateCond(Ptree*);
    virtual Ptree* TranslateInfix(Ptree*);
    virtual Ptree* TranslatePm(Ptree*);
    virtual Ptree* TranslateCast(Ptree*);
    virtual Ptree* TranslateUnary(Ptree*);
    virtual Ptree* TranslateThrow(Ptree*);
    virtual Ptree* TranslateSizeof(Ptree*);
    virtual Ptree* TranslateNew(Ptree*);
    virtual Ptree* TranslateNew3(Ptree* type);
    virtual Ptree* TranslateDelete(Ptree*);
    virtual Ptree* TranslateThis(Ptree*);
    virtual Ptree* TranslateVariable(Ptree*);
    virtual Ptree* TranslateFstyleCast(Ptree*);
    virtual Ptree* TranslateArray(Ptree*);
    virtual Ptree* TranslateFuncall(Ptree*);	// and fstyle cast
    virtual Ptree* TranslatePostfix(Ptree*);
    virtual Ptree* TranslateUserStatement(Ptree*);
    virtual Ptree* TranslateDotMember(Ptree*);
    virtual Ptree* TranslateArrowMember(Ptree*);
    virtual Ptree* TranslateParen(Ptree*);
    virtual Ptree* TranslateStaticUserStatement(Ptree*);

private:
    Parser* m_parser;
    Program* m_program;
    Builder* m_builder;
    Decoder* m_decoder;

    PtreeDeclaration* m_declaration;
    //. This pointer is used as a comparison to avoid creating new strings
    char* m_filename_ptr;
    //. The current filename as string. This way refcounting will be used
    string m_filename;
    int m_lineno;
    //. The source filename
    string m_source;

    //. True if should try and extract tail comments before }'s
    bool m_extract_tails;
    //. True if should parse whole ptree and store links
    bool m_store_links;
    //. Storage for links
    ostream* m_storage;
    //. True if this TranslateDeclarator should try to store the decl type
    bool m_store_decl;

    //. A dummy name used for tail comments
    vector<string> m_dummyname;

    //. An instance of TypeFormatter for formatting types
    TypeFormatter* m_type_formatter;

}; // class SWalker


#endif // header guard
