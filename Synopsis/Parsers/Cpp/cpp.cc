// $Id: cpp.cc,v 1.2 2004/01/02 03:58:05 stefan Exp $
//
// Copyright (C) 2003 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Python.h>
#include <vector>
#include <iostream>
#include <cstring>
#include <cstdio>

int verbose = 0;
int debug = 0;

extern "C"
{
  // ucpp_main is the renamed main() func of ucpp, since it is included in this
  // module
  int ucpp_main(int argc, char** argv);

  //. This function is a callback from the ucpp code to store macro
  //. expansions
  void synopsis_macro_hook(const char *name, int line, int start, int end, int diff)
  {
    if (debug) 
      std::cout << "macro : " << name << ' ' << line << ' ' << start << ' ' << end << ' ' << diff << std::endl;
#if 0
    LinkMap::instance()->add(name, line, start, end, diff);
#endif
  }

  //. This function is a callback from the ucpp code to store includes
  void synopsis_include_hook(const char *source_file, const char *target_file, int is_macro, int is_next)
  {
    if (debug) 
      std::cout << "include : " << source_file << ' ' << target_file << ' ' << is_macro << ' ' << is_next << std::endl;
#if 0
    // There is not enough code here to make another class..
    FileFilter *filter = FileFilter::instance();
    if (!filter) return;

    // Add another Include to the source's SourceFile
    // We don't deal with duplicates here. The Linker's Unduplicator will
    // take care of that.
    //std::cout << "Include: " << source_file << " -> " << target_file << std::endl;
    AST::SourceFile *file = filter->get_sourcefile(source_file);
    AST::SourceFile *target = filter->get_sourcefile(target_file);
    AST::Include *include = new AST::Include(target, is_macro, is_next);
    file->includes().push_back(include);
#endif
  }

  //. This function is a callback from the ucpp code to store macro
  //. definitions
  void synopsis_define_hook(const char* filename, int line, const char *name, int num_args, const char **args, int vaarg, const char *text)
  {
    if (debug) 
      std::cout << "define : " << filename << ' ' << line << ' ' << name << ' ' << num_args << ' ' << text << std::endl;
#if 0
    FileFilter *filter = FileFilter::instance();
    if (!filter) return;
    AST::SourceFile *file = filter->get_sourcefile(filename);
    if (!file->is_main()) return;
    if (!syn_macro_defines) syn_macro_defines = new std::vector<AST::Macro*>;
    AST::Macro::Parameters *params = 0;
    if (args)
    {
      params = new AST::Macro::Parameters;
      for (int i = 0; i < num_args; i++)
	params->push_back(args[i]);
      if (vaarg) params->push_back("...");
    }
    ScopedName macro_name;
    macro_name.push_back(name);
    AST::Macro* macro = new AST::Macro(file, line, macro_name, params, text);
    //. Don't know global NS yet.. Will have to extract these later
    file->declarations().push_back(macro);
    syn_macro_defines->push_back(macro);
#endif
  }
};

namespace
{

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

PyObject *ucpp_parse(PyObject *self, PyObject *args)
{
  PyObject *ast;
  char *input, *output;
  PyObject *py_cppflags;
  std::vector<const char *> cppflags;
  if (!PyArg_ParseTuple(args, "OszO!ii",
                        &ast,
                        &input,
                        &output,
                        &PyList_Type, &py_cppflags,
                        &verbose,
                        &debug)
      || !extract(py_cppflags, cppflags))
    return 0;

  Py_INCREF(ast);


  cppflags.insert(cppflags.begin(), "ucpp");
  cppflags.push_back("-C"); // keep comments
  cppflags.push_back("-lg"); // gcc-like line numbers
  if (output)
  {
    cppflags.push_back("-o"); // output to...
    cppflags.push_back(output);
  }
  cppflags.push_back(input);
  if (verbose)
  {
    std::cout << "calling ucpp\n";
    for (std::vector<const char *>::iterator i = cppflags.begin();
	 i != cppflags.end(); ++i)
      std::cout << ' ' << *i;
    std::cout << std::endl;
  }
    
  int status = ucpp_main(cppflags.size(), (char **)&*cppflags.begin());
  if (status != 0)
    std::cerr << "ucpp returned error flag. ignoring error." << std::endl;
  return ast;
}

PyMethodDef ucpp_methods[] = {{(char*)"parse", ucpp_parse, METH_VARARGS},
                             {0, 0}};
};

extern "C" void initucpp()
{
  PyObject* m = Py_InitModule((char*)"ucpp", ucpp_methods);
  PyObject_SetAttrString(m, (char*)"version", PyString_FromString("0.1"));
}
