// Synopsis C++ Parser: occ.cc source file
// Main entry point for the C++ parser module, and also debugging main
// function.

// $Id: occ.cc,v 1.88 2003/08/01 00:23:16 stefan Exp $
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

// $Log: occ.cc,v $
// Revision 1.88  2003/08/01 00:23:16  stefan
// accept '-Wp,-I,<filename>' and '-Wp,-D,<filename>'
//
// Revision 1.87  2003/03/21 21:31:23  stefan
// a fix to the fix...
//
// Revision 1.86  2003/03/14 17:09:50  stefan
// separate command and arguments in CC variable
//
// Revision 1.85  2003/01/27 06:53:37  chalky
// Added macro support for C++.
//
// Revision 1.84  2003/01/15 12:10:26  chalky
// Removed more global constructors
//
// Revision 1.83  2002/12/23 07:50:10  chalky
// Get rid of statically initialised objects due to non-deterministic
// initialisation order (particularly, the GC may not be available).
//
// Revision 1.82  2002/12/20 21:14:25  stefan
// adapt signal handler to new SourceFile code
//
// Revision 1.81  2002/12/12 17:25:34  chalky
// Implemented Include support for C++ parser. A few other minor fixes.
//
// Revision 1.80  2002/12/09 04:01:01  chalky
// Added multiple file support to parsers, changed AST datastructure to handle
// new information, added a demo to demo/C++. AST Declarations now have a
// reference to a SourceFile (which includes a filename) instead of a filename.
//
// Revision 1.79  2002/11/22 05:59:37  chalky
// Removed free() that shouldn't be there.
//
// Revision 1.78  2002/11/17 12:11:44  chalky
// Reformatted all files with astyle --style=ansi, renamed fakegc.hh
//
//

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include <occ/walker.h>
#include <occ/token.h>
#include <occ/buffer.h>
#include <occ/parse.h>
#include <occ/ptree-core.h>
#include <occ/ptree.h>
#include <occ/encoding.h>
#include <occ/mop.h>
#include <occ/metaclass.h>
#include <occ/env.h>
#include <occ/encoding.h>

// Stupid macro
#undef Scope

#include "synopsis.hh"
#include "swalker.hh"
#include "builder.hh"
#include "dumper.hh"
#include "link_map.hh"
#include "filter.hh"
#include "linkstore.hh"

// Define this to test refcounting
//#define SYN_TEST_REFCOUNT

// ucpp_main is the renamed main() func of ucpp, since it is included in this
// module
extern "C" int ucpp_main(int argc, char** argv);

/* The following aren't used anywhere. Though it has to be defined and initialized to some dummy default
 * values since it is required by the opencxx.a module, which I don't want to modify...
 */
bool showProgram, doCompile, makeExecutable, doPreprocess, doTranslate;
bool verboseMode, regularCpp, makeSharedLibrary, preprocessTwice;
char* sharedLibraryName;

/* These also are just dummies needed by the opencxx module */
void RunSoCompiler(const char *)
{}
void *LoadSoLib(char *)
{
    return 0;
}
void *LookupSymbol(void *, char *)
{
    return 0;
}

/* This implements the static var and method from fakegc.h */
cleanup* FakeGC::head = NULL;
void FakeGC::delete_all()
{
    cleanup* node = FakeGC::head;
    cleanup* next;
    size_t count = 0;
    while (node)
    {
        next = node->cleanup_next;
        delete node;
        node = next;
        count++;
    }
    FakeGC::head = NULL;
    //std::cout << "FakeGC::delete_all(): deleted " << count << " objects." << std::endl;
}

bool verbose;

// If true then everything but what's in the main file will be stripped
bool syn_main_only;
bool syn_extract_tails, syn_use_gcc, syn_fake_std;
bool syn_multi_files;

// If set then this is stripped from the start of all filenames
const char* syn_basename = "";

// If set then this is the prefix for the filename to store links to
const char* syn_syntax_prefix = 0;

// If set then this is the prefix for the filename to store xref info to
const char* syn_xref_prefix = 0;

// If set then this is the filename to store links to
const char* syn_file_syntax = 0;

// If set then this is the filename to store xref to
const char* syn_file_xref = 0;

// This is the compiler to emulate
const char* syn_emulate_compiler = "c++";

