// $Id: occ.cc,v 1.95 2003/12/17 15:07:24 stefan Exp $
//
// Copyright (C) 2000 Stefan Seefeld
// Copyright (C) 2000 Stephen Davies
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>

#if !defined(__WIN32__)
#  include <unistd.h>
#  include <signal.h>
#endif

// #include <occ/walker.h>
// #include <occ/token.h>
// #include <occ/buffer.h>
#include <occ/parse.h>
// #include <occ/ptree-core.h>
// #include <occ/ptree.h>
// #include <occ/mop.h>
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

extern const char *
RunPreprocessor(const char *preprocessor,
                const char *src, const std::vector<const char *> &flags,
                bool verbose);

/* The following aren't used anywhere. Though it has to be defined and initialized to some dummy default
 * values since it is required by the opencxx.a module, which I don't want to modify...
 */
bool showProgram = false;
bool doCompile = false;
bool makeExecutable = false;
bool doPreprocess = true;
bool doTranslate = false;
bool verboseMode = false;
bool regularCpp = false;
bool makeSharedLibrary = false;
bool preprocessTwice = false;
char* sharedLibraryName = 0;

/* These also are just dummies needed by the opencxx module */
void RunSoCompiler(const char *) {}
void *LoadSoLib(char *) { return 0;}
void *LookupSymbol(void *, char *) { return 0;}
/* This implements the static var and method from fakegc.h */
cleanup* FakeGC::head = 0;
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
  FakeGC::head = 0;
  //std::cout << "FakeGC::delete_all(): deleted " << count << " objects." << std::endl;
}

bool verbose;

// If true then everything but what's in the main file will be stripped
bool syn_main_only;
bool syn_fake_std;
bool syn_multi_files;

// If set then this is stripped from the start of all filenames
const char* syn_base_path = "";

// If set then this is the prefix for the filename to store links to
const char* syn_syntax_prefix = 0;

// If set then this is the prefix for the filename to store xref info to
const char* syn_xref_prefix = 0;

// A list of extra filenames to store info for
std::vector<const char*> syn_extra_filenames;

// A place to temporarily store Python's thread state
PyThreadState* pythread_save;


#ifdef DEBUG
// For use in gdb, since decl->Display() doesn't work too well..
void show(Ptree* p) { p->Display();}
#endif

namespace
{

//. Override unexpected() to print a message before we abort
void unexpected()
{
  std::cout << "Warning: Aborting due to unexpected exception." << std::endl;
  throw std::bad_exception();
}

//. Emulates the given compiler by calling the Python
//. module Synopsis.Parser.C++.emul to get a list of include paths and macros to
//. add to the args vector.
bool emulate_compiler(const char *compiler, std::vector<const char*>& args)
{
  PyObject* emul_module = PyImport_ImportModule("Synopsis.Parsers.Cxx.emul");
  if (!emul_module)
  {
     return false;
  }
  PyObject* info = PyObject_CallMethod(emul_module, "get_compiler_info", "s", compiler);
  if (!info)
  {
    PyErr_Print();
    return false;
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
      if (macro_value == Py_None) {} /* TODO: Do remove macro */
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
  return true;
}

#if !defined(__WIN32__)

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

#endif

void RunOpencxx(const char *src, const char *file, PyObject *ast)
{
  Trace trace("RunOpencxx");
  std::set_unexpected(unexpected);

#if !defined(__WIN32__)
  struct sigaction olda;
  struct sigaction newa;
  newa.sa_handler = &sighandler;
  sigaction(SIGSEGV, &newa, &olda);
  sigaction(SIGBUS, &newa, &olda);
  sigaction(SIGABRT, &newa, &olda);
#endif  
  std::ifstream ifs(file);
  if(!ifs)
  {
    perror(file);
    exit(1);
  }
  ProgramFile prog(ifs);
  Lex lex(&prog);
  Parser parse(&lex);

  FileFilter* filter = FileFilter::instance();

  AST::SourceFile* sourcefile = filter->get_sourcefile(src);
  
  Builder builder(sourcefile);
  if (syn_macro_defines)
    builder.add_macros(*syn_macro_defines);
  SWalker swalker(filter, &parse, &builder, &prog);
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
  while(parse.rProgram(def)) swalker.Translate(def);

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
  if (syn_main_only) dumper.onlyShow(src);
  dumper.visit_scope(builder.scope());
#endif
#else

  if (filter->should_link(sourcefile) || filter->should_xref(sourcefile))
    swalker.set_store_links(new LinkStore(filter, &swalker));
  try
  {
    while(parse.rProgram(def)) swalker.Translate(def);
  }
  catch (...)
  {
    std::cerr << "Warning: an uncaught exception occurred when translating the parse tree" << std::endl;
  }
  // Grab interpreter lock again so we can call python
  PyEval_RestoreThread(pythread_save);

  // Setup synopsis c++ to py convertor
  Synopsis synopsis(filter, ast);//declarations, types);
  synopsis.set_builtin_decls(builder.builtin_decls());
  // Convert!
  synopsis.translate(builder.scope());

#endif

  if(parse.NumOfErrors() != 0)
  {
    std::cerr << "Ignoring errors while parsing file: " << file << std::endl;
  }

  ifs.close();
#if !defined(__WIN32__)
  sigaction(SIGABRT, &olda, 0);
  sigaction(SIGBUS, &olda, 0);
  sigaction(SIGSEGV, &olda, 0);
#endif
}

bool extract(PyObject *list, std::vector<const char *> &out)
{
  size_t argsize = PyList_Size(list);
  for (size_t i = 0; i != argsize; ++i)
  {
    const char *value = PyString_AsString(PyList_GetItem(list, i));
    if (!value) return false;
    out.push_back(value);
  }
  return true;
}

PyObject *occParse(PyObject *self, PyObject *args)
{
  Trace trace("occParse");

  Class::do_init_static();
  Metaclass::do_init_static();
  Environment::do_init_static();
  Encoding::do_init_static();

  PyObject *ast;
  char *src;
  PyObject *py_extra_files;
  std::vector<const char *> extra_files;
  const char *syn_emulate_compiler = "c++";
  int verbose, main_file_only;
  char *preprocessor;
  PyObject *py_cppflags;
  std::vector<const char *> cppflags;
  if (!PyArg_ParseTuple(args, "OsO!iizzO!zzz",
                        &ast,
                        &src,
                        &PyList_Type, &py_extra_files,
                        &verbose,
                        &main_file_only,
                        &syn_base_path,
                        &preprocessor,
                        &PyList_Type, &py_cppflags,
                        &syn_syntax_prefix,
                        &syn_xref_prefix,
                        &syn_emulate_compiler)
      || !extract(py_extra_files, extra_files)
      || !(preprocessor || emulate_compiler(syn_emulate_compiler, cppflags))
      || !extract(py_cppflags, cppflags))
    return 0;

  Py_INCREF(ast);

  if (verbose) ::verbose = true;
  if (main_file_only) syn_main_only = true;

  for (std::vector<const char *>::iterator i = extra_files.begin();
       i != extra_files.end();
       ++i)
    syn_extra_filenames.push_back(*i);

  if (!src || *src == '\0')
  {
    PyErr_SetString(PyExc_RuntimeError, "no input file");
    return 0;
  }

  // Setup the filter
  FileFilter filter;
  filter.set_only_main(syn_main_only);
  filter.set_main_filename(src);
  filter.set_basename(syn_base_path);
  filter.add_extra_filenames(syn_extra_filenames);
  if (syn_syntax_prefix) filter.set_syntax_prefix(syn_syntax_prefix);
  if (syn_xref_prefix) filter.set_xref_prefix(syn_xref_prefix);

  // Run the preprocessor

  // Release Python's global interpreter lock
  pythread_save = PyEval_SaveThread();
  const char *cppfile = RunPreprocessor(preprocessor, src, cppflags, verbose);

  // Run OCC to generate the AST
  RunOpencxx(src, cppfile, ast);
  unlink(cppfile);

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

PyMethodDef occ_methods[] = {{(char*)"parse", occParse, METH_VARARGS},
                             {0, 0}};
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
  if (!all_args && argv[i][0] != '-') src = argv[i];
  else if (!strcmp(argv[i], "--")) all_args = true;
  else ++py_i;
  PyObject *pylist = PyList_New(py_i);
  all_args = false;
  for (i = 1, py_i = 0; i != argc; ++i)
    if (!all_args && argv[i][0] != '-') continue;
    else if (!strcmp(argv[i], "--")) all_args = true;
    else
      PyList_SetItem(pylist, py_i++, PyString_FromString(argv[i]));

  Class::do_init_static();
  Metaclass::do_init_static();
  Environment::do_init_static();
  Encoding::do_init_static();

  getopts(pylist, cppargs, occargs, NULL, NULL);
  if (!src || *src == '\0')
  {
    std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
    exit(-1);
  }
  PyObject* ast_module = PyImport_ImportModule("Synopsis.AST");
  PyObject* ast = PyObject_CallMethod(ast_module, "AST", 0);
  PyObject* type = PyImport_ImportModule("Synopsis.Type");
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
