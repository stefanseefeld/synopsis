//
// Copyright (C) 2002 Stephen Davies
// Copyright (C) 2002 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef filter_hh_
#define filter_hh_

#include <Python.h>
#include <string>
#include <vector>
#include "ASG.hh"

class FileFilter
{
public:
    //. Constructor
    FileFilter(PyObject *, const std::string &, const std::string &, bool);

    //. Destructor
    ~FileFilter();

    std::string get_main_file();

    //. Sets the prefix for syntax output filenames.
    //. The syntax filename will be the source filename prepended with the
    //. prefix, so you probably want the prefix to be a directory.
    void set_sxr_prefix(const char* filename);

    //. Returns the ASG::SourceFile for the given filename. If length is
    //. given, then the filename is assumed to be that length. This is useful
    //. since OCC returns filenames as a reference to the #line directive in
    //. the preprocessor output and is not null-terminated.
    ASG::SourceFile* get_sourcefile(const char* filename, size_t length = 0);

    //. Returns whether a function implementation in the given file should
    //. have it's Ptree walked. This will be true only if the given file is
    //. one of the files to be stored. 
    bool should_visit_function_impl(ASG::SourceFile* file);

    //. Returns true if links should be generated for the given sourcefile
    bool should_xref(ASG::SourceFile* file);

    //. Returns true if the given declaration should be stored in the final
    //. ASG. Note that a Declaration is taken instead of a SourceFile because
    //. Namespaces may be declared (and contain declarations) in multiple
    //. files, and Classes may have nested classes defined in other files.
    bool should_store(ASG::Declaration* decl);

    //. Strips a filename of the basename if present
    std::string strip_base_path(const std::string& filename);

    //. Returns the filename to use for storing sxr info
    std::string get_sxr_filename(ASG::SourceFile* file);

    //. Returns a list of all the sourcefiles
    void get_all_sourcefiles(ASG::SourceFile::vector&);

    //. Returns a pointer to the Filter instance. Note that Filter is *not* a
    //. regular singleton: instance() will return 0 if the Filter doesn't
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
