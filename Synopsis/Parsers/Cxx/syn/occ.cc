#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "synopsis.hh"
#include "walker.h"
#include "token.h"
#include "buffer.h"
#include "parse.h"
#include "ptree-core.h"
#include "ptree.h"
#include "encoding.h"

#ifdef DEBUG
#include "swalker.hh"
#include "builder.hh"
#include "dumper.hh"
#endif

/* The following aren't used anywhere. Though it has to be defined and initialized to some dummy default
 * values since it is required by the opencxx.a module, which I don't want to modify...
 */
bool showProgram, doCompile, makeExecutable, doPreprocess, doTranslate;
bool verboseMode, regularCpp, makeSharedLibrary, preprocessTwice;
char* sharedLibraryName;

/* These also are just dummies needed by the opencxx module */
void RunSoCompiler(const char *) {}
void *LoadSoLib(char *) { return 0;}
void *LookupSymbol(void *, char *) { return 0;}

//. A class that acts like a Visitor to the Ptree
class PyWalker : public Walker
{
public:
    PyWalker(Parser *, Synopsis *);
    virtual ~PyWalker();
    virtual Ptree *TranslateDeclaration(Ptree *);
    //   virtual Ptree *TranslateTemplateDecl(Ptree *);
    virtual Ptree *TranslateDeclarator(bool, PtreeDeclarator*);
    virtual Ptree *TranslateDeclarator(Ptree *);
    virtual Ptree *TranslateFunctionImplementation(Ptree *);
    virtual Ptree *TranslateTypedef(Ptree *);
    virtual Ptree *TranslateNamespaceSpec(Ptree *);
    virtual Ptree *TranslateClassSpec(Ptree *);
    virtual Ptree *TranslateTemplateClass(Ptree *, Ptree *);
    virtual vector<PyObject *> TranslateInheritanceSpec(Ptree *);
    virtual Ptree *TranslateClassBody(Ptree *, Ptree*, Class*);
    virtual Ptree *TranslateEnumSpec(Ptree *);
    virtual Ptree *TranslateAccessSpec(Ptree *);
private:
    //. update m_filename and m_lineno from the given ptree
    void updateLineNumber(Ptree*);

    //. extract the name of the node
    static string getName(Ptree *);

    //. return a Type object for the given name and the given list of scopes
    PyObject *lookupType(Ptree *, PyObject *);

    Synopsis *synopsis;
    PyObject *_result; // Current working value
    vector<PyObject *> _declarators;

    //. A string type for the encoded names and types
    typedef basic_string<unsigned char> code;
    //. A string iterator type for the encoded names and types
    typedef code::iterator code_iter;

    //. Add the given comment ptree to the given declaration
    void addComments(PyObject* decl, Ptree* comments);

    //. Return a Type object from the encoded type.
    //. It uses m_enctype and m_enc_iter, so you must call initTypeDecoder
    //. first.
    PyObject* decodeType();

    //. Decode a name starting from the position of m_enc_iter
    string decodeName();
    //. Decode a name starting from the given iterator
    string decodeName(code_iter);
    //. Initialise the type decoder
    void initTypeDecoder(code& encoding, code_iter&, string);

    //. The encoded type string currently being decoded
    code* m_enctype;
    //. The current position in m_enc_iter
    code_iter* m_enc_iter;
    //. A message for errors, generally the declaration name
    string m_encmessage;

    //. The current filename
    string m_filename;
    //. The current line number
    int m_lineno;

    //. The parser which is generating the Ptree
    Parser* m_parser;
    //. Current PtreeDeclaration being parsed
    PtreeDeclaration* m_declaration;
    //. Current ptree typedef being parsed
    Ptree* m_ptree;

    //. Output operator for code strings
    friend ostream& operator << (ostream& o, const PyWalker::code& code);
};

ostream& operator << (ostream& o, const PyWalker::code& code) {
    for (PyWalker::code::const_iterator iter = code.begin(); iter != code.end(); ++iter)
	if (*iter >= 0x80) o << '[' << int(*iter + - 0x80) << ']';
	else o << *iter;
    return o;
}

PyWalker::PyWalker(Parser *p, Synopsis *s)
        : Walker(p),
        synopsis(s),
	m_parser(p)
{
    Trace trace("PyWalker::PyWalker");
    m_ptree = m_declaration = 0;
}

PyWalker::~PyWalker()
{
    Trace trace("PyWalker::~PyWalker");
}

void PyWalker::updateLineNumber(Ptree* ptree)
{
    // Ask the Parser for the linenumber of the ptree. Unfortunately this is
    // rather expensive.
    char* fname;
    int fname_len;
    m_lineno = m_parser->LineNumber(ptree->LeftMost(), fname, fname_len);
    m_filename.assign(fname, fname_len);
    synopsis->set_filename(m_filename);
}

string PyWalker::getName(Ptree *node)
{
    Trace trace("PyWalker::getName");
    if (node && node->IsLeaf())
        return node ? string(node->GetPosition(), node->GetLength()) : string();
    else
	return node->ToString();
    /*{
        cerr << "occ internal error in 'PyWalker::getName' : node is ";
        node->Display();
        exit(-1);
    }*/
}

void PyWalker::addComments(PyObject* decl, Ptree* comments)
{
    while (comments) {
	synopsis->addComment(decl, comments->First()->ToString());
	comments = comments->Rest();
    }
}

PyObject *PyWalker::lookupType(Ptree *node, PyObject *scopes)
{
    if (node->IsLeaf())
        return synopsis->lookupType(getName(node), scopes);
    else if (node->Length() == 2) {
        //template
        // Ignore template for now and just return the name
        // FIXME: obviously this is not the right thing to do ;)
        return synopsis->lookupType(getName(node->First()), scopes);
    } else if (node->Length() > 2 && node->Second()->Eq("::")) {
        // Scoped name.. Join for now.
        Ptree* name = Ptree::Make(node->ToString());
        return synopsis->lookupType(getName(name), scopes);
    } else {
        cerr << "occ internal error in 'PyWalker::lookupType' : node is ";
        node->Display();
        exit(-1);
        return 0;
    }
}

//. Translates a declaration, either variable, typedef or function
//. Function prototype:
//.  [ [modifiers] name [declarators] ; ]
//. Function impl:
//.  [ [modifiers] name declarator [ { ... } ] ]
//. Typedef:
//.  ?
//. Variables:
//.  ?
Ptree *PyWalker::TranslateDeclaration(Ptree *declaration)
{
    Trace trace("PyWalker::TranslateDeclaration");
#ifdef DO_TRACE
    declaration->Display();
#endif
    m_declaration = static_cast<PtreeDeclaration*>(declaration);
    updateLineNumber(declaration);
    Walker::TranslateDeclaration(declaration);
    return declaration;
}

Ptree *PyWalker::TranslateFunctionImplementation(Ptree *p)
{
    TranslateDeclarator(false, (PtreeDeclarator*)p->Third());
    return p;
}

//. Translates a declarator
//. Function proto:
//.   [ { * | & }* name ( [params] ) ]
//. param:
//.   [ [types] { [ { * | & }* name ] { = value } } ]
Ptree *PyWalker::TranslateDeclarator(bool, PtreeDeclarator* decl)
{
    Trace trace("PyWalker::TranslateDeclarator(bool,PtreeDeclarator*)");
    if (decl->What() != ntDeclarator) return decl;
    unsigned char* encname = reinterpret_cast<unsigned char*>(decl->GetEncodedName());
    unsigned char* enctype = reinterpret_cast<unsigned char*>(decl->GetEncodedType());
    if (!encname || !enctype) return decl;

    // Figure out parameter types:
    code enctype_s(enctype);
    code_iter enc_iter = enctype_s.begin();
    initTypeDecoder(enctype_s, enc_iter, decl->ToString());
    while (*enc_iter == 'C') ++enc_iter;
    if (*enc_iter == 'F') {
	++enc_iter;

        // Create parameter objects
        vector<PyObject*> params;
        Ptree *p = decl->Rest();
        while (p && !p->Car()->Eq('(')) p = Ptree::Rest(p);
	if (!p) { cout << "Warning: error finding params!" << endl; return decl; }
        Ptree* pparams = p->Second();
        while (pparams) {
	    // A parameter has a type, possibly a name and possibly a value
            string name, value;
            if (pparams->Car()->Eq(',')) pparams = pparams->Cdr();
            Ptree* param = pparams->First();
	    // The type is stored in the encoded type string already
            PyObject* type = decodeType();
            if (!type) break; // NULL means end of encoding
	    // Find name and value
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
            params.push_back(synopsis->Parameter("", type, "", name, value));
            pparams = Ptree::Rest(pparams);
        }

        // Figure out the return type:
        while (*enc_iter++ != '_'); // in case of decoding error this is needed
        PyObject* returnType = decodeType();

        // Find name:
        // TODO: refactor
        string realname;
        if (*encname > 0x80) {
	    if (encname[1] == '@') {
		// conversion operator
		code encname_s(encname);
		initTypeDecoder(encname_s, enc_iter=encname_s.begin()+2, "");
		returnType = decodeType();
		realname = "(" + string(PyString_AsString(PyObject_Str(returnType))) + ")";
	    } else
		// simple name
		realname = decodeName(encname);
        } else if (*encname == 'Q') {
	    // If a function declaration has a scoped name, then it is not
	    // declaring a new function in that scope and can be ignored in
	    // the context of synopsis.
	    return decl;
        } else if (*encname == 'T') {
	    // Template specialisation.
	    // blah<int, int> is T4blah2ii ---> realname = foo<int,int>
	    code encname_s(encname);
	    initTypeDecoder(encname_s, enc_iter=encname_s.begin()+1, "");
            realname = decodeName()+"<";
            code_iter tend = enc_iter + *enc_iter++ - 0x80;
	    bool first = true;
	    // Append type names to realname
            while (enc_iter <= tend) {
		PyObject* type = decodeType();
		if (!first) realname+=","; else first=false;
		realname += PyString_AsString(PyObject_Str(type));
	    }
	    realname += ">";
	} else {
	    cout << "Warning: Unknown method name: " << encname << endl;
	}
	
	// Function names are mangled, but 0x80+ chars are changed
        string name = realname+"@"+reinterpret_cast<char*>(enctype);
        for (string::iterator ptr = name.begin(); ptr < name.end(); ptr++)
            if (*ptr < 0) (*ptr) += '0'-0x80;


        // Figure out premodifiers
        vector<string> premod;
        p = Ptree::First(m_declaration);
        while (p) {
            premod.push_back(p->ToString());
            p = Ptree::Rest(p);
        }

	// Create AST.Operation object
        PyObject* oper = synopsis->addOperation(m_lineno, true, premod, returnType, name, realname, params);
	if (m_declaration->GetComments()) addComments(oper, m_declaration->GetComments());
	if (decl->GetComments()) addComments(oper, decl->GetComments());
	// Post Modifier
	if (enctype_s[0] == 'C') {
	    PyObject* posties = PyObject_CallMethod(oper, "postmodifier", 0);
	    PyList_Append(posties, PyString_FromString("const"));
	}
    } else {
	// Variable declaration
	enc_iter = enctype_s.begin();
	// Get type
	PyObject* type = decodeType();
	string name;
        if (*encname > 0x80) name = decodeName(encname);
        else if (*encname == 'Q') {
	    cout << "Scoped name in variable decl!" << endl;
	    return decl;
        } else {
	    cout << "Unknown name in variable decl!" << endl;
	    return decl;
	}

	// TODO: implement sizes support
	vector<size_t> sizes;
	PyObject* declor = synopsis->addDeclarator(m_lineno, true, name, sizes);
	PyObject* var = synopsis->addVariable(m_lineno, true, name, type, false, declor);
	if (m_declaration->GetComments()) addComments(var, m_declaration->GetComments());
	if (decl->GetComments()) addComments(var, decl->GetComments());
    }
    return decl;
}

string PyWalker::decodeName()
{
    code_iter &iter = *m_enc_iter;
    size_t length = *iter++ - 0x80;
    string name(reinterpret_cast<char*>(iter),length);
    iter += length;
    return name;
}

string PyWalker::decodeName(code_iter iter)
{
    size_t length = *iter++ - 0x80;
    string name(reinterpret_cast<char*>(iter),length);
    iter += length;
    return name;
}

void PyWalker::initTypeDecoder(code& enctype, code_iter& iter, string msg)
{
    m_enctype = &enctype;
    m_enc_iter = &iter;
    m_encmessage = msg;
}

PyObject* PyWalker::decodeType()
{
    code_iter end = m_enctype->end(), &iter = *m_enc_iter;
    vector<string> premod, postmod;
    string name;
    PyObject *baseType = NULL;

    // Loop forever until broken
    while (iter != end && !name.length()) {
        int c = *iter++;
        if (c == 'P') { postmod.insert(postmod.begin(), "*"); }
        else if (c == 'R') { postmod.insert(postmod.begin(), "&"); }
        else if (c == 'S') { premod.push_back("signed"); }
        else if (c == 'U') { premod.push_back("unsigned"); }
        else if (c == 'C') { premod.push_back("const"); }
        else if (c == 'V') { premod.push_back("volatile"); }
        else if (c == 'A') { premod.push_back("[]"); }
        else if (c == '*') { name = "..."; }
        else if (c == 'i') { name = "int"; }
        else if (c == 'v') { name = "void"; }
        else if (c == 'b') { name = "bool"; }
        else if (c == 's') { name = "short"; }
        else if (c == 'c') { name = "char"; }
        else if (c == 'l') { name = "long"; }
        else if (c == 'j') { name = "long long"; }
        else if (c == 'f') { name = "float"; }
        else if (c == 'd') { name = "double"; }
        else if (c == 'r') { name = "long double"; }
        else if (c == 'e') { name = "..."; }
        else if (c == '?') { return Py_None; }
        else if (c > 0x80) { --iter; name = decodeName(); }
        else if (c == 'Q') {
            // Qualified type: first is num of scopes, each a name.
            int scopes = *iter++ - 0x80;
            vector<string> names;
            while (scopes--) {
                // Only handle two things here: names and templates
                if (*iter >= 0x80) { // Name
                    names.push_back(decodeName());
                } else if (*iter == 'T') {
                    // Template :(
                    ++iter;
                    string tname = decodeName();
                    code_iter tend = iter + *iter++ - 0x80;
                    vector<PyObject*> types;
                    while (iter < tend)
                        types.push_back(decodeType());
                    names.push_back(tname);
                } else {
                    cout << "Warning: Unknown type inside Q: " << *iter << endl;
		    cout << "         Decoding " << *m_enctype << endl;
                }
            }
            // Ask for qualified lookup
	    baseType = synopsis->lookupType(names);
	    if (!baseType) {
		// Not found! Add Type.Forward of scoped name
		name = names[0];
		vector<string>::iterator iter;
		for (iter = names.begin()+1; iter != names.end(); ++iter)
		    name += "::" + *iter;
		baseType = synopsis->addForward(name);
	    }
        }
	else if (c == '_') { --iter; return NULL; }
        else if (c == 'F') {
            // Function ptr. Encoded same as function
	    vector<PyObject*> params;
            while (1) {
		PyObject* type = decodeType();
		if (type) params.push_back(type);
		else break;
	    }
            ++iter; // skip over '_'
            PyObject* returnType = decodeType();
	    PyObject* func = synopsis->addFunctionType(returnType, postmod, params);
            return func;
        }
        else if (c == 'T') {
            // Template type: Name first, then size of arg field, then arg
            // types eg: T6vector54cell <-- 5 is len of 4cell
            name = decodeName();
            code_iter tend = iter + *iter++ - 0x80;
            vector<PyObject*> types;
            while (iter <= tend)
                types.push_back(decodeType());
            PyObject* templ = synopsis->lookupType(name);
            baseType = synopsis->addParametrized(templ, types);
        }
        else if (c == 'M') {
            // Pointer to member. Format is same as for named types
            name = decodeName() + "::*";
        }
        else { cout << m_encmessage << "\nUnknown char decoding '"<<*m_enctype<<"': "<<char(c)<<" "<<c<<" at "<<(iter-m_enctype->begin())<<endl; }
    }
    if (!baseType && !name.length()) { return Py_None; }

    if (!baseType)
        baseType = synopsis->lookupType(name);
    if (baseType == Py_None)
	baseType = synopsis->addForward(name);
    if (premod.empty() && postmod.empty())
        return baseType;
    return synopsis->addModifier(baseType, premod, postmod);
}

