// $Id: swalker.cc,v 1.42 2001/07/19 13:50:52 chalky Exp $
//
// This file is a part of Synopsis.
// Copyright (C) 2000, 2001 Stephen Davies
// Copyright (C) 2000, 2001 Stefan Seefeld
//
// Synopsis is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
// $Log: swalker.cc,v $
// Revision 1.42  2001/07/19 13:50:52  chalky
// Support 'using' somewhat, L"const" literals, and better Qual.Templ. names
//
// Revision 1.41  2001/07/03 11:32:40  chalky
// Fix phantom parameter identifier bug
//
// Revision 1.40  2001/06/11 10:37:30  chalky
// Operators! Arrays! (and probably more that I forget)
//
// Revision 1.39  2001/06/10 07:17:37  chalky
// Comment fix, better functions, linking etc. Better link titles
//
// Revision 1.38  2001/06/10 00:31:39  chalky
// Refactored link storage, better comments, better parsing
//
// Revision 1.37  2001/06/06 07:51:45  chalky
// Fixes and moving towards SXR
//
// Revision 1.36  2001/06/06 04:56:59  uid20151
// small optimisation (dont translate non-main func impls)
//
// Revision 1.35  2001/06/06 03:28:49  chalky
// Support anon structs
//
// Revision 1.34  2001/06/06 01:18:13  chalky
// Rewrote parameter parsing to test for different combos of name, value and
// keywords
//
// Revision 1.33  2001/06/05 17:59:25  stefan
// output some diagnostics when catching a segfault
//
// Revision 1.32  2001/06/05 15:34:11  chalky
// Allow keywords before function parameters, eg: register
//
// Revision 1.31  2001/06/05 05:47:02  chalky
// Added global g_swalker var
//
// Revision 1.30  2001/06/05 05:03:53  chalky
// Added support for qualified typedefs in storeLink
//
// Revision 1.29  2001/06/05 03:49:33  chalky
// Made my own wrong_type_cast exception. Added template support to qualified
// names (its bad but it doesnt crash). Added vector<string> output op to builder
//
// Revision 1.28  2001/05/25 03:08:49  chalky
// Fixes to compile with 3.0
//
// Revision 1.27  2001/05/23 05:08:47  stefan
// more std C++ issues. It still crashes...
//
// Revision 1.26  2001/05/06 20:15:03  stefan
// fixes to get std compliant; replaced some pass-by-value by pass-by-const-ref; bug fixes;
//
// Revision 1.25  2001/04/17 15:47:26  chalky
// Added declaration name mapper, and changed refmanual to use it instead of the
// old language mapping
//
// Revision 1.24  2001/04/03 23:01:38  chalky
// Small fixes and extra comments
//
// Revision 1.23  2001/03/19 07:53:45  chalky
// Small fixes.
//
// Revision 1.22  2001/03/16 04:42:00  chalky
// SXR parses expressions, handles differences from macro expansions. Some work
// on function call resolution.
//
// Revision 1.21  2001/02/16 06:59:32  chalky
// ScopePage summaries link to source
//
// Revision 1.20  2001/02/16 06:33:35  chalky
// parameterized types, return types, variable types, modifiers, etc.
//
// Revision 1.19  2001/02/16 04:57:50  chalky
// SXR: func parameters, namespaces, comments. Unlink temp file. a class=ref/def
//
// Revision 1.18  2001/02/16 02:29:55  chalky
// Initial work on SXR and HTML integration
//
// Revision 1.17  2001/02/13 05:20:04  chalky
// Made C++ parser mangle functions by formatting their parameter types
//
// Revision 1.16  2001/02/07 14:27:39  chalky
// Enums inside declarations swallow declaration comments.
//
//

// File: swalker.cc
//
// SWalker class

#include <iostream.h>
#include <string>
#include <typeinfo>

#include "ptree.h"
#include "parse.h"

#include "swalker.hh"
#include "strace.hh"
#include "type.hh"
#include "ast.hh"
#include "builder.hh"
#include "decoder.hh"
#include "dumper.hh"
#include "link_map.hh"
#include "linkstore.hh"

using namespace AST;

// ------------------------------------------------------------------
//                 -- CLASS STrace --
// ------------------------------------------------------------------

#ifdef DO_TRACE
// Class variable definitions
int STrace::slevel = 0, STrace::dlevel = 0;
std::ostrstream* STrace::stream = 0;
STrace::list STrace::m_list;
std::ostream& STrace::operator <<(Ptree* p)
{
    std::ostream& out = operator <<("-");
    p->Display2(out);
    return out;
}
#endif

// ------------------------------------------------------------------
//                 -- CLASS SWalker --
// ------------------------------------------------------------------

SWalker *SWalker::g_swalker = 0;


