// File: swalker.cc
//
// SWalker class

#include <iostream.h>
#include <string>

#include "ptree.h"
#include "parse.h"

#include "swalker.hh"
#include "type.hh"
#include "ast.hh"
#include "builder.hh"
#include "decoder.hh"

using namespace AST;
using std::string;

#if 0 && defined(DEBUG)
#define DO_TRACE
class Trace
{
public:
    Trace(const string &s) : scope(s) { cout << indent() << "entering " << scope << endl; ++slevel; }
    ~Trace() { --slevel; cout << indent() << "leaving  " << scope << endl; }
private:
    string indent() { return string(slevel, ' '); }
    static int slevel;
    string scope;
};
int Trace::slevel = 0;
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
    m_decoder = new Decoder(builder);
    m_filename_ptr = 0;
    m_extract_tails = false;
}

string SWalker::getName(Ptree *node)
{
    // Trace trace("SWalker::getName");
    if (node && node->IsLeaf())
        return string(node->GetPosition(), node->GetLength());
    return node->ToString();
}

void SWalker::updateLineNumber(Ptree* ptree)
{
    // Ask the Parser for the linenumber of the ptree. This used to be
    // expensive until I hacked buffer.cc to cache the last line number found.
    // Now it's okay as long as you are looking for lines sequentially.
    char* fname;
    int fname_len;
    m_lineno = m_parser->LineNumber(ptree->LeftMost(), fname, fname_len);
    if (fname != m_filename_ptr) {
	m_filename_ptr = fname;
	m_filename.assign(fname, fname_len);
	//cout << "### fname changed to "<<m_filename<<" at "<<m_lineno<<endl;
	m_builder->setFilename(m_filename);
    }
}


void SWalker::addComments(AST::Declaration* decl, Ptree* comments)
{
    while (comments) {
	decl->comments().push_back(new AST::Comment("", 0, comments->First()->ToString()));
	comments = comments->Rest();
    }
}

void SWalker::addComments(AST::Declaration* decl, CommentedLeaf* node)
{
    if (node) addComments(decl, node->GetComments());
}
void SWalker::addComments(AST::Declaration* decl, PtreeDeclaration* node)
{
    if (decl) addComments(decl, node->GetComments());
}
void SWalker::addComments(AST::Declaration* decl, PtreeDeclarator* node)
{
    if (decl) addComments(decl, node->GetComments());
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
    
    updateLineNumber(def);
    string name = getName(def->Cadr());
    /*Namespace* ns =*/ m_builder->startNamespace(name);

    Translate(Ptree::Third(def));

    m_builder->endNamespace();

    return 0;
}

vector<Inheritance*> SWalker::TranslateInheritanceSpec(Ptree *node)
{
    Trace trace("PyWalker::TranslateInheritanceSpec");
    vector<Inheritance*> ispec;
    Type::Type *type;
    while (node) {
        node = node->Cdr();		// skip : or ,
        // the attributes
        vector<string> attributes(node->Car()->Length() - 1);
        for (int i = 0; i != node->Car()->Length() - 1; ++i)
            attributes[i] = getName(node->Car()->Nth(i));
        // look up the parent type
	Ptree* name = node->Car()->Last()->Car();
	if (name->IsLeaf()) {
	    type = m_builder->lookupType(getName(node->Car()->Last()->Car()));
	} else {
	    char* encname = name->GetEncodedName();
	    m_decoder->init(encname);
	    type = m_decoder->decodeType();
	}

        node = node->Cdr();
        // add it to the list
        ispec.push_back(new AST::Inheritance(type, attributes));
    }
    return ispec;
}


Ptree* SWalker::TranslateClassSpec(Ptree* node) 
{
    Trace trace("SWalker::TranslateClassSpec");
    if (Ptree::Length(node) == 4) {
	// if Class definition (not just declaration)
	updateLineNumber(node);
	
	// Create AST.Class object
	AST::Class *clas;
        string type = getName(node->First());
        vector<Inheritance*> parents = TranslateInheritanceSpec(node->Nth(2));
	char* encname = node->GetEncodedName();
	//cout << "class encname: " << encname << endl;
	if (encname[0] == 'Q') {
	    vector<string> names;
	    m_decoder->init(encname);
	    m_decoder->decodeQualName(names);
	    clas = m_builder->startClass(m_lineno, type, names);
	} else {
	    string name = getName(node->Second());
	    clas = m_builder->startClass(m_lineno, type, name);
	}
	clas->parents() = parents;
	m_builder->updateBaseSearch();
	PtreeClassSpec* cspec = static_cast<PtreeClassSpec*>(node);
	addComments(clas, cspec->GetComments());

        // Translate the body of the class
	TranslateBlock(node->Nth(3));
	m_builder->endClass();

    } else if (Ptree::Length(node) == 2) {
	// Forward declaration
        string name = getName(node->Second());
	m_builder->addUnknown(name);
    } /* else {
	cout << "class spec not length 4:" << endl;
	node->Display2(cout);
    } */
    return 0;
}