// A list of extra filenames to store info for
std::vector<const char*>* syn_extra_filenames = NULL;

// A place to temporarily store Python's thread state
PyThreadState* pythread_save;


#ifdef DEBUG
// For use in gdb, since decl->Display() doesn't work too well..
void show(Ptree* p)
{
    p->Display();
}
#endif

namespace
{

//. Override unexpected() to print a message before we abort
void unexpected()
{
    std::cout << "Warning: Aborting due to unexpected exception." << std::endl;
    throw std::bad_exception();
}

void getopts(PyObject *args, std::vector<const char *> &cppflags, 
        std::vector<const char *> &occflags, PyObject* config, PyObject* extra_files)
{
    // Initialise defaults
    showProgram = doCompile = verboseMode = makeExecutable = false;
    doTranslate = regularCpp = makeSharedLibrary = preprocessTwice = false;
    doPreprocess = true;
    sharedLibraryName = 0;
    verbose = false;
    syn_main_only = false;
    syn_extract_tails = false;
    syn_use_gcc = false;
    syn_fake_std = false;
    syn_multi_files = false;
    Class::do_init_static();
    Metaclass::do_init_static();
    Environment::do_init_static();
    Encoding::do_init_static();

#define IsType(obj, type) (Py##type##_Check(obj))

#define OPT_FLAG(syn_flag, config_name) \
if ((value = PyObject_GetAttrString(config, config_name)) != 0)\
    syn_flag = PyObject_IsTrue(value);\
Py_XDECREF(value);

#define OPT_STRING(syn_name, config_name) \
if ((value = PyObject_GetAttrString(config, config_name)) != 0)\
{ if (!IsType(value, String)) throw "Error: " config_name " must be a string.";\
  syn_name = PyString_AsString(value);\
}\
Py_XDECREF(value);

    // Check config object first
    if (config)
    {
        PyObject* value;
        OPT_FLAG(verbose, "verbose");
        OPT_FLAG(syn_main_only, "main_file");
        // Grab the include paths
        if ((value = PyObject_GetAttrString(config, "include_path")) != 0)
        {
            if (!IsType(value, List))
            {
                std::cerr << "Error: include_path must be a list of strings." << std::endl;
                exit(1);
            }
            // Loop through the include paths
            for (int i=0, end=PyList_Size(value); i < end; i++)
            {
                PyObject* item = PyList_GetItem(value, i);
                if (!item || !IsType(item, String))
                {
                    std::cerr << "Error: include_path must be a list of strings." << std::endl;
                    exit(1);
                }
                // mem leak.. how to fix?
                char* buf = new char[PyString_Size(item)+3];
                strcpy(buf, "-I");
                strcat(buf, PyString_AsString(item));
                cppflags.push_back(buf);
            } // for
        }
        Py_XDECREF(value);
        // Grab the list of defines
        if ((value = PyObject_GetAttrString(config, "defines")) != 0)
        {
            if (!IsType(value, List))
            {
                std::cerr << "Error: defines must be a list of strings." << std::endl;
                exit(1);
            }
            // Loop through the include paths
            for (int i=0, end=PyList_Size(value); i < end; i++)
            {
                PyObject* item = PyList_GetItem(value, i);
                if (!item || !IsType(item, String))
                {
                    std::cerr << "Error: defines must be a list of strings." << std::endl;
                    exit(1);
                }
                // mem leak.. how to fix?
                char* buf = new char[PyString_Size(item)+3];
                strcpy(buf, "-D");
                strcat(buf, PyString_AsString(item));
                cppflags.push_back(buf);
            } // for
        }
        Py_XDECREF(value);

        // Basename is the prefix to strip from filenames
        OPT_STRING(syn_basename, "basename");
        // Extract tails tells the parser to find tail comments
        OPT_FLAG(syn_extract_tails, "extract_tails");
        // 'storage' defines the filename to write syntax hilite info to (OBSOLETE)
        OPT_STRING(syn_file_syntax, "storage");
        // 'syntax_prefix' defines the prefix for the filename to write syntax hilite info to
        OPT_STRING(syn_syntax_prefix, "syntax_prefix");
        // 'syntax_file' defines the filename to write syntax hilite info to
        OPT_STRING(syn_file_syntax, "syntax_file");
        // 'xref_prefix' defines the prefix for the xrefname to write syntax hilite info to
        OPT_STRING(syn_xref_prefix, "xref_prefix");
        // 'xref_file' defines the filename to write syntax hilite info to
        OPT_STRING(syn_file_xref, "xref_file");
        // 'preprocessor' defines whether to use gcc or not
        char* temp_string = NULL;
        OPT_STRING(temp_string, "preprocessor");
        if (temp_string)
        {
            syn_use_gcc = !strcmp("gcc", temp_string);
        }
        // 'emulate_compiler' specifies the compiler to emulate in terms of
        // include paths and macros
        OPT_STRING(syn_emulate_compiler, "emulate_compiler");
        OPT_FLAG(syn_fake_std, "fake_std");
        // If multiple_files is set then the parser handles multiple files
        // included from the main one at the same time (they get into the AST,
        // plus they get their own xref and links files).
        OPT_FLAG(syn_multi_files, "multiple_files");
    } // if config
#undef OPT_STRING
#undef OPT_FLAG
#undef IsType

    // Now check command line args
    size_t argsize = PyList_Size(args);
    for (size_t i = 0; i != argsize; ++i)
    {
        const char *argument = PyString_AsString(PyList_GetItem(args, i));
        if (strncmp(argument, "-I", 2) == 0)
	{
	  cppflags.push_back(argument);
	  if (strlen(argument) == 2)
	    cppflags.push_back(PyString_AsString(PyList_GetItem(args, ++i)));
	}
        else if (strncmp(argument, "-D", 2) == 0)
	{
	  cppflags.push_back(argument);
	  if (strlen(argument) == 2)
	    cppflags.push_back(PyString_AsString(PyList_GetItem(args, ++i)));
	}
        else if (strcmp(argument, "-v") == 0)
            verbose = true;
        else if (strcmp(argument, "-m") == 0)
            syn_main_only = true;
        else if (strcmp(argument, "-b") == 0)
            syn_basename = PyString_AsString(PyList_GetItem(args, ++i));
        else if (strcmp(argument, "-t") == 0)
            syn_extract_tails = true;
        else if (strcmp(argument, "-s") == 0)
            syn_file_syntax = PyString_AsString(PyList_GetItem(args, ++i));
        else if (strcmp(argument, "-x") == 0)
            syn_file_xref = PyString_AsString(PyList_GetItem(args, ++i));
        else if (strcmp(argument, "-g") == 0)
            syn_use_gcc = true;
        else if (strcmp(argument, "-f") == 0)
            syn_fake_std = true;
    }

    // If multi_files is set, we check the extra_files argument to see if it
    // has a list of filenames like it should do
    if (extra_files && PyList_Check(extra_files))
    {
        size_t extra_size = PyList_Size(extra_files);
        if (extra_size > 0)
        {
            PyObject* item;
            const char* string;
            syn_extra_filenames = new std::vector<const char*>;
            for (size_t i = 0; i < extra_size; i++)
            {
                item = PyList_GetItem(extra_files, i);
                string = PyString_AsString(item);
                syn_extra_filenames->push_back(string);
            }
        }
    }
}

//. Emulates the compiler in 'syn_emulate_compiler' by calling the Python
//. module Synopsis.Parser.C++.emul to get a list of include paths and macros to
//. add to the args vector.
void emulate_compiler(std::vector<const char*>& args)
{
    PyObject* emul_module = PyImport_ImportModule("Synopsis.Parser.C++.emul");
    if (!emul_module)
        return;
    PyObject* info = PyObject_CallMethod(emul_module, "get_compiler_info", "s", syn_emulate_compiler);
    if (!info)
    {
        PyErr_Print();
        return;
    }
    PyObject* paths = PyObject_GetAttrString(info, "include_paths");
    if (paths)
    {
        // Add each path..
        int num_paths = PyList_Size(paths);
        for (int i = 0; i < num_paths; i++)
        {
            PyObject* path = PyList_GetItem(paths, i);
            if (!path)
            {
                PyErr_Print();
                continue;
            }
            char* path_str = PyString_AsString(path);
            if (path_str)
            {
                // Add this path
                args.push_back("-I");
                args.push_back(path_str);
            }
        }
        Py_DECREF(paths);
    }
    PyObject* macros = PyObject_GetAttrString(info, "macros");
    if (macros)
    {
        // Add each macro..
        int num_macros = PyList_Size(macros);
        for (int i = 0; i < num_macros; i++)
        {
            PyObject* macro_tuple = PyList_GetItem(macros, i);
            if (!macro_tuple)
            {
                PyErr_Print();
                continue;
            }
            PyObject* macro_name = PyTuple_GetItem(macro_tuple, 0);
            if (!macro_name)
            {
                PyErr_Print();
                continue;
            }
            PyObject* macro_value = PyTuple_GetItem(macro_tuple, 1);
            if (!macro_value)
            {
                PyErr_Print();
                continue;
            }
            if (macro_value == Py_None)
            { /* TODO: Do remove macro */
            }
            else
            {
                // Add the argument in the form -DNAME=VALUE to the list of arguments
                char* def = (char*)malloc(4 + PyString_Size(macro_name) + PyString_Size(macro_value));
                strcpy(def, "-D");
                strcat(def, PyString_AsString(macro_name));
                strcat(def, "=");
                strcat(def, PyString_AsString(macro_value));
                args.push_back(def);
                // TODO: Figure out where to free this string
            }
        }
        Py_DECREF(macros);
    }
    Py_DECREF(info);
    Py_DECREF(emul_module);
}

char *RunPreprocessor(const char *file, const std::vector<const char *> &flags)
{
    static char dest[1024];
    strcpy(dest, "/tmp/synopsis-XXXXXX");
    int temp_fd = mkstemp(dest);
    if (temp_fd == -1)
    {
        perror("RunPreprocessor");
        exit(1);
    }
    // Not interested in the open file, just the unique filename
    close(temp_fd);

    if (syn_use_gcc)
    {
       // Release Python's global interpreter lock
       pythread_save = PyEval_SaveThread();
       
       switch(fork())
       {
       case 0:
       {
          std::vector<const char *> args;
          char *cc = getenv("CC");
          if (cc)
          {
             // separate command and arguments
             do
             {
                args.push_back(cc);
                cc = strchr(cc, ' ');                  // find next whitespace...
                while (cc && *cc == ' ') *cc++ = '\0'; // ...and skip to next non-ws
             }
             while (cc && *cc != '\0');
          }
          else
          {
             args.push_back("cpp");
          }
          args.insert(args.end(), flags.begin(), flags.end());
          args.push_back("-C"); // keep comments
          args.push_back("-E"); // stop after preprocessing
          args.push_back("-o"); // output to...
          args.push_back(dest);
          args.push_back("-x"); // language c++
          args.push_back("c++");
          args.push_back(file);
          if (verbose)
          {
             std::cout << "calling external preprocessor\n" << args[0];
             for (std::vector<const char *>::iterator i = args.begin(); i != args.end(); ++i)
                std::cout << ' ' << *i;
             std::cout << std::endl;
          }
          args.push_back(0);
          execvp(args[0], (char **)&*args.begin());
          perror("cannot invoke compiler");
          exit(-1);
          break;
       }
       case -1:
          perror("RunPreprocessor");
          exit(-1);
          break;
       default:
       {
          int status;
          wait(&status);
          if (status != 0)
          {
             if (WIFEXITED(status))
                std::cout << "exited with status " << WEXITSTATUS(status) << std::endl;
             else if (WIFSIGNALED(status))
                std::cout << "stopped with status " << WTERMSIG(status) << std::endl;
             exit(1);
          }
       }
       } // switch
    }
    else
    { // else use ucpp
        // Create argv vector
        std::vector<const char *> args = flags;
        char *cc = getenv("CC");
        args.insert(args.begin(), cc ? cc : "ucpp");
        args.push_back("-C"); // keep comments
        args.push_back("-lg"); // gcc-like line numbers
        emulate_compiler(args);
        args.push_back("-o"); // output to...
        args.push_back(dest);
        args.push_back(file);
        if (verbose)
        {
            std::cout << "calling ucpp\n";
            for (std::vector<const char *>::iterator i = args.begin(); i != args.end(); ++i)
                std::cout << ' ' << *i;
            std::cout << std::endl;
        }

        // Release Python's global interpreter lock
        pythread_save = PyEval_SaveThread();

        // Call ucpp
        int status = ucpp_main(args.size(), (char **)&*args.begin());
        if (status != 0)
            std::cerr << "ucpp returned error flag. ignoring error." << std::endl;
    }
    return dest;
}

void sighandler(int signo)
{
    std::string signame;
    switch (signo)
    {
    case SIGABRT:
        signame = "Abort";
        break;
    case SIGBUS:
        signame = "Bus error";
        break;
    case SIGSEGV:
        signame = "Segmentation Violation";
        break;
    default:
        signame = "unknown";
        break;
    };
    SWalker *instance = SWalker::instance();
    std::cerr << signame << " caught while processing " << instance->current_file()->filename()
              << " at line " << instance->current_lineno()
              << std::endl;
    exit(-1);
}

void RunOpencxx(const char *src, const char *file, const std::vector<const char *> &args, PyObject *ast, PyObject *types, PyObject *declarations, PyObject* files)
{
    Trace trace("RunOpencxx");
    std::set_unexpected(unexpected);
    struct sigaction olda;
    struct sigaction newa;
    newa.sa_handler = &sighandler;
    sigaction(SIGSEGV, &newa, &olda);
    sigaction(SIGBUS, &newa, &olda);
    sigaction(SIGABRT, &newa, &olda);

    std::ifstream ifs(file);
    if(!ifs)
    {
        perror(file);
        exit(1);
    }
    ProgramFile prog(ifs);
    Lex lex(&prog);
    Parser parse(&lex);
#if 0
    // Make sure basename ends in a '/'
    std::string basename = syn_basename;
    if (basename.size() > 0 && basename[basename.size()-1] != '/')
        basename.append("/");
    // Calculate source filename
    std::string source(src);
    if (source.substr(0, basename.size()) == basename)
        source.erase(0, basename.size());
#endif

    FileFilter* filter = FileFilter::instance();

    AST::SourceFile* sourcefile = filter->get_sourcefile(src);

    Builder builder(sourcefile);
    if (syn_macro_defines)
	builder.add_macros(*syn_macro_defines);
    SWalker swalker(filter, &parse, &builder, &prog);
    swalker.set_extract_tails(syn_extract_tails);
    Ptree *def;
    if (syn_fake_std)
    {
        builder.set_file(sourcefile);
        // Fake a using from "std" to global
        builder.start_namespace("std", NamespaceNamed);
        builder.add_using_namespace(builder.global()->declared());
        builder.end_namespace();
    }
#ifdef DEBUG
    swalker.set_extract_tails(syn_extract_tails);
    while(parse.rProgram(def))
        swalker.Translate(def);

    // Grab interpreter lock again so we can call python
    PyEval_RestoreThread(pythread_save);
#ifdef SYN_TEST_REFCOUNT
    // Test Synopsis
    Synopsis synopsis(src, declarations, types);
    synopsis.translate(builder.scope(), ast);
    synopsis.set_builtin_decls(builder.builtin_decls());
#else
    // Test Dumper
    Dumper dumper;
    if (syn_main_only)
        dumper.onlyShow(src);
    dumper.visit_scope(builder.scope());
#endif
#else

#if 0
    std::ofstream* of_syntax = 0;
    std::ofstream* of_xref = 0;
    char syn_buffer[1024];
    size_t baselen = basename.size();
    if (syn_file_syntax)
        of_syntax = new std::ofstream(syn_file_syntax);
    else if (syn_syntax_prefix)
    {
        strcpy(syn_buffer, syn_syntax_prefix);
        if (!strncmp(basename.c_str(), src, baselen))
            strcat(syn_buffer, src + baselen);
        else
            strcat(syn_buffer, src);
        makedirs(syn_buffer);
        of_syntax = new std::ofstream(syn_buffer);
    }
    if (syn_file_xref)
        of_xref = new std::ofstream(syn_file_xref);
    else if (syn_xref_prefix)
    {
        strcpy(syn_buffer, syn_xref_prefix);
        if (!strncmp(basename.c_str(), src, baselen))
            strcat(syn_buffer, src + baselen);
        else
            strcat(syn_buffer, src);
        makedirs(syn_buffer);
        of_xref = new std::ofstream(syn_buffer);
    }
    if (of_syntax || of_xref)
        swalker.set_store_links(true, of_syntax, of_xref);
#endif
    if (filter->should_link(sourcefile) || filter->should_xref(sourcefile))
        swalker.set_store_links(new LinkStore(filter, &swalker));
    try
    {
        while(parse.rProgram(def))
            swalker.Translate(def);
    }
    catch (...)
    {
        std::cerr << "Warning: an uncaught exception occurred when translating the parse tree" << std::endl;
    }
    // Grab interpreter lock again so we can call python
    PyEval_RestoreThread(pythread_save);

    // Setup synopsis c++ to py convertor
    Synopsis synopsis(filter, declarations, types);
    synopsis.set_builtin_decls(builder.builtin_decls());
    // Convert!
    synopsis.translate(builder.scope(), ast);
#endif

    if(parse.NumOfErrors() != 0)
    {
        std::cerr << "Ignoring errors while parsing file: " << file << std::endl;
    }

    if (files)
    {
        //PyObject_CallMethod(filenames, "append", "s", source.c_str());
    }
    ifs.close();
    sigaction(SIGABRT, &olda, 0);
    sigaction(SIGBUS, &olda, 0);
    sigaction(SIGSEGV, &olda, 0);
}

void do_parse(const char *src, 
	const std::vector<const char *>& cppargs, 
	const std::vector<const char *>& occargs, 
	PyObject *ast, PyObject *types, PyObject *declarations, PyObject* files)
{
    // Setup the filter
    FileFilter filter;
    filter.set_only_main(syn_main_only);
    filter.set_main_filename(src);
    filter.set_basename(syn_basename);
    if (syn_extra_filenames) filter.add_extra_filenames(*syn_extra_filenames);
    if (syn_file_syntax) filter.set_syntax_filename(syn_file_syntax);
    if (syn_file_xref) filter.set_xref_filename(syn_file_xref);
    if (syn_syntax_prefix) filter.set_syntax_prefix(syn_syntax_prefix);
    if (syn_xref_prefix) filter.set_xref_prefix(syn_xref_prefix);

    // Run the preprocessor
    char *cppfile = RunPreprocessor(src, cppargs);

    // Run OCC to generate the AST
    RunOpencxx(src, cppfile, occargs, ast, types, declarations, files);
    unlink(cppfile);
}

PyObject *occParse(PyObject *self, PyObject *args)
{
    Trace trace("occParse");
#if 0

    Ptree::show_encoded = true;
#endif

    char *src;
    PyObject *extra_files, *parserargs, *types, *declarations, *config, *ast;
    if (!PyArg_ParseTuple(args, "sOO!O", &src, &extra_files, &PyList_Type, &parserargs, &config))
        return 0;
    std::vector<const char *> cppargs;
    std::vector<const char *> occargs;
    getopts(parserargs, cppargs, occargs, config, extra_files);
    if (!src || *src == '\0')
    {
        std::cerr << "No source file" << std::endl;
        exit(-1);
    }
    // Make AST object
#define assertObject(pyo) if (!pyo) PyErr_Print(); assert(pyo)
    PyObject* ast_module = PyImport_ImportModule("Synopsis.Core.AST");
    assertObject(ast_module);
    ast = PyObject_CallMethod(ast_module, "AST", "");
    assertObject(ast);
    PyObject* files = PyObject_CallMethod(ast, "files", "");
    assertObject(files);
    declarations = PyObject_CallMethod(ast, "declarations", "");
    assertObject(declarations);
    types = PyObject_CallMethod(ast, "types", "");
    assertObject(types);
#undef assertObject

    do_parse(src, cppargs, occargs, ast, types, declarations, files);

    if (syn_extra_filenames)
    {
        delete syn_extra_filenames;
        syn_extra_filenames = 0;
    }
    Py_DECREF(ast_module);
    Py_DECREF(declarations);
    Py_DECREF(files);
    Py_DECREF(types);

#ifndef DONT_GC
    // Try to cleanup GC if being used
    //std::cout << "GC: Running Garbage Collection..." << std::endl;
    //size_t size = GC_get_heap_size();
    GC_gcollect();
    //size_t size2 = GC_get_heap_size();
    //std::cout << "GC: Heap went from " << size << " to " << size2 << std::endl;
#endif

#ifdef SYN_TEST_REFCOUNT
    // Now, there should *fingers crossed* be no python objects. Check..
    {
        PyGC_Head* node = _PyGC_generation0.gc.gc_next;
        size_t count = 0;
        while (node != &_PyGC_generation0)
        {
            //PyObject* obj = (PyObject*)(node + 1);
            //PyObject* str = PyObject_Repr(obj);
            //std::cout << obj->ob_refcnt << " " << PyString_AsString(str) << "\n";
            //Py_DECREF(str);
            node = node->gc.gc_next;
            count++;
        }
        //std::cout << "Collection list contains " << count << " objects." << std::endl;
    }
#endif

    // Delete all the AST:: and Types:: objects we created
    FakeGC::delete_all();

    // Clear the link map
    LinkMap::instance()->clear();

    return ast;
}

PyObject *occUsage(PyObject *self, PyObject *)
{
    Trace trace("occParse");
    std::cout
    << "  -I<path>                             Specify include path to be used by the preprocessor\n"
    << "  -D<macro>                            Specify macro to be used by the preprocessor\n"
    << "  -m                                   Unly keep declarations from the main file\n"
    << "  -b basepath                          Strip basepath from start of filenames" << std::endl;
    Py_INCREF(Py_None);
    return Py_None;
}

PyMethodDef occ_methods[] =
    {
        {(char*)"parse",            occParse,               METH_VARARGS},
        {(char*)"usage",            occUsage,               METH_VARARGS},
        {0, 0}
    };
};

