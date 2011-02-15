//
// Copyright (C) 2006 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include "ASGTranslator.hh"
#include <clang-c/Index.h>
#include <iostream>
#include <fstream>

#ifndef SXRGenerator_hh_
#define SXRGenerator_hh_

class SXRGenerator
{
public:
  SXRGenerator(ASGTranslator const &, bool, bool);

  void generate(CXTranslationUnit,
		std::string const &sxr, std::string const &abs_filename, std::string const &filename);

private:
  char const *token_kind_to_class(CXTokenKind);
  char const *xref(CXCursor);
  char const *from(CXCursor);
  void write_comment(std::string const &, unsigned &line);
  void write_xref(CXToken const &);
  void write(char const *, size_t = 0);
  void write_esc(std::string const &);
  void write_esc(char const *);

  CXTranslationUnit tu_;  
  ASGTranslator const &translator_;
  std::filebuf obuf_;
  bool verbose_;
  bool debug_;
};

#endif
