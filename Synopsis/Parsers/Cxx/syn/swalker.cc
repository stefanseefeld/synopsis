// File: swalker.cc
//
// SWalker class

#include <iostream.h>
#include <string>

#include "ptree.h"

#include "swalker.hh"
#include "ast.hh"
#include "builder.hh"

using namespace AST;
using std::string;

#if DEBUG
#define DO_TRACE
class Trace
{
public:
    Trace(const string &s) : scope(s) { cout << indent() << "entering " << scope << endl; ++level; }
    ~Trace() { --level; cout << indent() << "leaving  " << scope << endl; }
private:
    string indent() { return string(level, ' '); }
    static int level;
    string scope;
};
int Trace::level = 0;
#else
class Trace
{
public:
    Trace(const string &) {}
    ~Trace() {}
};
#endif


SWalker::SWalker(Parser* parser, Builder* builder)
    : Walker(parser)
{
    m_parser = parser;
    m_builder = builder;
}

string SWalker::getName(Ptree *node)
{
    Trace trace("SWalker::getName");
    if (node && node->IsLeaf())
        return string(node->GetPosition(), node->GetLength());
    return node->ToString();
}

Ptree* SWalker::TranslateArgDeclList(bool, Ptree*, Ptree*) { Trace trace("SWalker::TranslateArgDeclList NYI"); return 0; }
Ptree* SWalker::TranslateInitializeArgs(PtreeDeclarator*, Ptree*) { Trace trace("SWalker::TranslateInitializeArgs NYI"); return 0; }
Ptree* SWalker::TranslateAssignInitializer(PtreeDeclarator*, Ptree*) { Trace trace("SWalker::TranslateAssignInitializer NYI"); return 0; }
//Class* SWalker::MakeClassMetaobject(Ptree*, Ptree*, Ptree*) { Trace trace("SWalker::MakeClassMetaobject NYI"); return 0; }
//Ptree* SWalker::TranslateClassSpec(Ptree*, Ptree*, Ptree*, Class*) { Trace trace("SWalker::TranslateClassSpec NYI"); return 0; }
//Ptree* SWalker::TranslateClassBody(Ptree*, Ptree*, Class*) { Trace trace("SWalker::TranslateClassBody NYI"); return 0; }
//Ptree* SWalker::TranslateTemplateInstantiation(Ptree*, Ptree*, Ptree*, Class*) { Trace trace("SWalker::TranslateTemplateInstantiation NYI"); return 0; }

//
// Translate Methods
//

// Default translate
Ptree* SWalker::TranslatePtree(Ptree*) {
    cout << "Translate Ptree" << endl;
    return 0;
}

//. NamespaceSpec
//. [ namespace <identifier> [{ body }] ]
Ptree* SWalker::TranslateNamespaceSpec(Ptree* def) {
    Trace trace("SWalker::TranslateNamespaceSpec");
    
    string name = getName(def->Cadr());
    /*Namespace* ns =*/ m_builder->startNamespace(name);

    Translate(Ptree::Third(def));

    m_builder->endNamespace();

    return 0;
}

Ptree* SWalker::TranslateClassSpec(Ptree* node) 
{
    Trace trace("SWalker::TranslateClassSpec");
    if (Ptree::Length(node) == 4) {
	// if Class definition (not just declaration)
	//bool is_struct = node->First()->Eq("struct");
	
	// Create AST.Class object
        string type = getName(node->First());
        string name = getName(node->Second());
        /*AST::Class *clas =*/ m_builder->startClass(type, name);
	//PtreeClassSpec* cspec = static_cast<PtreeClassSpec*>(node);
	//addComments(clas, cspec->GetComments());

        // Add parents to Class object
        //vector<Inheritance*> parents = TranslateInheritanceSpec(node->Nth(2));
        //Synopsis::addInheritance(clas, parents);

        // Translate the body of the class
        //Class* meta = MakeClassMetaobject(node, NULL, node);
        //synopsis->pushClass(clas);
	//synopsis->pushAccess(is_struct ? Synopsis::Public : Synopsis::Private);
        //TranslateClassBody(node->Nth(3), node->Nth(2), meta);
	TranslateBlock(node->Nth(3));
	//synopsis->popAccess();
        //synopsis->popScope();
	m_builder->endClass();

    }
    return 0;
}

//. Block
//. [ { [ <statement>* ] } ]
Ptree* SWalker::TranslateBlock(Ptree* block) {
    Trace trace("SWalker::TranslateBlock");
    Ptree* rest = Ptree::Second(block);
    while (rest != nil) {
	Translate(rest->Car());
	rest = rest->Cdr();
    }
    return 0;
}

//. Brace
//. [ { [ <statement>* ] } ]
Ptree* SWalker::TranslateBrace(Ptree* brace) 
{
    Trace trace("SWalker::TranslateBrace");
    Ptree* rest = Ptree::Second(brace);
    while (rest != nil) {
	Translate(rest->Car());
	rest = rest->Cdr();
    }
    return 0;
}

//. TemplateDecl
//. [ template < [types] > [decl] ]
Ptree* SWalker::TranslateTemplateDecl(Ptree* def) 
{
    Trace trace("SWalker::TranslateTemplateDecl");
    Ptree* body = Ptree::Nth(def, 4);
    Ptree* class_spec = GetClassTemplateSpec(body);
    if(class_spec->IsA(ntClassSpec))
	TranslateTemplateClass(def, class_spec);
    else
	TranslateTemplateFunction(def, body);
    return 0;
}

//. Translates a declaration, either variable, typedef or function
//. Variables:
//.  [ [modifiers] name [declarators] ; ]
//. Function prototype:
//.  [ [modifiers] name [declarators] ; ]
//. Function impl:
//.  [ [modifiers] name declarator [ { ... } ] ]
//. Typedef:
//.  ?
//. Class definition:
//.  [ [modifiers] [class foo ...] [declarators]? ; ]
Ptree* SWalker::TranslateDeclaration(Ptree* def) 
{
    Trace trace("SWalker::TranslateDeclaration");
    def->Display(); cout << endl;
    Ptree* decls = Ptree::Third(def);
    if (decls->IsA(ntDeclarator))	// if it is a function
	TranslateFunctionImplementation(def);
    else {
	// if it is a function prototype or a variable declaration.
	/////TranslateStorageSpecifiers(Ptree::First(def));
	// Typespecifier may be a class {} etc.
	TranslateTypespecifier(Ptree::Second(def));
	if (!decls->IsLeaf())	// if it is not ";"
	    TranslateDeclarators(decls);
    }
    return 0;
}

//. [ [ declarator { = <expr> } ] , ... ]
Ptree* SWalker::TranslateDeclarators(Ptree* decls) 
{
    Trace trace("SWalker::TranslateDeclarators");
    Ptree* rest = decls, /* *exp,*/ *p;
    //int len;
    while (rest != nil) {
	p = rest->Car();
	if (p->IsA(ntDeclarator)) {
	    /*
	    len = p->Length();
	    if (len >= 2 && p->Nth(len - 2)->Eq('=')) {
		exp = p->ListTail(len - 2);
		TranslateAssignInitializer((PtreeDeclarator*)p, exp);
	    } else {
		Ptree* last = p->Last()->Car();
		if(last != nil && !last->IsLeaf() && last->Car()->Eq('(')){
		    // Function style initialiser
		    TranslateInitializeArgs((PtreeDeclarator*)p, last);
		}
	    }
	    */

	    TranslateDeclarator(p);
	} // if. There is no else..?
	rest = rest->Cdr();
	// Skip comma
	if (rest != nil) rest = rest->Cdr();
    }
    return 0;
}

//. TranslateDeclarator
//. Function proto:
//.   [ { * | & }* name ( [params] ) ]
//. param:
//.   [ [types] { [ { * | & }* name ] { = value } } ]
Ptree* SWalker::TranslateDeclarator(Ptree* decl) 
{
    Trace trace("SWalker::TranslateDeclarator *** NYI");
    // Insert code from occ.cc here
    decl->Display(); cout << endl;
    return 0;
}

