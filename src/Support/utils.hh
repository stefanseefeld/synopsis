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

// Some declarations are anonymous.
// Generate unique names for them so we can refer to them.
inline std::string make_anonymous_name()
{
  static int i = 0;
  std::ostringstream oss;
  oss << '`' << (i++);
  return oss.str();
}

inline std::string token_info(CXTranslationUnit tu, CXToken t)
{
  std::ostringstream oss;
  CXTokenKind kind = clang_getTokenKind(t);
  CXString text = clang_getTokenSpelling(tu, t);
  switch (kind)
  {
    case CXToken_Punctuation: oss << "punktuation"; break;
    case CXToken_Keyword: oss << "keyword"; break;
    case CXToken_Identifier: oss << "identifier"; break;
    case CXToken_Literal: oss << "literal"; break;
    case CXToken_Comment: oss << "comment"; break;
    default: throw std::runtime_error("Unknown token kind");
  }
  oss << ": '" << clang_getCString(text) << "'";
  clang_disposeString(text);
  return oss.str();
}

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
  CXString kind = clang_getTypeKindSpelling(t.kind);
  std::string type = clang_getCString(kind);
  clang_disposeString(kind);
  return type;
}

inline std::string cursor_location(CXCursor c, bool extent = false)
{
  std::ostringstream oss;
  CXSourceRange range = clang_getCursorExtent(c);
  CXSourceLocation start = clang_getRangeStart(range);
  CXFile sf;
  unsigned line, column;
  clang_getSpellingLocation(start, &sf, &line, &column, /*offset*/ 0);
  CXString f = clang_getFileName(sf);
  oss << clang_getCString(f);
  clang_disposeString(f);
  if (extent)
  {
    oss << '(' << line << ':' << column << '-';
    CXSourceLocation end = clang_getRangeEnd(range);
    clang_getSpellingLocation(end, 0/*file*/, &line, &column, /*offset*/ 0);
    oss << line << ':' << column << ')';
  }
  else
    oss << ':' << line << ':' << column;
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
