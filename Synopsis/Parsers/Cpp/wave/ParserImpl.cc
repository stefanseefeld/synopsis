//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/AST/ASTKit.hh>
#include <Synopsis/ErrorHandler.hh>

#include <Synopsis/Python/Module.hh>
#include <Synopsis/Trace.hh>
#include <Synopsis/ErrorHandler.hh>
#include "ASTTranslator.hh"
#include <memory>

using namespace Synopsis;
namespace wave = boost::wave;

namespace
{
#ifdef __WIN32__
  std::string const trash = "NUL";
#else
  std::string const trash = "/dev/null";
#endif

PyObject *error;

//. Override unexpected() to print a message before we abort
void unexpected()
{
  std::cout << "Error: Aborting due to unexpected exception." << std::endl;
  throw std::bad_exception();
}

bool extract(PyObject *py_flags, std::vector<char const *> &out)
{
  Py_INCREF(py_flags);
  Python::List list = Python::Object(py_flags);
  for (size_t i = 0; i != list.size(); ++i)
  {
    char const *value = Python::Object::narrow<char const *>(list.get(i));
    if (!value) return false;
    out.push_back(value);
  }
  return true;
}

PyObject *parse(PyObject *self, PyObject *args)
{
  char const *input_file;
  char const *base_path;
  char const *output_file;
  char const *language;
  PyObject *py_flags;
  std::vector<char const *> flags;
  PyObject *py_ast;
  int main_file_only = 0;
  int verbose = 0;
  int debug = 0;
  if (!PyArg_ParseTuple(args, "OszzsO!iii",
			&py_ast,
			&input_file,
			&base_path,
			&output_file,
			&language,
			&PyList_Type, &py_flags,
			&main_file_only,
			&verbose,
			&debug)
      || !extract(py_flags, flags))
    return 0;

  Py_INCREF(error);
  std::auto_ptr<Python::Object> error_type(new Python::Object(error));

  Py_INCREF(py_ast);
  AST::AST ast(py_ast);
  Py_INCREF(py_ast);

  std::set_unexpected(unexpected);
  ErrorHandler error_handler();

  if (debug) Synopsis::Trace::enable_debug();

  if (!input_file || *input_file == '\0')
  {
    PyErr_SetString(PyExc_RuntimeError, "no input file");
    return 0;
  }
  try
  {
    std::ifstream ifs(input_file);
    std::ofstream ofs(output_file ? output_file : trash.c_str());

    std::string input(std::istreambuf_iterator<char>(ifs.rdbuf()),
		      std::istreambuf_iterator<char>());

    typedef wave::cpplexer::lex_iterator<Token> lex_iterator_type;
    typedef wave::context<std::string::iterator,
                          lex_iterator_type,
                          wave::iteration_context_policies::load_file_to_string,
                          ASTTranslator> context_type;

    context_type ctx(input.begin(), input.end(), input_file,
		     ASTTranslator(input_file, base_path, main_file_only,
				   ast, verbose, debug));

    if (std::string(language) == "C")
      ctx.set_language(wave::support_c99);
    else
    {
      // Remove the '__cplusplus' macro as wave predefines it.
      flags.erase(std::remove(flags.begin(), flags.end(),
			      std::string("-D__cplusplus=1")),
		  flags.end());
    }
    ctx.add_include_path(".");
    ctx.set_sysinclude_delimiter();
    for (std::vector<char const *>::iterator i = flags.begin();
	 i != flags.end();
	 ++i)
    {
      if (**i == '-')
      {
	if (*(*i + 1) == 'I')
	  ctx.add_include_path(*i + 2);
	else if (*(*i + 1) == 'D')
	  ctx.add_macro_definition(*i + 2);
	else if (*(*i + 1) == 'U')
	  ctx.remove_macro_definition(*i + 2);
      }
    }

    context_type::iterator_type first = ctx.begin();
    context_type::iterator_type last = ctx.end();

    while (first != last)
    {
      ofs << (*first).get_value();
      ++first;
    }
  }
  catch (wave::cpp_exception &e) 
  {
    // some preprocessing error
    std::cerr << e.file_name() << "(" << e.line_no() << "): "
	      << e.description() << std::endl;
    Python::Object py_e((*error_type)());
    py_e.set_attr("file_name", e.file_name());
    py_e.set_attr("line_no", e.line_no());
    py_e.set_attr("description", e.description());
    PyErr_SetObject(error, py_e.ref());
    return 0;
  }
  catch (wave::cpplexer::lexing_exception &e) 
  {
    // some lexing error
    std::cerr << e.file_name() << "(" << e.line_no() << "): "
	      << e.description() << std::endl;
    Python::Object py_e((*error_type)());
    py_e.set_attr("file_name", e.file_name());
    py_e.set_attr("line_no", e.line_no());
    py_e.set_attr("description", e.description());
    PyErr_SetObject(error, py_e.ref());
    return 0;
  }
  catch (std::exception const &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
    Python::Object py_e((*error_type)());
    py_e.set_attr("file_name", "");
    py_e.set_attr("line_no", -1);
    py_e.set_attr("description", e.what());
    PyErr_SetObject(error, py_e.ref());
    return 0;
  }
  return py_ast;
}

PyMethodDef methods[] = {{(char*)"parse", parse, METH_VARARGS},
			 {0, 0}};
}

extern "C" void initwave()
{
  Python::Module module = Python::Module::define("wave", methods);
  module.set_attr("version", "0.1");
  error = PyErr_NewException("wave.Error", 0, 0);
  module.set_attr("Error", error);
}
