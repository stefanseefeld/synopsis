// Synopsis C++ Parser: filter.hh header file
// Defines the FileFilter class for filtering the AST based on filename

// $Id: filter.hh,v 1.3 2002/12/12 17:25:34 chalky Exp $
//
// This file is a part of Synopsis.
// Copyright (C) 2002 Stephen Davies
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

#ifndef H_SYNOPSIS_CPP_FILTER
#define H_SYNOPSIS_CPP_FILTER

#include <string>
#include <vector>
#include "ast.hh"

class FileFilter
{
public:
    //. Constructor
    FileFilter();

    //. Destructor
    ~FileFilter();

    //. Sets whether only declarations in the main file(s) should be stored
    void set_only_main(bool value);

    //. Sets the main filename
    void set_main_filename(const char* filename);

    //. Adds a list of extra filenames to store info for
    void add_extra_filenames(const std::vector<const char*>&);

    //. Sets the basename
    void set_basename(const char* basename);

    //. Sets the filename for syntax output.
    //. This should only be called if there are no extra files.
    void set_syntax_filename(const char* filename);

    //. Sets the filename for xref output.
    //. This should only be called if there are no extra files.
    void set_xref_filename(const char* filename);

    //. Sets the prefix for syntax output filenames.
    //. The syntax filename will be the source filename prepended with the
    //. prefix, so you probably want the prefix to be a directory.
    //. This can be called whether there are extra files or not.
    void set_syntax_prefix(const char* filename);

    //. Sets the prefix for xref output filenames.
    //. The xref filename will be the source filename prepended with the
    //. prefix, so you probably want the prefix to be a directory.
    //. This can be called whether there are extra files or not.
    void set_xref_prefix(const char* filename);


    //. Returns the AST::SourceFile for the given filename. If length is
    //. given, then the filename is assumed to be that length. This is useful
    //. since OCC returns filenames as a reference to the #line directive in
    //. the preprocessor output and is not null-terminated.
    AST::SourceFile* get_sourcefile(const char* filename, size_t length = 0);

    //. Returns whether a function implementation in the given file should
    //. have it's Ptree walked. This will be true only if the given file is
    //. one of the files to be stored. 
    bool should_visit_function_impl(AST::SourceFile* file);

    //. Returns true if links should be generated for the given sourcefile
    bool should_link(AST::SourceFile* file);

    //. Returns true if xref info should be generated for the given sourcefile
    bool should_xref(AST::SourceFile* file);

    //. Returns true if the given declaration should be stored in the final
    //. AST. Note that a Declaration is taken instead of a SourceFile because
    //. Namespaces may be declared (and contain declarations) in multiple
    //. files, and Classes may have nested classes defined in other files.
    bool should_store(AST::Declaration* decl);

    //. Strips a filename of the basename if present
    std::string strip_basename(const std::string& filename);

    //. Returns the filename to use for storing syntax info
    std::string get_syntax_filename(AST::SourceFile* file);

    //. Returns the filename to use for storing xref info
    std::string get_xref_filename(AST::SourceFile* file);

    //. Returns a list of all the sourcefiles
    void get_all_sourcefiles(AST::SourceFile::vector&);

    //. Returns all the filenames given to the parser.
    //. @param main a pointer to the main filename will be stored here
    //. @param extra a pointer to the extra filenames will be stored here
    void get_all_filenames(const std::string*& main, const std::vector<std::string>*& extra);

    //. Returns a pointer to the Filter instance. Note that Filter is *not* a
    //. regular singleton: instance() will return NULL if the Filter doesn't
    //. exist, and the constructor/destructor control this. The reason for
    //. this method is so the C function synopsis_include_hook can use the
    //. Filter object without having a reference to it.
    static FileFilter* instance();

private:
    struct Private;

    //. Compiler firewalled private data
    Private* m;

    //. Returns true if the given SourceFile is one of the main files
    //. (including extra files)
    bool is_main(std::string filename);

};

#endif

// vim: set ts=8 sts=4 sw=4 et:
