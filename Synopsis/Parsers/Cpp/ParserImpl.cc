//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/AST/ASTKit.hh>
#include <Synopsis/Python/Module.hh>
#include <Synopsis/Trace.hh>
#include <Synopsis/Timer.hh>
#include <Support/ErrorHandler.hh>
#include "ASTTranslator.hh"
#include <memory>
#include <sstream>

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
  PyObject *py_system_flags;
  PyObject *py_user_flags;
  std::vector<char const *> system_flags;
  std::vector<char const *> user_flags;
  PyObject *py_ast;
  int primary_file_only = 0;
  int verbose = 0;
  int debug = 0;
  int profile = 0;
  if (!PyArg_ParseTuple(args, "OszzsO!O!iiii",
			&py_ast,
			&input_file,
			&base_path,
			&output_file,
			&language,
			&PyList_Type, &py_system_flags,
			&PyList_Type, &py_user_flags,
			&primary_file_only,
			&verbose,
			&debug,
			&profile)
      || !extract(py_system_flags, system_flags)
      || !extract(py_user_flags, user_flags))
    return 0;

  Py_INCREF(error);
  std::auto_ptr<Python::Object> error_type(new Python::Object(error));

  Py_INCREF(py_ast);
  AST::AST ast(py_ast);
  Py_INCREF(py_ast);

  std::set_unexpected(unexpected);
  ErrorHandler error_handler();

  if (debug) Synopsis::Trace::enable(Trace::ALL);

  if (!input_file || *input_file == '\0')
  {
    PyErr_SetString(error, "no input file");
    return 0;
  }
  try
  {
    std::ifstream ifs(input_file);
    if (!ifs)
    {
      std::string msg = "unable to read '";
      msg += input_file;
      msg += '\'';
      PyErr_SetString(error, msg.c_str());
      return 0;
    }
    std::ofstream ofs(output_file ? output_file : trash.c_str());

    std::string input(std::istreambuf_iterator<char>(ifs.rdbuf()),
		      std::istreambuf_iterator<char>());

    Timer timer;
    typedef wave::cpplexer::lex_iterator<Token> lex_iterator_type;
    typedef wave::context<std::string::iterator,
                          lex_iterator_type,
                          wave::iteration_context_policies::load_file_to_string,
                          ASTTranslator> context_type;

    context_type ctx(input.begin(), input.end(), input_file,
		     ASTTranslator(language, input_file, base_path, primary_file_only,
				   ast, verbose, debug));

    if (std::string(language) == "C")
    {
      ctx.set_language(wave::support_c99);
      // Remove the '__STDC_HOSTED__' macro as wave predefines it.
      system_flags.erase(std::remove(system_flags.begin(), system_flags.end(),
                                     std::string("-D__STDC_HOSTED__=1")),
                         system_flags.end());
      // Remove the '__STDC__' macro as wave predefines it.
      system_flags.erase(std::remove(system_flags.begin(), system_flags.end(),
                                     std::string("-D__STDC__=1")),
                         system_flags.end());
    }
    else
    {
      ctx.set_language(wave::enable_variadics(ctx.get_language()));
      // FIXME: should only enable in GCC compat mode.
      ctx.set_language(wave::enable_long_long(ctx.get_language()));
      // Remove the '__cplusplus' macro as wave predefines it.
      system_flags.erase(std::remove(system_flags.begin(), system_flags.end(),
                                     std::string("-D__cplusplus=1")),
                         system_flags.end());
    }
    ctx.set_language(wave::enable_preserve_comments(ctx.get_language()));

    std::vector<std::string> includes;
    // Insert the system_flags from the Emulator.
    for (std::vector<char const *>::iterator i = system_flags.begin();
	 i != system_flags.end();
	 ++i)
    {
      if (**i == '-')
      {
	if (*(*i + 1) == 'I')
	{
	  ctx.add_include_path(*i + 2);
	  ctx.add_sysinclude_path(*i + 2);
	}
	else if (*(*i + 1) == 'D')
          ctx.add_macro_definition(*i + 2, /*is_predefined=*/true);
	else if (*(*i + 1) == 'U')
	  ctx.remove_macro_definition(*i + 2, /*even_predefined=*/false);
	else if (*(*i + 1) == 'i')
	  includes.push_back(*i + 2);
      }
    }

    // Insert the user_flags
    for (std::vector<char const *>::iterator i = user_flags.begin();
	 i != user_flags.end();
	 ++i)
    {
      if (**i == '-')
      {
	if (*(*i + 1) == 'I')
	{
	  ctx.add_include_path(*i + 2);
	  ctx.add_sysinclude_path(*i + 2);
	}
	else if (*(*i + 1) == 'D')
	  ctx.add_macro_definition(*i + 2, /*is_predefined=*/false);
	else if (*(*i + 1) == 'U')
	  ctx.remove_macro_definition(*i + 2, /*even_predefined=*/false);
	else if (*(*i + 1) == 'i')
	  includes.push_back(*i + 2);
      }
    }

    context_type::iterator_type first = ctx.begin();
    context_type::iterator_type end = ctx.end();

    for (std::vector<std::string>::const_reverse_iterator i = includes.rbegin(),
         e = includes.rend();
         i != e; /**/)
    {
      std::string filename(*i);
      first.force_include(filename.c_str(), ++i == e);
    }

    while (first != end)
    {
      ofs << (*first).get_value();
      ++first;
    }
    if (profile)
      std::cout << "preprocessor took " << timer.elapsed() 
		<< " seconds" << std::endl;
  }
  catch (wave::cpp_exception &e) 
  {
    // some preprocessing error
    std::cerr << e.file_name() << "(" << e.line_no() << "): "
	      << e.description() << std::endl;
    std::ostringstream oss;
    oss << e.file_name() << ':' << e.line_no() << " : " << e.description();
    std::string what = oss.str();
    Python::Object py_e((*error_type)(Python::Tuple(what.c_str())));
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
    std::ostringstream oss;
    oss << e.file_name() << ':' << e.line_no() << " : " << e.description();
    std::string what = oss.str();
    Python::Object py_e((*error_type)(Python::Tuple(what.c_str())));
    py_e.set_attr("file_name", e.file_name());
    py_e.set_attr("line_no", e.line_no());
    py_e.set_attr("description", e.description());
    PyErr_SetObject(error, py_e.ref());
    return 0;
  }
  catch (std::exception const &e)
  {
    std::cerr << "Caught exception : " << e.what() << std::endl;
    Python::Object py_e((*error_type)(Python::Tuple(e.what())));
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

extern "C" void initParserImpl()
{
  Python::Module module = Python::Module::define("ParserImpl", methods);
  module.set_attr("version", "0.1");
  Python::Object processor = Python::Object::import("Synopsis.Processor");
  Python::Object error_base = processor.attr("Error");
  error = PyErr_NewException("ParserImpl.CppError", error_base.ref(), 0);
  module.set_attr("Error", error);
}