extern "C" void initocc()
{
    PyObject* m = Py_InitModule((char*)"occ", occ_methods);
    PyObject_SetAttrString(m, (char*)"version", PyString_FromString("0.1"));
}

#ifdef DEBUG

int main(int argc, char **argv)
{
    char *src = 0;
    std::vector<const char *> cppargs;
    std::vector<const char *> occargs;
    //   getopts(argc, argv, cppargs, occargs);
    Py_Initialize();
    int i, py_i;
    bool all_args = false;
    for (i = 1, py_i = 0; i != argc; ++i)
        if (!all_args && argv[i][0] != '-')
            src = argv[i];
        else if (!strcmp(argv[i], "--"))
            all_args = true;
        else
            ++py_i;
    PyObject *pylist = PyList_New(py_i);
    all_args = false;
    for (i = 1, py_i = 0; i != argc; ++i)
        if (!all_args && argv[i][0] != '-')
            continue;
        else if (!strcmp(argv[i], "--"))
            all_args = true;
        else
            PyList_SetItem(pylist, py_i++, PyString_FromString(argv[i]));
    getopts(pylist, cppargs, occargs, NULL, NULL);
    if (!src || *src == '\0')
    {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        exit(-1);
    }
    PyObject* ast_module = PyImport_ImportModule("Synopsis.Core.AST");
    PyObject* ast = PyObject_CallMethod(ast_module, "AST", 0);
    PyObject* type = PyImport_ImportModule("Synopsis.Core.Type");
    PyObject* types = PyObject_CallMethod(type, "Dictionary", 0);
    PyObject* decls = PyList_New(0);
    
    do_parse(src, cppargs, occargs, ast, types, decls, NULL);

#ifdef SYN_TEST_REFCOUNT

    Py_DECREF(pylist);
    Py_DECREF(type);
    Py_DECREF(types);
    Py_DECREF(decls);
    Py_DECREF(ast);
    Py_DECREF(ast_module);
    
    // Now, there should *fingers crossed* be no python objects. Check..
    if (0)
    {
        PyGC_Head* node = _PyGC_generation0.gc.gc_next;
        size_t count = 0;
        while (node != &_PyGC_generation0)
        {
            PyObject* obj = (PyObject*)(node + 1);
            PyObject* str = PyObject_Repr(obj);
            //std::cout << obj->ob_refcnt << " " << PyString_AsString(str) << "\n";
            Py_DECREF(str);
            node = node->gc.gc_next;
            count++;
        }
        //std::cout << "Collection list contains " << count << " objects." << std::endl;
    }
#endif
#ifndef DONT_GC
    // Try to cleanup GC if being used
    //size_t size = GC_get_heap_size();
    GC_gcollect();
    //size_t size2 = GC_get_heap_size();
    //std::cout << "Collection: Heap went from " << size << " to " << size2 << std::endl;
#endif

    Py_Finalize();

    FakeGC::delete_all();
}

#endif
// vim: set ts=8 sts=4 sw=4 et:
