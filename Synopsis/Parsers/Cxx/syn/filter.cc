// Synopsis C++ Parser: filter.cc source file
// Implementation of the FileFilter class

// $Id: filter.cc,v 1.3 2002/12/09 12:14:10 chalky Exp $
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

#include "filter.hh"
#include <map>

struct FileFilter::Private
{
    //. Whether only main declarations should be stored
    bool only_main;

    //. The main filename
    std::string main_filename;

    //. The basename
    std::string basename;

    //. A vector of strings
    typedef std::vector<std::string> string_vector;

    //. The extra filenames
    string_vector extra_filenames;

    //. The main syntax filename
    std::string syntax_filename;

    //. The main xref filename
    std::string xref_filename;

    //. The syntax filename prefix
    std::string syntax_prefix;

    //. The xref filename prefix
    std::string xref_prefix;

    //. Type of the map from filename to SourceFile
    typedef std::map<std::string, AST::SourceFile*> file_map_t;

    //. A map from filename to SourceFile
    file_map_t file_map;

    //. The type of syntax or xref being used
    enum StoreType
    {
        None,
        Filename,
        Prefix
    };

    //. The type of syntax filename being used
    StoreType syntax;

    //. The type of xref filename being used
    StoreType xref;
};

// Constructor
FileFilter::FileFilter()
{
    m = new Private;
    m->only_main = true;
    m->syntax = Private::None;
    m->xref = Private::None;
}

// Destructor
FileFilter::~FileFilter()
{
    delete m;
}

// Sets whether only declarations in the main file(s) should be stored
void FileFilter::set_only_main(bool value)
{
    m->only_main = value;
}

// Sets the main filename
void FileFilter::set_main_filename(const char* filename)
{
    m->main_filename = filename;
}

// Adds a list of extra filenames to store info for
void FileFilter::add_extra_filenames(const std::vector<const char*>& filenames)
{
    std::vector<const char*>::const_iterator iter;
    for (iter = filenames.begin(); iter != filenames.end(); iter++)
        m->extra_filenames.push_back(*iter);
}

// Sets the basename
void FileFilter::set_basename(const char* basename)
{
    m->basename = basename;
    // Make sure it ends in / if it is set
    if (m->basename.size() > 0 && m->basename[m->basename.size()-1] != '/')
        m->basename.append("/");
}

// Sets the filename for syntax output.
void FileFilter::set_syntax_filename(const char* filename)
{
    m->syntax_filename = filename;
    m->syntax = Private::Filename;
}

// Sets the filename for xref output.
void FileFilter::set_xref_filename(const char* filename)
{
    m->xref_filename = filename;
    m->xref = Private::Filename;
}

// Sets the prefix for syntax output filenames.
void FileFilter::set_syntax_prefix(const char* filename)
{
    m->syntax_prefix = filename;
    m->syntax = Private::Prefix;
}

// Sets the prefix for xref output filenames.
void FileFilter::set_xref_prefix(const char* filename)
{
    m->xref_prefix = filename;
    m->xref = Private::Prefix;
}


// Returns the AST::SourceFile for the given filename.
AST::SourceFile* FileFilter::get_sourcefile(const char* filename_ptr, size_t length)
{
    std::string filename;
    if (length != 0)
        filename.assign(filename_ptr, length);
    else
        filename.assign(filename_ptr);
    
    // Look in map
    Private::file_map_t::iterator iter = m->file_map.find(filename);
    if (iter != m->file_map.end())
        // Found
        return iter->second;
    
    // Not found, create a new SourceFile. Note filename in the object is
    // stripped of the basename
    AST::SourceFile* file = new AST::SourceFile(
            strip_basename(filename), 
            is_main(filename));

    // Add to the map
    m->file_map[filename] = file;

    return file;
}

bool FileFilter::is_main(std::string filename)
{
    //std::cout << " Comparing " << filename << " to main: " << m->main_filename << std::endl;
    if (filename == m->main_filename)
        return true;

    Private::string_vector::iterator iter = m->extra_filenames.begin();
    while (iter != m->extra_filenames.end())
        if (filename == *iter++)
            return true;

    return false;
}

// Returns whether a function implementation in the given file should
// have it's Ptree walked. This will be true only if the given file is
// one of the files to be stored. 
bool FileFilter::should_visit_function_impl(AST::SourceFile* file)
{
    // First check if not linking or xreffing
    if (m->syntax == Private::None && m->xref == Private::None)
        return false;

    return file->is_main();
}


// Returns true if links should be generated for the given sourcefile
bool FileFilter::should_link(AST::SourceFile* file)
{
    if (m->syntax == Private::None)
        return false;

    return file->is_main();
}

// Returns true if xref info should be generated for the given sourcefile
bool FileFilter::should_xref(AST::SourceFile* file)
{
    if (m->xref == Private::None)
        return false;

    return file->is_main();
}

// Returns true if the given declaration should be stored in the final
// AST.
bool FileFilter::should_store(AST::Declaration* decl)
{
    // Sanity check (this can happen)
    if (!decl)
        return false;

    // Check the decl itself first, although for namespaces the SourceFile
    // referenced by file() can be any of the files that opened it
    if (decl->file()->is_main())
        return true;

    if (AST::Scope* scope = dynamic_cast<AST::Scope*>(decl))
    {
        // Check all members of the namespace or class
        AST::Declaration::vector::iterator iter;
        AST::Declaration::vector& declarations = scope->declarations();
        for (iter = declarations.begin(); iter != declarations.end(); iter++)
            if (should_store(*iter))
                // Only takes one to store the decl. Each contained declaration
                // will then be evaluated separately
                return true;
    }

    return false;
}

// Strip basename from filename
std::string FileFilter::strip_basename(const std::string& filename)
{
    if (m->basename.size() == 0)
        return filename;
    size_t length = m->basename.size();
    if (length > filename.size())
        return filename;
    if (strncmp(filename.data(), m->basename.data(), length) == 0)
        return filename.substr(length);
    return filename;
}

// Return syntax filename
std::string FileFilter::get_syntax_filename(AST::SourceFile* file)
{
    if (m->syntax == Private::Filename)
        return m->syntax_filename;

    return m->syntax_prefix + file->filename();
}

// Return xref filename
std::string FileFilter::get_xref_filename(AST::SourceFile* file)
{
    if (m->xref == Private::Filename)
        return m->xref_filename;

    return m->xref_prefix + file->filename();
}

// Returns all sourcefiles
void FileFilter::get_all_sourcefiles(AST::SourceFile::vector& all)
{
    Private::file_map_t::iterator iter;
    for (iter = m->file_map.begin(); iter != m->file_map.end(); iter++)
        all.push_back(iter->second);
}

// Returns all filenames
void FileFilter::get_all_filenames(const std::string*& main, const std::vector<std::string>*& extra)
{
    main = &m->main_filename;
    extra = &m->extra_filenames;
}

// vim: set ts=8 sts=4 sw=4 et:
