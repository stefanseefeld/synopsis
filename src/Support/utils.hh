//
// Copyright (C) 2011 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef utils_hh_
#define utils_hh_

#include <clang-c/Index.h>
#include <iostream>
#include <sstream>

inline std::string cursor_info(CXCursor c)
{
  std::ostringstream oss;
  CXString kind = clang_getCursorKindSpelling(c.kind);
  CXString name = clang_getCursorDisplayName(c);
  oss << "kind=" << clang_getCString(kind) << ", name=" << clang_getCString(name);
  clang_disposeString(name);
  clang_disposeString(kind);
  return oss.str();
}

inline std::string type_info(CXType t)
{
  std::ostringstream oss;
  CXString kind = clang_getTypeKindSpelling(t.kind);
  oss << "type " << clang_getCString(kind);
  clang_disposeString(kind);
  return oss.str();
}

inline std::string stringify_range(CXTranslationUnit tu, CXSourceRange r)
{
  CXToken *tokens;
  unsigned num_tokens;
  clang_tokenize(tu, r, &tokens, &num_tokens);
  std::ostringstream oss;
  for (unsigned i = 0; i != num_tokens; ++i) 
  {
    CXString t = clang_getTokenSpelling(tu, tokens[i]);
    oss << ' ' << clang_getCString(t);
    clang_disposeString(t);
  }
  clang_disposeTokens(tu, tokens, num_tokens);  
  return oss.str();
}

#endif