Ptree *PyWalker::TranslateDeclarator(Ptree *declarator)
{
    // This is only called from TranslateTypedef
    Trace trace("PyWalker::TranslateDeclarator");
#ifdef DO_TRACE
    declarator->Display();
#endif
    if (declarator->What() != ntDeclarator) return declarator;
    unsigned char* encname = reinterpret_cast<unsigned char*>(declarator->GetEncodedName());
    unsigned char* enctype = reinterpret_cast<unsigned char*>(declarator->GetEncodedType());
    if (!encname || !enctype) return declarator;

    // Get type of declarator
    code enctype_s = enctype;
    code_iter iter = enctype_s.begin();
    initTypeDecoder(enctype_s, iter, declarator->ToString());
    PyObject* type = decodeType();
    // Get name of typedef
    string name = decodeName(encname);
    vector<size_t> sizes;
    // Create declarator object with name
    PyObject* declor = synopsis->addDeclarator(m_lineno,true,name,sizes);
    // Create typedef object
    PyObject* typedf = synopsis->addTypedef(m_lineno,true,"typedef",name,type,false,declor);
    synopsis->addDeclared(name, typedf);
    ///...
    return declarator;
}

/*
 * a typedef declaration contains three items:
 *         * 'typedef'
 *         * the alias type
 *         * the type declarators
 * unfortunately they may be folded together, such as if the alias type
 * is a derived type (pointer, reference), or a fuction pointer.
 * for now let's ignore these cases and simply treat typedefs as a 
 * list of three nodes...
 */
Ptree *PyWalker::TranslateTypedef(Ptree *node)
{
    Trace trace("PyWalker::TranslateTypedef");
    _declarators.clear();
    m_ptree = node;
    updateLineNumber(node);
    /* Ptree *tspec = */ TranslateTypespecifier(node->Second());
    for (Ptree *declarator = node->Third(); declarator; declarator = declarator->ListTail(2))
        TranslateDeclarator(declarator->Car());
    //. now traverse the declarators list and insert them into the AST...
    return node;
}

/*
 * a namespace declaration contains three items:
 *         * 'namespace'
 *         * the name
 *         * the body (a list)
 */
Ptree *PyWalker::TranslateNamespaceSpec(Ptree *node)
{
    //Trace trace("PyWalker::TranslateNamespaceSpec");
    updateLineNumber(node);
    PtreeNamespaceSpec* nspec = static_cast<PtreeNamespaceSpec*>(node);
    string name = getName(node->Cadr());
    PyObject* module = synopsis->addModule(m_lineno, 1, name);
    addComments(module, nspec->GetComments());
    synopsis->addDeclared(name, module);

    synopsis->pushScope(module);
    Translate(Ptree::Third(node));
    synopsis->popScope();
    return node;
}

/*
 * a class declaration contains four items:
 *         * 'class'
 *         * the name
 *         * inheritance spec
 *         * the body (a list)
 */
Ptree *PyWalker::TranslateClassSpec(Ptree *node)
{
    updateLineNumber(node);

    // TODO: Handle nested classes properly (see ThreadData.hh)
    if(Ptree::Length(node) == 4 /* && node->Second()->IsLeaf()*/)
    {
	bool is_struct = node->First()->Eq("struct");
	
	// Create AST.Class object
        string type = getName(node->First());
        string name = getName(node->Second());
        PyObject *clas = synopsis->addClass(m_lineno, true, type, name);
        synopsis->addDeclared(name, clas);
	PtreeClassSpec* cspec = static_cast<PtreeClassSpec*>(node);
	addComments(clas, cspec->GetComments());

        // Add parents to Class object
        vector<PyObject *> parents = TranslateInheritanceSpec(node->Nth(2));
        Synopsis::addInheritance(clas, parents);

        // Translate the body of the class
        Class* meta = MakeClassMetaobject(node, NULL, node);
        synopsis->pushClass(clas);
	synopsis->pushAccess(is_struct ? Synopsis::Public : Synopsis::Private);
        TranslateClassBody(node->Nth(3), node->Nth(2), meta);
	synopsis->popAccess();
        synopsis->popScope();

    }
    return node;
}

/* Template Class:
 *  temp_def:
 *    [ 'template' < [ typespecs ] > class_spec ]
 *  typespec:
 *    [ 'typename' name ]
 */
Ptree *PyWalker::TranslateTemplateClass(Ptree *temp_def, Ptree *node/*class_spec*/)
{
    updateLineNumber(node);

    // TODO: Handle nested classes properly (see ThreadData.hh)
    if(Ptree::Length(node) == 4 /* && node->Second()->IsLeaf()*/)
    {
	bool is_struct = node->First()->Eq("struct");
	
	// Create AST.Class object
        string type = getName(node->First());
        string name = getName(node->Second());
        PyObject *clas = synopsis->addClass(m_lineno, true, type, name);
        synopsis->addDeclared(name, clas);
	PtreeClassSpec* cspec = static_cast<PtreeClassSpec*>(node);
	addComments(clas, cspec->GetComments());

        // Add parents to Class object
        vector<PyObject *> parents = TranslateInheritanceSpec(node->Nth(2));
        Synopsis::addInheritance(clas, parents);

	// Find template typenames
	vector<PyObject*> params;
	Ptree* typespec = temp_def->Third();
	while (typespec) {
	    params.push_back(PyString_FromString(typespec->First()->Second()->ToString()));
	    do { typespec = typespec->Rest(); }
	    while (typespec && typespec->First()->Eq(","));
	}

	// Create Type.Template object
	PyObject* templ = synopsis->addTemplate(name, clas, synopsis->V2L(params));
	PyObject_CallMethod(clas, "set_template", "O", templ);

        // Translate the body of the class
        Class* meta = MakeClassMetaobject(node, NULL, node);
        synopsis->pushClass(clas);
	synopsis->pushAccess(is_struct ? Synopsis::Public : Synopsis::Private);
        TranslateClassBody(node->Nth(3), node->Nth(2), meta);
	synopsis->popAccess();
        synopsis->popScope();

    }
    return temp_def;
}

vector<PyObject *> PyWalker::TranslateInheritanceSpec(Ptree *node)
{
    //Trace trace("PyWalker::TranslateInheritanceSpec");
    vector<PyObject *> ispec;
    while(node)
    {
        node = node->Cdr();		// skip : or ,
        //. the attributes
        vector<string> attributes(node->Car()->Length() - 1);
        for (int i = 0; i != node->Car()->Length() - 1; ++i)
            attributes[i] = getName(node->Car()->Nth(i));
        //. look up the parent class
        PyObject *pdecl = lookupType(node->Car()->Last()->Car(), Py_BuildValue("[]"));
        node = node->Cdr();
        if (pdecl == Py_None) continue;
        //       assertObject(pdecl);
        //. add it to the list
        ispec.push_back(synopsis->Inheritance(pdecl, attributes));
    }
    return ispec;
}

