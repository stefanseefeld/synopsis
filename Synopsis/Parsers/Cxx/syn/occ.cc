//
// Copyright (C) 2000 Stefan Seefeld
// Copyright (C) 2000 Stephen Davies
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/Python/Module.hh>
#include <Synopsis/Trace.hh>
#include <Synopsis/ErrorHandler.hh>
#include "Translator.hh"
#include "swalker.hh"
#include "builder.hh"
#include "dumper.hh"
#include "filter.hh"
#include "linkstore.hh"

#include <occ/PTree/Display.hh>
#include <occ/Buffer.hh>
#include <occ/Lexer.hh>
#include <occ/Parser.hh>
#include <occ/MetaClass.hh>
#include <occ/Environment.hh>

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <stdexcept>

using namespace Synopsis;

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

namespace
{

//. Override unexpected() to print a message before we abort
void unexpected()
{
  std::cout << "Warning: Aborting due to unexpected exception." << std::endl;
  throw std::bad_exception();
}

const char *strip_prefix(const char *filename, const char *prefix)
{
  if (!prefix) return filename;
  size_t length = strlen(prefix);
  if (strncmp(filename, prefix, length) == 0)
    return filename + length;
  return filename;
}

void error()
{
  SWalker *instance = SWalker::instance();
  std::cerr << "processing " << instance->current_file()->filename()
            << " at line " << instance->current_lineno()
            << std::endl;
}

void RunOpencxx(AST::SourceFile *sourcefile, const char *file, PyObject *ast)
{
  Trace trace("RunOpencxx");
  std::set_unexpected(unexpected);

  ErrorHandler error_handler(error);

  std::ifstream ifs(file);
  if(!ifs)
  {
    perror(file);
    exit(1);
  }
  Buffer buffer(ifs.rdbuf());
  Lexer lexer(&buffer);
  Parser parse(&lexer);

  FileFilter* filter = FileFilter::instance();

  Builder builder(sourcefile);
  SWalker swalker(filter, &parse, &builder, &buffer);
  if (syn_fake_std)
  {
    // Fake a using from "std" to global
    builder.start_namespace("std", NamespaceNamed);
    builder.add_using_namespace(builder.global()->declared());
    builder.end_namespace();
  }

  if (filter->should_link(sourcefile) || filter->should_xref(sourcefile))
    swalker.set_store_links(new LinkStore(filter, &swalker));
  try
  {
    PTree::Node *def;
    while(parse.rProgram(def))
      swalker.Translate(def);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error : " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "Error: an uncaught exception occurred when translating the parse tree" << std::endl;
  }

  // Setup synopsis c++ to py convertor
  Translator translator(filter, ast);//declarations, types);
  translator.set_builtin_decls(builder.builtin_decls());
  // Convert!
  translator.translate(builder.scope());

  if(parse.NumOfErrors() != 0)
  {
    std::cerr << "Ignoring errors while parsing file: " << file << std::endl;
  }

  ifs.close();
}

PyObject *occ_parse(PyObject *self, PyObject *args)
{
  Class::do_init_static();
  Metaclass::do_init_static();
  Environment::do_init_static();
  PTree::Encoding::do_init_static();

  PyObject *ast;
  const char *src, *cppfile;
  int main_file_only, verbose, debug;
  if (!PyArg_ParseTuple(args, "Ossizzzii",
                        &ast, &cppfile, &src,
                        &main_file_only,
                        &syn_base_path,
                        &syn_syntax_prefix,
                        &syn_xref_prefix,
                        &verbose,
                        &debug))
    return 0;

  Py_INCREF(ast);

  if (verbose) ::verbose = true;
  if (debug) Trace::enable_debug();
  if (main_file_only) syn_main_only = true;

  if (!src || *src == '\0')
  {
    PyErr_SetString(PyExc_RuntimeError, "no input file");
    return 0;
  }

  // Setup the filter
  FileFilter filter(ast, src, syn_base_path, syn_main_only);
  if (syn_syntax_prefix) filter.set_syntax_prefix(syn_syntax_prefix);
  if (syn_xref_prefix) filter.set_xref_prefix(syn_xref_prefix);

  AST::SourceFile *source_file = filter.get_sourcefile(src);
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

PyObject *occ_print(PyObject *self, PyObject *args)
{
  Class::do_init_static();
  Metaclass::do_init_static();
  Environment::do_init_static();
  PTree::Encoding::do_init_static();

  const char *src;
  int verbose, debug;
  if (!PyArg_ParseTuple(args, "sii",
                        &src,
                        &verbose,
                        &debug))
    return 0;

  if (verbose) ::verbose = true;
  if (debug) Trace::enable_debug();

  if (!src || *src == '\0')
  {
    PyErr_SetString(PyExc_RuntimeError, "no input file");
    return 0;
  }

  std::ifstream ifs(src);
  if(!ifs)
  {
    perror(src);
    exit(1);
  }
  try
  {
    Buffer buffer(ifs.rdbuf());
    Lexer lexer(&buffer);
    Parser parse(&lexer);

    PTree::Display display(std::cout, true);
    PTree::Node *def;
    while(parse.rProgram(def)) display.display(def);
  }
  catch (...)
  {
    std::cerr << "Warning: an uncaught exception occurred when translating the parse tree" << std::endl;
  }
  Py_INCREF(Py_None);
  return Py_None;
}

PyMethodDef methods[] = {{(char*)"parse", occ_parse, METH_VARARGS},
			 {(char*)"dump", occ_print, METH_VARARGS},
			 {0, 0}};
}

extern "C" void initocc()
{
  Python::Module module = Python::Module::define("occ", methods);
  module.set_attr("version", "0.1");
}