//. Class or Enum
Ptree* SWalker::TranslateTypespecifier(Ptree* tspec) 
{
    Trace trace("SWalker::TranslateTypespecifier");
    Ptree *class_spec = GetClassOrEnumSpec(tspec);
    if (class_spec) Translate(class_spec);
    return 0;
}

Ptree* SWalker::TranslateTypedef(Ptree*) { Trace trace("SWalker::TranslateTypedef NYI"); return 0; }
Ptree* SWalker::TranslateTemplateInstantiation(Ptree*) { Trace trace("SWalker::TranslateTemplateInstantiation NYI"); return 0; }
Ptree* SWalker::TranslateExternTemplate(Ptree*) { Trace trace("SWalker::TranslateExternTemplate NYI"); return 0; }
Ptree* SWalker::TranslateTemplateClass(Ptree*, Ptree*) { Trace trace("SWalker::TranslateTemplateClass NYI"); return 0; }
Ptree* SWalker::TranslateTemplateFunction(Ptree*, Ptree*) { Trace trace("SWalker::TranslateTemplateFunction NYI"); return 0; }
Ptree* SWalker::TranslateMetaclassDecl(Ptree*) { Trace trace("SWalker::TranslateMetaclassDecl NYI"); return 0; }
Ptree* SWalker::TranslateLinkageSpec(Ptree*) { Trace trace("SWalker::TranslateLinkageSpec NYI"); return 0; }
Ptree* SWalker::TranslateUsing(Ptree*) { Trace trace("SWalker::TranslateUsing NYI"); return 0; }

Ptree* SWalker::TranslateStorageSpecifiers(Ptree*) { Trace trace("SWalker::TranslateStorageSpecifiers NYI"); return 0; }
Ptree* SWalker::TranslateFunctionImplementation(Ptree*) { Trace trace("SWalker::TranslateFunctionImplementation NYI"); return 0; }
Ptree* SWalker::TranslateFunctionBody(Ptree*) { Trace trace("SWalker::TranslateFunctionBody NYI"); return 0; }

Ptree* SWalker::TranslateEnumSpec(Ptree*) { Trace trace("SWalker::TranslateEnumSpec NYI"); return 0; }

Ptree* SWalker::TranslateAccessSpec(Ptree*) { Trace trace("SWalker::TranslateAccessSpec NYI"); return 0; }
Ptree* SWalker::TranslateAccessDecl(Ptree*) { Trace trace("SWalker::TranslateAccessDecl NYI"); return 0; }
Ptree* SWalker::TranslateUserAccessSpec(Ptree*) { Trace trace("SWalker::TranslateUserAccessSpec NYI"); return 0; }

Ptree* SWalker::TranslateIf(Ptree*) { Trace trace("SWalker::TranslateIf NYI"); return 0; }
Ptree* SWalker::TranslateSwitch(Ptree*) { Trace trace("SWalker::TranslateSwitch NYI"); return 0; }
Ptree* SWalker::TranslateWhile(Ptree*) { Trace trace("SWalker::TranslateWhile NYI"); return 0; }
Ptree* SWalker::TranslateDo(Ptree*) { Trace trace("SWalker::TranslateDo NYI"); return 0; }
Ptree* SWalker::TranslateFor(Ptree*) { Trace trace("SWalker::TranslateFor NYI"); return 0; }
Ptree* SWalker::TranslateTry(Ptree*) { Trace trace("SWalker::TranslateTry NYI"); return 0; }
Ptree* SWalker::TranslateBreak(Ptree*) { Trace trace("SWalker::TranslateBreak NYI"); return 0; }
Ptree* SWalker::TranslateContinue(Ptree*) { Trace trace("SWalker::TranslateContinue NYI"); return 0; }
Ptree* SWalker::TranslateReturn(Ptree*) { Trace trace("SWalker::TranslateReturn NYI"); return 0; }
Ptree* SWalker::TranslateGoto(Ptree*) { Trace trace("SWalker::TranslateGoto NYI"); return 0; }
Ptree* SWalker::TranslateCase(Ptree*) { Trace trace("SWalker::TranslateCase NYI"); return 0; }
Ptree* SWalker::TranslateDefault(Ptree*) { Trace trace("SWalker::TranslateDefault NYI"); return 0; }
Ptree* SWalker::TranslateLabel(Ptree*) { Trace trace("SWalker::TranslateLabel NYI"); return 0; }
Ptree* SWalker::TranslateExprStatement(Ptree*) { Trace trace("SWalker::TranslateExprStatement NYI"); return 0; }