// ------------------------------------
// SWalker Constructor
SWalker::SWalker(const std::string &source, Parser* parser, Builder* builder, Program* program)
    : Walker(parser),
      m_parser(parser),
      m_program(program),
      m_builder(builder),
      m_decoder(new Decoder(m_builder)),
      m_declaration(0),
      m_filename_ptr(0),
      m_lineno(0),
      m_source(source),
      m_extract_tails(false),
      m_links(0),
      m_store_decl(false),
      m_type_formatter(new TypeFormatter()),
      m_operation(0),
      m_type(0),
      m_scope(0),
      m_postfix_flag(Postfix_Var)
{
    g_swalker = this;
}

// The name returned is just the node's text if the node is a leaf. Otherwise,
// the ToString method of Ptree is used, which is rather expensive since it
// creates a temporary write buffer and reifies the node tree into it.
std::string SWalker::getName(Ptree *node) const
{
    // STrace trace("SWalker::getName");
    if (node && node->IsLeaf())
        return std::string(node->GetPosition(), node->GetLength());
    return node->ToString();
}

Parser* SWalker::getParser()
{
    return m_parser;
}

Program* SWalker::getProgram()
{
    return m_program;
}

Builder* SWalker::getBuilder()
{
    return m_builder;
}

TypeFormatter* SWalker::getTypeFormatter()
{
    return m_type_formatter;
}

int SWalker::getLine(Ptree* node)
{
    updateLineNumber(node);
    return m_lineno;
}

bool SWalker::isMainFile()
{
    return (m_filename == m_source);
}

void SWalker::setStoreLinks(bool value, std::ostream* storage)
{
    // Create a new LinkStore
    if (value) {
	m_links = new LinkStore(*storage, this);
    } else {
	m_links = 0;
    }
}

// Updates the line number stored in this SWalker instance, and the filename
// stored in the Builder instance at m_builder. The filename is only set if
// the actual char* changed (which only happens when the preprocessor places
// another #line directive)
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


// Adds the given comments to the given declaration. If m_links is set,
// then syntax highlighting information is also stored.
void SWalker::addComments(AST::Declaration* decl, Ptree* comments)
{
    while (comments) {
	if (comments->First()) {
	    if (decl) decl->comments().push_back(new AST::Comment("", 0, comments->First()->ToString()));
	    if (m_links) m_links->longSpan(comments->First(), "file-comment");
	    // Set comments to nil so we dont accidentally do them twice (eg:
	    // when parsing expressions)
	    comments->SetCar(nil);
	}
	comments = comments->Rest();
    }
}

// -- These methods implement addComments for various node types that store
// comment pointers
void SWalker::addComments(AST::Declaration* decl, CommentedLeaf* node)
{
    if (node) addComments(decl, node->GetComments());
}
void SWalker::addComments(AST::Declaration* decl, PtreeDeclaration* node)
{
    if (node) addComments(decl, node->GetComments());
}
void SWalker::addComments(AST::Declaration* decl, PtreeDeclarator* node)
{
    if (node) addComments(decl, node->GetComments());
}
void SWalker::addComments(AST::Declaration* decl, PtreeNamespaceSpec* node)
{
    if (node) addComments(decl, node->GetComments());
}
void SWalker::findComments(Ptree* node)
{
    Ptree* leaf, *parent;
    leaf = FindLeftLeaf(node, parent);
    if (leaf) addComments(NULL, dynamic_cast<CommentedLeaf*>(leaf));
}

Ptree* SWalker::TranslateArgDeclList(bool, Ptree*, Ptree*) { STrace trace("SWalker::TranslateArgDeclList NYI"); return 0; }
Ptree* SWalker::TranslateInitializeArgs(PtreeDeclarator*, Ptree*) { STrace trace("SWalker::TranslateInitializeArgs NYI"); return 0; }
Ptree* SWalker::TranslateAssignInitializer(PtreeDeclarator*, Ptree*) { STrace trace("SWalker::TranslateAssignInitializer NYI"); return 0; }
//Class* SWalker::MakeClassMetaobject(Ptree*, Ptree*, Ptree*) { STrace trace("SWalker::MakeClassMetaobject NYI"); return 0; }
//Ptree* SWalker::TranslateClassSpec(Ptree*, Ptree*, Ptree*, Class*) { STrace trace("SWalker::TranslateClassSpec NYI"); return 0; }
//Ptree* SWalker::TranslateClassBody(Ptree*, Ptree*, Class*) { STrace trace("SWalker::TranslateClassBody NYI"); return 0; }
//Ptree* SWalker::TranslateTemplateInstantiation(Ptree*, Ptree*, Ptree*, Class*) { STrace trace("SWalker::TranslateTemplateInstantiation NYI"); return 0; }

