//
// Copyright (C) 2000 Stefan Seefeld
// Copyright (C) 2000 Stephen Davies
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "synopsis.hh"
#include "swalker.hh"
#include "builder.hh"
#include "dumper.hh"
#include "link_map.hh"
#include "filter.hh"
#include "linkstore.hh"

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

#include <occ/parse.h>
#include <occ/metaclass.h>
#include <occ/env.h>
#include <occ/encoding.h>

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

namespace
{

//. Override unexpected() to print a message before we abort
void unexpected()
{
  std::cout << "Warning: Aborting due to unexpected exception." << std::endl;
  throw std::bad_exception();
}

//. restore the relevant parts from the python ast
//. The returned SourceFile object is a slice of the original python object
//. and should be merged back at the end of the parsing such that the other
//. parts not mirrored here are preserved.
AST::SourceFile *restore_source_file(PyObject *ast, const std::string &src)
{
  LinkMap *map = LinkMap::instance();
  map->clear();

  FileFilter* filter = FileFilter::instance();
  AST::SourceFile *sourcefile = filter->get_sourcefile(src.c_str());

  PyObject *files = PyObject_CallMethod(ast, "files", "");
  assert(files);
  PyObject *source_file = PyDict_GetItemString(files, src.c_str());
  Py_DECREF(files);
  if (!source_file) return sourcefile; // the given file wasn't preprocessed into the AST

  PyObject *macro_calls = PyObject_CallMethod(source_file, "macro_calls", "");
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
	map->add(PyString_AsString(name), l,
		 PyInt_AsLong(start),
		 PyInt_AsLong(end),
		 PyInt_AsLong(diff));
      }
    }
    Py_DECREF(macro_calls);
  }
  return sourcefile;
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

void RunOpencxx(AST::SourceFile *sourcefile, const char *file, PyObject *ast)
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

  Builder builder(sourcefile);
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

  // Setup synopsis c++ to py convertor
  Synopsis synopsis(filter, ast);//declarations, types);
  synopsis.set_builtin_decls(builder.builtin_decls());
  // Convert!
  synopsis.translate(builder.scope());

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

PyObject *occ_parse(PyObject *self, PyObject *args)
{
  Trace trace("occ_parse");

  Class::do_init_static();
  Metaclass::do_init_static();
  Environment::do_init_static();
  Encoding::do_init_static();

  PyObject *ast;
  const char *src, *cppfile;
  PyObject *py_extra_files;
  std::vector<const char *> extra_files;
  int verbose, main_file_only;
  if (!PyArg_ParseTuple(args, "OssO!iizzz",
                        &ast, &cppfile, &src,
                        &PyList_Type, &py_extra_files,
                        &verbose,
                        &main_file_only,
                        &syn_base_path,
                        &syn_syntax_prefix,
                        &syn_xref_prefix)
      || !extract(py_extra_files, extra_files))
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

  AST::SourceFile *source_file = restore_source_file(ast, src);
  // Run OCC to generate the AST
  RunOpencxx(source_file, cppfile, ast);

#ifndef DONT_GC
  // Try to cleanup GC if being used
  //std::cout << "GC: Running Garbage Collection..." << std::endl;
  //size_t size = GC_get_heap_size();
  GC_gcollect();
  //size_t size2 = GC_get_heap_size();
  //std::cout << "GC: Heap went from " << size << " to " << size2 << std::endl;
#endif

  // Delete all the AST:: and Types:: objects we created
  FakeGC::delete_all();

  return ast;
}

PyMethodDef occ_methods[] = {{(char*)"parse", occ_parse, METH_VARARGS},
                             {0, 0}};
};

extern "C" void initocc()
{
  PyObject* m = Py_InitModule((char*)"occ", occ_methods);
  PyObject_SetAttrString(m, (char*)"version", PyString_FromString("0.1"));
}