Ptree *PyWalker::TranslateClassBody(Ptree *block, Ptree *bases, Class *meta)
{
    //Trace trace("PyWalker::TranslateClassBody");
    Walker::TranslateClassBody(block, bases, meta);
    return block;
}

/**
 * Translate accessability specifier.
 *  [ <spec> : ]
 * spec is "public", "protected" or "private"
 */
Ptree *PyWalker::TranslateAccessSpec(Ptree* spec)
{
    Synopsis::Accessability axs = Synopsis::Default;
    switch (spec->First()->What()) {
	case PUBLIC: axs = Synopsis::Public; break;
	case PROTECTED: axs = Synopsis::Protected; break;
	case PRIVATE: axs = Synopsis::Private; break;
    }
    synopsis->setAccessability(axs);
    return spec;
}

/* Enum spec looks like:
 *  [ enum [name] [{ [name [= value] ]* }] ]
 */
Ptree *PyWalker::TranslateEnumSpec(Ptree *spec)
{
    updateLineNumber(spec);
    if (!spec->Second()) { return spec; /* anonymous enum */ }
    string name = spec->Second()->ToString();

    // Parse enumerators
    vector<PyObject*> enumerators;
    Ptree* penum = spec->Third()->Second();
    PyObject* enumor;
    while (penum) {
	Ptree* penumor = penum->First();
	if (penumor->IsLeaf()) {
	    // Just a name
	    enumor = synopsis->Enumerator(m_lineno, true, penumor->ToString(), "");
	    addComments(enumor, static_cast<CommentedLeaf*>(penumor)->GetComments());
	} else {
	    // Name = Value
	    string name = penumor->First()->ToString(), value;
	    if (penumor->Length() == 3) {
		value = penumor->Third()->ToString();
	    }
	    enumor = synopsis->Enumerator(m_lineno, true, name, value);
	    addComments(enumor, static_cast<CommentedLeaf*>(penumor->First())->GetComments());
	}
	enumerators.push_back(enumor);
	penum = Ptree::Rest(penum);
	// Skip comma
	if (penum && penum->Car() && penum->Car()->Eq(','))
	    penum = Ptree::Rest(penum);
    }

    // Create AST.Enum object
    PyObject* theEnum = synopsis->addEnum(m_lineno,true,name,enumerators);
    synopsis->addDeclared(name, theEnum);
    addComments(theEnum, m_declaration->GetComments());
    return spec;
}

static void getopts(PyObject *args, vector<const char *> &cppflags, vector<const char *> &occflags)
{
    showProgram = doCompile = verboseMode = makeExecutable = false;
    doTranslate = regularCpp = makeSharedLibrary = preprocessTwice = false;
    doPreprocess = true;
    sharedLibraryName = 0;

    size_t argsize = PyList_Size(args);
    for (size_t i = 0; i != argsize; ++i)
    {
        const char *argument = PyString_AsString(PyList_GetItem(args, i));
        if (strncmp(argument, "-I", 2) == 0) cppflags.push_back(argument);
        else if (strncmp(argument, "-D", 2) == 0) cppflags.push_back(argument);
    }
}

static char *RunPreprocessor(const char *file, const vector<const char *> &flags)
{
    static char dest[1024] = "/tmp/synopsis-XXXXXX";
    //tmpnam(dest);
    if (mkstemp(dest) == -1) {
	perror("RunPreprocessor");
	exit(1);
    }
    switch(fork())
    {
    case 0:
        {
            vector<const char *> args = flags;
            char *cc = getenv("CC");
            args.insert(args.begin(), cc ? cc : "c++");
            args.push_back("-C"); // keep comments
            args.push_back("-E"); // stop after preprocessing
            args.push_back("-o"); // output to...
            args.push_back(dest);
            args.push_back("-x"); // language c++
            args.push_back("c++");
            args.push_back(file);
            args.push_back(0);
            execvp(args[0], (char **)args.begin());
            perror("cannot invoke compiler");
            exit(-1);
            break;
        }
    case -1:
        perror("RunPreprocessor");
        exit(-1);
        break;
    default:
        {
            int status;
            wait(&status);
            if(status != 0)
            {
                if (WIFEXITED(status))
                    cout << "exited with status " << WEXITSTATUS(status) << endl;
                else if (WIFSIGNALED(status))
                    cout << "stopped with status " << WTERMSIG(status) << endl;
                exit(1);
            }
        }
    }
    return dest;
}