Ptree* SWalker::TranslateTemplateClass(Ptree* def, Ptree* node)
{
    Trace trace("SWalker::TranslateTemplateClass");
    if (Ptree::Length(node) == 4) {
	// if Class definition (not just declaration)
	updateLineNumber(def);
	
	// Create AST.Class object
	AST::Class *clas;
        string type = getName(node->First());
        vector<Inheritance*> parents = TranslateInheritanceSpec(node->Nth(2));
	char* encname = node->GetEncodedName();
	//cout << "class encname: " << encname << endl;
	if (encname[0] == 'Q') {
	    vector<string> names;
	    m_decoder->init(encname);
	    m_decoder->decodeQualName(names);
	    clas = m_builder->startClass(m_lineno, type, names);
	} else {
	    string name = getName(node->Second());
	    clas = m_builder->startClass(m_lineno, type, name);
	}
	clas->parents() = parents;
	m_builder->updateBaseSearch();
	PtreeClassSpec* cspec = static_cast<PtreeClassSpec*>(node);
	addComments(clas, cspec->GetComments());

	// Create Template type
	Type::Type::vector_t templ_params;
	Ptree* params = def->Third();
	while (params) {
	    Ptree* param = params->First();
	    //param->Display2(cout);
	    if (param->First()->Eq("class") || param->First()->Eq("typename")) {
		// This parameter specifies a type, add as base
		Type::Base* base = m_builder->Base(getName(param->Second()));
		m_builder->add(base);
		templ_params.push_back(base);
	    } else {
		// This parameter specifies a value or something
		// TODO: read spec and figure out wtf this is.
	    }
	    params = Ptree::Rest(params->Rest());
	}
	Type::Template* templ = new Type::Template(clas->name(), clas, templ_params);
	clas->setTemplateType(templ);

	// Now that template args have been created, translate parents
	clas->parents() = TranslateInheritanceSpec(node->Nth(2));
	m_builder->updateBaseSearch();

        // Translate the body of the class
	TranslateBlock(node->Nth(3));
	m_builder->endClass();
    } else if (Ptree::Length(node) == 2) {
	// Forward declaration
        string name = getName(node->Second());
	m_builder->addUnknown(name);
    }
    return 0; 
}

//. Linkage Spec
//. [ extern ["C++"] [{ body }] ]
Ptree* SWalker::TranslateLinkageSpec(Ptree* node)
{
    Trace trace("SWalker::TranslateLinkageSpec");
    Translate(node->Third());
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

    updateLineNumber(def);

    m_declaration = dynamic_cast<PtreeDeclaration*>(def);
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
    Trace trace("SWalker::TranslateDeclarator");
    // Insert code from occ.cc here
    char* encname = decl->GetEncodedName();
    char* enctype = decl->GetEncodedType();
    if (!encname || !enctype) {
	cout << "encname or enctype null!" << endl;
	return 0;
    }

    // Decide if this is a function or variable
    m_decoder->init(enctype);
    code_iter& iter = m_decoder->iter();
    while (*iter == 'C') ++iter;
    if (*iter == 'F') {
	// This is a function
	++iter;

        // Create parameter objects
        Ptree *p_params = decl->Rest();
        while (p_params && !p_params->Car()->Eq('(')) p_params = Ptree::Rest(p_params);
	if (!p_params) { cout << "Warning: error finding params!" << endl; return 0; }
        vector<AST::Parameter*> params;
	TranslateParameters(p_params->Second(), params);

        // Figure out the return type:
        while (*iter++ != '_'); // in case of decoding error this is needed
        Type::Type* returnType = m_decoder->decodeType();

        // Find name:
        // TODO: refactor
        string realname;
	TranslateFunctionName(encname, realname, returnType);

	// Function names are mangled, but 0x80+ chars are changed
        string name = realname+"@"+enctype;
        for (string::iterator ptr = name.begin(); ptr < name.end(); ptr++)
            if (*ptr < 0) (*ptr) += '0'-0x80;


        // Figure out premodifiers
        vector<string> premod;
        Ptree* p = Ptree::First(m_declaration);
        while (p) {
            premod.push_back(p->ToString());
            p = Ptree::Rest(p);
        }

	// Create AST::Operation object
        AST::Operation* oper = m_builder->addOperation(m_lineno, name, premod, returnType, realname);
	oper->parameters() = params;
	addComments(oper, m_declaration);
	addComments(oper, dynamic_cast<PtreeDeclarator*>(decl));
	//if (m_declaration->GetComments()) addComments(oper, m_declaration->GetComments());
	//if (decl->GetComments()) addComments(oper, decl->GetComments());
	// Post Modifier
	//if (enctype_s[0] == 'C') {
	//    PyObject* posties = PyObject_CallMethod(oper, "postmodifier", 0);
	//    PyList_Append(posties, PyString_FromString("const"));
	//}
    } else {
	// Variable declaration
	m_decoder->init(enctype);
	// Get type
	Type::Type* type = m_decoder->decodeType();
	string name;
        if (*encname < 0) name = m_decoder->decodeName(encname);
        else if (*encname == 'Q') {
	    cout << "Scoped name in variable decl!" << endl;
	    return 0;
        } else {
	    cout << "Unknown name in variable decl!" << endl;
	    return 0;
	}

	// TODO: implement sizes support
	vector<size_t> sizes;
	AST::Variable* var = m_builder->addVariable(m_lineno, name, type, false);
	//if (m_declaration->GetComments()) addComments(var, m_declaration->GetComments());
	//if (decl->GetComments()) addComments(var, decl->GetComments());
	addComments(var, m_declaration);
	addComments(var, dynamic_cast<PtreeDeclarator*>(decl));
    }
    return 0;
}

