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
#include <functional>

using namespace Synopsis::AST; // import all AST objects...
namespace Python = Synopsis; // ...and the others into 'Python'

namespace
{

int verbose = 0;
int debug = 0;
int main_file_only = 0;
bool active = true;
const char *language;
const char *base_path = 0;
// these python objects need to be referenced as pointers
// since the python runtime environment has to be initiaized first
std::auto_ptr<ASTKit> kit;
std::auto_ptr<TypeKit> types;
std::auto_ptr<AST> ast;
std::auto_ptr<SourceFile> source_file;
const char *input = 0;

//. return portably the current working directory
const std::string &get_cwd()
{
  static std::string path;
  if (path.empty())
#ifdef __WIN32__
  {
    DWORD size;
    if ((size = ::GetCurrentDirectoryA(0, 0)) == 0)
    {
      delete [] buf;
      throw std::runtime_error("error accessing current working directory");
    }
    char *buf = new char[size];
    if (::GetCurrentDirectoryA(size, buf) == 0)
    {
      delete [] buf;
      throw std::runtime_error("error accessing current working directory");
    }
    path = buf;
    delete [] buf;
  }
#else
    for (long path_max = 32;; path_max *= 2)
    {
      char *buf = new char[path_max];
      if (::getcwd(buf, path_max) == 0)
      {
	if (errno != ERANGE)
	{
	  delete [] buf;
	  throw std::runtime_error(strerror(errno));
	}
      }
      else
      {
	path = buf;
	delete [] buf;
	return path;
      }
      delete [] buf;
    }
#endif
  return path;
}

// normalize and absolutize the given path path
std::string normalize_path(std::string filename)
{
#ifdef __WIN32__
  char separator = '\\';
  const char *pat1 = "\\.\\";
  const char *pat2 = "\\..\\";
#else
  char separator = '/';
  const char *pat1 = "/./";
  const char *pat2 = "/../";
#endif
  if (filename[0] != separator)
    filename.insert(0, get_cwd() + separator);

  // nothing to do...
  if (filename.find(pat1) == std::string::npos &&
      filename.find(pat2) == std::string::npos) return filename;
  
  // for the rest we'll operate on a decomposition of the filename...
  typedef std::vector<std::string> Path;
  Path path;

  std::string::size_type b = 0;
  while (b < filename.size())
  {
    std::string::size_type e = filename.find(separator, b);
    path.push_back(std::string(filename, b, e-b));
    b = e == std::string::npos ? std::string::npos : e + 1;
  }

  // remove all '.' and '' components
  path.erase(std::remove(path.begin(), path.end(), "."), path.end());
  path.erase(std::remove(path.begin(), path.end(), ""), path.end());
  // now collapse '..' components with the preceding one
  while (true)
  {
    Path::iterator i = std::find(path.begin(), path.end(), "..");
    if (i == path.end()) break;
    if (i == path.begin()) throw std::invalid_argument("invalid path");
    path.erase(i - 1, i + 1); // remove two components
  }

  // now rebuild the path as a string
  std::string retn = '/' + path.front();
  for (Path::iterator i = path.begin() + 1; i != path.end(); ++i)
    retn += '/' + *i;
  return retn;
}

const char *strip_base_path(const char *filename)
{
  if (!base_path) return filename;
  size_t length = strlen(base_path);
  if (strncmp(filename, base_path, length) == 0)
    return filename + length;
  return filename;
}

//. creates new SourceFile object and store it into ast
SourceFile create_source_file(const char *filename, bool is_main)
{
  const char *name = strip_base_path(filename);
  SourceFile sf = kit->create_source_file(name, filename, language);
  Python::Dict files = ast->files();
  files.set(name, sf);
  if (is_main) sf.is_main(true);
  return sf;
}

//. creates new or returns existing SourceFile object
//. with the given filename
SourceFile lookup_source_file(const char *filename, bool main)
{
  Python::Dict files = ast->files();
  SourceFile sf = files.get(strip_base_path(filename));
  if (sf && main) sf.is_main(true);
  return sf ? sf : create_source_file(filename, main);
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
    if (!PyArg_ParseTuple(args, "OszzsO!iii",
                          &py_ast,
                          &input,
                          &base_path,
                          &output,
                          &language,
                          &PyList_Type, &py_flags,
                          &main_file_only,
                          &verbose,
                          &debug)
        || !extract(py_flags, flags))
      return 0;
    
    Py_INCREF(py_ast);
    // since everything in this file is accessed only during the execution
    // of ucpp_parse, we can safely manage these objects in this scope yet
    // reference them globally (for convenience)
    ast.reset(new AST(py_ast));
    kit.reset(new ASTKit());
    types.reset(new TypeKit());
    source_file.reset(new SourceFile(lookup_source_file(input, true)));

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
    files.set(strip_base_path(input), *source_file);

    py_ast = ast->ref(); // add new reference
    // make sure these objects are deleted before the python runtime
    source_file.reset();
    types.reset();
    kit.reset();
    ast.reset();
    return py_ast;
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
    std::string abs_filename = normalize_path(filename);

    bool activate = false;
    if ((main_file_only && strcmp(input, filename)) || 
	(base_path && 
	 strncmp(abs_filename.c_str(), base_path, strlen(base_path)) != 0))
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

    source_file.reset(new SourceFile(lookup_source_file(abs_filename.c_str(), true)));
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
    line.append(kit->create_macro_call(name, start, end, diff));
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

    std::string abs_target = normalize_path(target);

    SourceFile target_file = lookup_source_file(abs_target.c_str(), false);

    Include include = kit->create_include(target_file, name, is_macro, is_next);
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
			 {0, 0}};
};

extern "C" void initucpp()
{
  Python::Module module = Python::Module::define("ucpp", methods);
  module.set_attr("version", "0.1");
}
