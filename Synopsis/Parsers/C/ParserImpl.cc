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

bpl::dict annotations(bpl::object o) 
{ return bpl::extract<bpl::dict>(o.attr("annotations"));}

//. Merge new files to files already present in the IR.
//. If a file doesn't exist yet, add it. If it does, 
//. upgrade the 'primary' flag if it is set now.
void merge_files(bpl::dict ir_files, bpl::dict new_files)
{
  bpl::list keys = new_files.keys();
  for (unsigned i = 0; i != bpl::len(keys); ++i)
  {
    bpl::object filename = keys[i];
    bpl::object file = new_files[filename];
    if (ir_files.has_key(filename))
    {
      if (bpl::extract<bool>(annotations(file)["primary"]))
       	annotations(ir_files[filename])["primary"] = true;
    }
    else
      ir_files[filename] = file;
  }
}


bpl::object parse(bpl::object ir,
                  char const *cpp_file, char const *input_file, char const *base_path,
                  bool primary_file_only, char const *sxr_prefix,
		  bpl::list cpp_flags,
                  bool verbose, bool debug, bool profile)
{
  std::set_unexpected(unexpected);

  // if (debug) Synopsis::Trace::enable(Trace::TRANSLATION);

  if (!input_file || *input_file == '\0') throw std::runtime_error("no input file");

  char const **args = new char const*[bpl::len(cpp_flags) + 2];
  args[0] = "-x";
  args[1] = "c";
  for (size_t i = 0; i != bpl::len(cpp_flags); ++i)
    args[i + 2] = bpl::extract<char const *>(cpp_flags[i]);

  Timer timer;
  // first arg: exclude declarations from PCH
  // second arg: display diagnostics
  CXIndex idx = clang_createIndex(0, 1);
  CXTranslationUnit_Flags flags = CXTranslationUnit_DetailedPreprocessingRecord;
  CXTranslationUnit tu = 
    clang_parseTranslationUnit(idx, input_file,
			       args,
			       bpl::len(cpp_flags) + 2,
			       0,  // unsaved_files
			       0,  // num_unsaved_files
			       flags);
  delete [] args;
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
  bpl::object asg = ir.attr("asg");
  bpl::dict files;
  timer.reset();
  ASGTranslator translator(input_file, base_path, primary_file_only, asg, files, verbose, debug);
  translator.translate(tu);

  if (profile)
    std::cout << "ASG translation took " << timer.elapsed() 
	      << " seconds" << std::endl;
  if (sxr_prefix)
  {
    timer.reset();
    SXRGenerator generator(translator, verbose, debug);
    for (size_t i = 0; i != bpl::len(files); ++i)
    {
      bpl::object sf = files.attr("values")()[i];
      if (!bpl::extract<bpl::dict>(sf.attr("annotations"))().get("primary", false)) continue;
      std::string abs_name = bpl::extract<char const *>(sf.attr("abs_name"))();
      std::string name = bpl::extract<char const *>(sf.attr("name"))();
      std::string sxr = std::string(sxr_prefix) + "/" + name + ".sxr";
      if (!boost::filesystem::exists(sxr))
      {
	create_directories(fs::path(sxr).branch_path());
	generator.generate(tu, sxr, abs_name, name);
      }
    }
    if (profile)
      std::cout << "SXR generation took " << timer.elapsed() 
  		<< " seconds" << std::endl;
  }
  merge_files(bpl::extract<bpl::dict>(ir.attr("files")), files);

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