// Format the given parameters. m_type_formatter is used to format the given
// list of parameters into a string, suitable for use as the name of a
// Function object.
std::string SWalker::formatParameters(std::vector<AST::Parameter*>& params)
{
    // Set scope for formatter
    AST::Scope* scope = m_builder->scope();
    if (scope) {
	m_type_formatter->setScope(scope->name());
    } else {
	AST::Name empty;
	m_type_formatter->setScope(empty);
    }
    std::vector<AST::Parameter*>::iterator iter = params.begin(), end = params.end();
    if (iter == end) { return "()"; }
    std::string str = "(" + m_type_formatter->format((*iter++)->type());
    while (iter != end) {
	str += ",";
	str += m_type_formatter->format((*iter++)->type());
    }
    return str+")";
}




//
// Translate Methods
//

// Overrides Walker::Translate to catch any exceptions
void SWalker::Translate(Ptree* node) {
    STrace trace("SWalker::Translate");
    try {
	Walker::Translate(node);
    }
    catch (TranslateError e) {
	// This error usually means that the syntax highlighting failed, and
	// can be safely ignored (with an appropriate log message for
	// debugging :)
#ifdef DEBUG
	std::string at;
	if (e.node) node = e.node;
	char* fname;
	int fname_len;
	int lineno = m_parser->LineNumber(node->LeftMost(), fname, fname_len);
	std::ostrstream buf;
	buf << at << " (" << std::string(fname, fname_len) << ":" << lineno << ")";
	at.assign(buf.str(), buf.pcount());
	LOG("Warning: An exception occurred:" << at);
	LOG("- " << e.str());
#endif
    }
    catch (std::exception e) {
	cout << "Warning: An exception occurred: " << e.what() << endl;
    }
    catch (...) {
	cout << "Warning: An exception occurred (unknown)" << endl;
    }
}

// Default translate, usually means a literal
Ptree* SWalker::TranslatePtree(Ptree* node) {
    // Determine type of node
    char* str = node->ToString();
    if (*str >= '0' && *str <= '9' || *str == '.') {
	// Assume whole node is a number
	if (m_links) m_links->span(node, "file-number");
	// TODO: decide if Long, Float, Double, etc
	const char* num_type = (*str == '.') ? "double" : "int";
	while (*++str) {
	    if (*str >= '0' && *str <= '9')
		;
	    else if (*str == 'e' || *str == 'E') {
		// Might be followed by + or -
		++str;
		if (*str == '+' || *str == '-') ++str;
	    } else if (*str == '.') {
		num_type = "double";
	    } else if (*str == 'f' || *str == 'F') {
		num_type = "float";
		break;
	    } else if (*str == 'l' || *str == 'L') {
		if (num_type == "int") num_type = "long";
		else if (num_type == "unsigned") num_type = "unsigned long";
		else if (num_type == "float") num_type = "long double";
		else std::cout << "Unknown num type: " << num_type << std::endl;
	    } else if (*str == 'u' || *str == 'U') {
		if (num_type == "int") num_type = "unsigned";
		else if (num_type == "long") num_type = "unsigned long";
		else std::cout << "Unknown num type: " << num_type << std::endl;
	    } else
		// End of numeric constant
		break;
	}
	m_type = m_builder->lookupType(num_type);
    } else if (*str == '\'') {
	// Whole node is a char literal
	if (m_links) m_links->span(node, "file-string");
	m_type = m_builder->lookupType("char");
    } else if (*str == '"') {
	// Assume whole node is a string
	if (m_links) m_links->span(node, "file-string");
	m_type = m_builder->lookupType("char");
	Type::Type::Mods pre, post; pre.push_back("const"); post.push_back("*");
	m_type = new Type::Modifier(m_type, pre, post);
    } else {
	cout << "Warning: Unknown Ptree "<<node->What(); node->Display2(cout);
	*((char*)0) = 1;
    }
    return 0;
}

//. NamespaceSpec
//. [ namespace <identifier> [{ body }] ]
Ptree* SWalker::TranslateNamespaceSpec(Ptree* def) {
    STrace trace("SWalker::TranslateNamespaceSpec");
    
    if (m_links) m_links->span(def->First(), "file-keyword");
    else updateLineNumber(def);

    AST::Namespace* ns;
    if (def->Cadr()) ns = m_builder->startNamespace(getName(def->Cadr()), Builder::NamespaceNamed);
    else ns = m_builder->startNamespace(m_filename, Builder::NamespaceAnon);

    addComments(ns, dynamic_cast<PtreeNamespaceSpec*>(def));
    if (m_links && def->Cadr()) m_links->link(def->Second(), ns);

    Translate(Ptree::Third(def));

    m_builder->endNamespace();
    return 0;
}

//. [ : (public|private|protected|nil) <name> {, ...} ]
std::vector<Inheritance*> SWalker::TranslateInheritanceSpec(Ptree *node)
{
    STrace trace("PyWalker::TranslateInheritanceSpec");
    std::vector<Inheritance*> ispec;
    Type::Type *type;
    while (node) {
        node = node->Cdr();		// skip : or ,
        // the attributes
        std::vector<std::string> attributes(node->Car()->Length() - 1);
        for (int i = 0; i != node->Car()->Length() - 1; ++i) {
            attributes[i] = getName(node->Car()->Nth(i));
	    if (m_links) m_links->span(node->Car()->Nth(i), "file-keyword");
	}
        // look up the parent type
	Ptree* name = node->Car()->Last()->Car();
	if (name->IsLeaf()) {
	    try {
		type = m_builder->lookupType(getName(name));
	    } catch (TranslateError) {
		// Ignore error, and put an Unknown in, instead
		Type::Name uname; uname.push_back(getName(name));
		type = new Type::Unknown(uname);
	    }
	} else {
	    char* encname = name->GetEncodedName();
	    m_decoder->init(encname);
	    type = m_decoder->decodeType();
	}
	if (m_links) m_links->link(name, type);

        node = node->Cdr();
        // add it to the list
        ispec.push_back(new AST::Inheritance(type, attributes));
    }
    return ispec;
}


//. [ class|struct <name> <inheritance> [{ body }] ]
Ptree* SWalker::TranslateClassSpec(Ptree* node) 
{
    STrace trace("SWalker::TranslateClassSpec");
    if (Ptree::Length(node) == 4) {
	// if Class definition (not just declaration)

	if (m_links) m_links->span(node->First(), "file-keyword");
	else updateLineNumber(node);
	
	// Create AST.Class object
	AST::Class *clas;
        std::string type = getName(node->First());
	char* encname = node->GetEncodedName();
	if (encname[0] == 'Q') {
	    std::vector<std::string> names;
	    m_decoder->init(encname);
	    m_decoder->decodeQualName(names);
	    clas = m_builder->startClass(m_lineno, type, names);
	} else {
	    std::string name = getName(node->Second());
	    clas = m_builder->startClass(m_lineno, type, name);
	}
	if (m_links) m_links->link(node->Second(), clas);

	// Get parents
        std::vector<Inheritance*> parents = TranslateInheritanceSpec(node->Nth(2));
	clas->parents() = parents;
	m_builder->updateBaseSearch();
	PtreeClassSpec* cspec = static_cast<PtreeClassSpec*>(node);
	addComments(clas, cspec->GetComments());

	// Push the impl stack for a cache of func impls
	m_func_impl_stack.push_back(FuncImplVec());

        // Translate the body of the class
	TranslateBlock(node->Nth(3));

	// Translate any func impls inlined in the class
	FuncImplVec& vec = m_func_impl_stack.back();
	FuncImplVec::iterator iter = vec.begin();
	while (iter != vec.end())
	    TranslateFuncImplCache(*iter++);
	m_func_impl_stack.pop_back();
	    
	m_builder->endClass();
    } else if (Ptree::Length(node) == 2) {
	// Forward declaration
        std::string name = getName(node->Second());
	m_builder->addUnknown(name);
	if (m_links) { // highlight the comments, at least
	    PtreeClassSpec* cspec = static_cast<PtreeClassSpec*>(node);
	    addComments(NULL, cspec->GetComments());
	}
    } else if (Ptree::Length(node) == 3) {
	// Most likely anonymous struct
	// [ struct [nil nil] [{ ... }] ]
	if (node->Second()->IsLeaf() || node->Second()->First() != 0) return 0;

	if (m_links) m_links->span(node->First(), "file-keyword");
	else updateLineNumber(node);
	
	// Create AST.Class object
	AST::Class *clas;
        std::string type = getName(node->First());
	char* encname = node->GetEncodedName();
	m_decoder->init(encname);
	if (encname[0] == 'Q') {
	    std::vector<std::string> names;
	    m_decoder->decodeQualName(names);
	    clas = m_builder->startClass(m_lineno, type, names);
	} else {
	    std::string name = m_decoder->decodeName();
	    clas = m_builder->startClass(m_lineno, type, name);
	}

	PtreeClassSpec* cspec = static_cast<PtreeClassSpec*>(node);
	addComments(clas, cspec->GetComments());

	// Push the impl stack for a cache of func impls
	m_func_impl_stack.push_back(FuncImplVec());

        // Translate the body of the class
	TranslateBlock(node->Third());

	// Translate any func impls inlined in the class
	FuncImplVec& vec = m_func_impl_stack.back();
	FuncImplVec::iterator iter = vec.begin();
	while (iter != vec.end())
	    TranslateFuncImplCache(*iter++);
	m_func_impl_stack.pop_back();
	    
	m_builder->endClass();
    } /* else {
	cout << "class spec not length 4:" << endl;
	node->Display2(cout);
    } */
    return 0;
}

Ptree* SWalker::TranslateTemplateClass(Ptree* def, Ptree* node)
{
    STrace trace("SWalker::TranslateTemplateClass");
    if (Ptree::Length(node) == 4) {
	// if Class definition (not just declaration)
	updateLineNumber(def);
	
	// Create AST.Class object
	AST::Class *clas;
        std::string type = getName(node->First());
        std::vector<Inheritance*> parents = TranslateInheritanceSpec(node->Nth(2));
	char* encname = node->GetEncodedName();
	//cout << "class encname: " << encname << endl;
	if (encname[0] == 'Q') {
	    std::vector<std::string> names;
	    m_decoder->init(encname);
	    m_decoder->decodeQualName(names);
	    clas = m_builder->startClass(m_lineno, type, names);
	} else if (encname[0] == 'T') {
	    // Specialisation.. ignore for now. FIXME
	    return 0;
	} else {
	    std::string name = getName(node->Second());
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

	// Push the impl stack for a cache of func impls
	m_func_impl_stack.push_back(FuncImplVec());

        // Translate the body of the class
	TranslateBlock(node->Nth(3));
	
	// Translate any func impls inlined in the class
	FuncImplVec& vec = m_func_impl_stack.back();
	FuncImplVec::iterator iter = vec.begin();
	while (iter != vec.end())
	    TranslateFuncImplCache(*iter++);
	m_func_impl_stack.pop_back();
	    
	m_builder->endClass();
    } else if (Ptree::Length(node) == 2) {
	// Forward declaration
        std::string name = getName(node->Second());
	m_builder->addUnknown(name);
    }
    return 0; 
}

//. Linkage Spec
//. [ extern ["C++"] [{ body }] ]
Ptree* SWalker::TranslateLinkageSpec(Ptree* node)
{
    STrace trace("SWalker::TranslateLinkageSpec");
    Translate(node->Third());
    return 0;
}

//. Block
//. [ { [ <statement>* ] } ]
Ptree* SWalker::TranslateBlock(Ptree* block) {
    STrace trace("SWalker::TranslateBlock");
    Ptree* rest = Ptree::Second(block);
    while (rest != nil) {
	Translate(rest->Car());
	rest = rest->Cdr();
    }
    if (m_extract_tails) {
	Ptree* close = Ptree::Third(block);
	AST::Declaration* decl;
	decl = m_builder->addTailComment(m_lineno);
	addComments(decl, dynamic_cast<CommentedLeaf*>(close));
    }

    return 0;
}

//. Brace
//. [ { [ <statement>* ] } ]
Ptree* SWalker::TranslateBrace(Ptree* brace) 
{
    STrace trace("SWalker::TranslateBrace");
    Ptree* rest = Ptree::Second(brace);
    while (rest != nil) {
	Translate(rest->Car());
	rest = rest->Cdr();
    }
    if (m_extract_tails) {
	Ptree* close = Ptree::Third(brace);
	AST::Declaration* decl;
	decl = m_builder->addTailComment(m_lineno);
	addComments(decl, dynamic_cast<CommentedLeaf*>(close));
    }

    return 0;
}

//. TemplateDecl
//. [ template < [types] > [decl] ]
Ptree* SWalker::TranslateTemplateDecl(Ptree* def) 
{
    STrace trace("SWalker::TranslateTemplateDecl");
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
    STrace trace("SWalker::TranslateDeclaration");
    // Link any comments added because we are inside a function body
    if (m_links) findComments(def);

    updateLineNumber(def);

    m_declaration = dynamic_cast<PtreeDeclaration*>(def);
    m_store_decl = true;
    Ptree* decls = Ptree::Third(def);
    
    // Typespecifier may be a class {} etc.
    TranslateTypespecifier(Ptree::Second(def));

    if (decls->IsA(ntDeclarator))	// if it is a function
	TranslateFunctionImplementation(def);
    else {
	// if it is a function prototype or a variable declaration.
	/////TranslateStorageSpecifiers(Ptree::First(def));
	if (!decls->IsLeaf())	// if it is not ";"
	    TranslateDeclarators(decls);
    }
    m_declaration = NULL;
    return 0;
}

//. [ [ declarator { = <expr> } ] , ... ]
Ptree* SWalker::TranslateDeclarators(Ptree* decls) 
{
    STrace trace("SWalker::TranslateDeclarators");
    Ptree* rest = decls, *p;
    while (rest != nil) {
	p = rest->Car();
	if (p->IsA(ntDeclarator)) {
	    TranslateDeclarator(p);
	    m_store_decl = false;
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
    STrace trace("SWalker::TranslateDeclarator");
    // Insert code from occ.cc here
    char* encname = decl->GetEncodedName();
    char* enctype = decl->GetEncodedType();
    if (!encname || !enctype) {
	cout << "encname or enctype null!" << endl;
	return 0;
    }
    
    try {

	// Decide if this is a function or variable
	m_decoder->init(enctype);
	code_iter& iter = m_decoder->iter();
	bool is_const = false;
	while (*iter == 'C') { ++iter; is_const = true; }
	if (*iter == 'F') {
	    // This is a function
	    ++iter;

	    // Create parameter objects
	    Ptree *p_params = decl->Rest();
	    while (p_params && !p_params->Car()->Eq('(')) p_params = Ptree::Rest(p_params);
	    if (!p_params) { cout << "Warning: error finding params!" << endl; return 0; }
	    std::vector<AST::Parameter*> params;
	    TranslateParameters(p_params->Second(), params);
	    m_param_cache = params;

	    // Figure out the return type:
	    while (*iter++ != '_'); // in case of decoding error this is needed
	    Type::Type* returnType = m_decoder->decodeType();

	    // Figure out premodifiers
	    std::vector<std::string> premod;
	    Ptree* p = Ptree::First(m_declaration);
	    while (p) {
		premod.push_back(p->ToString());
		p = Ptree::Rest(p);
	    }

	    AST::Operation* oper = 0;
	    // Find name:
	    if (encname[0] == 'Q') {
		// The name is qualified, which introduces a bit of difficulty
		std::vector<std::string> names;
		m_decoder->init(encname);
		m_decoder->decodeQualName(names);
		names.back() += formatParameters(params);
		// A qual name must already be declared, so find it:
		try {
		    Type::Named* named_type = m_builder->lookupType(names, true);
		    oper = Type::declared_cast<AST::Operation>(named_type);
		} catch (const Type::wrong_type_cast &) {
		    throw ERROR("Qualified function name wasn't a function:" << names);
		}
		// expand param info, since we now have names for them
		std::vector<AST::Parameter*>::iterator piter = oper->parameters().begin();
		std::vector<AST::Parameter*>::iterator pend = oper->parameters().end();
		std::vector<AST::Parameter*>::iterator new_piter = params.begin();
		while (piter != pend) {
		    AST::Parameter* param = *piter++, *new_param = *new_piter++;
		    if (!param->name().size() && new_param->name().size()) {
			param->setName(new_param->name());
		    }
		}
	    } else {
		// Decode the function name
		std::string realname;
		TranslateFunctionName(encname, realname, returnType);
		// Name is same as realname, but with parameters added
		std::string name = realname+formatParameters(params);
		// Append const after params if this is a const function
		if (is_const) name += "const";
		// Create AST::Operation object
		oper = m_builder->addOperation(m_lineno, name, premod, returnType, realname);
		oper->parameters() = params;
	    }
	    addComments(oper, m_declaration);
	    addComments(oper, dynamic_cast<PtreeDeclarator*>(decl));

	    // if storing links, find name
	    if (m_links) {
		// Store for use by TranslateFunctionImplementation
		m_operation = oper;

		// Do decl type first
		if (m_store_decl && m_declaration->Second())
		    m_links->link(m_declaration->Second(), returnType);

		p = decl;
		while (p && p->Car()->IsLeaf() && (p->Car()->Eq('*') || p->Car()->Eq('&'))) p = Ptree::Rest(p);
		if (p) {
		    // p should now be at the name
		    m_links->link(p->Car(), oper);
		}
	    }
	} else {
	    // Variable declaration
	    m_decoder->init(enctype);
	    // Get type
	    Type::Type* type = m_decoder->decodeType();
	    std::string name;
	    if (*encname < 0) name = m_decoder->decodeName(encname);
	    else if (*encname == 'Q') {
		cout << "Scoped name in variable decl!" << endl;
		return 0;
	    } else {
		cout << "Unknown name in variable decl!" << endl;
		return 0;
	    }

	    // TODO: implement sizes support
	    std::vector<size_t> sizes;
	    std::string var_type = m_builder->scope()->type();
	    if (var_type == "function") var_type = "local";
	    var_type += " variable";
	    AST::Variable* var = m_builder->addVariable(m_lineno, name, type, false, var_type);
	    //if (m_declaration->GetComments()) addComments(var, m_declaration->GetComments());
	    //if (decl->GetComments()) addComments(var, decl->GetComments());
	    addComments(var, m_declaration);
	    addComments(var, dynamic_cast<PtreeDeclarator*>(decl));
	    
	    // if storing links, find name
	    if (m_links) {
		// Do decl type first
		if (m_store_decl && m_declaration->Second()) 
		    m_links->link(m_declaration->Second(), type);

		Ptree* p = decl;
		while (p && p->Car()->IsLeaf() && (p->Car()->Eq('*') || p->Car()->Eq('&'))) p = Ptree::Rest(p);
		if (p) {
		    // p should now be at the name
		    m_links->link(p->Car(), var);

		    // Next might be '=' then expr
		    p = p->Rest();
		    if (p && p->Car() && p->Car()->Eq('=')) {
			p = p->Rest();
			if (p && p->Car()) Translate(p->Car());
		    }
		}
	    }
	}
    } catch (TranslateError e) {
	e.set_node(decl);
	throw;
    }
    return 0;
}

// Fills the vector of Parameter types by parsing p_params.
void SWalker::TranslateParameters(Ptree* p_params, std::vector<AST::Parameter*>& params)
{
    while (p_params)
    {
	// A parameter has a type, possibly a name and possibly a value
	std::string name, value, premods;
	if (p_params->Car()->Eq(',')) p_params = p_params->Cdr();
	Ptree* param = p_params->First();
	// The type is stored in the encoded type string already
	Type::Type* type = m_decoder->decodeType();
	if (!type)
	{
	    std::cout << "Premature end of decoding!" << std::endl;
	    break; // NULL means end of encoding
	}
        // Discover contents. Ptree may look like:
        //[register iostate [* a] = [0 + 2]]
        //[register iostate [nil] = 0]
        //[register iostate [nil]]
        //[iostate [nil] = 0]
        //[iostate [nil]]   etc
        if (param->Length() > 1) {
	    // There is a parameter
	    int type_ix, value_ix = -1, len = param->Length();
	    if (len >= 4 && param->Nth(len-2)->Eq('=')) {
		// There is an =, which must be followed by the value
		value_ix = len-1;
		type_ix = len-4;
	    } else {
		// No =, so last is name and second last is type
		type_ix = len-2;
	    }
	    // Link type
	    if (m_links && !param->IsLeaf() && param->Nth(type_ix)) 
		m_links->link(param->Nth(type_ix), type);
	    // Skip keywords (eg: register) which are Leaves
	    for (int ix = 0; ix < type_ix && param->Nth(ix)->IsLeaf(); ix++) {
		Ptree* leaf = param->Nth(ix);
		if (premods.size() > 0) premods.append(" ");
		premods.append(leaf->GetPosition(), leaf->GetLength());
	    }
	    // Find name
	    if (Ptree* pname = param->Nth(type_ix+1)) {
	        if (!pname->IsLeaf() && pname->Last() && pname->Last()->Car()) {
		    // * and & modifiers are stored with the name so we must skip them
		    Ptree* last = pname->Last()->Car();
		    if (!last->Eq('*') && !last->Eq('&'))
			// The last node is the name:
			name = last->ToString();
	      	}
	    }
	    // Find value
	    if (value_ix >= 0) value = param->Nth(value_ix)->ToString();
        }
        // Add the AST.Parameter type to the list
        params.push_back(new AST::Parameter(premods, type, "", name, value));
        p_params = Ptree::Rest(p_params);
    }
}

void SWalker::TranslateFunctionName(char* encname, std::string& realname, Type::Type*& returnType)
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
    STrace trace("SWalker::TranslateTypespecifier");
    Ptree *class_spec = GetClassOrEnumSpec(tspec);
    if (class_spec) Translate(class_spec);
    return 0;
}

Ptree* SWalker::TranslateTypedef(Ptree* node) 
{
    STrace trace("SWalker::TranslateTypedef");
    if (m_links) m_links->span(node->First(), "file-keyword");
    /* Ptree *tspec = */ TranslateTypespecifier(node->Second());
    m_declaration = static_cast<PtreeDeclaration*>(node); // this may be bad, but we only use as ptree anyway
    m_store_decl = true;
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
    std::string name = m_decoder->decodeName(encname);
    // Create typedef object
    AST::Typedef* tdef = m_builder->addTypedef(m_lineno, name, type, false);
    addComments(tdef, dynamic_cast<PtreeDeclarator*>(node));
    
    // if storing links, find name
    if (m_links) {
	if (m_store_decl && m_declaration->Second()) {
	    m_links->link(m_declaration->Second(), type);
	}
	Ptree* p = node;
	while (p && p->Car()->IsLeaf() && (p->Car()->Eq('*') || p->Car()->Eq('&'))) p = Ptree::Rest(p);
	if (p) {
	    // p should now be at the name
	    m_links->link(p->Car(), tdef);
	}
    }
}

Ptree* SWalker::TranslateFunctionImplementation(Ptree* node)
{
    STrace trace("SWalker::TranslateFunctionImplementation");
    m_operation = 0; m_params.clear();
    TranslateDeclarator(node->Third());
    if (!m_links) return 0; // Dont translate if not storing links
    if (m_filename != m_source) return 0; // Dont translate if not main file
    if (!m_operation) { cerr << "Warning: operation was null!" << endl; return 0; }

    FuncImplCache cache;
    cache.oper = m_operation;
    cache.params = m_param_cache;
    cache.body = node->Nth(3);

    if (dynamic_cast<AST::Class*>(m_builder->scope())) {
	m_func_impl_stack.back().push_back(cache);
    } else {
	TranslateFuncImplCache(cache);
    }
    return 0;
}

void SWalker::TranslateFuncImplCache(const FuncImplCache& cache)
{
    STrace trace("SWalker::TranslateFuncImplCache");
    // We create a dummy namespace with the name of the function. Any
    // declarations in the function are added to this dummy namespace. Once we
    // are done, we remove it from the parent scope (its not much use in the
    // documents)
    std::vector<std::string> name = cache.oper->name();
    name.back() = "`"+name.back();
    m_builder->startFunctionImpl(name);
    try {
	// Add parameters
	std::vector<AST::Parameter*>::const_iterator iter, end;
	iter = cache.params.begin();
	end = cache.params.end();
	while (iter != end) {
	    AST::Parameter* param = *iter++;
	    m_builder->addVariable(m_lineno, param->name(), param->type(), false, "parameter");
	}
	// Add 'this' if method
	m_builder->addThisVariable();
	// Translate the function body
	TranslateBlock(cache.body);

    } catch (...) {
	LOG("Cleaning up func impl cache");
	m_builder->endFunctionImpl();
	throw;
    }
    m_builder->endFunctionImpl();
}

Ptree* SWalker::TranslateAccessSpec(Ptree* spec)
{
    STrace trace("SWalker::TranslateAccessSpec");
    AST::Access axs = AST::Default;
    switch (spec->First()->What()) {
	case PUBLIC: axs = AST::Public; break;
	case PROTECTED: axs = AST::Protected; break;
	case PRIVATE: axs = AST::Private; break;
    }
    m_builder->setAccess(axs);
    if (m_links) m_links->span(spec->First(), "file-keyword");
    return 0;
}

/* Enum Spec
 *  [ enum [name] [{ [name [= value] ]* }] ]
 */
Ptree *SWalker::TranslateEnumSpec(Ptree *spec)
{
    //updateLineNumber(spec);
    if (m_links) m_links->span(spec->First(), "file-keyword");
    if (!spec->Second()) { return 0; /* anonymous enum */ }
    std::string name = spec->Second()->ToString();

    updateLineNumber(spec);
    int enum_lineno = m_lineno;
    // Parse enumerators
    std::vector<AST::Enumerator*> enumerators;
    Ptree* penum = spec->Third()->Second();
    AST::Enumerator* enumor;
    while (penum) {
	updateLineNumber(penum);
	Ptree* penumor = penum->First();
	if (penumor->IsLeaf()) {
	    // Just a name
	    enumor = m_builder->addEnumerator(m_lineno, penumor->ToString(), "");
	    addComments(enumor, static_cast<CommentedLeaf*>(penumor)->GetComments());
	    if (m_links) m_links->link(penumor, enumor);
	} else {
	    // Name = Value
	    std::string name = penumor->First()->ToString(), value;
	    if (penumor->Length() == 3) {
		value = penumor->Third()->ToString();
	    }
	    enumor = m_builder->addEnumerator(m_lineno, name, value);
	    addComments(enumor, dynamic_cast<CommentedLeaf*>(penumor->First()));
	    if (m_links) m_links->link(penumor->First(), enumor);
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
    if (m_declaration) {
	// Enum declared inside declaration. Comments for the declaration
	// belong to the enum. This is policy. #TODO review policy
	m_declaration->SetComments(nil);
    }
    if (m_links) m_links->link(spec->Second(), theEnum);
    return 0;
}


Ptree* SWalker::TranslateUsing(Ptree* node) {
    STrace trace("SWalker::TranslateUsing");
    // [ using Foo :: x ; ]
    // [ using namespace Foo ; ]
    // [ using namespace Foo = Bar ; ]
    if (m_links) m_links->span(node->First(), "file-keyword");
    bool is_namespace = false;
    Ptree *p = node->Rest();
    if (p->First()->Eq("namespace")) {
	if (m_links) m_links->span(p->First(), "file-keyword");
	// Find namespace to alias
	p = p->Rest();
	is_namespace = true;
    }
    // Find name that we are looking up, and make a new ptree list for linking it
    Ptree *p_name = nil;
    p_name = Ptree::Snoc(p_name, p->Car());
    AST::Name name;
    name.push_back(getName(p->First()));
    p = p->Rest(); 
    while (p->First()->Eq("::")) {
	p_name = Ptree::Snoc(p_name, p->Car()); // Add '::' to p_name
	p = p->Rest();
	name.push_back(getName(p->First()));
	p_name = Ptree::Snoc(p_name, p->Car()); // Add identifier to p_name
	p = p->Rest();
    }
    // Resolve and link name
    Type::Named* type = m_builder->lookupType(name);
    if (m_links) m_links->link(p_name, type);
    if (is_namespace) {
	// Check for '=' alias
	if (p->First()->Eq("=")) {
	    p = p->Rest();
	    std::string alias = getName(p->First());
	    m_builder->usingNamespace(type, alias);
	} else {
	    m_builder->usingNamespace(type);
	}
    } else {
	// Let builder do all the work
	m_builder->usingDeclaration(type);
    }
    return 0;
}