void SWalker::TranslateParameters(Ptree* p_params, vector<Parameter*>& params)
{
    while (p_params) {
	// A parameter has a type, possibly a name and possibly a value
	string name, value;
	if (p_params->Car()->Eq(',')) p_params = p_params->Cdr();
	Ptree* param = p_params->First();
	// The type is stored in the encoded type string already
	Type::Type* type = m_decoder->decodeType();
	if (!type) {
	    cout << "Premature end of decoding!" << endl;
	    break; // NULL means end of encoding
	}
	// Find name and value
	// FIXME: this doesnt account for anon but initialised params!
	if (param->Length() > 1) {
	    Ptree* pname = param->Second();
	    if (pname && pname->Car()) {
		// * and & modifiers are stored with the name so we must skip them
		while (pname && (pname->Car()->Eq('*') || pname->Car()->Eq('&'))) pname = pname->Cdr();
		// Extract name
		if (pname) {
		    name = pname->Car()->ToString();
		}
	    }
	    // If there are three cells, they are 1:name 2:= 3:value
	    if (param->Length() > 2) {
		value = param->Nth(3)->ToString();
	    }
	}
	// Add the AST.Parameter type to the list
	params.push_back(new AST::Parameter("", type, "", name, value));
	p_params = Ptree::Rest(p_params);
    }
}

void SWalker::TranslateFunctionName(char* encname, string& realname, Type::Type*& returnType)
{
   if (*encname < 0) {
	if (encname[1] == '@') {
	    // conversion operator
	    m_decoder->init(encname);
	    m_decoder->iter() += 2;
	    returnType = m_decoder->decodeType();
	    realname = "(conversion)"; // "("+returnType.ToString()+")";
	} else
	    // simple name
	    realname = m_decoder->decodeName(encname);
    } else if (*encname == 'Q') {
	// If a function declaration has a scoped name, then it is not
	// declaring a new function in that scope and can be ignored in
	// the context of synopsis.
	// TODO: exception?
	return;
    } else if (*encname == 'T') {
	// Template specialisation.
	// blah<int, int> is T4blah2ii ---> realname = foo<int,int>
	m_decoder->init(encname);
	code_iter& iter = ++m_decoder->iter();
	realname = m_decoder->decodeName()+"<";
	code_iter tend = iter + *iter++ - 0x80;
	bool first = true;
	// Append type names to realname
	while (iter <= tend) {
	    /*Type::Type* type = */m_decoder->decodeType();
	    if (!first) realname+=","; else first=false;
	    realname += "type"; //type->ToString();
	}
	realname += ">";
    } else {
	cout << "Warning: Unknown function name: " << encname << endl;
    }
}
	
//. Class or Enum
Ptree* SWalker::TranslateTypespecifier(Ptree* tspec) 
{
    Trace trace("SWalker::TranslateTypespecifier");
    Ptree *class_spec = GetClassOrEnumSpec(tspec);
    if (class_spec) Translate(class_spec);
    return 0;
}

Ptree* SWalker::TranslateTypedef(Ptree* node) 
{
    Trace trace("SWalker::TranslateTypedef");
    // /* Ptree *tspec = */ TranslateTypespecifier(node->Second());
    for (Ptree *declarator = node->Third(); declarator; declarator = declarator->ListTail(2))
        TranslateTypedefDeclarator(declarator->Car());
    return 0; 
}

