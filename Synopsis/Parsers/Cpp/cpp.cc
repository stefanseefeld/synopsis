//
// Copyright (C) 2003 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/AST/ASTKit.hh>
#include <Synopsis/AST/TypeKit.hh>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <memory>

using namespace Synopsis::AST; // import all AST objects...
namespace Python = Synopsis; // ...and the others into 'Python'

namespace
{

int verbose = 0;
int debug = 0;
const char *language;
const char *prefix = 0;
ASTKit *kit;
TypeKit *types;
AST *ast;
SourceFile *source_file;
const char *input = 0;

const char *strip_prefix(const char *filename, const char *prefix)
{
  if (!prefix) return filename;
  size_t length = strlen(prefix);
  if (length > strlen(filename)) return filename; // ??
  if (strncmp(filename, prefix, length) == 0)
    return filename + length;
  return filename;
}

//. creates new SourceFile object and store it into ast
SourceFile create_source_file(const char *filename, bool is_main)
{
  const char *name = strip_prefix(filename, prefix);
  SourceFile sf = kit->create_source_file(name, filename, language);
  Python::Dict files = ast->files();
  files.set(name, sf);
  sf.is_main(true);
  return sf;
}

//. creates new or returns existing SourceFile object
//. with the given filename
SourceFile lookup_source_file(const char *filename)
{
  Python::Dict files = ast->files();
  SourceFile sf = files.get(strip_prefix(filename, prefix));
  return sf ? sf : create_source_file(filename, false);
}

//. creates new Macro object
void create_macro(const char *filename, int line,
		  const char *macro_name, int num_args, const char **args,
		  int vaarg, const char *text)
{
  Python::Tuple name(macro_name);
  SourceFile sf = lookup_source_file(filename);
  Python::List params;
  if (args)
  {
    for (int i = 0; i < num_args; ++i) params.append(args[i]);
    if (vaarg) params.append("...");
  }
  Macro macro = kit->create_macro(sf, line, language, name, params, text);
  Declared declared = types->create_declared(language, name, macro);

  Python::List declarations = ast->declarations();
  declarations.append(macro);

  // FIXME: the 'types' attribute is not (yet) a dict type
  // so we have to do the call conversions manually...
  Python::Object types = ast->types();
  types.attr("__setitem__")(Python::Tuple(name, declared));
}

bool extract(PyObject *py_flags, std::vector<const char *> &out)
{
  Py_INCREF(py_flags);
  Python::List list = Python::Object(py_flags);
  for (size_t i = 0; i != list.size(); ++i)
  {
    const char *value = Python::Object::narrow<const char *>(list.get(i));
    if (!value) return false;
    out.push_back(value);
  }
  return true;
}

extern "C"
{
  // ucpp_main is the renamed main() func of ucpp, since it is included in this
  // module
  int ucpp_main(int argc, char** argv);
}

PyObject *ucpp_parse(PyObject *self, PyObject *args)
{
  try
  {
    char *output;
    PyObject *py_flags;
    std::vector<const char *> flags;
    PyObject *py_ast;
    if (!PyArg_ParseTuple(args, "OszzsO!ii",
                          &py_ast,
                          &input,
                          &prefix,
                          &output,
                          &language,
                          &PyList_Type, &py_flags,
                          &verbose,
                          &debug)
        || !extract(py_flags, flags))
      return 0;
    
    Py_INCREF(py_ast);

    // since everything in this file is accessed only during the execution
    // of ucpp_parse, we can safely manage these objects in this scope yet
    // reference them globally (for convenience)
    std::auto_ptr<AST> ast_ptr(new AST(py_ast));
    ast = ast_ptr.get();

    std::auto_ptr<ASTKit> kit_ptr(new ASTKit());
    kit = kit_ptr.get();

    std::auto_ptr<TypeKit> types_ptr(new TypeKit());
    types = types_ptr.get();

    std::auto_ptr<SourceFile> sf_ptr(new SourceFile(create_source_file(input, true)));
    source_file = sf_ptr.get();

    flags.insert(flags.begin(), "ucpp");
    flags.push_back("-C"); // keep comments
    flags.push_back("-lg"); // gcc-like line numbers
    if (output)
    {
      flags.push_back("-o"); // output to...
      flags.push_back(output);
    }
    else
    {
      flags.push_back("-o"); // output to...
      flags.push_back("/dev/null");
    }
    flags.push_back(input);
    if (verbose)
    {
      std::cout << "calling ucpp\n";
      for (std::vector<const char *>::iterator i = flags.begin();
	   i != flags.end(); ++i)
	std::cout << ' ' << *i;
      std::cout << std::endl;
    }
    
    int status = ucpp_main(flags.size(), (char **)&*flags.begin());
    if (status != 0)
      std::cerr << "ucpp returned error flag. ignoring error." << std::endl;

    Python::Dict files = ast->files();
    files.set(strip_prefix(input, prefix), *source_file);
    return ast->ref();
  }
  catch (const std::exception &e)
  {
    std::cerr << "Internal error : " << e.what() << std::endl;
    return 0;
  }
}

extern "C"
{
  //. This function is a callback from the ucpp code to store macro
  //. expansions
  void synopsis_macro_hook(const char *name, int line_num,
			   int start, int end, int diff)
  {
    if (debug) 
      std::cout << "macro : " << name << ' ' << line_num << ' '
		<< start << ' ' << end << ' ' << diff << std::endl;
    
    Python::Dict mmap = source_file->macro_calls();
    Python::List line = mmap.get(line_num, Python::List());
    line.append(kit->create_macro_call(name, start, end, diff));
    mmap.set(line_num, line);
  }

  //. This function is a callback from the ucpp code to store includes
  void synopsis_include_hook(const char *source, const char *target,
			     int is_macro, int is_next)
  {
    if (debug) 
      std::cout << "include : " << source << ' ' << target << ' ' 
		<< is_macro << ' ' << is_next << std::endl;

    if (strcmp(input, source) != 0) return;
    SourceFile target_file = lookup_source_file(target);

    Include include = kit->create_include(target_file, is_macro, is_next);
    Python::List includes = source_file->includes();
    includes.append(include);
  }

  //. This function is a callback from the ucpp code to store macro
  //. definitions
  void synopsis_define_hook(const char *filename, int line,
			    const char *name,
			    int num_args, const char **args, int vaarg, 
			    const char *text)
  {
    if (debug) 
      std::cout << "define : " << filename << ' ' << line << ' ' 
		<< name << ' ' << num_args << ' ' << text << std::endl;

    if (strcmp(input, filename) != 0) return;
    create_macro(filename, line, name, num_args, args, vaarg, text);
  }
};

PyMethodDef methods[] = {{(char*)"parse", ucpp_parse, METH_VARARGS},
			 {0, 0}};
};

extern "C" void initucpp()
{
  Python::Module module = Python::Module::define("ucpp", methods);
  module.set_attr("version", "0.1");
}
