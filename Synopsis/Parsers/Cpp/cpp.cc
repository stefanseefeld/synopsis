// $Id: cpp.cc,v 1.4 2004/01/13 07:44:25 stefan Exp $
//
// Copyright (C) 2003 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/AST/ASTModule.hh>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>

namespace
{

int verbose = 0;
int debug = 0;
const char *language;
const char *prefix = 0;
Synopsis::ASTModule ast_module;
Synopsis::AST ast;
Synopsis::SourceFile source_file;
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

//. creates new SourceFile object
Synopsis::SourceFile create_source_file(const char *filename, bool is_main)
{
  Synopsis::SourceFile sf = ast_module.create_source_file(strip_prefix(filename,
								       prefix),
							  filename,
							  language);
  sf.is_main(true);
  return sf;
}

//. creates new or returns existing SourceFile object
//. with the given filename
Synopsis::SourceFile lookup_source_file(const char *filename)
{
  Synopsis::Dict files = ast.files();
  Synopsis::SourceFile sf = files.get(strip_prefix(filename, prefix));
  return sf ? sf : create_source_file(filename, false);
}

//. creates new Macro object
void create_macro(const char *filename, int line,
		  const char *macro_name, int num_args, const char **args,
		  int vaarg, const char *text)
{
  Synopsis::Tuple name(macro_name);
  Synopsis::SourceFile sf = lookup_source_file(filename);
  std::vector<std::string> params;
  if (args)
  {
    params.resize(vaarg ? num_args + 1 : num_args);
    for (int i = 0; i < num_args; ++i)
      params[i] = args[i];
    if (vaarg)
      params[num_args] = "...";
  }
  Synopsis::Macro macro = ast_module.create_macro(sf, line, language,
						  "macro", name,
						  params, text);

  Synopsis::List declarations = ast.declarations();
  declarations.append(macro);
}

bool extract(PyObject *py_flags, std::vector<const char *> &out)
{
  Py_INCREF(py_flags);
  Synopsis::List list = Synopsis::Object(py_flags);
  for (size_t i = 0; i != list.size(); ++i)
  {
    const char *value = Synopsis::Object::narrow<const char *>(list.get(i));
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
    if (!PyArg_ParseTuple(args, "OszssO!ii",
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
    ast = Synopsis::AST(py_ast);
    ast_module = Synopsis::ASTModule();
    source_file = create_source_file(input, true);
    
    flags.insert(flags.begin(), "ucpp");
    flags.push_back("-C"); // keep comments
    flags.push_back("-lg"); // gcc-like line numbers
    if (output)
    {
      flags.push_back("-o"); // output to...
      flags.push_back(output);
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

    Synopsis::Dict files = ast.files();
    files.set(input, source_file);
    return ast.ref();
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
    
    Synopsis::Dict mmap = source_file.macro_calls();
    Synopsis::List line = mmap.get(line_num, Synopsis::List());
    line.append(ast_module.create_macro_call(name, start, end, diff));
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
    Synopsis::SourceFile target_file = lookup_source_file(target);

    Synopsis::Include include = ast_module.create_include(target_file, is_macro, is_next);
    Synopsis::List includes = source_file.includes();
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

PyMethodDef ucpp_methods[] = {{(char*)"parse", ucpp_parse, METH_VARARGS},
                             {0, 0}};
};

extern "C" void initucpp()
{
  PyObject* m = Py_InitModule((char*)"ucpp", ucpp_methods);
  PyObject_SetAttrString(m, (char*)"version", PyString_FromString("0.1"));
}
