//
// Copyright (C) 2003 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/ASG/ASGKit.hh>
#include <Synopsis/ASG/TypeKit.hh>
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

using namespace Synopsis;

PyObject *py_error;

namespace
{

int verbose = 0;
int debug = 0;
int profile = 0;
int primary_file_only = 0;
bool active = true;
const char *language;
std::string base_path;
// these python objects need to be referenced as pointers
// since the python runtime environment has to be initiaized first
std::auto_ptr<ASG::ASGKit> ast_kit;
std::auto_ptr<SourceFileKit> sf_kit;
std::auto_ptr<ASG::TypeKit> types;
std::auto_ptr<IR> ir;
std::auto_ptr<SourceFile> source_file;
const char *input = 0;

//. creates new SourceFile object and store it into ast
SourceFile create_source_file(const std::string &filename, bool primary)
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
SourceFile lookup_source_file(const std::string &filename, bool primary)
{
  Python::Dict files = ir->files();
  Path path = Path(filename).abs();
  path.strip(base_path);
  SourceFile sf = files.get(path.str());
  if (sf && primary) sf.set_primary(true);
  return sf ? sf : create_source_file(filename, primary);
}

//. creates new Macro object
void create_macro(const char *filename, int line,
		  const char *macro_name, int num_args, const char **args,
		  int vaarg, const char *text)
{
  Python::Tuple name(macro_name);
  SourceFile sf = lookup_source_file(filename, true);
  Python::List params;
  if (args)
  {
    for (int i = 0; i < num_args; ++i) params.append(args[i]);
    if (vaarg) params.append("...");
  }
  ASG::Macro macro = ast_kit->create_macro(sf, line, name, params, text);
  ASG::Declared declared = types->create_declared(name, macro);

  Python::List declarations = ir->declarations();
  declarations.append(macro);

  // FIXME: the 'types' attribute is not (yet) a dict type
  // so we have to do the call conversions manually...
  Python::Object types = ir->types();
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
    const char *py_base_path;
    char *output;
    PyObject *py_system_flags, *py_flags;
    std::vector<const char *> flags;
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
        || !extract(py_system_flags, flags) || !extract(py_flags, flags))
      return 0;
    
    Py_INCREF(py_error);
    std::auto_ptr<Python::Object> error_type(new Python::Object(py_error));
    Py_INCREF(py_ir);
    // since everything in this file is accessed only during the execution
    // of ucpp_parse, we can safely manage these objects in this scope yet
    // reference them globally (for convenience)
    ir.reset(new IR(py_ir));
    ast_kit.reset(new ASG::ASGKit());
    sf_kit.reset(new SourceFileKit(language));
    types.reset(new ASG::TypeKit(language));

    if (py_base_path)
    {
      Path path(py_base_path);
      base_path = path.abs().str();
    }

    source_file.reset(new SourceFile(lookup_source_file(input, true)));

    // ucpp considers __STDC__ to be predefined, and doesn't allow
    // us to set it explicitely.
    flags.erase(std::remove(flags.begin(), flags.end(), std::string("-D__STDC__=1")),
                flags.end());

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
#ifdef __WIN32__
      flags.push_back("NUL");
#else
      flags.push_back("/dev/null");
#endif
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
    {
      std::string msg = "ucpp could not parse ";
      msg += input;
      Python::Object py_e((*error_type)(Python::Tuple(msg)));
      PyErr_SetObject(py_error, py_e.ref());
    }

    Python::Dict files = ir->files();
    Path path = Path(input).abs();
    path.strip(base_path);
    files.set(path.str(), *source_file);

    py_ir = ir->ref(); // add new reference
    // make sure these objects are deleted before the python runtime
    source_file.reset();
    types.reset();
    ast_kit.reset();
    sf_kit.reset();
    ir.reset();
    return py_ir;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Internal error : " << e.what() << std::endl;
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
  void synopsis_file_hook(const char *filename, int new_file)
  {
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
  }

  //. This function is a callback from the ucpp code to store macro
  //. expansions
  void synopsis_macro_hook(const char *name, int line_num,
			   int start, int end, int diff)
  {
    if (!active) return;
    if (debug) 
      std::cout << "macro : " << name << ' ' << line_num << ' '
		<< start << ' ' << end << ' ' << diff << std::endl;

    Python::Dict mmap = source_file->macro_calls();
    Python::List line = mmap.get(line_num, Python::List());
    line.append(sf_kit->create_macro_call(name, start, end, diff));
    mmap.set(line_num, line);
  }

  //. This function is a callback from the ucpp code to store includes
  void synopsis_include_hook(const char *source, const char *target,
			     const char *fname, int is_system,
			     int is_macro, int is_next)
  {
    if (!active) return;

    std::string name = fname;
    if (is_system) name = '"' + name + '"';
    else name = '<' + name + '>';

    if (debug) 
      std::cout << "include : " << source << ' ' << target << ' ' << name << ' '
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
  void synopsis_define_hook(const char *filename, int line,
			    const char *name,
			    int num_args, const char **args, int vaarg, 
			    const char *text)
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