void SWalker::TranslateTypedefDeclarator(Ptree* node)
{
    if (node->What() != ntDeclarator) return;
    char* encname = node->GetEncodedName();
    char* enctype = node->GetEncodedType();
    if (!encname || !enctype) return;

    updateLineNumber(node);

    // Get type of declarator
    m_decoder->init(enctype);
    Type::Type* type = m_decoder->decodeType();
    // Get name of typedef
    string name = m_decoder->decodeName(encname);
    // Create typedef object
    AST::Typedef* tdef = m_builder->addTypedef(m_lineno, name, type, false);
    addComments(tdef, dynamic_cast<PtreeDeclarator*>(node));
}

Ptree* SWalker::TranslateFunctionImplementation(Ptree* node)
{
    Trace trace("SWalker::TranslateFunctionImplementation");
    TranslateDeclarator(node->Third());
    return 0;
}

Ptree* SWalker::TranslateAccessSpec(Ptree* spec)
{
    Trace trace("SWalker::TranslateAccessSpec");
    AST::Access axs = AST::Default;
    switch (spec->First()->What()) {
	case PUBLIC: axs = AST::Public; break;
	case PROTECTED: axs = AST::Protected; break;
	case PRIVATE: axs = AST::Private; break;
    }
    m_builder->setAccess(axs);
    return 0;
}

/* Enum Spec
 *  [ enum [name] [{ [name [= value] ]* }] ]
 */
Ptree *SWalker::TranslateEnumSpec(Ptree *spec)
{
    //updateLineNumber(spec);
    if (!spec->Second()) { return 0; /* anonymous enum */ }
    string name = spec->Second()->ToString();

    updateLineNumber(spec);
    int enum_lineno = m_lineno;
    // Parse enumerators
    vector<AST::Enumerator*> enumerators;
    Ptree* penum = spec->Third()->Second();
    AST::Enumerator* enumor;
    while (penum) {
	updateLineNumber(penum);
	Ptree* penumor = penum->First();
	if (penumor->IsLeaf()) {
	    // Just a name
	    enumor = m_builder->addEnumerator(m_lineno, penumor->ToString(), "");
	    addComments(enumor, static_cast<CommentedLeaf*>(penumor)->GetComments());
	} else {
	    // Name = Value
	    string name = penumor->First()->ToString(), value;
	    if (penumor->Length() == 3) {
		value = penumor->Third()->ToString();
	    }
	    enumor = m_builder->addEnumerator(m_lineno, name, value);
	    addComments(enumor, dynamic_cast<CommentedLeaf*>(penumor->First()));
	}
	enumerators.push_back(enumor);
	penum = Ptree::Rest(penum);
	// Skip comma
	if (penum && penum->Car() && penum->Car()->Eq(','))
	    penum = Ptree::Rest(penum);
    }
    if (m_extract_tails) {
	Ptree* close = spec->Third()->Third();
	enumor = new AST::Enumerator(m_builder->filename(), m_lineno, "dummy", m_dummyname, "");
	addComments(enumor, static_cast<CommentedLeaf*>(close));
	enumerators.push_back(enumor);
    }

    // Create AST.Enum object
    AST::Enum* theEnum = m_builder->addEnum(enum_lineno,name,enumerators);
    addComments(theEnum, m_declaration);
    return 0;
}


Ptree* SWalker::TranslateTemplateInstantiation(Ptree*) { Trace trace("SWalker::TranslateTemplateInstantiation NYI"); return 0; }
Ptree* SWalker::TranslateExternTemplate(Ptree*) { Trace trace("SWalker::TranslateExternTemplate NYI"); return 0; }
Ptree* SWalker::TranslateTemplateFunction(Ptree*, Ptree*) { Trace trace("SWalker::TranslateTemplateFunction NYI"); return 0; }
Ptree* SWalker::TranslateMetaclassDecl(Ptree*) { Trace trace("SWalker::TranslateMetaclassDecl NYI"); return 0; }
Ptree* SWalker::TranslateUsing(Ptree*) { Trace trace("SWalker::TranslateUsing NYI"); return 0; }

Ptree* SWalker::TranslateStorageSpecifiers(Ptree*) { Trace trace("SWalker::TranslateStorageSpecifiers NYI"); return 0; }
Ptree* SWalker::TranslateFunctionBody(Ptree*) { Trace trace("SWalker::TranslateFunctionBody NYI"); return 0; }

//Ptree* SWalker::TranslateEnumSpec(Ptree*) { Trace trace("SWalker::TranslateEnumSpec NYI"); return 0; }

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


