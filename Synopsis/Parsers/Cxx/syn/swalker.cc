// $Id: swalker.cc,v 1.28 2001/05/25 03:08:49 chalky Exp $
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

using namespace AST;

// ------------------------------------------------------------------
//                 -- CLASS STrace --
// ------------------------------------------------------------------

#ifdef DO_TRACE
// Class variable definitions
int STrace::slevel = 0, STrace::dlevel = 0;
std::ostrstream* STrace::stream = 0;
STrace::list STrace::m_list;
#endif

// ------------------------------------------------------------------
//                 -- CLASS SWalker --
// ------------------------------------------------------------------


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
      m_store_links(false),
      m_storage(0),
      m_store_decl(false),
      m_type_formatter(new TypeFormatter()),
      m_operation(0),
      m_type(0),
      m_scope(0),
      m_postfix_flag(Postfix_Var)
{
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

// Returns the column number of 'find', by counting backwards until a newline
// is encountered, or 'start' (the start of the buffer). The returned column
// number is mapped by the link_map.
int SWalker::find_col(const char* start, const char* find)
{
    const char* pos = find;
    while (pos > start && *--pos != '\n');
    int col = find - pos;
    // Resolve macro maps
    return link_map::instance().map(m_lineno, col);
}

//. Store a link at the given node
void SWalker::storeLink(Ptree* node, bool def, const std::vector<std::string>& name)
{
    if (!m_store_links) return;
    updateLineNumber(node);
    if (m_filename != m_source) return;
    int col = find_col(m_program->Read(0), node->LeftMost());
    if (col < 0) return; // inside macro
    int len = node->RightMost() - node->LeftMost();
    
    (*m_storage) << m_lineno << " " << col << " " << len << (def ? " DEF" : " REF");
    for (std::vector<std::string>::const_iterator iter = name.begin(); iter != name.end(); ++iter) {
	std::string word = *iter;
	for (std::string::size_type pos = word.find(' '); pos != std::string::npos; pos = word.find(' ', pos)) {
	    word[pos] = 160; // 'unbreakable space', for want of something better
	}
	(*m_storage) << " " << word;
    }
    (*m_storage) << "\n";
}

// Store if type is named
void SWalker::storeLink(Ptree* node, bool def, Type::Type* type)
{
    Type::Base* base = dynamic_cast<Type::Base*>(type);
    if (base) return;
    Type::Named* named = dynamic_cast<Type::Named*>(type);
    if (named) { storeLink(node, def, named->name()); return; }
    Type::Modifier* mod = dynamic_cast<Type::Modifier*>(type);
    if (mod) {
	// If mod is a "const" premod, then we have to skip the "const" bit:
	if (mod->pre().size() && mod->pre().front() == "const") {
	    node = node->Last()->First();
	}
	storeLink(node, def, mod->alias());
	return;
    }
    Type::Parameterized* param = dynamic_cast<Type::Parameterized*>(type);
    if (param) {
	// Do template
	storeLink(node->First(), false, param->templateType());
	// Do params
	node = node->Second();
	Type::Type::vector_t::iterator iter = param->parameters().begin();
	while (node) {
	    // Skip '<' or ','
	    if (!(node = node->Rest())) break;
	    if (node->Car() && node->Car()->Car() && !node->Car()->Car()->IsLeaf() && node->Car()->Car()->Car())
		storeLink(node->Car()->Car()->Car(), false, *iter++);
	    if (iter == param->parameters().end()) break;
	    node = node->Rest();
	}
	return;
    }

    //cout << "Unknown type for storage: "; node->Display2(cout);
}

//. Store a span at the given node
void SWalker::storeSpan(Ptree* node, const char* clas)
{
    if (!m_store_links) return;
    updateLineNumber(node);
    if (m_filename != m_source) return;
    int col = find_col(m_program->Read(0), node->LeftMost());
    if (col < 0) return; // inside macro
    int len = node->RightMost() - node->LeftMost();
    
    (*m_storage) << m_lineno<<" "<<col<<" "<<len<<" "<<clas<<"\n";
}

//. Store a possibly multi-line span at the given node
void SWalker::storeLongSpan(Ptree* node, const char* clas)
{
    if (!m_store_links) return;
    // Find left edge
    updateLineNumber(node);
    if (m_filename != m_source) return;
    int left_col = find_col(m_program->Read(0), node->LeftMost());
    if (left_col < 0) return; // inside macro
    int len = node->RightMost() - node->LeftMost();
    // Find right edge
    char* fname; int fname_len;
    int right_line = m_parser->LineNumber(node->RightMost(), fname, fname_len);
    if (right_line == m_lineno) {
	// Same line, normal output
	(*m_storage) << m_lineno<<" "<<left_col<<" "<<len<<" "<<clas<<"\n";
    } else {
	// One for each line
	int right_col = find_col(m_program->Read(0), node->RightMost());
	for (int line = m_lineno; line < right_line; line++, left_col = 0)
	    (*m_storage)<<line<<" "<<left_col<<" -1 "<<clas<<"\n";
	// Last line is a bit different
	(*m_storage)<<right_line<<" 0 "<<right_col<<" "<<clas<<"\n";
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


// Adds the given comments to the given declaration. If m_store_links is set,
// then syntax highlighting information is also stored.
void SWalker::addComments(AST::Declaration* decl, Ptree* comments)
{
    while (comments) {
	if (decl) decl->comments().push_back(new AST::Comment("", 0, comments->First()->ToString()));
	if (m_store_links) storeLongSpan(comments->First(), "file-comment");
	comments = comments->Rest();
    }
}

// -- These methods implement addComments for various node types that store
// comment pointers
void SWalker::addComments(AST::Declaration* decl, CommentedLeaf* node)
{
    if (node && node) addComments(decl, node->GetComments());
}
void SWalker::addComments(AST::Declaration* decl, PtreeDeclaration* node)
{
    if (decl && node) addComments(decl, node->GetComments());
}
void SWalker::addComments(AST::Declaration* decl, PtreeDeclarator* node)
{
    if (decl && node) addComments(decl, node->GetComments());
}
void SWalker::addComments(AST::Declaration* decl, PtreeNamespaceSpec* node)
{
    if (decl && node) addComments(decl, node->GetComments());
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
	if (e.node) {
	    char* fname;
	    int fname_len;
	    int lineno = m_parser->LineNumber(node->LeftMost(), fname, fname_len);
	    std::ostrstream buf;
	    buf << at << " (" << std::string(fname, fname_len) << ":" << lineno << ")";
	    at.assign(buf.str(), buf.pcount());
	}
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
    if (*str >= '0' && *str <= '9') {
	// Assume whole node is a number
	storeSpan(node, "file-number");
	// TODO: decide if Long, Float, Double, etc
	m_type = m_builder->lookupType("int");
    } else if (*str == '\'') {
	// Whole node is a char literal
	storeSpan(node, "file-string");
	m_type = m_builder->lookupType("char");
    } else if (*str == '"') {
	// Assume whole node is a string
	storeSpan(node, "file-string");
	m_type = m_builder->lookupType("char");
	Type::Type::Mods pre, post; pre.push_back("const"); post.push_back("*");
	m_type = new Type::Modifier(m_type, pre, post);
    } else {
	cout << "Warning: Unknown Ptree "; node->Display2(cout);
    }
    return 0;
}

//. NamespaceSpec
//. [ namespace <identifier> [{ body }] ]
Ptree* SWalker::TranslateNamespaceSpec(Ptree* def) {
    STrace trace("SWalker::TranslateNamespaceSpec");
    
    if (m_store_links) storeSpan(def->First(), "file-keyword");
    else updateLineNumber(def);

    AST::Namespace* ns;
    if (def->Cadr()) ns = m_builder->startNamespace(getName(def->Cadr()), Builder::NamespaceNamed);
    else ns = m_builder->startNamespace(m_filename, Builder::NamespaceAnon);

    addComments(ns, dynamic_cast<PtreeNamespaceSpec*>(def));
    if (m_store_links && def->Cadr()) storeLink(def->Second(), true, ns->name());

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
	    if (m_store_links) storeSpan(node->Car()->Nth(i), "file-keyword");
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
	if (m_store_links) storeLink(name, false, type);

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

	if (m_store_links) storeSpan(node->First(), "file-keyword");
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
	if (m_store_links) storeLink(node->Second(), true, clas->name());

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
	if (m_store_links) { // highlight the comments, at least
	    PtreeClassSpec* cspec = static_cast<PtreeClassSpec*>(node);
	    addComments(NULL, cspec->GetComments());
	}
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
	m_params = params;

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
	    } catch (const std::bad_cast &) {
// 		throw ERROR("Qualified function name wasn't a function:" << names);
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
	if (m_store_links) {
	    // Store for use by TranslateFunctionImplementation
	    m_operation = oper;

	    // Do decl type first
	    if (m_store_decl) {
		if (m_declaration->Second()) storeLink(m_declaration->Second(), false, returnType);
	    }

	    p = decl;
	    while (p && p->Car()->IsLeaf() && (p->Car()->Eq('*') || p->Car()->Eq('&'))) p = Ptree::Rest(p);
	    if (p) {
		// p should now be at the name
		storeLink(p->Car(), true, oper->name());
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
	AST::Variable* var = m_builder->addVariable(m_lineno, name, type, false);
	//if (m_declaration->GetComments()) addComments(var, m_declaration->GetComments());
	//if (decl->GetComments()) addComments(var, decl->GetComments());
	addComments(var, m_declaration);
	addComments(var, dynamic_cast<PtreeDeclarator*>(decl));
	
	// if storing links, find name
	if (m_store_links) {
	    // Do decl type first
	    if (m_store_decl) {
		if (m_declaration->Second()) storeLink(m_declaration->Second(), false, type);
	    }

	    Ptree* p = decl;
	    while (p && p->Car()->IsLeaf() && (p->Car()->Eq('*') || p->Car()->Eq('&'))) p = Ptree::Rest(p);
	    if (p) {
		// p should now be at the name
		storeLink(p->Car(), true, var->name());

		// Next might be '=' then expr
		p = p->Rest();
		if (p && p->Car() && p->Car()->Eq('=')) {
		    p = p->Rest();
		    if (p && p->Car()) Translate(p->Car());
		}
	    }
	}
    }
    return 0;
}

// Fills the vector of Parameter types by parsing p_params.
void SWalker::TranslateParameters(Ptree* p_params, std::vector<Parameter*>& params)
{
  while (p_params)
    {
      // A parameter has a type, possibly a name and possibly a value
      std::string name, value;
      if (p_params->Car()->Eq(',')) p_params = p_params->Cdr();
      Ptree* param = p_params->First();
      // The type is stored in the encoded type string already
      Type::Type* type = m_decoder->decodeType();
      if (!type)
	{
	  std::cout << "Premature end of decoding!" << std::endl;
	  break; // NULL means end of encoding
	}
      // Link type
      if (m_store_links && !param->IsLeaf() && param->First()) 
	storeLink(param->First(), false, type);
      // Find name and value
      // FIXME: this doesnt account for anon but initialised params!
      if (param->Length() > 1)
	{
	  Ptree* pname = param->Second();
	  if (pname && pname->Car())
	    {
	      // * and & modifiers are stored with the name so we must skip them
	      while (pname && (pname->Car()->Eq('*') || pname->Car()->Eq('&'))) pname = pname->Cdr();
	      // Extract name
	      if (pname) name = pname->Car()->ToString();
	    }
	  // If there are three cells, they are 1:name 2:= 3:value
	  if (param->Length() > 2) value = param->Nth(3)->ToString();
	}
      // Add the AST.Parameter type to the list
      params.push_back(new AST::Parameter("", type, "", name, value));
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
    // /* Ptree *tspec = */ TranslateTypespecifier(node->Second());
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
    if (m_store_links) {
	if (m_store_decl) {
	    if (m_declaration->Second()) storeLink(m_declaration->Second(), false, type);
	}
	Ptree* p = node;
	while (p && p->Car()->IsLeaf() && (p->Car()->Eq('*') || p->Car()->Eq('&'))) p = Ptree::Rest(p);
	if (p) {
	    // p should now be at the name
	    storeLink(p->Car(), true, tdef->name());
	}
    }
}

Ptree* SWalker::TranslateFunctionImplementation(Ptree* node)
{
    STrace trace("SWalker::TranslateFunctionImplementation");
    m_operation = 0; m_params.clear();
    TranslateDeclarator(node->Third());
    if (!m_store_links) return 0;
    if (!m_operation) { cerr << "Warning: operation was null!" << endl; return 0; }
    FuncImplCache cache;
    cache.oper = m_operation;
    cache.params = m_params;
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
    name.back() = "{"+name.back()+"}";
    m_builder->startFunctionImpl(name);
    try {
	// Add parameters
	std::vector<AST::Parameter*>::const_iterator iter, end;
	iter = cache.params.begin();
	end = cache.params.end();
	while (iter != end) {
	    AST::Parameter* param = *iter++;
	    m_builder->addVariable(m_lineno, param->name(), param->type(), false);
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
    if (m_store_links) storeSpan(spec->First(), "file-keyword");
    return 0;
}

/* Enum Spec
 *  [ enum [name] [{ [name [= value] ]* }] ]
 */
Ptree *SWalker::TranslateEnumSpec(Ptree *spec)
{
    //updateLineNumber(spec);
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
	    storeLink(penumor, true, enumor->name());
	} else {
	    // Name = Value
	    std::string name = penumor->First()->ToString(), value;
	    if (penumor->Length() == 3) {
		value = penumor->Third()->ToString();
	    }
	    enumor = m_builder->addEnumerator(m_lineno, name, value);
	    addComments(enumor, dynamic_cast<CommentedLeaf*>(penumor->First()));
	    storeLink(penumor->First(), true, enumor->name());
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
    storeLink(spec->Second(), true, theEnum->name());
    return 0;
}




