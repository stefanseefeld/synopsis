// $Id: linkstore.hh,v 1.2 2002/01/25 14:24:33 chalky Exp $
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
// $Log: linkstore.hh,v $
// Revision 1.2  2002/01/25 14:24:33  chalky
// Start of refactoring and restyling effort.
//
// Revision 1.1  2001/06/10 00:31:39  chalky
// Refactored link storage, better comments, better parsing
//
//

#ifndef H_LINKSTORE
#define H_LINKSTORE

#include "ast.hh"
#include <iostream>
class Parser;
class Ptree;
class SWalker;

//. Stores link information about the file. Link info is stored in two files
//. with two purposes. The first stores all links and non-link spans, in a
//. simple text file with one record per line and with spaces as field
//. separators.
class LinkStore {
public:
    //. Enumeration of record types
    enum Type {
	Reference,
	Definition,
	Span
    };
    
    //. Constructor.
    //. @param out the output stream to write the links to
    //. @param swalker the SWalker object we are linking for
    LinkStore(std::ostream& out, SWalker* swalker);

    //. Store a link
    void link(int line, int col, int len, Type type, const ScopedName& name, const std::string& desc);

    //. Store a link for the given Ptree node
    void link(Ptree* node, Type, const ScopedName& name, const std::string& desc);

    //. Store a Definition link for the given Ptree node using the AST node
    void link(Ptree* node, const AST::Declaration* decl);

    //. Store a Reference link for the given node using the given Type
    void link(Ptree* node, Types::Type*);

    //. Store a span
    void span(int line, int col, int len, const char* desc);

    //. Store a span for the given Ptree node
    void span(Ptree* node, const char* desc);

    //. Store a long (possibly multi-line) span
    void longSpan(Ptree* node, const char* desc);

    //. Returns the SWalker
    SWalker* getSWalker();

protected:
    //. Calculates the column number of 'ptr'. m_buffer_start is used as a
    //. lower bounds, since the function counts backwards until it finds a
    //. newline. As an added bonus, the returned column number is adjusted
    //. using the link map generated from expanding macros so it can be output
    //. straight to the link file :) The adjustment requires the line number.
    int find_col(int line, const char* ptr);

    //. The field separator
    static char* const FS = " ";

    //. The record separator
    static char* const RS = "\n";
    
    //. The start of the program buffer
    const char* m_buffer_start;

    //. The output file for links
    std::ostream& m_link_file;

    //. The Parser object
    Parser* m_parser;

    //. The SWalker object
    SWalker* m_walker;

    //. Names for the Type enum
    static const char* m_type_names[];
};

#endif
