// Synopsis C++ Parser: filter.cc source file
// Implementation of the FileFilter class

// $Id: filter.cc,v 1.6 2003/11/05 17:23:01 stefan Exp $
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

#include <Synopsis/Python/Module.hh>
#include <Synopsis/Trace.hh>
#include <Support/Path.hh>
#include "filter.hh"
#include <map>
#include <iostream>
#include <stdexcept>

using namespace Synopsis;

struct FileFilter::Private
{

    //. used during SourceFile imports
    PyObject *ast;

    //. Whether only main declarations should be stored
    bool only_main;

    //. The main filename
    std::string main_filename;

    //. The basename
    std::string base_path;

    //. A vector of strings
    typedef std::vector<std::string> string_vector;

    //. The syntax filename prefix
    std::string syntax_prefix;

    //. The xref filename prefix
    std::string xref_prefix;

    //. Type of the map from filename to SourceFile
    typedef std::map<std::string, AST::SourceFile*> file_map_t;

    //. A map from filename to SourceFile
    file_map_t file_map;
};

namespace
{
    //. The FileFilter instance
    FileFilter* filter_instance = 0;

//. restore the relevant parts from the python ast
//. The returned SourceFile object is a slice of the original python object
//. and should be merged back at the end of the parsing such that the other
//. parts not mirrored here are preserved.
AST::SourceFile *import_source_file(PyObject *ir,
				    const std::string &name,
				    const std::string &long_name,
				    bool main)
{
  AST::SourceFile *sourcefile = new AST::SourceFile(name, long_name, main);

  PyObject *files = PyObject_GetAttrString(ir, "files");
  assert(files);
  PyObject *py_source_file = PyDict_GetItemString(files, const_cast<char *>(name.c_str()));
  Py_DECREF(files);
  if (!py_source_file) return sourcefile; // the given file wasn't preprocessed into the AST

  PyObject *macro_calls = PyObject_GetAttrString(py_source_file, "macro_calls");
  if (macro_calls)
  {
    PyObject *lines = PyDict_Keys(macro_calls);
    long size = PyObject_Length(lines);
    for (long i = 0; i != size; ++i)
    {
      PyObject *line_number = PyList_GetItem(lines, i);
      int l = PyInt_AsLong(line_number);
      PyObject *line = PyDict_GetItem(macro_calls, line_number);
      long line_length = PyObject_Length(line);
      for (long j = 0; j != line_length; ++j)
      {
	PyObject *call = PyList_GetItem(line, j);
	PyObject *name, *start, *end, *diff;
	name = PyObject_GetAttrString(call, "name");
	start = PyObject_GetAttrString(call, "start");
	end = PyObject_GetAttrString(call, "end");
	diff = PyObject_GetAttrString(call, "diff");
	sourcefile->macro_calls().add(PyString_AsString(name), l,
				      PyInt_AsLong(start),
				      PyInt_AsLong(end),
				      PyInt_AsLong(diff));
      }
    }
    Py_DECREF(macro_calls);
  }
  return sourcefile;
}
}

// Constructor
FileFilter::FileFilter(PyObject *ast,
		       const std::string &filename,
		       const std::string &base_path,
		       bool main)
{
    m = new Private;
    m->ast = ast;
    m->main_filename = filename;
    m->base_path = base_path;
    m->only_main = main;
    filter_instance = this;
}

// Destructor
FileFilter::~FileFilter()
{
    delete m;
    filter_instance = 0;
}

// Return instance pointer
FileFilter* FileFilter::instance()
{
    return filter_instance;
}

// Sets the prefix for syntax output filenames.
void FileFilter::set_syntax_prefix(const char* filename)
{
    m->syntax_prefix = filename;
    if (m->syntax_prefix.size() > 0
        && m->syntax_prefix[m->syntax_prefix.size()-1] != '/')
       m->syntax_prefix.append("/");
}

// Sets the prefix for xref output filenames.
void FileFilter::set_xref_prefix(const char* filename)
{
    m->xref_prefix = filename;
    if (m->xref_prefix.size() > 0
        && m->xref_prefix[m->xref_prefix.size()-1] != '/')
       m->xref_prefix.append("/");
}


// Returns the AST::SourceFile for the given filename.
AST::SourceFile* FileFilter::get_sourcefile(const char* filename_ptr, size_t length)
{
    std::string filename;
    if (length != 0)
        filename.assign(filename_ptr, length);
    else
        filename.assign(filename_ptr);
    
    Path path = Path(filename).abs();
    std::string long_filename = path.str();
    path.strip(m->base_path);
    std::string short_filename = path.str();
    // Look in map
    Private::file_map_t::iterator iter = m->file_map.find(long_filename);
    if (iter != m->file_map.end())
        // Found
        return iter->second;
    
    // Not found, create a new SourceFile. Note filename in the object is
    // stripped of the basename
    AST::SourceFile* file = import_source_file(m->ast,
					       short_filename,
					       long_filename, 
					       is_main(long_filename));

    // Add to the map
    m->file_map[long_filename] = file;

    return file;
}

bool FileFilter::is_main(std::string filename)
{
    if (filename == m->main_filename) return true;
    else if (m->only_main) return false;

    if (m->base_path.size() == 0) return true;

    size_t length = m->base_path.size();
    if (length > filename.size()) return false;
    return strncmp(filename.data(), m->base_path.data(), length) == 0;
}

// Returns whether a function implementation in the given file should
// have it's Ptree walked. This will be true only if the given file is
// one of the files to be stored. 
bool FileFilter::should_visit_function_impl(AST::SourceFile* file)
{
  // First check if not linking or xreffing
  if (m->syntax_prefix.empty() || m->xref_prefix.empty())
    return false;

  return file->is_main();
}


// Returns true if links should be generated for the given sourcefile
bool FileFilter::should_link(AST::SourceFile* file)
{
  return !m->syntax_prefix.empty() && file->is_main();
}

// Returns true if xref info should be generated for the given sourcefile
bool FileFilter::should_xref(AST::SourceFile* file)
{
  return !m->xref_prefix.empty() && file->is_main();
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

// Strip base path from filename
std::string FileFilter::strip_base_path(const std::string& filename)
{
    if (m->base_path.size() == 0)
        return filename;
    size_t length = m->base_path.size();
    if (length > filename.size())
        return filename;
    if (strncmp(filename.data(), m->base_path.data(), length) == 0)
        return filename.substr(length);
    return filename;
}

// Return syntax filename
std::string FileFilter::get_syntax_filename(AST::SourceFile* file)
{
    return m->syntax_prefix + file->filename();
}

// Return xref filename
std::string FileFilter::get_xref_filename(AST::SourceFile* file)
{
    return m->xref_prefix + file->filename();
}

// Returns all sourcefiles
void FileFilter::get_all_sourcefiles(AST::SourceFile::vector& all)
{
    Private::file_map_t::iterator iter;
    for (iter = m->file_map.begin(); iter != m->file_map.end(); iter++)
        all.push_back(iter->second);
}

std::string FileFilter::get_main_file()
{
  return m->main_filename;
}
