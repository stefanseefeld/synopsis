//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "ASGTranslator.hh"
#include "SXRGenerator.hh"
#include <Support/Timer.hh>
#include <Support/path.hh>
#include <boost/filesystem/convenience.hpp>
#include <clang-c/Index.h>
#include <fstream>
#include <cstring>
#include <unistd.h>

using namespace Synopsis;
namespace fs = boost::filesystem;

namespace
{
bpl::object error_type;

//. Override unexpected() to print a message before we abort
void unexpected()
{
  std::cout << "Warning: Aborting due to unexpected exception." << std::endl;
  throw std::bad_exception();
}

bpl::object parse(bpl::object ir,
                  char const *cpp_file, char const *input_file, char const *base_path,
                  bool primary_file_only, char const *sxr_prefix,
                  bool verbose, bool debug, bool profile)
{
  std::set_unexpected(unexpected);

  // if (debug) Synopsis::Trace::enable(Trace::TRANSLATION);

  if (!input_file || *input_file == '\0') throw std::runtime_error("no input file");

  Timer timer;
  // first arg: exclude declarations from PCH
  // second arg: display diagnostics
  CXIndex idx = clang_createIndex(0, 1);
  CXTranslationUnit_Flags flags = CXTranslationUnit_DetailedPreprocessingRecord;
  CXTranslationUnit tu = 
    clang_parseTranslationUnit(idx, input_file,
			       0,  // command-line args
			       0,  // number of command-line args
			       0,  // unsaved_files
			       0,  // num_unsaved_files
			       flags);
  if (!tu) 
  {
    std::cerr << "unable to parse input\n";
    return ir;
  }

  if (profile)
    std::cout << "C++ parser took " << timer.elapsed() 
              << " seconds" << std::endl;
  unsigned diagnostics = clang_getNumDiagnostics(tu);
  if (diagnostics)
  {
    for (unsigned i = 0; i != diagnostics; ++i)
    {
      CXDiagnostic d = clang_getDiagnostic(tu, i);
      CXString diagnostic = clang_formatDiagnostic(d, flags);
      std::cerr << clang_getCString(diagnostic) << std::endl;
      clang_disposeString(diagnostic);
      clang_disposeDiagnostic(d);
    }
    throw std::runtime_error("The input contains errors.");
  }
  timer.reset();
  ASGTranslator translator(input_file, base_path, primary_file_only, ir, verbose, debug);
  translator.translate(tu);

  if (profile)
    std::cout << "ASG translation took " << timer.elapsed() 
	      << " seconds" << std::endl;
  if (sxr_prefix)
  {
    timer.reset();

    std::string long_filename = make_full_path(input_file);
    std::string short_filename = make_short_path(input_file, base_path);
    //bpl::object file = ir.attr("files")[short_filename];
  //   // Undo the preprocesser's macro expansions.
  //   bpl::dict macro_calls = bpl::dict(file.attr("macro_calls"));
    std::string sxr = std::string(sxr_prefix) + "/" + short_filename + ".sxr";
    create_directories(fs::path(sxr).branch_path());
    SXRGenerator generator(sxr, translator);
    generator.generate(tu, short_filename);
    if (profile)
      std::cout << "SXR generation took " << timer.elapsed() 
  		<< " seconds" << std::endl;
  }
  clang_disposeTranslationUnit(tu);
  clang_disposeIndex(idx);
  return ir;
}

}

BOOST_PYTHON_MODULE(ParserImpl)
{
  bpl::scope scope;
  scope.attr("version") = "0.2";
  bpl::def("parse", parse);
  bpl::object module = bpl::import("Synopsis.Processor");
  bpl::object error_base = module.attr("Error");
  error_type = bpl::object(bpl::handle<>(PyErr_NewException("ParserImpl.ParseError",
                                                            error_base.ptr(), 0)));
  scope.attr("ParseError") = error_type;
}
