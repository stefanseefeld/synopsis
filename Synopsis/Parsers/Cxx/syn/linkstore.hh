// Synopsis C++ Parser: linkstore.hh header file
// The LinkStore class which stores syntax and xref link info

// $Id: linkstore.hh,v 1.7 2002/11/17 12:11:43 chalky Exp $
//
// This file is a part of Synopsis.
// Copyright (C) 2000-2002 Stephen Davies
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
// Revision 1.7  2002/11/17 12:11:43  chalky
// Reformatted all files with astyle --style=ansi, renamed fakegc.hh
//
// Revision 1.6  2002/11/02 06:37:38  chalky
// Allow non-frames output, some refactoring of page layout, new modules.
//
// Revision 1.5  2002/02/19 09:05:16  chalky
// Applied patch from David Abrahams to help compilation on Cygwin
//
// Revision 1.4  2002/01/30 11:53:15  chalky
// Couple bug fixes, some cleaning up.
//
// Revision 1.3  2002/01/28 13:17:24  chalky
// More cleaning up of code. Combined xref into LinkStore. Encoded links file.
//
// Revision 1.2  2002/01/25 14:24:33  chalky
// Start of refactoring and restyling effort.
//
// Revision 1.1  2001/06/10 00:31:39  chalky
// Refactored link storage, better comments, better parsing
//

#ifndef H_SYNOPSIS_CPP_LINKSTORE
#define H_SYNOPSIS_CPP_LINKSTORE

#include "ast.hh"
#include <iostream>
class Parser;
class Ptree;
class SWalker;

//. Stores link information about the file. Link info is stored in two files
//. with two purposes.
//.
//. The first file stores all links and non-link spans, in a
//. simple text file with one record per line and with spaces as field
//. separators. The fields themselves are encoded using URL-style %FF encoding
//. of non alpha-numeric characters (including spaces, brackers, commas etc).
//. The purpose of this file is for syntax-hightlighting of source files.
//.
//. The second file stores only cross-reference information, which is a subset
//. of the first file.
class LinkStore
{
public:
    //. Enumeration of record types
    enum Context
    {
        Reference,	        //.< General name reference
        Definition,         //.< Definition of the declaration
        Span,               //.< Non-declarative span of text
        Implementation,     //.< Implementation of a declaration
        UsingDirective,     //.< Referenced in a using directive
        UsingDeclaration,   //.< Referenced in a using declaration
        FunctionCall,       //.< Called as a function
        NumContext          //.< Marker used to check size of array
    };

    //. Constructor. Either or both of the streams may be null.
    //. @param syntax_stream the output stream to write the syntax links to
    //. @param xref_stream the output stream to write the xref records to
    //. @param swalker the SWalker object we are linking for
    //. @param basename the basename to strip from xref records
    LinkStore(std::ostream* syntax_stream, std::ostream* xref_stream, SWalker* swalker, const std::string& basename);

    //. Store a link for the given Ptree node. If a decl is given, store an
    //. xref too
    void link(Ptree* node, Context, const ScopedName& name, const std::string& desc, const AST::Declaration* decl = NULL);

    //. Store a Definition link for the given Ptree node using the AST node
    void link(Ptree* node, const AST::Declaration* decl);

    //. Store a link for the given node using the given Context, which defaults
    //. to a Reference
    void link(Ptree* node, Types::Type*, Context = Reference);

    //. Store a span
    void span(int line, int col, int len, const char* desc);

    //. Store a span for the given Ptree node
    void span(Ptree* node, const char* desc);

    //. Store a long (possibly multi-line) span
    void long_span(Ptree* node, const char* desc);

    //. Returns the SWalker
    SWalker* swalker();

    //. Encode a string to the output stream
    //. Usage: cout << encode("some string") << endl; output--> "some%20string\n"
    class encode;

    //. Encode a ScopedName to the output stream
    class encode_name;

protected:
    //. Store a link in the Syntax File
    void store_syntax_record(int line, int col, int len, Context context, const ScopedName& name, const std::string& desc);

    //. Store a link in the CrossRef File
    void store_xref_record(const AST::Declaration* decl, const std::string& file, int line, Context context);

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

    //. The output stream for syntax links
    std::ostream* m_syntax_stream;

    //. The output stream for xref entries
    std::ostream* m_xref_stream;

    //. The Parser object
    Parser* m_parser;

    //. The SWalker object
    SWalker* m_walker;

    //. Names for the Context enum
    static const char* m_context_names[];

    //. The basename to strip from xref records
    std::string m_basename;
};

#endif
// vim: set ts=8 sts=4 sw=4 et:
