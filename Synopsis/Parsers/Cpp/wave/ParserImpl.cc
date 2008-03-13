//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <boost/version.hpp>
#include <boost/python.hpp>
#include <Synopsis/Trace.hh>
#include <Synopsis/Timer.hh>
#include <Support/ErrorHandler.hh>

#if BOOST_VERSION < 103500 || BOOST_WAVE_USE_DEPRECIATED_PREPROCESSING_HOOKS != 0
# include "IRGenerator-depreciated.hh"
#else
# include "IRGenerator.hh"
#endif
#include <memory>
#include <sstream>

using namespace Synopsis;
namespace wave = boost::wave;
namespace bpl = boost::python;

namespace
{
#ifdef __WIN32__
  std::string const trash = "NUL";
#else
  std::string const trash = "/dev/null";
#endif

bpl::object error_type;

//. Override unexpected() to print a message before we abort
void unexpected()
{
  std::cout << "Error: Aborting due to unexpected exception." << std::endl;
  throw std::bad_exception();
}

bpl::object parse(bpl::object ir,
                  char const *input_file, char const *base_path, char const *output_file,
                  char const *language, bpl::list py_system_flags, bpl::list py_user_flags,
                  bool primary_file_only, bool verbose, bool debug, bool profile)
{
  std::vector<char const *> system_flags(bpl::len(py_system_flags));
  for (unsigned int i = 0; i != bpl::len(py_system_flags); ++i)
    system_flags[i] = bpl::extract<char const *>(py_system_flags[i]);
  std::vector<char const *> user_flags(bpl::len(py_user_flags));
  for (unsigned int i = 0; i != bpl::len(py_user_flags); ++i)
    user_flags[i] = bpl::extract<char const *>(py_user_flags[i]);

  std::set_unexpected(unexpected);
  ErrorHandler error_handler();

  if (debug) Synopsis::Trace::enable(Trace::ALL);

  if (!input_file || *input_file == '\0') throw std::runtime_error("no input file");
  std::ifstream ifs(input_file);
  if (!ifs)
  {
    std::string msg = "unable to read '";
    msg += input_file;
    msg += '\'';
    throw std::runtime_error(msg);
  }
  std::ofstream ofs(output_file ? output_file : trash.c_str());

  std::string input(std::istreambuf_iterator<char>(ifs.rdbuf()),
                    std::istreambuf_iterator<char>());

  Timer timer;

  IRGenerator generator(language, input_file, base_path, primary_file_only,
                        ir, verbose, debug);

  IRGenerator::Context ctx(input.begin(), input.end(), input_file, generator);
  if (std::string(language) == "C")
  {
    ctx.set_language(wave::language_support(wave::support_c99|
                                            wave::support_option_convert_trigraphs|
                                            wave::support_option_emit_line_directives|
                                            wave::support_option_insert_whitespace));
    // Remove the '__STDC_HOSTED__' macro as wave predefines it.
    system_flags.erase(std::remove(system_flags.begin(), system_flags.end(),
                                   std::string("-D__STDC_HOSTED__=1")),
                       system_flags.end());
    // Remove the '__STDC__' macro as wave predefines it.
    system_flags.erase(std::remove(system_flags.begin(), system_flags.end(),
                                   std::string("-D__STDC__=1")),
                       system_flags.end());
  }
  else if (std::string(language) == "C++")
  {
    ctx.set_language(wave::enable_variadics(ctx.get_language()));
    // FIXME: should only enable in GCC compat mode.
    ctx.set_language(wave::enable_long_long(ctx.get_language()));
    // Remove the '__cplusplus' macro as wave predefines it.
    system_flags.erase(std::remove(system_flags.begin(), system_flags.end(),
                                   std::string("-D__cplusplus=1")),
                       system_flags.end());
  }
  else if (std::string(language) == "IDL")
  {
    ctx.set_language(wave::language_support(wave::support_c99|
                                            wave::support_option_convert_trigraphs|
                                            wave::support_option_emit_line_directives|
                                            wave::support_option_insert_whitespace));
  }
  ctx.set_language(wave::enable_preserve_comments(ctx.get_language()));
  if (verbose)
  {
    std::cout << "system flags :" << std::endl;
    for (std::vector<char const *>::iterator i = system_flags.begin();
         i != system_flags.end();
         ++i)
      std::cout << *i << ' ';
    std::cout << "\nuser flags :" << std::endl;
    for (std::vector<char const *>::iterator i = user_flags.begin();
         i != user_flags.end();
         ++i)
      std::cout << *i << ' ';
    std::cout << std::endl;
  }


  std::vector<std::string> includes;
  // Insert the user_flags
  for (std::vector<char const *>::iterator i = user_flags.begin();
       i != user_flags.end();
       ++i)
  {
    if (**i == '-')
    {
      if (*(*i + 1) == 'I')
        ctx.add_sysinclude_path(*i + 2);
      else if (*(*i + 1) == 'D')
        ctx.add_macro_definition(*i + 2, /*is_predefined=*/false);
      else if (*(*i + 1) == 'U')
        ctx.remove_macro_definition(*i + 2, /*even_predefined=*/false);
      else if (*(*i + 1) == 'i')
        includes.push_back(*i + 2);
    }
  }

  // Insert the system_flags from the Emulator.
  for (std::vector<char const *>::iterator i = system_flags.begin();
       i != system_flags.end();
       ++i)
  {
    if (**i == '-')
    {
      if (*(*i + 1) == 'I')
        ctx.add_sysinclude_path(*i + 2);
      else if (*(*i + 1) == 'D')
        ctx.add_macro_definition(*i + 2, /*is_predefined=*/true);
      else if (*(*i + 1) == 'U')
        ctx.remove_macro_definition(*i + 2, /*even_predefined=*/false);
      else if (*(*i + 1) == 'i')
        includes.push_back(*i + 2);
    }
  }

  IRGenerator::Context::iterator_type first = ctx.begin();
  IRGenerator::Context::iterator_type end = ctx.end();

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
    std::cout << "preprocessor took " << timer.elapsed() << " seconds" << std::endl;
  return ir;
}

void cpp_error(wave::cpp_exception const &e)
{
  std::ostringstream oss;
  oss << e.file_name() << ':' << e.line_no() << " : " << e.description();
  std::string what = oss.str();
  bpl::object error(error_type(what.c_str()));
//   error.attr("file_name") = e.file_name();
//   error.attr("line_no") = e.line_no();
//   error.attr("description") = e.description();
  PyErr_SetObject(error_type.ptr(), error.ptr());
}

void lexer_error(wave::cpplexer::lexing_exception const &e)
{
  std::ostringstream oss;
  oss << e.file_name() << ':' << e.line_no() << " : " << e.description();
  std::string what = oss.str();
  bpl::object error(error_type(what.c_str()));
//   error.attr("file_name") = e.file_name();
//   error.attr("line_no") = e.line_no();
//   error.attr("description") = e.description();
  PyErr_SetObject(error_type.ptr(), error.ptr());
}

}

BOOST_PYTHON_MODULE(ParserImpl)
{
  bpl::register_exception_translator<wave::cpp_exception>(cpp_error);
  bpl::register_exception_translator<wave::cpplexer::lexing_exception>(lexer_error);

  bpl::scope scope;
  scope.attr("version") = "0.2";
  bpl::def("parse", parse);
  bpl::object processor = bpl::import("Synopsis.Processor");
  bpl::object error_base = processor.attr("Error");
  error_type = bpl::object(bpl::handle<>(PyErr_NewException("ParserImpl.CppError",
                                                            error_base.ptr(), 0)));
  scope.attr("CppError") = error_type;
}