static char *RunOpencxx(const char *src, const char *file, const vector<const char *> &args, PyObject *types, PyObject *declarations)
{
    Trace trace("RunOpencxx");
    static char dest[1024] = "/tmp/synopsis-XXXXXX";
    //tmpnam(dest);
    if (mkstemp(dest) == -1) {
	perror("RunPreprocessor");
	exit(1);
    }
    
    ifstream ifs(file);
    if(!ifs)
    {
        perror(file);
        exit(1);
    }
    ProgramFile prog(ifs);
    Lex lex(&prog);
    Parser parse(&lex);
    Synopsis synopsis(src, declarations, types);
#ifdef DEBUG
    // Test SWalker
    Builder builder;
    SWalker swalker(&parse, &builder);
    Ptree *def;
    while(parse.rProgram(def))
	swalker.Translate(def);
    Dumper dumper;
    //dumper.visitScope(builder.scope());
#else
    PyWalker walker(&parse, &synopsis);
    Ptree *def;
    while(parse.rProgram(def)) {
        walker.Translate(def);
    }
#endif
    
    if(parse.NumOfErrors() != 0)
    {
        cerr << "errors while parsing file " << file << endl;
        exit(1);
    }
    return dest;
}

extern "C" {

    static PyObject *occParse(PyObject *self, PyObject *args)
    {
        Trace trace("occParse");
#if 0
        Ptree::show_encoded = true;
#endif
        char *src;
        PyObject *parserargs, *types, *declarations;
        if (!PyArg_ParseTuple(args, "sO!OO!", &src, &PyList_Type, &parserargs, &types, &PyList_Type, &declarations)) return 0;
        vector<const char *> cppargs;
        vector<const char *> occargs;
        getopts(parserargs, cppargs, occargs);
        if (!src || *src == '\0')
        {
            cerr << "No source file" << endl;
            exit(-1);
        }
        char *cppfile = RunPreprocessor(src, cppargs);
        char *occfile = RunOpencxx(src, cppfile, occargs, types, declarations);
        unlink(cppfile);
        unlink(occfile);
        Py_INCREF(Py_None);
        return Py_None;
    }

    static PyMethodDef occ_methods[] =
        {
            {(char*)"parse",            occParse,               METH_VARARGS},
            {NULL, NULL}
        };

    void initocc()
    {
        PyObject* m = Py_InitModule((char*)"occ", occ_methods);
        PyObject_SetAttrString(m, (char*)"version", PyString_FromString("0.1"));
    }
}

#ifdef DEBUG

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        cout << "Usage: " << argv[0] << " <filename>" << endl;
        exit(-1);
    }
    char *src = argv[1];
    vector<const char *> cppargs;
    vector<const char *> occargs;
    //   getopts(argc, argv, cppargs, occargs);
    Py_Initialize();
    PyObject *pylist = PyList_New(argc - 1);
    for (int i = 1; i != argc; ++i)
        PyList_SetItem(pylist, i - 1, PyString_FromString(argv[i]));
    getopts(pylist, cppargs, occargs);
    if (!src || *src == '\0')
    {
        cerr << "No source file" << endl;
        exit(-1);
    }
    char *cppfile = RunPreprocessor(src, cppargs);
    PyObject* type = PyImport_ImportModule("Synopsis.Core.Type");
    PyObject* types = PyObject_CallMethod(type, "Dictionary", 0);
    char *occfile = RunOpencxx(src, cppfile, occargs, types, PyList_New(0));
    unlink(cppfile);
    unlink(occfile);
}

#endif