Ptree* SWalker::TranslateComma(Ptree*) { Trace trace("SWalker::TranslateComma NYI"); return 0; }
Ptree* SWalker::TranslateAssign(Ptree*) { Trace trace("SWalker::TranslateAssign NYI"); return 0; }
Ptree* SWalker::TranslateCond(Ptree*) { Trace trace("SWalker::TranslateCond NYI"); return 0; }
Ptree* SWalker::TranslateInfix(Ptree*) { Trace trace("SWalker::TranslateInfix NYI"); return 0; }
Ptree* SWalker::TranslatePm(Ptree*) { Trace trace("SWalker::TranslatePm NYI"); return 0; }
Ptree* SWalker::TranslateCast(Ptree*) { Trace trace("SWalker::TranslateCast NYI"); return 0; }
Ptree* SWalker::TranslateUnary(Ptree*) { Trace trace("SWalker::TranslateUnary NYI"); return 0; }
Ptree* SWalker::TranslateThrow(Ptree*) { Trace trace("SWalker::TranslateThrow NYI"); return 0; }
Ptree* SWalker::TranslateSizeof(Ptree*) { Trace trace("SWalker::TranslateSizeof NYI"); return 0; }
Ptree* SWalker::TranslateNew(Ptree*) { Trace trace("SWalker::TranslateNew NYI"); return 0; }
Ptree* SWalker::TranslateNew3(Ptree* type) { Trace trace("SWalker::TranslateNew3 NYI"); return 0; }
Ptree* SWalker::TranslateDelete(Ptree*) { Trace trace("SWalker::TranslateDelete NYI"); return 0; }
Ptree* SWalker::TranslateThis(Ptree*) { Trace trace("SWalker::TranslateThis NYI"); return 0; }
Ptree* SWalker::TranslateVariable(Ptree*) { Trace trace("SWalker::TranslateVariable NYI"); return 0; }
Ptree* SWalker::TranslateFstyleCast(Ptree*) { Trace trace("SWalker::TranslateFstyleCast NYI"); return 0; }
Ptree* SWalker::TranslateArray(Ptree*) { Trace trace("SWalker::TranslateArray NYI"); return 0; }
Ptree* SWalker::TranslateFuncall(Ptree*) { Trace trace("SWalker::TranslateFuncall NYI"); return 0; }	// and fstyle cast
Ptree* SWalker::TranslatePostfix(Ptree*) { Trace trace("SWalker::TranslatePostfix NYI"); return 0; }
Ptree* SWalker::TranslateUserStatement(Ptree*) { Trace trace("SWalker::TranslateUserStatement NYI"); return 0; }
Ptree* SWalker::TranslateDotMember(Ptree*) { Trace trace("SWalker::TranslateDotMember NYI"); return 0; }
Ptree* SWalker::TranslateArrowMember(Ptree*) { Trace trace("SWalker::TranslateArrowMember NYI"); return 0; }
Ptree* SWalker::TranslateParen(Ptree*) { Trace trace("SWalker::TranslateParen NYI"); return 0; }
Ptree* SWalker::TranslateStaticUserStatement(Ptree*) { Trace trace("SWalker::TranslateStaticUserStatement NYI"); return 0; }


