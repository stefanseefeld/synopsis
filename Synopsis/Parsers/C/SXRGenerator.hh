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
  SXRGenerator(std::string const &, ASGTranslator const &);
  ~SXRGenerator();

  void generate(CXTranslationUnit, std::string const &filename);

private:
  void xref(CXToken const &);
  char const *token_kind_to_class(CXTokenKind);
  void write(char const *, size_t = 0);
  void write_esc(char const *);

  CXTranslationUnit tu_;  
  ASGTranslator const &translator_;
  std::filebuf obuf_;
};

#endif
