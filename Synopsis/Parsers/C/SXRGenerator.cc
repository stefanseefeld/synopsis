//
// Copyright (C) 2006 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include "SXRGenerator.hh"
#include <cstring>
#include <boost/filesystem/operations.hpp>
#include <sstream>

namespace fs = boost::filesystem;


namespace
{
void print_extent(CXSourceRange range)
{
  CXSourceLocation start = clang_getRangeStart(range);
  unsigned line, column;
  clang_getSpellingLocation(start, 0/*file*/, &line, &column, /*offset*/ 0);
  std::cout << "start: " << line << ':' << column;
  CXSourceLocation end = clang_getRangeEnd(range);
  clang_getSpellingLocation(end, 0/*file*/, &line, &column, /*offset*/ 0);
  std::cout << " end: " << line << ':' << column;
  std::cout << std::endl;
}

std::string cursor_info(CXCursor c)
{
  std::ostringstream oss;
  CXString kind = clang_getCursorKindSpelling(c.kind);
  CXString name = clang_getCursorDisplayName(c);
  oss << "kind=" << clang_getCString(kind) << ", name=" << clang_getCString(name);
  clang_disposeString(name);
  clang_disposeString(kind);
  return oss.str();
}

}

SXRGenerator::SXRGenerator(std::string const &sxr, ASGTranslator const &t)
  : translator_(t)
{
  obuf_.open(sxr.c_str(), std::ios_base::out);
}

SXRGenerator::~SXRGenerator()
{
  obuf_.close();
}

void SXRGenerator::generate(CXTranslationUnit tu, std::string const &filename)
{
  tu_ = tu;
  // FIXME: This function is broken. Construct the range manually instead...
  // CXSourceRange range = clang_getCursorExtent(clang_getTranslationUnitCursor(tu));
  fs::path path(filename, fs::native);
  size_t size = fs::file_size(path);
  CXFile f = clang_getFile(tu, filename.c_str());
  CXSourceLocation begin = clang_getLocationForOffset(tu, f, 0);
  CXSourceLocation end = clang_getLocationForOffset(tu, f, size);
  CXSourceRange range = clang_getRange(begin, end);

  CXToken *tokens;
  unsigned num_tokens;
  clang_tokenize(tu, range, &tokens, &num_tokens);
  write("<sxr filename=\"");
  write(filename.c_str());
  write("\">\n");
  write("<line>");

  // Why does clang start counting at '1' ?
  unsigned line = 1, column = 1;
  for (unsigned i = 0; i != num_tokens; ++i) 
  {
    // CXSourceRange extent = clang_getTokenExtent(tu, tokens[i]);
    // CXSourceLocation start = clang_getRangeStart(extent);
    CXSourceLocation start = clang_getTokenLocation(tu, tokens[i]);
    unsigned l, c;
    clang_getSpellingLocation(start, 0, &l, &c, 0);
    if (l > line) column = 1;
    while (l > line) { write("</line>\n<line>"); ++line;}
    while (c > column) { obuf_.sputc(' '); ++column;}
    //  Now fill the output buffer till the given location (l,c)
    CXString token_string = clang_getTokenSpelling(tu, tokens[i]);
    char const *s = clang_getCString(token_string);
    size_t len = strlen(s);
    CXTokenKind kind = clang_getTokenKind(tokens[i]);
    switch (kind)
    {
      case CXToken_Comment:
      case CXToken_Keyword:
      case CXToken_Literal:
	write("<span class=\"");
	write(token_kind_to_class(kind));
	write("\">");
	write_esc(s);
	write("</span>");
	break;
      case CXToken_Identifier:
	xref(tokens[i]);
	break;
      default:
	write_esc(s);
	break;
    }
    clang_disposeString(token_string);
    column += len;
  }
  write("</line>\n</sxr>\n");
  clang_disposeTokens(tu, tokens, num_tokens);
}

void SXRGenerator::xref(CXToken const &t)
{
  CXSourceLocation l = clang_getTokenLocation(tu_, t);
  CXString s = clang_getTokenSpelling(tu_, t);
  std::string text = clang_getCString(s);
  clang_disposeString(s);
  CXCursor c = clang_getCursor(tu_, l);
  CXCursor r = clang_getCursorReferenced(c);
  if (clang_isCursorDefinition(c))
  {
    bpl::object decl = translator_.lookup(c);
    if (!decl)
      throw std::runtime_error("Error: can't find symbol " + cursor_info(c));
    char const *qname = bpl::extract<char const *>(bpl::str(decl.attr("name")));
    write("<a href=\"");
    write_esc(qname);
    write("\" from=\"#\"");
    write(" type=\"definition\">");
    write_esc(text.c_str());
    write("</a>");
  }
  else if (clang_isDeclaration(c.kind))
  {
    bpl::object decl = translator_.lookup(c);
    if (!decl)
      throw std::runtime_error("Error: can't find symbol " + cursor_info(c));
    char const *qname = bpl::extract<char const *>(bpl::str(decl.attr("name")));
    write("<a href=\"");
    write_esc(qname);
    write("\" from=\"#\"");
    write(" type=\"definition\">");
    write_esc(text.c_str());
    write("</a>");
  }
  else if (!clang_equalCursors(r, clang_getNullCursor()))
  {
    bpl::object decl = translator_.lookup(r);
    if (!decl)
      throw std::runtime_error("Error: can't find symbol " + cursor_info(c));
    char const *qname = bpl::extract<char const *>(bpl::str(decl.attr("name")));
    // A reference
    write("<a href=\"");
    write_esc(qname);
    write("\" from=\"#\"");
    write(" type=\"reference\">");
    write_esc(text.c_str());
    write("</a>");
  }
  else 
    throw std::runtime_error("unimplemented: " + cursor_info(c));
}

char const *SXRGenerator::token_kind_to_class(CXTokenKind k)
{
  switch (k)
  {
    case CXToken_Comment: return "comment";
    case CXToken_Keyword: return "keyword";
    case CXToken_Literal: return "literal";
    default: "unknown";
  }
}

void SXRGenerator::write(char const *text, size_t len)
{
  if (len) obuf_.sputn(text, len);
  else obuf_.sputn(text, strlen(text));
}

void SXRGenerator::write_esc(char const *text)
{
  // output each character, escaping it as necessary
  do
  {
    switch (*text)
    {
      case '&': write("&amp;"); break;
      case '<': write("&lt;"); break;
      case '>': write("&gt;"); break;
      case '"': write("&quot;"); break;
      default: obuf_.sputc(*text); break;
    }
  }
  while (*++text);
};
