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
#undef Scope

#include <vector>

// Forward declarations
class ostream;
class Builder; class Program;
class Decoder; class TypeFormatter;

namespace AST { class Parameter; class Inheritance; class Declaration; 
    class Operation; class Scope; }
namespace Type { class Type; }

//. A walker that creates an AST. All Translate* methods have been overridden
//. to remove the translation code.
class SWalker : public Walker {
public:
    //. Constructor
    SWalker(const std::string &source, Parser*, Builder*, Program*);
    virtual ~SWalker() {}

    //. Sets extract tails to true.
    //. This will cause the parser to create dummy declarations for comments
    //. before close braces or the end of the file
    void setExtractTails(bool value) { m_extract_tails = value; }
    //. Sets store links to true.
    //. This will cause the whole ptree to be traversed, and any linkable
    //. identifiers found will be stored
    void setStoreLinks(bool value, std::ostream* storage) { m_store_links = value; m_storage = storage; }

    //. Get a name from the ptree
    std::string getName(Ptree*) const;

    //. Update the line number
    void updateLineNumber(Ptree*);

    //. Store a link at the given node
    void storeLink(Ptree* node, bool def, const std::vector<std::string> &name);
    //. Store a link at the given node, if the type is amenable to it
    void storeLink(Ptree* node, bool def, Type::Type*);
    //. Store a span of the given class at the given node
    void storeSpan(Ptree* node, const char* clas);
    //. Sotre a possibly multi-line span of the given class at the given node
    void storeLongSpan(Ptree* node, const char* clas);

    void addComments(AST::Declaration* decl, Ptree* comments);
    void addComments(AST::Declaration* decl, CommentedLeaf* node);
    void addComments(AST::Declaration* decl, PtreeDeclarator* node);
    void addComments(AST::Declaration* decl, PtreeDeclaration* decl);
    void addComments(AST::Declaration* decl, PtreeNamespaceSpec* decl);

    // Takes the (maybe nil) args list and puts them in m_params
    void TranslateFunctionArgs(Ptree* args);
    void TranslateParameters(Ptree* p_params, std::vector<AST::Parameter*>& params);
    void TranslateFunctionName(char* encname, std::string& realname, Type::Type*& returnType);
    virtual Ptree* TranslateDeclarator(Ptree*);
    void TranslateTypedefDeclarator(Ptree* node);
    std::vector<AST::Inheritance*> TranslateInheritanceSpec(Ptree *node);
    //. Returns a formatter string of the parameters. The idea is that this
    //. string will be appended to the function name to form the 'name' of the
    //. function.
    std::string formatParameters(std::vector<AST::Parameter*>& params);

    // default translation
    virtual Ptree* TranslatePtree(Ptree*);

    //. Overridden to catch exceptions
    void Translate(Ptree*);

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

    std::string current_file() const { return m_filename;}
    int         current_lineno() const { return m_lineno;}
    std::string main_file() const { return m_source;}
    static SWalker *instance() { return g_swalker;}
private:
    // the 'current' walker is a debugging aid.
    static SWalker* g_swalker;

    Parser* m_parser;
    Program* m_program;
    Builder* m_builder;
    Decoder* m_decoder;

    PtreeDeclaration* m_declaration;
    //. This pointer is used as a comparison to avoid creating new strings
    char* m_filename_ptr;
    //. The current filename as string. This way refcounting will be used
    std::string m_filename;
    int m_lineno;
    //. The source filename
    std::string m_source;

    //. True if should try and extract tail comments before }'s
    bool m_extract_tails;
    //. True if should parse whole ptree and store links
    bool m_store_links;
    //. Storage for links
    std::ostream* m_storage;
    //. True if this TranslateDeclarator should try to store the decl type
    bool m_store_decl;

    //. A dummy name used for tail comments
    std::vector<std::string> m_dummyname;

    //. An instance of TypeFormatter for formatting types
    TypeFormatter* m_type_formatter;

    //. The current operation, if in a function block
    AST::Operation* m_operation;
    //. The params found before a function block. These may be different from
    //. the ones that are in the original declaration(s), but it is these names
    //. we need for referencing names inside the block, so a reference is stored
    //. here.
    std::vector<AST::Parameter*> m_params;
    //. The type returned from the expression-type translators
    Type::Type* m_type;
    //. The Scope to use for name lookups, or NULL to use enclosing default
    //. scope rules. This is for when we are at a Variable, and already know it
    //. must be part of a given class (eg, foo->bar .. bar must be in foo's
    //. class)
    AST::Scope* m_scope;

    //. The state of postfix translation. This is needed for constructs like
    //. foo->var versus var or foo->var(). The function call resolution needs
    //. to be done in the final TranslateVariable, since that's where the last
    //. name (which is to be linked) is handled.
    enum Postfix_Flag {
	Postfix_Var, //.< Lookup as a variable
	Postfix_Func, //.< Lookup as a function, using m_params for parameters
    } m_postfix_flag;

    //. Info about one stored function impl. Function impls must be stored
    //. till the end of a class.
    struct FuncImplCache {
	AST::Operation* oper;
	std::vector<AST::Parameter*> params;
	Ptree* body;
    };
    //. A vector of function impls
    typedef std::vector<FuncImplCache> FuncImplVec;
    //. A stack of function impl vectors
    typedef std::vector<FuncImplVec> FuncImplStack;
    //. The stack of function impl vectors
    FuncImplStack m_func_impl_stack;
    void SWalker::TranslateFuncImplCache(const FuncImplCache& cache);

    //. Finds the column given the start ptr and the current position. The
    //. derived column number is processed with the link_map before returning,
    //. so -1 may be returned to indicate "inside macro".
    int find_col(const char* start, const char* find);

}; // class SWalker


#endif // header guard
