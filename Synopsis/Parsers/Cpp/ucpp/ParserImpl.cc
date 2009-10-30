//
// Copyright (C) 2003 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/ASG/ASGKit.hh>
#include <Support/Path.hh>
#include <Support/ErrorHandler.hh>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <memory>
#include <functional>
#include <algorithm>
#include "comments.hh"

using namespace Synopsis;

void ucpp(char const *input, char const *output,
          std::vector<std::string> const &flags);

namespace
{

PyObject *py_error;

int verbose = 0;
int debug = 0;
int profile = 0;
int primary_file_only = 0;
bool active = true;
char const *language;
std::string base_path;
// these python objects need to be referenced as pointers
// since the python runtime environment has to be initiaized first
std::auto_ptr<ASG::ASGKit> asg_kit;
std::auto_ptr<SourceFileKit> sf_kit;
std::auto_ptr<IR> ir;
std::auto_ptr<SourceFile> source_file;
char const *input = 0;

//. creates new SourceFile object and store it into ast
SourceFile create_source_file(std::string const &filename, bool primary)
{
  Path path = Path(filename).abs();
  path.strip(base_path);
  std::string name = path.str();
  SourceFile sf = sf_kit->create_source_file(name, filename);
  Python::Dict files = ir->files();
  files.set(name, sf);
  if (primary) sf.set_primary(true);
  return sf;
}

//. creates new or returns existing SourceFile object
//. with the given filename
SourceFile lookup_source_file(std::string const &filename, bool primary)
{
  Python::Dict files = ir->files();
  Path path = Path(filename).abs();
  path.strip(base_path);
  SourceFile sf = files.get(path.str());
  if (sf && primary) sf.set_primary(true);
  return sf ? sf : create_source_file(filename, primary);
}

//. creates new Macro object
void create_macro(char const *filename, int line,
		  char const *macro_name, int num_args, char const **args,
		  int vaarg, char const *text)
{
  Python::Tuple name(macro_name);
  SourceFile sf = lookup_source_file(filename, true);
  Python::List params;
  if (args)
  {
    for (int i = 0; i < num_args; ++i) params.append(args[i]);
    if (vaarg) params.append("...");
  }
  ASG::Macro macro = asg_kit->create_macro(sf, line, name, params, text);
  Python::List comments;
  for (std::vector<std::string>::iterator i = comment_cache.begin();
       i != comment_cache.end();
       ++i)
  {
    if (*i->rbegin() == '\n') i->erase(i->size() - 1);
    comments.append(*i);
  }
  clear_comment_cache();
  macro.annotations().set("comments", comments);

  Python::Module qn = Python::Module::import("Synopsis.QualifiedName");
  Python::Object qname_ = qn.attr("QualifiedCxxName");
  ASG::DeclaredTypeId declared =
    asg_kit->create_declared_type_id(qname_(Python::Tuple(Python::Object(name))), macro);

  Python::List declarations = ir->declarations();
  declarations.append(macro);

  Python::Dict types = ir->types();
  types.set(qname_(name), declared);
}

bool extract(PyObject *py_flags, std::vector<std::string> &out)
{
  Py_INCREF(py_flags);
  Python::List list = Python::Object(py_flags);
  for (size_t i = 0; i != list.size(); ++i)
  {
    std::string value = Python::Object::narrow<std::string>(list.get(i));
    out.push_back(value);
  }
  return true;
}

PyObject *ucpp_parse(PyObject *self, PyObject *args)
{
  char const *py_base_path;
  char *output;
  PyObject *py_system_flags, *py_flags;
  std::vector<std::string> flags;
  PyObject *py_ir;
  if (!PyArg_ParseTuple(args, "OszzsO!O!iiii",
                        &py_ir,
                        &input,
                        &py_base_path,
                        &output,
                        &language,
                        &PyList_Type, &py_system_flags,
                        &PyList_Type, &py_flags,
                        &primary_file_only,
                        &verbose,
                        &debug,
                        &profile)
      || !extract(py_flags, flags) || !extract(py_system_flags, flags))
    return 0;
    
  Py_INCREF(py_error);
  std::auto_ptr<Python::Object> error_type(new Python::Object(py_error));
  Py_INCREF(py_ir);
  // since everything in this file is accessed only during the execution
  // of ucpp_parse, we can safely manage these objects in this scope yet
  // reference them globally (for convenience)
  ir.reset(new IR(py_ir));
  asg_kit.reset(new ASG::ASGKit(language));
  sf_kit.reset(new SourceFileKit(language));

  try
  {
    if (py_base_path)
    {
      Path path(py_base_path);
      base_path = path.abs().str();
    }
    source_file.reset(new SourceFile(lookup_source_file(input, true)));
    start_file_preamble();

    // ucpp considers __STDC__ to be predefined, and doesn't allow
    // us to set it explicitely.
    flags.erase(std::remove(flags.begin(), flags.end(), std::string("-D__STDC__=1")),
                flags.end());
    if (verbose)
    {
      std::cout << "calling ucpp\n";
      for (std::vector<std::string>::iterator i = flags.begin();
 	   i != flags.end(); ++i)
 	std::cout << ' ' << *i;
      std::cout << std::endl;
    }
    ucpp(input, output, flags);
    
    Python::Dict files = ir->files();
    Path path = Path(input).abs();
    path.strip(base_path);
    files.set(path.str(), *source_file);

    py_ir = ir->ref(); // add new reference
    // make sure these objects are deleted before the python runtime
    source_file.reset();
    asg_kit.reset();
    sf_kit.reset();
    ir.reset();
    return py_ir;
  }
  catch (std::exception const &e)
  {
    Python::Object py_e((*error_type)(Python::Tuple(e.what())));
    PyErr_SetObject(py_error, py_e.ref());
    source_file.reset();
    asg_kit.reset();
    sf_kit.reset();
    ir.reset();
    return 0;
  }
}

extern "C"
{
  //. This function is a callback from the ucpp code to report
  //. a context switch in the parser, i.e. either to enter a
  //. new (included) file (@new_file == true) or to return control
  //. to an already open file once parsing an included file is
  //. terminated (@new_file == false)
  //. Depending on whether the new filename matches with the base_path,
  //. the parser is activated or deactivated
  void synopsis_file_hook(char const *filename, int new_file)
  {
    // We are about to enter a new file. Flush the comment cache.
    if (active)
      end_file_preamble();
    clear_comment_cache();

    // turn 'filename' into an absolute path so we can match it against
    // base_path
    std::string abs_filename = Path(filename).abs().str();
    bool activate = false;
    if ((primary_file_only && strcmp(input, filename)) || 
	(base_path.size() && abs_filename.substr(0, base_path.size()) != base_path))
      active = false;
    else
    {
      if (!active) active = activate = true;
    }
    if (!active) return;

    // just for symmetry, don't report if we were just activated 
    if (debug && !activate)
      if (new_file)
	std::cout << "entering new file " << abs_filename << std::endl;
      else
	std::cout << "returning to file " << abs_filename << std::endl;

    source_file.reset(new SourceFile(lookup_source_file(abs_filename, true)));
    if (new_file) start_file_preamble();
  }

  //. This function is called on the first non-comment token in
  //. a file, so we can collect all comments encountered so far
  //. and attach them to the source file object.
  void synopsis_file_preamble_hook()
  {
    Python::List comments;
    for (std::vector<std::string>::iterator i = comment_cache.begin();
	 i != comment_cache.end();
	 ++i)
    {
      if (*i->rbegin() == '\n') i->erase(i->size() - 1);
      comments.append(*i);
    }
    source_file->annotations().set("comments", comments);
  }

  //. This function is a callback from the ucpp code to store macro
  //. expansions
  void synopsis_macro_hook(char const *name,
                           int start_line, int start_col, int end_line, int end_col,
			   int e_start_line, int e_start_col, int e_end_line, int e_end_col)
  {
    if (!active) return;
    if (debug) 
      std::cout << "macro : " << name << " (" << start_line << ':' << start_col << ")<->("
		<< end_line << ':' << end_col << ") expanded to ("
                << e_start_line << ':' << e_start_col << ")<->(" << e_end_line << ':' << e_end_col << ')'
                << std::endl;

    Python::List calls = source_file->macro_calls();
    calls.append(sf_kit->create_macro_call(name,
                                           start_line, start_col, end_line, end_col,
                                           e_start_line, e_start_col, e_end_line, e_end_col));
  }

  //. This function is a callback from the ucpp code to store includes
  void synopsis_include_hook(char const *source, char const *target,
			     char const *fname, int is_system,
			     int is_macro, int is_next)
  {
    if (!active) return;
    std::string name = fname;
    if (is_system) name = '"' + name + '"';
    else name = '<' + name + '>';

    if (debug) 
      // source may be 0
      std::cout << "include : " << target << ' ' << name << ' '
		<< is_macro << ' ' << is_next << std::endl;

    Path path = Path(target).abs();

//     bool main = !base_path || strncmp(path.str().c_str(), base_path, strlen(base_path));
    SourceFile target_file = lookup_source_file(path.str(), false);
    Include include = sf_kit->create_include(target_file, name, is_macro, is_next);
    Python::List includes = source_file->includes();
    includes.append(include);
  }

  //. This function is a callback from the ucpp code to store macro
  //. definitions
  void synopsis_define_hook(char const *filename, int line,
			    char const *name,
			    int num_args, char const **args, int vaarg, 
			    char const *text)
  {
    if (!active) return;
    if (debug) 
      std::cout << "define : " << filename << ' ' << line << ' ' 
		<< name << ' ' << num_args << ' ' << text << std::endl;

    create_macro(filename, line, name, num_args, args, vaarg, text);
  }
};

PyMethodDef methods[] = {{(char*)"parse", ucpp_parse, METH_VARARGS},
			 {0}};
};

extern "C" void initParserImpl()
{
  Python::Module module = Python::Module::define("ParserImpl", methods);
  module.set_attr("version", "0.1");
  Python::Object processor = Python::Object::import("Synopsis.Processor");
  Python::Object error_base = processor.attr("Error");
  py_error = PyErr_NewException("ParserImpl.ParseError", error_base.ref(), 0);
  module.set_attr("ParseError", py_error);
}
