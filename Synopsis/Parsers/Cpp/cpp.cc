// $Id: cpp.cc,v 1.3 2004/01/04 02:08:26 stefan Exp $
//
// Copyright (C) 2003 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Python.h>
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
PyObject *ast;
const char *prefix = 0;
PyObject *language = 0;
PyObject *source_file = 0;
const char *input = 0;

//. MacroMap is a map from preprocessed file positions to input file positions.
//. This may eventually be part of the AST (if more than just declarations are
//. supported), but for now it is persisted into a file in parallel so it can be
//. read in by C/C++ parsers for correct symbol cross referencing.
struct MacroMap
{
  struct Node
  {
    std::string name;
    int start;
    int end;
    enum Type { START, END } type;
    int diff;
    bool operator <(const Node &other) const
    {
        return start < other.start;
    }
  };
  typedef std::set<Node> Line;
  typedef std::map<int, Line> LineMap;

  //. Adds a map at the given line. out_{start,end} define the start and
  //. past-the-end markers for the macro in the output file. diff defines
  //. the signed difference to add to the pos.
  void add(const char *name, int linenum, int start, int end, int diff)
  {
    Line &line = lines[linenum];
    Node node;
    node.name = name; // name isn't really needed. For now keep it for debugging...
    node.start = start;
    node.end = end;
    node.type = Node::START;
    node.diff = diff;
    line.insert(node);
  }
  void write(const char *filename)
  {
    std::ofstream ofs(filename);
    for (LineMap::iterator i = lines.begin();
	 i != lines.end(); ++i)
      for (MacroMap::Line::iterator j = (*i).second.begin();
	   j != (*i).second.end(); ++j)
      ofs << (*i).first << ' '
	  << (*j).name << ' '
	  << (*j).start << ' '
	  << (*j).end << ' '
	  << (*j).type << ' '
	  << (*j).diff << '\n';
  }

  LineMap lines;
} mmap;

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
PyObject *create_source_file(const char *filename, bool is_main)
{
  PyObject *short_filename = PyString_FromString(strip_prefix(filename, prefix));
  PyObject *long_filename = PyString_FromString(filename);
  PyObject *ast_module = PyImport_ImportModule("Synopsis.AST");
  PyObject *source_file = PyObject_CallMethod(ast_module, "SourceFile", "OOO",
				    short_filename, long_filename, language);
  Py_DECREF(ast_module);
  if (is_main) PyObject_CallMethod(source_file, "set_is_main", "i", 1);
  PyObject *pyfiles = PyObject_CallMethod(ast, "files", 0);
  PyDict_SetItem(pyfiles, short_filename, source_file);
  Py_DECREF(short_filename);
  Py_DECREF(long_filename);
  Py_DECREF(pyfiles);
  return source_file;
}

//. creates new or returns existing SourceFile object
//. with the given filename
PyObject *lookup_source_file(const char *filename)
{
  PyObject *short_filename = PyString_FromString(strip_prefix(filename, prefix));
  PyObject *pyfiles = PyObject_CallMethod(ast, "files", 0);
  PyObject *source_file = PyDict_GetItem(pyfiles, short_filename);
  if (source_file) Py_INCREF(source_file);
  Py_DECREF(pyfiles);
  Py_DECREF(short_filename);
  return source_file ? source_file : create_source_file(filename, false);
}

void create_include(PyObject *source, PyObject *target,
		    bool is_macro, bool is_next)
{
  PyObject *ast_module = PyImport_ImportModule("Synopsis.AST");
  PyObject *include = PyObject_CallMethod(ast_module, "Include", "Oii",
					  target, is_macro, is_next);
  Py_DECREF(ast_module);
  PyObject *includes = PyObject_CallMethod(source, "includes", 0);
  PyObject_CallMethod(includes, "append", "O", include);
  Py_DECREF(includes);
  Py_DECREF(include);
}

//. creates new Macro object
void create_macro(const char *filename, int line,
		  const char *macro_name, int num_args, const char **args,
		  int vaarg, const char *text)
{
  PyObject *file = lookup_source_file(filename);
  PyObject *type = PyString_FromString("macro");
  PyObject *name = PyTuple_New(1);
  PyTuple_SET_ITEM(name, 0, PyString_FromString(macro_name));
  PyObject *params = 0;
  if (args)
  {
    params = PyList_New(vaarg ? num_args + 1 : num_args);
    for (int i = 0; i < num_args; i++)
      PyList_SET_ITEM(params, i, PyString_FromString(args[i]));
    if (vaarg)
      PyList_SET_ITEM(params, num_args, PyString_FromString("..."));
  }
  else
  {
    params = Py_None;
    Py_INCREF(Py_None);
  }
  PyObject *pytext = PyString_FromString(text);
  PyObject *ast_module = PyImport_ImportModule("Synopsis.AST");
  PyObject *macro = PyObject_CallMethod(ast_module, "Macro", "OiOOOOO",
					file, line, language, type,
					name, params, pytext);
  Py_DECREF(ast_module);
  Py_DECREF(pytext);
  Py_DECREF(params);
  Py_DECREF(name);
  Py_DECREF(type);
  Py_DECREF(file);
  PyObject *declarations = PyObject_CallMethod(ast, "declarations", 0);
  PyObject_CallMethod(declarations, "append", "O", macro);
  Py_DECREF(macro);
  Py_DECREF(declarations);
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

extern "C"
{
  // ucpp_main is the renamed main() func of ucpp, since it is included in this
  // module
  int ucpp_main(int argc, char** argv);
}

PyObject *ucpp_parse(PyObject *self, PyObject *args)
{
  char *output;
  char *mmap_file;
  PyObject *py_cppflags;
  std::vector<const char *> cppflags;
  if (!PyArg_ParseTuple(args, "OszszOO!ii",
                        &ast,
                        &input,
			&prefix,
                        &output,
			&mmap_file,
			&language,
                        &PyList_Type, &py_cppflags,
                        &verbose,
                        &debug)
      || !extract(py_cppflags, cppflags))
    return 0;

  Py_INCREF(ast);
  source_file = create_source_file(input, true);

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

  if (mmap_file) mmap.write(mmap_file);

  return ast;
}

extern "C"
{
  //. This function is a callback from the ucpp code to store macro
  //. expansions
  void synopsis_macro_hook(const char *name, int line, int start, int end, int diff)
  {
    if (debug) 
      std::cout << "macro : " << name << ' ' << line << ' ' << start << ' ' << end << ' ' << diff << std::endl;
    
    mmap.add(name, line, start, end, diff);
  }

  //. This function is a callback from the ucpp code to store includes
  void synopsis_include_hook(const char *source, const char *target, int is_macro, int is_next)
  {
    if (debug) 
      std::cout << "include : " << source << ' ' << target << ' ' << is_macro << ' ' << is_next << std::endl;

    if (strcmp(input, source) != 0) return;
    PyObject *target_file = lookup_source_file(target);
    create_include(source_file, target_file, is_macro, is_next);
    Py_DECREF(target_file);
  }

  //. This function is a callback from the ucpp code to store macro
  //. definitions
  void synopsis_define_hook(const char *filename, int line, const char *name, int num_args, const char **args, int vaarg, const char *text)
  {
    if (debug) 
      std::cout << "define : " << filename << ' ' << line << ' ' << name << ' ' << num_args << ' ' << text << std::endl;

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
