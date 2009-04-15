//
// Copyright (C) 2002 Stephen Davies
// Copyright (C) 2002 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/Python/Module.hh>
#include <Synopsis/Trace.hh>
#include <Support/Path.hh>
#include "Filter.hh"
#include <map>
#include <iostream>
#include <stdexcept>

using namespace Synopsis;

struct FileFilter::Private
{

    //. used during SourceFile imports
    PyObject *asg;

    //. Whether only main declarations should be stored
    bool only_main;

    //. The main filename
    std::string main_filename;

    //. The basename
    std::string base_path;

    //. A vector of strings
    typedef std::vector<std::string> string_vector;

    //. The sxr filename prefix
    std::string sxr_prefix;

    //. Type of the map from filename to SourceFile
    typedef std::map<std::string, ASG::SourceFile*> file_map_t;

    //. A map from filename to SourceFile
    file_map_t file_map;
};

namespace
{
    //. The FileFilter instance
    FileFilter* filter_instance = 0;

//. restore the relevant parts from the python asg
//. The returned SourceFile object is a slice of the original python object
//. and should be merged back at the end of the parsing such that the other
//. parts not mirrored here are preserved.
ASG::SourceFile *import_source_file(PyObject *ir,
				    const std::string &name,
				    const std::string &abs_name,
				    bool primary)
{
  ASG::SourceFile *sourcefile = new ASG::SourceFile(name, abs_name, primary);

  PyObject *files = PyObject_GetAttrString(ir, "files");
  assert(files);
  PyObject *py_source_file = PyDict_GetItemString(files, const_cast<char *>(name.c_str()));
  Py_DECREF(files);
  if (!py_source_file) return sourcefile; // the given file wasn't preprocessed into the ASG

  PyObject *macro_calls = PyObject_GetAttrString(py_source_file, "macro_calls");
  long size = PyObject_Length(macro_calls);
  for (long i = 0; i != size; ++i)
  {
    PyObject *call = PyList_GetItem(macro_calls, i);
    PyObject *call_name = PyObject_GetAttrString(call, "name");
    PyObject *start = PyObject_GetAttrString(call, "start");
    PyObject *end = PyObject_GetAttrString(call, "end");
    PyObject *expanded_start = PyObject_GetAttrString(call, "expanded_start");
    PyObject *expanded_end = PyObject_GetAttrString(call, "expanded_end");
    char const *macro_name = PyString_AsString(call_name);
    long start_line = PyInt_AsLong(PyTuple_GetItem(start, 0));
    long start_col = PyInt_AsLong(PyTuple_GetItem(start, 1));
    long end_col = PyInt_AsLong(PyTuple_GetItem(end, 1));
    long e_start_line = PyInt_AsLong(PyTuple_GetItem(expanded_start, 0));
    long e_start_col = PyInt_AsLong(PyTuple_GetItem(expanded_start, 1));
    long e_end_line = PyInt_AsLong(PyTuple_GetItem(expanded_end, 0));
    long e_end_col = PyInt_AsLong(PyTuple_GetItem(expanded_end, 1));
    if (e_start_line == e_end_line)
      sourcefile->add_macro_call(macro_name, start_line, start_col,
                                 e_start_line, e_start_col, e_end_line, e_end_col, e_end_col - end_col, false);
    else
    {
      // first line
      sourcefile->add_macro_call(macro_name, start_line, start_col,
                                 e_start_line, e_start_col, -1, -1, 0, false);
      for (int line = e_start_line + 1; line != e_end_line; ++line)
        sourcefile->add_macro_call(macro_name, start_line, start_col,
                                   line, 0, -1, -1, 0, true);
      // last line
      sourcefile->add_macro_call(macro_name, start_line, start_col,
                                 e_end_line, 0, e_end_line, e_end_col, e_end_col - end_col, true);
    }
    Py_DECREF(expanded_end);
    Py_DECREF(expanded_start);
    Py_DECREF(end);
    Py_DECREF(call_name);
  }
  Py_DECREF(macro_calls);
  return sourcefile;
}
}

// Constructor
FileFilter::FileFilter(PyObject *asg,
		       const std::string &filename,
		       const std::string &base_path,
		       bool main)
{
    m = new Private;
    m->asg = asg;
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

// Sets the prefix for sxr output filenames.
void FileFilter::set_sxr_prefix(const char* filename)
{
    m->sxr_prefix = filename;
    if (m->sxr_prefix.size() > 0
        && m->sxr_prefix[m->sxr_prefix.size()-1] != '/')
       m->sxr_prefix.append("/");
}

// Returns the ASG::SourceFile for the given filename.
ASG::SourceFile* FileFilter::get_sourcefile(const char* filename_ptr, size_t length)
{
    std::string filename;
    if (length != 0)
        filename.assign(filename_ptr, length);
    else
        filename.assign(filename_ptr);
    
    Path path = Path(filename).abs();
    std::string abs_filename = path.str();
    path.strip(m->base_path);
    filename = path.str();
    // Look in map
    Private::file_map_t::iterator iter = m->file_map.find(abs_filename);
    if (iter != m->file_map.end())
        // Found
        return iter->second;
    
    // Not found, create a new SourceFile. Note filename in the object is
    // stripped of the basename
    ASG::SourceFile* file = import_source_file(m->asg,
					       filename,
					       abs_filename, 
					       is_main(abs_filename));

    // Add to the map
    m->file_map[abs_filename] = file;

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
bool FileFilter::should_visit_function_impl(ASG::SourceFile* file)
{
  // First check if not linking or xreffing
  if (m->sxr_prefix.empty())
    return false;

  return file->is_primary();
}


// Returns true if links should be generated for the given sourcefile
bool FileFilter::should_xref(ASG::SourceFile* file)
{
  return !m->sxr_prefix.empty() && file->is_primary();
}

// Returns true if the given declaration should be stored in the final
// ASG.
bool FileFilter::should_store(ASG::Declaration* decl)
{
    // Sanity check (this can happen)
    if (!decl)
        return false;
    // Check the decl itself first, although for namespaces the SourceFile
    // referenced by file() can be any of the files that opened it
    if (decl->file()->is_primary())
        return true;
    if (ASG::Scope* scope = dynamic_cast<ASG::Scope*>(decl))
    {
        // Check all members of the namespace or class
        ASG::Declaration::vector::iterator iter;
        ASG::Declaration::vector& declarations = scope->declarations();
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
std::string FileFilter::get_sxr_filename(ASG::SourceFile* file)
{
    return m->sxr_prefix + file->name() + ".sxr";
}

// Returns all sourcefiles
void FileFilter::get_all_sourcefiles(ASG::SourceFile::vector& all)
{
    Private::file_map_t::iterator iter;
    for (iter = m->file_map.begin(); iter != m->file_map.end(); iter++)
        all.push_back(iter->second);
}

std::string FileFilter::get_main_file()
{
  return m->main_filename;
}
