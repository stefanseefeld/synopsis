// $Id: linkstore.cc,v 1.5 2001/07/23 15:29:35 chalky Exp $
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
// $Log: linkstore.cc,v $
// Revision 1.5  2001/07/23 15:29:35  chalky
// Fixed some regressions and other mis-features
//
// Revision 1.4  2001/07/23 11:51:22  chalky
// Better support for name lookup wrt namespaces.
//
// Revision 1.3  2001/06/13 14:04:37  chalky
// s/std::cout/LOG/ or so
//
// Revision 1.2  2001/06/10 07:17:37  chalky
// Comment fix, better functions, linking etc. Better link titles
//
// Revision 1.1  2001/06/10 00:31:39  chalky
// Refactored link storage, better comments, better parsing
//
//

#include "swalker.hh"
#include "linkstore.hh"
#include "ast.hh"
#include "type.hh"
#include "link_map.hh"
#include "dumper.hh"
#include "builder.hh"
#include "strace.hh"

#include <strstream>

#include "../ptree.h"
#include "../parse.h"
#include "../buffer.h"


const char* LinkStore::m_type_names[] = {
    "REF",
    "DEF",
    "SPAN"
};

LinkStore::LinkStore(std::ostream& out, SWalker* swalker)
    : m_link_file(out),
      m_walker(swalker)
{
      m_buffer_start = swalker->getProgram()->Read(0);
      m_parser = swalker->getParser();
}

SWalker* LinkStore::getSWalker()
{
    return m_walker;
}

int LinkStore::find_col(int line, const char* ptr)
{
    const char* pos = ptr;
    while (pos > m_buffer_start && *--pos != '\n')
	; // do nothing inside loop
    int col = ptr - pos;
    // Resolve macro maps
    return link_map::instance().map(line, col);
}

void LinkStore::link(int line, int col, int len, Type type, const AST::Name& name, const std::string& desc)
{
    m_link_file << line << FS << col << FS << len << FS;
    m_link_file << m_type_names[type] << FS;
    for (AST::Name::const_iterator iter = name.begin(); iter != name.end(); ++iter) {
	std::string word = *iter;
	for (std::string::size_type pos = word.find(' '); pos != std::string::npos; pos = word.find(' ', pos)) {
	    word[pos] = 160; // 'unbreakable space', for want of something better
	}
	m_link_file << (iter == name.begin() ? "" : "\t") << word;
    }
    // Add the name to the description
    std::vector<AST::Scope*> scopes;
    ::Type::Named* vtype;
    AST::Name short_name;
    if (m_walker->getBuilder()->mapName(name, scopes, vtype)) {
	for (size_t i = 0; i < scopes.size(); i++) {
	    if (AST::Namespace* ns = dynamic_cast<AST::Namespace*>(scopes[i])) {
		if (ns->type() == "function") {
		    // Restart description at function scope
		    short_name.clear();
		    continue;
		}
	    }
	    // Add the name to the short name
	    short_name.push_back(scopes[i]->name().back());
	}
	// Add the final type name to the short name
	short_name.push_back(vtype->name().back());
    } else {
	STrace trace("LinkStore::link");
	LOG("WARNING: couldnt map name " << name);
	short_name = name;
    }
    m_link_file << FS << desc << " " << short_name << RS;
}

void LinkStore::link(Ptree* node, Type type, const AST::Name& name, const std::string& desc)
{
    if (!m_walker->isMainFile()) return;
    int line = m_walker->getLine(node);
    int col = find_col(line, node->LeftMost());
    if (col < 0) return; // inside macro
    int len = node->RightMost() - node->LeftMost();
    
    link(line, col, len, type, name, desc);
}

class TypeStorer : public Type::Visitor {
    // Variables to pass to link()
    LinkStore* links;
    Ptree* node;
    LinkStore::Type link_type;
public:
    //. Constructor
    TypeStorer(LinkStore* ls, Ptree* n, LinkStore::Type l)
	: links(ls), node(n), link_type(l) {}
    //. Returns a suitable description for the given type
    std::string describe(Type::Type* type) {
    	std::string desc;
	try {
	    AST::Declaration* decl = Type::declared_cast<AST::Declaration>(type);
	    return decl->type();
	} catch (const Type::wrong_type_cast&) {
	    return links->getSWalker()->getTypeFormatter()->format(type);
	}
    }

    // Visitor methods
    void visitBase(Type::Base* base) {
	// Not a link, but a keyword
	links->span(node, "file-keyword");
    }
    void visitNamed(Type::Named* named) {
	// All other nameds get stored
	links->link(node, link_type, named->name(), describe(named));
    }
    void visitModifier(Type::Modifier* mod) {
	// We recurse on the mod's alias, but dont link the const bit
	if (mod->pre().size() && mod->pre().front() == "const") {
	    if (!node->IsLeaf() && node->First()->Eq("const")) {
		links->span(node->First(), "file-keyword");
		node = node->Last()->First();
	    }
	}
	mod->alias()->accept(this);
    }
    void visitParameterized(Type::Parameterized* param) {
	// For qualified template names the ptree is:
	//  [ std :: [ vector [ < ... , ... > ] ] ]
	// Skip the qualifieds (and just link the final name)
	while (node->Second() && node->Second()->Eq("::")) {
	    node = node->Third();
	}
	// Do template
	links->link(node->First(), param->templateType());
	// Do params
	node = node->Second();
	using Type::Type;
	Type::vector_t::iterator iter = param->parameters().begin();
	Type::vector_t::iterator end = param->parameters().end();
	while (node && iter != end) {
	    // Skip '<' or ','
	    if ( !(node = node->Rest()) ) break;
	    if (node->Car() && node->Car()->Car() && !node->Car()->Car()->IsLeaf() && node->Car()->Car()->Car())
		links->link(node->Car()->Car()->Car(), *iter);
	    iter++;
	    node = node->Rest();
	}
    }
    // Other types ignored, for now
};

// Store if type is named
void LinkStore::link(Ptree* node, ::Type::Type* type)
{
    if (!m_walker->isMainFile() || !type) return;
    TypeStorer storer(this, node, Reference);
    type->accept(&storer);
}

void LinkStore::link(Ptree* node, const AST::Declaration* decl)
{
    if (!m_walker->isMainFile() || !decl) return;
    link(node, Definition, decl->name(), decl->type());
}

void LinkStore::span(int line, int col, int len, const char* desc)
{
    m_link_file << line << FS << col << FS << len << FS;
    m_link_file << m_type_names[Span] << FS << desc << RS;
}

void LinkStore::span(Ptree* node, const char* desc)
{
    int line = m_walker->getLine(node);
    if (!m_walker->isMainFile()) return;
    int col = find_col(line, node->LeftMost());
    if (col < 0) return; // inside macro
    int len = node->RightMost() - node->LeftMost();
    
    span(line, col, len, desc);
}

void LinkStore::longSpan(Ptree* node, const char* desc)
{
    // Find left edge
    int left_line = m_walker->getLine(node);
    if (!m_walker->isMainFile()) return;
    int left_col = find_col(left_line, node->LeftMost());
    if (left_col < 0) return; // inside macro
    int len = node->RightMost() - node->LeftMost();
    
    // Find right edge
    char* fname; int fname_len;
    int right_line = m_parser->LineNumber(node->RightMost(), fname, fname_len);
    
    if (right_line == left_line) {
	// Same line, so normal output
	span(left_line, left_col, len, desc);
    } else {
	// Must output one for each line
	int right_col = find_col(right_line, node->RightMost());
	for (int line = left_line; line < right_line; line++, left_col = 0)
	    span(line, left_col, -1, desc);
	// Last line is a bit different
	span(right_line, 0, right_col, desc);
    }
}
    
